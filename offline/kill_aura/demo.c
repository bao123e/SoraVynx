/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "kill_aura.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--evaluate") == 0) {
        /* Whitespace-delimited fixture input, one snapshot per line:
         * blueprint x y z player_x player_y player_z radius_m monsters animals
         * Use '-' to represent an empty blueprint string.
         */
        for (;;) {
            char blueprint[64]; KaVec3 player; KaEntity entity;
            KaConfig config; int monsters, animals;
            const int fields = scanf("%63s %lf %lf %lf %lf %lf %lf %lf %d %d",
                blueprint, &entity.position_cm.x, &entity.position_cm.y,
                &entity.position_cm.z, &player.x, &player.y, &player.z,
                &config.radius_m, &monsters, &animals);
            if (fields == EOF) return 0;
            if (fields != 10 || (monsters != 0 && monsters != 1) ||
                (animals != 0 && animals != 1)) {
                fputs("Invalid fixture row.\n", stderr); return 2;
            }
            config.monsters_enabled = monsters != 0;
            config.animals_enabled = animals != 0;
            entity.blueprint_type = strcmp(blueprint, "-") == 0 ? "" : blueprint;
            puts(ka_action_name(ka_evaluate(&config, &player, &entity)));
        }
    }
    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--evaluate]\n", argv[0]); return 2;
    }
    const KaConfig config = {true, true, 10.0};
    const KaVec3 player = {0, 0, 0};
    const KaEntity entities[] = {
        {"MonsterNear", {300, 400, 0}},
        {"MonsterAtBoundary", {1000, 0, 0}},
        {"MonsterFar", {1500, 0, 0}},
        {"NpcGuide", {10, 0, 0}},
        {"Animal001", {100000, 0, 0}},
        {"Animal005", {0, 0, 0}},
        {"Animal013", {0, 0, 0}}
    };
    puts("Offline C rule evaluation. No game connection or entity mutation.");
    puts("Monster radius: 10 m (1000 cm); strict boundary.");
    for (size_t i = 0; i < sizeof(entities) / sizeof(entities[0]); ++i)
        printf("%-20s %s\n", entities[i].blueprint_type,
               ka_action_name(ka_evaluate(&config, &player, &entities[i])));
    return 0;
}
