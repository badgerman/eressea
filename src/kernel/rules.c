#include <platform.h>

#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/xmlreader.h>

#include <util/gamedata.h>

#include <storage.h>

#define RULES_RACES 1
#define RULES_RC_DEF_DAMAGE 2
#define RULES_VERSION RULES_RC_DEF_DAMAGE

enum {
    TYPE_NONE,
    TYPE_RACE,
    TYPE_SHIP,
    TYPE_BUILDING,
};

int write_rules(const char *filename) {
    gamedata *data;
    const race *rc;

    data = gamedata_open(filename, "wb", RULES_VERSION);
    if (!data) {
        return -1;
    }
    for (rc = races; rc; rc = rc->next) {
        WRITE_INT(data->store, TYPE_RACE);
        write_race(data, rc);
    }
    WRITE_INT(data->store, TYPE_NONE);

    write_resources(data);
    gamedata_close(data);
    return 0;
}

int read_rules(const char *filename)
{
    gamedata *data;
    int type;

    data = gamedata_open(filename, "rb", RULES_VERSION);
    if (!data) {
        return -1;
    }
    if (data->version != RULES_VERSION) {
        return -2;
    }
    do {
        READ_INT(data->store, &type);
        if (type == TYPE_RACE) {
            read_race(data);
        }
    } while (type != TYPE_NONE);

    read_resources(data);
    gamedata_close(data);
    return 0;
}

