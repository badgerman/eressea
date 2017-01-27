#pragma once

#include <stddef.h>

int xmlconf_read(const char *filename);
int xmlconf_parse(const char *text, size_t len);
