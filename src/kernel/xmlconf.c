#include <platform.h>
#include "xmlconf.h"

static void handle_start(void *userData, const XML_Char *name, const XML_Char **atts) {
}

static void handle_end(void *userData, const XML_Char *name) {
}

static void handle_text(void *userData, const XML_Char *s, int len){
}

void xml_read_config(const char *filename)
{
    XML_Parser p = XML_ParserCreate("UTF-8");
    XML_SetCharacterDataHandler(p, handle_text);
    XML_SetElementHandler(p, handle_start, handle_end);
    XML_ParserFree(p);
}
