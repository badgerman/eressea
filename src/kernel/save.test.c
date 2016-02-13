#include <platform.h>
#include <kernel/config.h>
#include <util/attrib.h>
#include <util/gamedata.h>
#include <attributes/key.h>

#include "save.h"
#include "unit.h"
#include "faction.h"
#include "version.h"
#include <storage.h>
#include <memstream.h>
#include <CuTest.h>
#include <tests.h>

#include <stdio.h>

static void test_readwrite_data(CuTest * tc)
{
    const char *filename = "test.dat";
    char path[MAX_PATH];
    test_cleanup();
    CuAssertIntEquals(tc, 0, writegame(filename));
    CuAssertIntEquals(tc, 0, readgame(filename, false));
    CuAssertIntEquals(tc, RELEASE_VERSION, global.data_version);
    join_path(datapath(), filename, path, sizeof(path));
    CuAssertIntEquals(tc, 0, remove(path));
    test_cleanup();
}

static void test_readwrite_unit(CuTest * tc)
{
    gamedata data;
    storage store;
    struct unit *u;
    struct region *r;
    struct faction *f;
    int fno;

    test_cleanup();
    r = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    fno = f->no;
    u = test_create_unit(f, r);

    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    write_unit(&data, u);
    
    data.strm.api->rewind(data.strm.handle);
    free_gamedata();
    f = test_create_faction(0);
    renumber_faction(f, fno);
    gamedata_init(&data, &store, RELEASE_VERSION);
    u = read_unit(&data);
    mstream_done(&data.strm);
    gamedata_done(&data);

    CuAssertPtrNotNull(tc, u);
    CuAssertPtrEquals(tc, f, u->faction);
    CuAssertPtrEquals(tc, 0, u->region);
    test_cleanup();
}

static void test_readwrite_attrib(CuTest *tc) {
    gamedata data;
    storage store;
    attrib *a = NULL;

    test_cleanup();
    key_set(&a, 41);
    key_set(&a, 42);
    mstream_init(&data.strm);
    gamedata_init(&data, &store, RELEASE_VERSION);
    global.data_version = RELEASE_VERSION; // FIXME: hack!
    write_attribs(data.store, a, NULL);
    a_removeall(&a, NULL);
    CuAssertPtrEquals(tc, 0, a);

    data.strm.api->rewind(data.strm.handle);
    read_attribs(&data, &a, NULL);
    mstream_done(&data.strm);
    gamedata_done(&data);
    CuAssertTrue(tc, key_get(a, 41));
    CuAssertTrue(tc, key_get(a, 42));
    a_removeall(&a, NULL);

    test_cleanup();
}

CuSuite *get_save_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_readwrite_attrib);
    SUITE_ADD_TEST(suite, test_readwrite_data);
    SUITE_ADD_TEST(suite, test_readwrite_unit);
    return suite;
}
