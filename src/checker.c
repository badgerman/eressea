#ifdef _MSV_VER
#include <platform.h>
#endif

#include "util/order_parser.h"
#include "util/keyword.h"
#include "util/language.h"
#include "util/param.h"
#include "util/parser.h"
#include "util/path.h"
#include "util/pofile.h"

#include <iniparser.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef struct parser_state {
    FILE * F;
    struct locale *lang;
} parser_state;

static void handle_order(void *userData, const char *line) {
    parser_state * state = (parser_state*)userData;
    const char *str;
    keyword_t kwd;
    init_tokens_str(line);
    str = getstrtoken();
    kwd = get_keyword(str, state->lang);
    if (kwd == NOKEYWORD) {
        fprintf(state->F, "unknown command: %s\n", str);
    }
}

int parsefile(FILE *F, const char *loc) {
    OP_Parser parser;
    char buf[1024];
    int done = 0, err = 0;
    parser_state state = { NULL };

    state.F = stdout;
    state.lang = get_locale(loc);

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

static void read_language(dictionary *d, const char *respath, const char *loc) {
    char buffer[PATH_MAX];
    size_t len = strlen(loc);
    struct locale *lang = get_or_create_locale(loc);

    if (len + 7 < sizeof(buffer)) {
        const char *files;
        
        memcpy(buffer, loc, len);
        memcpy(buffer + len, ":files", 7);
        files = iniparser_getstring(d, buffer, NULL);
        if (files) {
            char path[PATH_MAX];
            const char *del = strchr(files, ':');
            while (del) {
                len = del - files;
                memcpy(buffer, files, len);
                buffer[len] = '\0';
                path_join(respath, buffer, path, sizeof(path));
                pofile_read(path, handle_po, lang);
                files = del + 1;
                del = strchr(files, ':');
            }
            path_join(respath, files, path, sizeof(path));
            pofile_read(path, handle_po, lang);
        }
    }
}

static void read_config(const char *cfgfile) {
    dictionary *d;

    d = iniparser_load(cfgfile);
    if (d) {
        char buffer[8];
        const char *respath = iniparser_getstring(d, ":res", "res");
        const char *languages = iniparser_getstring(d, ":languages", "de");
        const char *del = strchr(languages, ',');
        while (del) {
            size_t len = del - languages;
            if (len < sizeof(buffer)) {
                memcpy(buffer, languages, len);
                buffer[len] = '\0';
                read_language(d, respath, buffer);
            }
            languages = del + 1;
            del = strchr(languages, ':');
        }
        read_language(d, respath, languages);
        iniparser_freedict(d);
    }
    
}

int main(int argc, char **argv) {
    const char *loc = "de";
    const char *cfgfile = "checker.ini";
    FILE * F = stdin;
    if (argc > 1) {
        const char *filename = argv[1];
        F = fopen(filename, "r");
        if (!F) {
            perror(filename);
            return -1;
        }
    }
    read_config(cfgfile);
    parsefile(F, loc);
    if (F != stdin) {
        fclose(F);
    }
    return 0;
}
