#include <platform.h>

#include <kernel/race.h>
#include <kernel/xmlreader.h>

#include <races/races.h>

#include <util/gamedata.h>
#include <util/xml.h>

#include <storage.h>

#define RULES_RACES 1
#define RULES_VERSION RULES_RACES

enum {
    TYPE_NONE,
    TYPE_RACE,
    TYPE_SHIP,
    TYPE_BUILDING,
};

int main(int argc, char **argv) {
    const char * xmlfile, *catalog;
    gamedata *data;
    const race *rc;

    register_races();
    register_xmlreader();

    if (argc < 3) return -1;
    xmlfile = argv[1];
    catalog = argv[2];
    read_xml(xmlfile, catalog);

    data = gamedata_open("rules.dat", "wb", RULES_VERSION);
    for (rc = races; rc; rc = rc->next) {
        WRITE_INT(data->store, TYPE_RACE);
        write_race(data, rc);
    }
    WRITE_INT(data->store, TYPE_NONE);
    gamedata_close(data);
    return 0;
}
