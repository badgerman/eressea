/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef MY_LOCALE_H
#define MY_LOCALE_H

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAXLOCALES 3

    struct locale;
    struct critbit_tree;

    extern const char *localenames[];
    extern struct locale *default_locale;
    extern struct locale *locales;
    extern struct locale *nextlocale(const struct locale *lang);

    /** managing multiple locales: **/
    struct locale *get_locale(const char *name);
    struct locale *get_or_create_locale(const char *key);
    typedef void(*locale_handler)(struct locale *lang);
    void init_locales(locale_handler callback);
    void free_locales(void);
    void reset_locales(void);

    /** operations on locales: **/
    enum setstring_t {
        SETSTRING_OK,
        SETSTRING_NO_MEMORY,
        SETSTRING_DUPLICATE,
        SETSTRING_CONFLICT,
    };
    enum setstring_t locale_setstring(struct locale *lang, const char *key,
        const char *value);
    void locale_setstring_depr(struct locale *lang, const char *key,
        const char *value);
    const char *locale_getstring(const struct locale *lang,
        const char *key);
    const char *locale_string(const struct locale *lang, const char *key, bool warn); /* does fallback */
    unsigned int locale_index(const struct locale *lang);
    const char *locale_name(const struct locale *lang);

    const char *mkname(const char *namespc, const char *key);
    char *mkname_buf(const char *namespc, const char *key, char *buffer);

    void make_locales(const char *str);

    void po_write_msg(FILE *F, const char *id, const char *str, const char *ctxt);

#define LOC(lang, s) (lang?locale_string(lang, s, true):s)

    enum {
        UT_PARAMS,
        UT_KEYWORDS,
        UT_SKILLS,
        UT_RACES,
        UT_OPTIONS,
        UT_DIRECTIONS,
        UT_ARCHETYPES,
        UT_MAGIC,
        UT_TERRAINS,
        UT_SPECDIR,
        UT_MAX
    };

    void ** get_translations(const struct locale *lang, int type);
    void * get_translation(const struct locale *lang, const char *str, int type);
    void init_translations(const struct locale *lang, int ut, const char * (*string_cb)(int i), int maxstrings);
    void add_translation(struct critbit_tree **cb, const char *str, int i);

#ifdef __cplusplus
}
#endif
#endif
