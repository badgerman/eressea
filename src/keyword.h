#ifndef H_KEYWORD_H
#define H_KEYWORD_H

#include "kernel/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct locale;

    extern const char *keywords[MAXKEYWORDS];

    keyword_t findkeyword(const char *s);
    keyword_t get_keyword(const char *s, const struct locale *lang);
    void init_keywords(const struct locale *lang);
    void init_keyword(const struct locale *lang, keyword_t kwd, const char *str);
    bool keyword_disabled(keyword_t kwd);
    void enable_keyword(keyword_t kwd, bool enabled);
    const char *keyword(keyword_t kwd);

#ifdef __cplusplus
}
#endif
#endif
