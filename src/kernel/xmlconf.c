#include <platform.h>
#include "xmlconf.h"

#include <expat.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAXSTACK 16
typedef struct ParseInfo {
    int sp;
    XML_Char *stack[MAXSTACK];
} ParseInfo;

static void handle_start(void *userData, const XML_Char *name, const XML_Char **atts) {
    ParseInfo *pi = (ParseInfo *)userData;
    assert(pi && pi->sp<MAXSTACK);
    pi->stack[pi->sp++] = strdup(name);
}

static void handle_end(void *userData, const XML_Char *name) {
    ParseInfo *pi = (ParseInfo *)userData;
    assert(pi);
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

    XML_ParserFree(p);
    return err != XML_STATUS_OK;
}
