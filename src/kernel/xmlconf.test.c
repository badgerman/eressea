#include <platform.h>
#include "xmlconf.h"

#include <CuTest.h>
#include <tests.h>

#include <string.h>

static void test_xmlconf(CuTest *tc) {
    const char *xml = "<eressea>"
        "<strings>"
        "<string name=\"groot\">"
        "<text locale=\"de\">Ich bin Groot</text>"
        "<text locale=\"en\">I am groot</text>"
        "</string>"
        "<xi:include href=\"config://default/more.xml\"/>"
        "</strings>"
        "</eressea>";
    test_setup();
    CuAssertIntEquals(tc, 0, xmlconf_parse(xml, strlen(xml)));
    test_cleanup();
}

CuSuite *get_xmlconf_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_xmlconf);
    return suite;
}

