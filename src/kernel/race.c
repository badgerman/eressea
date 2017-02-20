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

#include <platform.h>
#include <kernel/config.h>
#include "race.h"

#include "alchemy.h"
#include "build.h"
#include "building.h"
#include "equipment.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "names.h"
#include "pathfinder.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "spell.h"
#include "terrain.h"
#include "unit.h"

/* util includes */
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/functions.h>
#include <util/gamedata.h>
#include <util/umlaut.h>
#include <util/language.h>
#include <util/log.h>
#include <util/rng.h>
#include <util/variant.h>

#include <storage.h>

/* attrib includes */
#include <attributes/raceprefix.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/** external variables **/
race *races;
int num_races = 0;
static int rc_changes = 1;

static const char *racenames[MAXRACES] = {
    "dwarf", "elf", NULL, "goblin", "human", "troll", "demon", "insect",
    "halfling", "cat", "aquarian", "orc", "snotling", "undead", NULL,
    "youngdragon", "dragon", "wyrm", "ent", "catdragon", "dracoid",
    NULL, "spell", "irongolem", "stonegolem", "shadowdemon",
    "shadowmaster", "mountainguard", "alp", "toad", "braineater", "peasant",
    "wolf", NULL, NULL, NULL, NULL, "songdragon", NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, "seaserpent",
    "shadowknight", NULL, "skeleton", "skeletonlord", "zombie",
    "juju-zombie", "ghoul", "ghast", NULL, NULL, "template",
    "clone"
};

#define MAXOPTIONS 4
typedef struct rcoption {
    unsigned char key[MAXOPTIONS];
    variant value[MAXOPTIONS];
} rcoption;

enum {
    RCO_NONE,
    RCO_SCARE,   /* races that scare and eat peasants */
    RCO_OTHER,   /* may recruit from another race */
    RCO_STAMINA, /* every n levels of stamina add +1 RC */
    RCO_HUNGER,  /* custom hunger.damage override (char *) */
    RCO_TRADELUX,
    RCO_TRADEHERB
};

static variant * rc_makeoption(race *rc, int k) {
    unsigned char key = (unsigned char)k;
    int i;
    variant *v = NULL;

    if (!rc->options) {
        rc->options = malloc(sizeof(rcoption));
        rc->options->key[0] = key;
        rc->options->key[1] = RCO_NONE;
        v = rc->options->value;
    }
    else {
        for (i = 0; !v && i < MAXOPTIONS && rc->options->key[i] != RCO_NONE; ++i) {
            if (rc->options->key[i] == key) {
                v = rc->options->value + i;
            }
        }
        if (!v) {
            assert(i<MAXOPTIONS || !"MAXOPTIONS too small for race");
            v = rc->options->value + i;
            rc->options->key[i] = key;
            if (i + 1<MAXOPTIONS) {
                rc->options->key[i + 1] = RCO_NONE;
            }
        }
    }
    assert(v);
    return v;
}

static void rc_setoption_int(race *rc, int key, int value) {
    variant *v = rc_makeoption(rc, key);
    v->i = value;
}

static void rc_setoption(race *rc, int key, const char *value) {
    variant *v = rc_makeoption(rc, key);

    if (key == RCO_SCARE) {
        v->i = atoi(value);
    }
    else if (key == RCO_STAMINA) {
        v->i = atoi(value);
    }
    else if (key == RCO_OTHER) {
        v->v = rc_get_or_create(value);
    }
    else if (key == RCO_HUNGER) {
        v->v = strdup(value);
    }
    else if (key == RCO_TRADEHERB) {
        v->i = atoi(value);
    }
    else if (key == RCO_TRADELUX) {
        v->i = atoi(value);
    }
}

static variant *rc_getoption(const race *rc, int key) {
    if (rc->options) {
        int i;
        for (i=0;i!=MAXOPTIONS && rc->options->key[i]!=RCO_NONE;++i) {
            if (rc->options->key[i]==key) {
                return rc->options->value+i;
            }
        }
    }
    return NULL;
}

const struct race *findrace(const char *s, const struct locale *lang)
{
    void **tokens = get_translations(lang, UT_RACES);
    variant token;

    assert(lang);
    if (tokens && findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
        return (const struct race *)token.v;
    }
    return NULL;
}

const struct race *get_race(race_t rt) {
    const char * name;

    assert(rt < MAXRACES);
    name = racenames[rt];
    if (!name) {
        return NULL;
    }
    return rc_find(name);
}

typedef struct xref {
    race_t id;
    const race *rc;
} rc_xref;

int cmp_xref(const void *a, const void *b)
{
    const rc_xref *l = (const rc_xref *)a;
    const rc_xref *r = (const rc_xref *)b;
    if (l->rc<r->rc) return -1;
    if (l->rc>r->rc) return 1;
    return 0;
}

static rc_xref *xrefs;
race_t old_race(const struct race * rc)
{
    static int cache;
    int i, l, r;
    
    if (rc_changed(&cache)) {
        if (!xrefs) {
            xrefs = malloc(sizeof(rc_xref) * MAXRACES);
        }
        for (i = 0; i != MAXRACES; ++i) {
            xrefs[i].rc = get_race(i);
            xrefs[i].id = (race_t)i;
        }
        qsort(xrefs, MAXRACES, sizeof(rc_xref), cmp_xref);
    }
    l=0; r=MAXRACES-1;
    while (l<=r) {
        int m = (l+r)/2;
        if (rc<xrefs[m].rc) {
            r = m-1;
        } else if (rc>xrefs[m].rc) {
            l = m+1;
        } else {
            return (race_t)xrefs[m].id;
        }
    }
    return NORACE;
}

race_list *get_familiarraces(void)
{
    static int init = 0;
    static race_list *familiarraces;

    if (!init) {
        race *rc = races;
        for (; rc != NULL; rc = rc->next) {
            if (rc->flags & RCF_FAMILIAR) {
                racelist_insert(&familiarraces, rc);
            }
        }
        init = false;
    }
    return familiarraces;
}

void racelist_clear(struct race_list **rl)
{
    while (*rl) {
        race_list *rl2 = (*rl)->next;
        free(*rl);
        *rl = rl2;
    }
}

void racelist_insert(struct race_list **rl, const struct race *r)
{
    race_list *rl2 = (race_list *)malloc(sizeof(race_list));

    rl2->data = r;
    rl2->next = *rl;

    *rl = rl2;
}

void free_races(void) {
    while (races) {
        int i;
        race * rc = races->next;
        rcoption * opt = races->options;
        
        if (opt) {
            for (i=0;i!=MAXOPTIONS && opt->key[i]!=RCO_NONE;++i) {
                if (opt->key[i]==RCO_HUNGER) {
                    free(opt->value[i].v);
                }
            }
            free(opt);
        }
        for (i = 0; races->attack[i].type!=AT_NONE; ++i) {
            att *at = races->attack + i;
            if (at->type == AT_SPELL) {
                spellref_free(at->data.spell.ref);
            }
            else {
                free(at->data.dice);
            }
        }
        free(xrefs);
        xrefs = 0;
        free(races->_name);
        free(races->def_damage);
        free(races);
        races = rc;
    }
    num_races = 0;
    ++rc_changes;
}

static race *rc_find_i(const char *name)
{
    const char *rname = name;
    race *rc = races;

    while (rc && strcmp(rname, rc->_name) != 0) {
        rc = rc->next;
    }
    if (!rc) {
        const char *rc_depr[] = { "uruk", "orc", "illusion", "template", NULL };
        int i;
        for (i = 0; rc_depr[i]; i += 2) {
            if (strcmp(name, rc_depr[i]) == 0) {
                rc = rc_find_i(rc_depr[i + 1]);
                log_warning("a reference was made to the retired race '%s', returning '%s'.", name, rc->_name);
                break;
            }
        }
    }
    return rc;
}

const race * rc_find(const char *name) {
    return rc_find_i(name);
}

bool rc_changed(int *cache) {
    assert(cache);
    if (*cache != rc_changes) {
        *cache = rc_changes;
        return true;
    }
    return false;
}

race *rc_create(const char *zName)
{
    race *rc;
    int i;
    char zText[64];

    assert(zName);
    rc = (race *)calloc(sizeof(race), 1);
    rc->hitpoints = 1;
    rc->weight = PERSON_WEIGHT;
    rc->capacity = 540;
    rc->income = 20;
    rc->recruit_multi = 1.0F;
    rc->regaura = 1.0F;
    rc->speed = 1.0F;
    rc->battle_flags = 0;
    if (strchr(zName, ' ') != NULL) {
        log_error("race '%s' has an invalid name. remove spaces\n", zName);
        assert(strchr(zName, ' ') == NULL);
    }
    rc->_name = strdup(zName);

    rc->attack[0].type = AT_COMBATSPELL;
    for (i = 1; i < RACE_ATTACKS; ++i)
        rc->attack[i].type = AT_NONE;
    rc->index = num_races++;
    ++rc_changes;
    rc->next = races;

    snprintf(zText, sizeof(zText), "age_%s", zName);
    rc->age = (void(*) (struct unit *))get_function(zText);
    return races = rc;
}

race *rc_get_or_create(const char *zName)
{
    race *rc;

    assert(zName);
    rc = rc_find_i(zName);
    return rc ? rc : rc_create(zName);
}

/** dragon movement **/
bool allowed_dragon(const region * src, const region * target)
{
    if (fval(src->terrain, ARCTIC_REGION) && fval(target->terrain, SEA_REGION))
        return false;
    return allowed_fly(src, target);
}

bool r_insectstalled(const region * r)
{
    return fval(r->terrain, ARCTIC_REGION);
}

double rc_magres(const race *rc) {
    return rc->magres / 100.0;
}

double rc_maxaura(const race *rc) {
    return rc->maxaura / 100.0;
}

const char * rc_hungerdamage(const race *rc)
{
    variant *v = rc_getoption(rc, RCO_HUNGER);
    return v ? (const char *)v->v : NULL;
}

int rc_armor_bonus(const race *rc)
{
    variant *v = rc_getoption(rc, RCO_STAMINA);
    return v ? v->i : 0;
}

int rc_scare(const struct race *rc)
{
    variant *v = rc_getoption(rc, RCO_SCARE);
    return v ? v->i : 0;
}

int rc_luxury_trade(const struct race *rc)
{
    if (rc) {
        variant *v = rc_getoption(rc, RCO_TRADELUX);
        if (v) return v->i;
    }
    return 1000;
}

int rc_herb_trade(const struct race *rc)
{
    if (rc) {
        variant *v = rc_getoption(rc, RCO_TRADEHERB);
        if (v) return v->i;
    }
    return 500;
}

const race *rc_otherrace(const race *rc)
{
    variant *v = rc_getoption(rc, RCO_OTHER);
    return v ? (const race *)v->v : NULL;
}

int rc_migrants_formula(const race *rc)
{
    return (rc->flags&RCF_MIGRANTS) ? MIGRANTS_LOG10 : MIGRANTS_NONE;
}

void rc_set_param(struct race *rc, const char *key, const char *value) {
    if (strcmp(key, "recruit_multi") == 0) {
        rc->recruit_multi = atof(value);
    }
    else if (strcmp(key, "migrants.formula") == 0) {
        if (value[0] == '1') {
            rc->flags |= RCF_MIGRANTS;
        }
    }
    else if (strcmp(key, "other_race")==0) {
        rc_setoption(rc, RCO_OTHER, value);
    }
    else if (strcmp(key, "ai.scare")==0) {
        rc_setoption(rc, RCO_SCARE, value);
    }
    else if (strcmp(key, "hunger.damage")==0) {
        rc_setoption(rc, RCO_HUNGER, value);
    }
    else if (strcmp(key, "armor.stamina")==0) {
        rc_setoption(rc, RCO_STAMINA, value);
    }
    else if (strcmp(key, "luxury_trade")==0) {
        rc_setoption(rc, RCO_TRADELUX, value);
    }
    else if (strcmp(key, "herb_trade")==0) {
        rc_setoption(rc, RCO_TRADEHERB, value);
    }
    else {
        log_error("unknown property for race %s: %s=%s", rc->_name, key, value);
    }
}

const char* rc_name(const race * rc, name_t n, char *name, size_t size) {
    const char * postfix = 0;
    if (!rc) {
        return NULL;
    }
    switch (n) {
    case NAME_SINGULAR: postfix = ""; break;
    case NAME_PLURAL: postfix = "_p"; break;
    case NAME_DEFINITIVE: postfix = "_d"; break;
    case NAME_CATEGORY: postfix = "_x"; break;
    default: assert(!"invalid name_t enum in rc_name_s");
    }
    if (postfix) {
        snprintf(name, size, "race::%s%s", rc->_name, postfix);
        return name;
    }
    return NULL;
}

const char *rc_name_s(const race * rc, name_t n)
{
    static char name[64];  /* FIXME: static return value */
    return rc_name(rc, n, name, sizeof(name));
}

const char *raceprefix(const unit * u)
{
    attrib *asource = u->faction->attribs;

    if (fval(u, UFL_GROUP)) {
        attrib *agroup = a_find(u->attribs, &at_group);
        if (agroup != NULL)
            asource = ((const group *)(agroup->data.v))->attribs;
    }
    return get_prefix(asource);
}

const char *racename(const struct locale *loc, const unit * u, const race * rc)
{
    const char *str, *prefix = raceprefix(u);

    if (prefix != NULL) {
        static char lbuf[80]; /* FIXME: static return value */
        char *bufp = lbuf;
        size_t size = sizeof(lbuf) - 1;
        int ch, bytes;

        bytes = (int)strlcpy(bufp, LOC(loc, mkname("prefix", prefix)), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();

        bytes = (int)strlcpy(bufp, LOC(loc, rc_name_s(rc, u->number != 1)), size);
        assert(~bufp[0] & 0x80 || !"unicode/not implemented");
        ch = tolower(*(unsigned char *)bufp);
        bufp[0] = (char)ch;
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        *bufp = 0;

        return lbuf;
    }
    str = LOC(loc, rc_name_s(rc, (u->number == 1) ? NAME_SINGULAR : NAME_PLURAL));
    return str ? str : rc->_name;
}

void write_race_reference(const race * rc, struct storage *store)
{
    WRITE_TOK(store, rc ? rc->_name : "none");
}

race *read_race_reference(struct storage *store)
{
    char zName[20];
    READ_TOK(store, zName, sizeof(zName));

    if (strcmp(zName, "none") != 0) {
        race *rc = rc_get_or_create(zName);
        return rc;
    }
    return NULL;
}

void register_race_name_function(race_name_func func, const char *name) {
    register_function((pf_generic)func, name);
}

void write_race(gamedata *data, const race *rc)
{
    int i;

    WRITE_TOK(data->store, rc->_name);
    WRITE_TOK(data->store, rc->def_damage);
    WRITE_INT(data->store, rc->flags);
    WRITE_INT(data->store, rc->battle_flags);
    WRITE_INT(data->store, rc->ec_flags);
    WRITE_INT(data->store, rc->magres);
    WRITE_INT(data->store, rc->healing);
    WRITE_INT(data->store, rc->maxaura);
    WRITE_INT(data->store, rc->recruitcost);
    WRITE_INT(data->store, rc->maintenance);
    WRITE_INT(data->store, rc->splitsize);
    WRITE_INT(data->store, rc->weight);
    WRITE_INT(data->store, rc->capacity);
    WRITE_INT(data->store, rc->income);
    WRITE_INT(data->store, rc->hitpoints);
    WRITE_INT(data->store, rc->armor);
    WRITE_INT(data->store, rc->at_default);
    WRITE_INT(data->store, rc->df_default);
    WRITE_INT(data->store, rc->at_bonus);
    WRITE_INT(data->store, rc->df_bonus);
    WRITE_FLT(data->store, (float)rc->regaura);
    WRITE_FLT(data->store, (float)rc->recruit_multi);
    WRITE_FLT(data->store, rc->speed);

    for (i = 0; i != RACE_ATTACKS; ++i) {
        const att *a = rc->attack + i;
        WRITE_INT(data->store, a->type);
        if (a->type == AT_NONE) break;
        if (a->type == AT_SPELL) {
            WRITE_INT(data->store, a->data.spell.level);
            WRITE_TOK(data->store, a->data.spell.ref->name);
        }
        else {
            WRITE_TOK(data->store, a->data.dice);
        }
        WRITE_INT(data->store, a->flags);
    }

    for (i = 0; i != MAXSKILLS; ++i) {
        WRITE_INT(data->store, rc->bonus[i]);
    }

    if (rc->study_speed) {
        for (i = 0; i != MAXSKILLS; ++i) {
            if (rc->study_speed[i] != 0) {
                WRITE_INT(data->store, i);
                WRITE_INT(data->store, rc->study_speed[i]);
            }
        }
    }
    WRITE_INT(data->store, NOSKILL);

    if (rc->options) {
        for (i = 0; i != MAXOPTIONS; ++i) {
            int key = rc->options->key[i];
            if (key == RCO_NONE) {
                break;
            }
            WRITE_INT(data->store, key);
            switch (key) {
            case RCO_OTHER:
                write_race_reference((const race *)rc->options->value[i].v, data->store);
                break;
            case RCO_HUNGER:
                WRITE_TOK(data->store, (const char *)rc->options->value[i].v);
                break;
            case RCO_SCARE:
            case RCO_STAMINA:
            case RCO_TRADELUX:
            case RCO_TRADEHERB:
                WRITE_INT(data->store, rc->options->value[i].i);
                break;
            default:
                log_error("race.option %d not implemented", key);
            }
        }
    }
    WRITE_INT(data->store, RCO_NONE);

    for (i = 0; i != MAXMAGIETYP; ++i) {
        write_race_reference(rc->familiars[i], data->store);
    }
}

struct race * read_race(struct gamedata *data)
{
    race * rc;
    float flt;
    int i;
    char zName[64];

    READ_TOK(data->store, zName, sizeof(zName));
    rc = rc_get_or_create(zName);
    READ_TOK(data->store, zName, sizeof(zName));
    rc->def_damage = strdup(zName);
    READ_INT(data->store, &rc->flags);
    READ_INT(data->store, &rc->battle_flags);
    READ_INT(data->store, &rc->ec_flags);
    READ_INT(data->store, &rc->magres);
    READ_INT(data->store, &rc->healing);
    READ_INT(data->store, &rc->maxaura);
    READ_INT(data->store, &rc->recruitcost);
    READ_INT(data->store, &rc->maintenance);
    READ_INT(data->store, &rc->splitsize);
    READ_INT(data->store, &rc->weight);
    READ_INT(data->store, &rc->capacity);
    READ_INT(data->store, &rc->income);
    READ_INT(data->store, &rc->hitpoints);
    READ_INT(data->store, &rc->armor);
    READ_INT(data->store, &rc->at_default);
    READ_INT(data->store, &rc->df_default);
    READ_INT(data->store, &rc->at_bonus);
    READ_INT(data->store, &rc->df_bonus);
    READ_FLT(data->store, &flt);
    rc->regaura = flt;
    READ_FLT(data->store, &flt);
    rc->recruit_multi = flt;
    READ_FLT(data->store, &flt);
    rc->speed = flt;

    for (i = 0; i != RACE_ATTACKS; ++i) {
        att *a = rc->attack + i;
        READ_INT(data->store, &a->type);
        if (a->type == AT_NONE) break;
        if (a->type == AT_SPELL) {
            READ_INT(data->store, &a->data.spell.level);
            READ_TOK(data->store, zName, sizeof(zName));
            a->data.spell.ref = spellref_create(NULL, zName);
        }
        else {
            READ_TOK(data->store, zName, sizeof(zName));
            a->data.dice = strdup(zName);
        }
        READ_INT(data->store, &a->flags);
    }

    for (i = 0; i != MAXSKILLS; ++i) {
        int b;
        READ_INT(data->store, &b);
        rc->bonus[i] = (char)b;
    }

    READ_INT(data->store, &i);
    if (i!=NOSKILL) {
        rc->study_speed = malloc(MAXSKILLS);
        do {
            int m;
            READ_INT(data->store, &m);
            rc->study_speed[i] = (char)m;
            READ_INT(data->store, &i);
        } while (i != NOSKILL);
    }

    READ_INT(data->store, &i);
    if (i != RCO_NONE) {
        do {
            int m;
            variant *v;

            switch (i) {
            case RCO_SCARE:
            case RCO_STAMINA:
            case RCO_TRADELUX:
            case RCO_TRADEHERB:
                READ_INT(data->store, &m);
                rc_setoption_int(rc, i, m);
                break;
            case RCO_OTHER:
                v = rc_makeoption(rc, i);
                v->v = read_race_reference(data->store);
                break;
            case RCO_HUNGER:
                READ_TOK(data->store, zName, sizeof(zName));
                rc_setoption(rc, i, zName);
                break;
            }
            READ_INT(data->store, &i);
        } while (i != RCO_NONE);
    }

    for (i = 0; i != MAXMAGIETYP; ++i) {
        rc->familiars[i] = read_race_reference(data->store);
    }

    return rc;
}
