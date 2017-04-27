#include <platform.h>


#ifdef USE_LIBXML2
#include <kernel/xmlreader.h>
#include <util/xml.h>
#endif
#include <util/log.h>

#include <kernel/building.h>
#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/rules.h>

#include <races/races.h>
#include <items/weapons.h>

#include <storage.h>

#include <string.h>
#include <stdio.h>

static int usage(void) {
    fputs("usage: convert rules config.xml catalog.xml [config.dat]\n", stderr);
    return -1;
}

int main(int argc, char **argv) {
    const char *mode;

    log_to_file(LOG_LEVELS | LOG_FLUSH | LOG_BRIEF, stderr);
    register_races();
    register_resources();
    register_weapons();
#ifdef USE_LIBXML2
    register_xmlreader();
#endif
    if (argc < 2) return usage();
    mode = argv[1];
#ifdef USE_LIBXML2
    if (strcmp(mode, "rules")==0) {
        const char *xmlfile, *catalog;
        if (argc < 4) return usage();
        xmlfile = argv[2];
        catalog = argv[3];
        read_xml(xmlfile, catalog);
        write_rules("rules.dat");
        return 0;
    } else {
        return usage();
    }
#endif
    if (strcmp(mode, "po")==0) {
        return 0;
    }
    return usage();
}
