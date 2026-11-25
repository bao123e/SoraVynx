/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef KILL_AURA_RULES_H
#define KILL_AURA_RULES_H

#include <stdbool.h>

typedef struct { double x, y, z; } KaVec3;

typedef struct {
    bool monsters_enabled;
    bool animals_enabled;
    double radius_m;
} KaConfig;

typedef struct {
    const char *blueprint_type;
    KaVec3 position_cm;
} KaEntity;

typedef enum {
    KA_NONE = 0,
    KA_MONSTER_MATCH,
    KA_ANIMAL_MATCH,
    KA_DROP_MATCH
} KaAction;

/* Pure evaluation of caller-owned snapshots. This does not modify entities,
 * call a game API, or send a request. Coordinates are in centimetres.
 * As in the source, the radius check is strictly '<', not '<='.
 */
bool ka_within_radius(const KaVec3 *player_cm, const KaEntity *entity,
                      double radius_m);

/* The animal rules in the source have no distance requirement. A player
 * position is therefore optional for animal classification.
 */
KaAction ka_evaluate(const KaConfig *config, const KaVec3 *player_cm,
                     const KaEntity *entity);

const char *ka_action_name(KaAction action);

#endif
