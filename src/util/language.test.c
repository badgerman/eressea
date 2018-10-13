#include <platform.h>
#include "language.h"

#include <CuTest.h>
#include <tests.h>

extern const char *directions[];

static void test_language(CuTest *tc)
{
    const char *str;
    test_setup();
    default_locale = test_create_locale();
    str = directions[1];
    CuAssertStrEquals(tc, str, locale_getstring(default_locale, str));
    test_teardown();
}

static void test_set_string(CuTest *tc)
{
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("strings");
    CuAssertIntEquals(tc, SETSTRING_OK, locale_setstring(lang, "foo", "bar"));
    CuAssertStrEquals(tc, "bar", locale_getstring(default_locale, "foo"));
    CuAssertIntEquals(tc, SETSTRING_DUPLICATE, locale_setstring(lang, "foo", "bar"));
    CuAssertStrEquals(tc, "bar", locale_getstring(default_locale, "foo"));
    CuAssertIntEquals(tc, SETSTRING_CONFLICT, locale_setstring(lang, "foo", "BAR"));
    CuAssertStrEquals(tc, "BAR", locale_getstring(default_locale, "foo"));
    test_teardown();
}

static void test_make_locales(CuTest *tc)
{
    test_setup();
    make_locales("aa,bb,cc");
    CuAssertPtrNotNull(tc, get_locale("aa"));
    CuAssertPtrNotNull(tc, get_locale("bb"));
    CuAssertPtrNotNull(tc, get_locale("cc"));
    test_teardown();
}

CuSuite *get_language_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_language);
    SUITE_ADD_TEST(suite, test_set_string);
    SUITE_ADD_TEST(suite, test_make_locales);
    return suite;
}
