#pragma once

#include <stddef.h>

struct ParseInfo;

typedef int (*xmlconf_include)(struct ParseInfo *info, const char *href);

int xmlconf_read(const char *filename);
int xmlconf_parse(const char *text, size_t len);

int xmlconf_set_include_handler(xmlconf_include h);
