#ifdef _MSV_VER
#include <platform.h>
#endif

#include "util/order_parser.h"
#include "util/keyword.h"
#include "util/language.h"
#include "util/log.h"
#include "util/param.h"
#include "util/parser.h"
#include "util/path.h"
#include "util/pofile.h"
#include "util/strings.h"

#include <cJSON.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct parser_state {
    FILE * F;
    struct locale *lang;
} parser_state;

static void handle_order(void *userData, const char *line) {
    parser_state * state = (parser_state*)userData;
    const char *tok;
    keyword_t kwd;
    init_tokens_str(line);
    tok = getstrtoken();
    kwd = get_keyword(tok, state->lang);
    if (kwd == NOKEYWORD) {
        param_t par;
        par = findparam(tok, state->lang);
        switch (par) {
        case P_UNIT:
        case P_FACTION:
        case P_GAMENAME:
        case P_REGION:
        case P_LOCALE:
        case P_NEXT:
            break;
        default:
            fprintf(state->F, "unknown command: %s\n", tok);
        }
    }
}

int parsefile(FILE *F, struct locale *lang) {
    OP_Parser parser;
    char buf[1024];
    int done = 0, err = 0;
    parser_state state = { NULL };

    state.F = stdout;
    state.lang = lang;

    parser = OP_ParserCreate();
    OP_SetOrderHandler(parser, handle_order);
    OP_SetUserData(parser, &state);

    while (!done) {
        size_t len = (int)fread(buf, 1, sizeof(buf), F);
        if (ferror(F)) {
            /* TODO: error message */
            err = errno;
            break;
        }
        done = feof(F);
        if (OP_Parse(parser, buf, len, done) == OP_STATUS_ERROR) {
            /* TODO: error message */
            err = (int)OP_GetErrorCode(parser);
            break;
        }
    }
    OP_ParserFree(parser);
    return err;
}

static int handle_po(const char *msgid, const char *msgstr, const char *msgctxt, void *data) {
    struct locale *lang = (struct locale *)data;
    if (msgctxt) {
        if (strcmp(msgctxt, "keyword") == 0) {
            keyword_t kwd = findkeyword(msgid);
            init_keyword(lang, kwd, msgstr);
            locale_setstring(lang, mkname("keyword", keywords[kwd]), msgstr);
        }
    }
    else if (msgid[0]>='A' && msgid[0] <= 'Z') {
        int i;
        /* parameters have no msgctxt, this is lame */
        for (i = 0; i != MAXPARAMS; ++i) {
            if (strcmp(msgid, parameters[i]) == 0) {
                init_parameter(lang, (param_t)i, msgstr);
                break;
            }
        }
    }
    return 0;
}

static char path[PATH_MAX];

static void parse_translations(cJSON *config, const char *cfgdir, struct locale *lang) {
    cJSON *json;
    assert(config->type == cJSON_Array);
    for (json = config->child; json; json = json->next) {
        if (json->type == cJSON_String) {
            path_join(cfgdir, json->valuestring, path, sizeof(path));
            pofile_read(path, handle_po, lang);
        }
    }
}

static void parse_keywords(cJSON *config) {
    struct locale *lang;
    assert(config->type == cJSON_Object);
    lang = get_locale(config->string);
    if (lang) {
        cJSON *json;
        for (json = config->child; json; json = json->next) {
            keyword_t kwd = findkeyword(json->string);
            if (kwd != NOKEYWORD && keywords[kwd]) {
                if (json->type == cJSON_String) {
                    init_keyword(lang, kwd, json->valuestring);
                    locale_setstring(lang, mkname("keyword", keywords[kwd]), json->valuestring);
                }
                else if (json->type == cJSON_Array) {
                    cJSON *child;
                    for (child = json->child; child; child = child->next) {
                        init_keyword(lang, kwd, child->valuestring);
                        if (child == json->child) {
                            locale_setstring(lang, mkname("keyword", keywords[kwd]), child->valuestring);
                        }
                    }
                }
            }
        }
    }
}

static cJSON *json_parse(const char *filename) {
    cJSON *config = NULL;
    FILE *F = fopen(filename, "r");
    if (F) {
        long pos;
        fseek(F, 0, SEEK_END);
        pos = ftell(F);
        rewind(F);
        if (pos > 0) {
            char *data;
            size_t sz;

            data = malloc(pos + 1);
            sz = fread(data, 1, (size_t)pos, F);
            data[sz] = 0;
            config = cJSON_Parse(data);
            if (!config) {
                int line;
                char buffer[10];
                const char *xp = data, *lp, *ep = cJSON_GetErrorPtr();
                for (line = 1, lp = xp; xp && xp<ep; ++line, lp = xp + 1) {
                    xp = strchr(lp, '\n');
                    if (xp >= ep) break;
                }
                xp = (ep > data + 10) ? ep - 10 : data;
                str_strlcpy(buffer, xp, sizeof(buffer));
                buffer[9] = 0;
                log_error("json parse error in line %d, position %d, near `%s`\n", line, ep - lp, buffer);
            }
            free(data);
        }
        fclose(F);
    }
    return config;
}

static int read_keywords(const char *filename) {
    cJSON *json, *config = json_parse(filename);

    if (config) {
        for (json = config->child; json; json = json->next) {
            if (json->type == cJSON_Object) {
                if (0 == strcmp("keywords", json->string)) {
                    break;
                }
            }
        }
        if (json) {
            for (json = json->child; json; json = json->next) {
                if (json->type == cJSON_Object) {
                    parse_keywords(json);
                }
            }
        }
        cJSON_Delete(config);
    }
    else {
        log_error("could not parse %s.", filename);
        return - 1;
    }
    return 0;
}

static int read_config(const char *cfgfile) {
    cJSON *config = json_parse(cfgfile);
    if (config) {
        const char *cfgdir = "res";
        const char *kwdfile = "keywords.json";
        cJSON *json, *files = NULL;
        for (json = config->child; json; json = json->next) {
            if (json->type == cJSON_String) {
                if (0 == strcmp(json->string, "config-dir")) {
                    cfgdir = json->valuestring;
                }
                else if (0 == strcmp(json->string, "keywords")) {
                    kwdfile = json->valuestring;
                }
            }
            else if (json->type == cJSON_Object) {
                if (0 == strcmp(json->string, "translations")) {
                    files = json;
                }
            }
        }
        if (files) {
            for (json = files->child; json; json = json->next) {
                struct locale * lang = get_locale(json->string);
                if (lang) {
                    parse_translations(json, cfgdir, lang);
                }
            }
        }
        path_join(cfgdir, kwdfile, path, sizeof(path));
        cJSON_Delete(config);
        return read_keywords(path);
    }
    log_error("could not parse %s.", cfgfile);
    return -1;
}

int main(int argc, char **argv) {
    const char *loc = "de";
    const char *cfgfile = "checker.json";
    FILE * F = stdin;
    int err = 0;
    struct locale *lang;

    if (argc > 1) {
        const char *filename = argv[1];
        F = fopen(filename, "r");
        if (!F) {
            perror(filename);
            return -1;
        }
    }
    lang = get_or_create_locale(loc);
    err = read_config(cfgfile);
    if (0 == err) {
        err = parsefile(F, lang);
        if (F != stdin) {
            fclose(F);
        }
    }
    return err;
}
