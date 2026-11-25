/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "kill_aura.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static bool contains(const char *value, const char *const *list, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(value, list[i]) == 0) return true;
    }
    return false;
}

bool ka_within_radius(const KaVec3 *player_cm, const KaEntity *entity,
                      double radius_m)
{
    if (player_cm == NULL || entity == NULL) return false;
    const double dx = entity->position_cm.x - player_cm->x;
    const double dy = entity->position_cm.y - player_cm->y;
    const double dz = entity->position_cm.z - player_cm->z;
    const double distance_cm = sqrt(dx * dx + dy * dy + dz * dz);
    return distance_cm < radius_m * 100.0;
}

KaAction ka_evaluate(const KaConfig *config, const KaVec3 *player_cm,
                     const KaEntity *entity)
{
    static const char *const animals[] = {
        "Animal001", "Animal002", "Animal003", "Animal004", "Animal012"
    };
    static const char *const drops[] = {
        "Animal006", "Animal005", "Animal035"
    };

    if (config == NULL || entity == NULL || entity->blueprint_type == NULL)
        return KA_NONE;

    if (config->monsters_enabled &&
        strncmp(entity->blueprint_type, "Monster", 7) == 0 &&
        ka_within_radius(player_cm, entity, config->radius_m))
        return KA_MONSTER_MATCH;

    if (config->animals_enabled) {
        if (contains(entity->blueprint_type, animals,
                     sizeof(animals) / sizeof(animals[0])))
            return KA_ANIMAL_MATCH;
        if (contains(entity->blueprint_type, drops,
                     sizeof(drops) / sizeof(drops[0])))
            return KA_DROP_MATCH;
    }
    return KA_NONE;
}

const char *ka_action_name(KaAction action)
{
    switch (action) {
    case KA_MONSTER_MATCH: return "monster_match";
    case KA_ANIMAL_MATCH: return "animal_match";
    case KA_DROP_MATCH: return "drop_match";
    default: return "none";
    }
}
