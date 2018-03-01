#include <platform.h>
#include "market.h"
#include "tests.h"

#include <kernel/building.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/language.h>
#include <util/message.h>

#include <CuTest.h>
#include <tests.h>

#include <stdlib.h>
#include <string.h>

static void test_market_curse(CuTest * tc)
{
    region *r;
    building *b;
    unit *u;
    faction *f;
    int x, y;
    const terrain_type *terrain;
    item_type *htype, *ltype;
    luxury_type *lux;
    building_type *btype;

    test_setup();
    mt_register(mt_new_va("buyamount", "unit:unit", "amount:int", "resource:resource", MT_NEW_END));

    htype = test_create_itemtype("herb");
    htype->flags |= ITF_HERB;
    htype->rtype->flags |= (RTF_ITEM | RTF_POOLED);

    ltype = test_create_itemtype("balm");
    ltype->rtype->flags |= (RTF_ITEM | RTF_POOLED);
    lux = new_luxurytype(ltype, 0);

    config_set("rules.region_owners", "1");

    btype = bt_get_or_create("market");

    terrain = get_terrain("plain");

    for (x = 0; x != 3; ++x) {
        for (y = 0; y != 3; ++y) {
            r = findregion(x, y);
            if (!r) {
                r = test_create_region(x, y, terrain);
            }
            else {
                terraform_region(r, terrain);
            }
            rsetpeasants(r, 5000);
            r_setdemand(r, lux, 0);
            rsetherbtype(r, htype);
        }
    }
    r = findregion(1, 1);
    b = test_create_building(r, btype);
    b->flags |= BLD_MAINTAINED;
    b->size = b->type->maxsize;

    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    u_set_building(u, b);

    do_markets();

    CuAssertIntEquals(tc, 70, i_get(u->items, htype));
    CuAssertIntEquals(tc, 35, i_get(u->items, ltype));
    test_teardown();
}

static void test_rc_trade(CuTest *tc) {
    race *rc;
    test_setup();
    rc = test_create_race("human");
    CuAssertIntEquals(tc, 1000, rc_luxury_trade(rc));
    CuAssertIntEquals(tc, 500, rc_herb_trade(rc));
    rc_set_param(rc, "luxury_trade", "100");
    rc_set_param(rc, "herb_trade", "50");
    CuAssertIntEquals(tc, 100, rc_luxury_trade(rc));
    CuAssertIntEquals(tc, 50, rc_herb_trade(rc));
    test_teardown();
}

CuSuite *get_market_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_market_curse);
    SUITE_ADD_TEST(suite, test_rc_trade);
    return suite;
}
