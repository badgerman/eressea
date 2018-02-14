/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef ERESSEA_H
#define ERESSEA_H

#ifdef __cplusplus
extern "C" {
#endif

    /* this should always be the first thing included after platform.h */
#include <stddef.h>
#include <stdbool.h>
#include "types.h"

    struct param;
    struct _dictionary_;

#define DISPLAYSIZE         8192        /* max. L�nge einer Beschreibung, incl trailing 0 */
#define ORDERSIZE           (DISPLAYSIZE*2) /* max. length of an order */
#define NAMESIZE            128 /* max. L�nge eines Namens, incl trailing 0 */
#define IDSIZE              16  /* max. L�nge einer no (als String), incl trailing 0 */
#define OBJECTIDSIZE        (NAMESIZE+5+IDSIZE) /* max. L�nge der Strings, die
     * von struct unitname, etc. zur�ckgegeben werden. ohne die 0 */

#define fval(u, i) ((u)->flags & (i))
#define fset(u, i) ((u)->flags |= (i))
#define freset(u, i) ((u)->flags &= ~(i))

    int findoption(const char *s, const struct locale *lang);

    param_t findparam(const char *s, const struct locale *lang);
    param_t findparam_block(const char *s, const struct locale *lang, bool any_locale);
    param_t findparam_ex(const char *s, const struct locale * lang);
    bool isparam(const char *s, const struct locale * lang, param_t param);
    param_t getparam(const struct locale *lang);

    const char * game_name(void);
    const char * game_mailcmd(void);
    int game_id(void);
    /* returns a value between [0..xpct_2], generated with two dice */

    void init_locale(struct locale *lang);
    void init_races(struct locale *lang);

    int forbiddenid(int id);
    int newcontainerid(void);

    bool rule_region_owners(void);
    bool rule_stealth_other(void); /* units can pretend to be another faction, TARNE PARTEI <no> */
    bool rule_stealth_anon(void);  /* units can anonymize their faction, TARNE PARTEI [NICHT] */
    int rule_alliance_limit(void);
    int rule_faction_limit(void);
#define HARVEST_WORK  0x00
#define HARVEST_TAXES 0x01
    int rule_blessed_harvest(void);
#define GIVE_SELF 1
#define GIVE_PEASANTS 2
#define GIVE_LUXURIES 4
#define GIVE_HERBS 8
#define GIVE_GOODS 16
#define GIVE_ONDEATH 32
#define GIVE_ALLITEMS (GIVE_GOODS|GIVE_HERBS|GIVE_LUXURIES)
#define GIVE_DEFAULT (GIVE_SELF|GIVE_PEASANTS|GIVE_LUXURIES|GIVE_HERBS|GIVE_GOODS)
    int rule_give(void);

    /* grammatik-flags: */
#define GF_NONE 0
    /* singular, ohne was dran */
#define GF_PLURAL 1
    /* Angaben in Mehrzahl */
#define GF_ARTICLE 8
    /* der, die, eine */
#define GF_SPECIFIC 16
    /* der, die, das vs. ein, eine */
#define GF_DETAILED 32
    /* mehr Informationen. z.b. stra�e zu 50% */
#define GF_PURE 64
    /* untranslated */

    const char *datapath(void);
    void set_datapath(const char *path);

    const char *basepath(void);
    void set_basepath(const char *);

    const char *reportpath(void);
    void set_reportpath(const char *);

    int create_directories(void);

    void kernel_init(void);
    void kernel_done(void);

    /* globale settings des Spieles */
    typedef struct settings {
        struct attrib *attribs;
        void *vm_state;
    } settings;

    void set_param(struct param **p, const char *key, const char *value);
    const char *get_param(const struct param *p, const char *key);
    int get_param_int(const struct param *p, const char *key, int def);
    int check_param(const struct param *p, const char *key, const char *searchvalue);
    double get_param_flt(const struct param *p, const char *key, double def);
    void free_params(struct param **pp);

    void config_set(const char *key, const char *value);
    void config_set_int(const char *key, int value);
    void config_set_from(const struct _dictionary_ *d, const char *keys[]);
    const char *config_get(const char *key);
    int config_get_int(const char *key, int def);
    double config_get_flt(const char *key, double def);
    bool config_token(const char *key, const char *tok);
    bool config_changed(int *cache_key);

    char * join_path(const char *p1, const char *p2, char *dst, size_t len);

    struct order *default_order(const struct locale *lang);

    void init_parameters(struct locale *lang);

    void free_gamedata(void);
    void free_config(void);

    extern const char *parameters[];
    extern settings global;

#ifdef __cplusplus
}
#endif
#endif
