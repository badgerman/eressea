#include <platform.h>

#include <kernel/race.h>
#include <kernel/xmlreader.h>

#include <util/gamedata.h>

#include <storage.h>

#define RULES_RACES 1
#define RULES_VERSION RULES_RACES

enum {
    TYPE_NONE,
    TYPE_RACE,
    TYPE_SHIP,
    TYPE_BUILDING,
};

void write_rules(const char *filename) {
    gamedata *data;
    const race *rc;

    data = gamedata_open(filename, "wb", RULES_VERSION);
    for (rc = races; rc; rc = rc->next) {
        WRITE_INT(data->store, TYPE_RACE);
        write_race(data, rc);
    }
    WRITE_INT(data->store, TYPE_NONE);
    gamedata_close(data);
}

void read_rules(const char *filename)
{
}

