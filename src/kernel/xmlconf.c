#include <platform.h>
#include "xmlconf.h"

#include <kernel/race.h>
#include <kernel/spell.h>
#include <util/log.h>
#include <expat.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAXSTACK 16
typedef struct ParseInfo {
    int sp;
    union {
        struct {
            race *rc;
            int attacks;
        } race;
    } parent;
    XML_Char *stack[MAXSTACK];
} ParseInfo;

int get_attr_index(const XML_Char **atts, const XML_Char *key) {
    int i;
    for (i = 0; atts[i]; i += 2) {
        if (strcmp(atts[i], key) == 0) {
            return i;
        }
    }
    return -1;
}

const XML_Char *get_attr_value(const XML_Char **atts, const XML_Char *key) {
    int i = get_attr_index(atts, key);
    return (i >= 0) ? atts[i+1] : NULL;
}

static bool parse_int(int * val, const XML_Char** atts, const XML_Char* name, int i) {
    if (strcmp(atts[i], name) == 0) {
        assert(val);
        *val = atoi(atts[i + 1]);
        return true;
    }
    return false;
}

static bool parse_float(float * val, const XML_Char** atts, const XML_Char* name, int i) {
    if (strcmp(atts[i], name) == 0) {
        assert(val);
        *val = (float)atof(atts[i + 1]);
        return true;
    }
    return false;
}

static bool parse_double(double * val, const XML_Char** atts, const XML_Char* name, int i) {
    if (strcmp(atts[i], name) == 0) {
        assert(val);
        *val = atof(atts[i + 1]);
        return true;
    }
    return false;
}

static void parse_attack(ParseInfo *pi, const XML_Char **atts) {
    race *rc = pi->parent.race.rc;
    int a = pi->parent.race.attacks;
    int type = AT_NONE, i, t;
    t = get_attr_index(atts, "type");
    if (t >= 0) {
        type = atoi(atts[t + 1]);
    }
    assert(type > AT_NONE && type <= AT_STRUCTURAL);
    rc->attack[a].type = type;
    for (i = 0; atts[i]; i += 2) {
        if (i != t) {
            const XML_Char *key = atts[i];
            const XML_Char *value = atts[i + 1];
            switch (type) {
            case AT_STANDARD:
            case AT_NATURAL:
            case AT_STRUCTURAL:
            case AT_DRAIN_ST:
            case AT_DRAIN_EXP:
                if (strcmp(key, "damage") == 0) {
                    rc->attack[a].data.dice = strdup(value);
                }
                else {
                    log_error("invalid attribute %s=%s for attack type %d.", key, value, type);
                }
                break;
            case AT_DAZZLE:
                log_error("invalid attribute %s=%s for attack type %d.", key, value, type);
                break;
            case AT_SPELL:
                if (strcmp(key, "spell") == 0) {
                    rc->attack[a].data.spell.ref = spellref_create(NULL, value);
                }
                else if (strcmp(key, "level") == 0) {
                    rc->attack[a].data.spell.level = atoi(value);
                }
                else {
                    log_error("invalid attribute %s=%s for attack type %d.", key, value, type);
                }
                break;
            case AT_COMBATSPELL:
            default:
                log_error("invalid attack type %d.", type);
                break;
            }
        }
    }
    ++pi->parent.race.attacks;
}

static void parse_race(ParseInfo *pi, const XML_Char **atts) {
    int n = get_attr_index(atts, "name");
    const XML_Char *name = atts[n+1];
    if (name) {
        int i;
        race *rc;
        rc = rc_get_or_create(name);
        for (i = 0; atts[i]; i += 2) {
            if (i != n) {
                if (
                    !parse_int(&rc->recruitcost, atts, "recruitcost", i) &&
                    !parse_int(&rc->maintenance, atts, "maintenance", i) &&
                    !parse_int(&rc->weight, atts, "weight", i) &&
                    !parse_float(&rc->magres, atts, "magres", i) &&
                    !parse_float(&rc->healing, atts, "healing", i) &&
                    !parse_float(&rc->speed, atts, "speed", i) &&
                    !parse_double(&rc->maxaura, atts, "maxaura", i) &&
                    !parse_double(&rc->regaura, atts, "regaura", i)
                ) {
                    const XML_Char *key = atts[i];
                    const XML_Char *value = atts[i + 1];
                    log_error("invalid attribute %s=%s for race %s.", key, value, name);
                }
            }
        }
        pi->parent.race.rc = rc;
        pi->parent.race.attacks = 0;
    }
}

static void handle_include(ParseInfo *pi, const XML_Char **atts) {
    const XML_Char *href = get_attr_value(atts, "href");
    if (href) {
        (void)href;
    }
}

static void handle_start(void *userData, const XML_Char *name, const XML_Char **atts) {
    ParseInfo *pi = (ParseInfo *)userData;
    assert(pi && pi->sp<MAXSTACK);
    if (strcmp(name, "xi:include") == 0) {
        handle_include(pi, atts);
    }
    else if (pi->sp == 2) {
        if (strcmp(name, "race") == 0) {
            parse_race(pi, atts);
        }
    }
    else if (pi->sp == 3) {
        if (strcmp(name, "attack") == 0) {
            parse_attack(pi, atts);
        }
    }
    pi->stack[pi->sp++] = strdup(name);
}

static void handle_end(void *userData, const XML_Char *name) {
    ParseInfo *pi = (ParseInfo *)userData;
    assert(pi && pi->sp > 0);

    free(pi->stack[--pi->sp]);
    if (pi->sp == 2) {
        if (strcmp(name, "race") == 0) {
            pi->parent.race.rc->attack[pi->parent.race.attacks].type = AT_NONE;
            pi->parent.race.rc = NULL;
        }
    }
}

static void handle_text(void *userData, const XML_Char *s, int len){
    ParseInfo *pi = (ParseInfo *)userData;
    assert(pi);
}

static void xmlconf_init(XML_Parser p, ParseInfo *pi) {
    XML_SetUserData(p, pi);
    XML_SetCharacterDataHandler(p, handle_text);
    XML_SetElementHandler(p, handle_start, handle_end);
}

int xmlconf_read(const char *filename)
{
    FILE * F;
    XML_Parser p = XML_ParserCreate("UTF-8");
    enum XML_Status err = XML_STATUS_OK;

    ParseInfo info = { 0 };
    xmlconf_init(p, &info);

    F = fopen(filename, "r");
    if (F) {
        while (!feof(F) && err!=XML_STATUS_ERROR) {
            char text[128];
            size_t len;
            len = fread(text, 1, sizeof(text), F);
            err = XML_Parse(p, text, len, len == 0);
        }
        fclose(F);
    }

    XML_ParserFree(p);
    return err != XML_STATUS_OK;
}

int xmlconf_parse(const char *text, size_t len)
{
    XML_Parser p = XML_ParserCreate("UTF-8");
    enum XML_Status err;

    ParseInfo info = { 0 };
    xmlconf_init(p, &info);

    err = XML_Parse(p, text, len, XML_TRUE);

    if (err != XML_STATUS_OK) {
        enum XML_Error ec;
        ec = XML_GetErrorCode(p);
        log_error(XML_ErrorString(ec));
    }
    XML_ParserFree(p);
    return err != XML_STATUS_OK;
}
