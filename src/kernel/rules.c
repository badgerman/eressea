#include <platform.h>
#include "rules.h"

#include <kernel/building.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/xmlreader.h>

#include <util/gamedata.h>

#include <storage.h>

enum {
    TYPE_NONE,
    TYPE_RACE,
    TYPE_SHIP,
    TYPE_BUILDING,
};

int write_rules(const char *filename) {
    gamedata *data;
    const race *rc;

    data = gamedata_open(filename, "wb", RELEASE_VERSION);
    if (!data) {
        return -1;
    }
    for (rc = races; rc; rc = rc->next) {
        WRITE_INT(data->store, TYPE_RACE);
        write_race(data, rc);
    }
    WRITE_INT(data->store, TYPE_NONE);

    write_resources(data);
    write_buildings(data);
    write_ships(data);
    write_spells(data);
    write_spellbooks(data);
    gamedata_close(data);
    return 0;
}

int read_rules(const char *filename)
{
    gamedata *data;
    int type;

    data = gamedata_open(filename, "rb", RELEASE_VERSION);
    if (!data) {
        return -1;
    }
    if (data->version != RELEASE_VERSION) {
        return -2;
    }
    READ_INT(data->store, &type);
    while (type != TYPE_NONE) {
        if (type == TYPE_RACE) {
            read_race(data);
        }
        READ_INT(data->store, &type);
    }

    read_resources(data);
    read_buildings(data);
    read_ships(data);
    read_spells(data);
    read_spellbooks(data);
    gamedata_close(data);
    return 0;
}
