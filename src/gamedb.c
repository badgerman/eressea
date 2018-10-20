#ifdef _MSC_VER
#include <platform.h>
#endif

#include "gamedb.h"

#include "kernel/config.h"
#include "kernel/calendar.h"
#include "kernel/faction.h"
#include "kernel/db/driver.h"

int gamedb_update(void)
{
    faction *f;
    int err;
    const char *dbname;

    dbname = config_get("game.dbname");

    err = db_driver_open(DB_GAME, dbname);
    if (err == 0) {
        for (f = factions; f; f = f->next) {
            int uid = db_driver_faction_save(f->uid, f->no, turn, f->email, f->_password);
            if (uid > 0) {
                f->uid = uid;
            }
        }
        db_driver_close(DB_GAME);
    }
    return err;
}