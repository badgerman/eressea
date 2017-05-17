#include <platform.h>
#include "xmlconf.h"

#include <kernel/spell.h>
#include <util/log.h>
#include <expat.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAXSTACK 16
typedef struct ParseInfo {
    int sp;
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
    pi->stack[pi->sp++] = strdup(name);
}

static void handle_end(void *userData, const XML_Char *name) {
    ParseInfo *pi = (ParseInfo *)userData;
    assert(pi && pi->sp > 0);

    free(pi->stack[--pi->sp]);
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
