/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "kill_aura.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned tests, failures;
static void check(bool condition, const char *description)
{
    ++tests;
    printf("%s %s\n", condition ? "PASS" : "FAIL", description);
    if (!condition) ++failures;
}

int main(void)
{
    KaConfig config = {true, true, 10};
    KaVec3 player = {0, 0, 0};
    KaEntity entity = {"Monster001", {300, 400, 0}};
    check(ka_evaluate(&config, &player, &entity) == KA_MONSTER_MATCH,
          "monster inside 3D radius is selected");
    entity.position_cm = (KaVec3){1000, 0, 0};
    check(ka_evaluate(&config, &player, &entity) == KA_NONE,
          "monster exactly at radius is excluded");
    entity.position_cm.x = 999.99;
    check(ka_evaluate(&config, &player, &entity) == KA_MONSTER_MATCH,
          "monster just inside radius is selected");
    entity.position_cm.x = 1000.01;
    check(ka_evaluate(&config, &player, &entity) == KA_NONE,
          "monster just outside radius is excluded");
    entity.position_cm = (KaVec3){0, 0, 1100};
    check(ka_evaluate(&config, &player, &entity) == KA_NONE,
          "vertical distance is included");
    player = (KaVec3){1000, 2000, 3000};
    entity.position_cm = (KaVec3){1300, 2400, 3000};
    check(ka_evaluate(&config, &player, &entity) == KA_MONSTER_MATCH,
          "player position is respected");
    config.radius_m = 5;
    check(ka_evaluate(&config, &player, &entity) == KA_NONE,
          "metres are converted to centimetres before comparison");
    config.radius_m = 10; config.monsters_enabled = false;
    check(ka_evaluate(&config, &player, &entity) == KA_NONE,
          "disabled monster rule produces no action");
    config.monsters_enabled = true;
    const char *const ignored[] = {"Npc001", "Player001", "monster001", "", "Monste"};
    for (size_t i = 0; i < sizeof(ignored)/sizeof(ignored[0]); ++i) {
        entity.blueprint_type = ignored[i];
        check(ka_evaluate(&config, &player, &entity) == KA_NONE,
              "non-matching or differently cased blueprint is ignored");
    }
    entity.blueprint_type = "Monster";
    check(ka_evaluate(&config, &player, &entity) == KA_MONSTER_MATCH,
          "prefix matching also accepts the exact Monster string");
    const char *const animals[] = {"Animal001", "Animal002", "Animal003", "Animal004", "Animal012"};
    const char *const drops[] = {"Animal006", "Animal005", "Animal035"};
    const char *const unused[] = {"Animal013", "Animal014", "Animal015", "Animal033", "Animal999"};
    entity.position_cm = (KaVec3){1000000, 1000000, 1000000};
    for (size_t i = 0; i < sizeof(animals)/sizeof(animals[0]); ++i) {
        entity.blueprint_type = animals[i];
        check(ka_evaluate(&config, &player, &entity) == KA_ANIMAL_MATCH,
              "listed small animal is selected without a radius gate");
    }
    for (size_t i = 0; i < sizeof(drops)/sizeof(drops[0]); ++i) {
        entity.blueprint_type = drops[i];
        check(ka_evaluate(&config, &player, &entity) == KA_DROP_MATCH,
              "listed drop animal selects the separate action");
    }
    for (size_t i = 0; i < sizeof(unused)/sizeof(unused[0]); ++i) {
        entity.blueprint_type = unused[i];
        check(ka_evaluate(&config, &player, &entity) == KA_NONE,
              "unused large-animal list and unknown IDs remain inactive");
    }
    entity.blueprint_type = "Animal001"; config.animals_enabled = false;
    check(ka_evaluate(&config, &player, &entity) == KA_NONE,
          "disabled animal rule produces no action");
    config.animals_enabled = true;
    check(ka_evaluate(&config, NULL, &entity) == KA_ANIMAL_MATCH,
          "animal evaluation does not require player position");
    entity.blueprint_type = "Monster001";
    check(ka_evaluate(&config, NULL, &entity) == KA_NONE,
          "missing player position cannot match a monster");
    check(ka_evaluate(NULL, &player, &entity) == KA_NONE, "null config is handled");
    check(ka_evaluate(&config, &player, NULL) == KA_NONE, "null entity is handled");
    entity.blueprint_type = NULL;
    check(ka_evaluate(&config, &player, &entity) == KA_NONE, "null blueprint is handled");
    entity.blueprint_type = "Monster001"; entity.position_cm = player;
    config.radius_m = 0;
    check(ka_evaluate(&config, &player, &entity) == KA_NONE, "zero radius is strict");
    config.radius_m = -1;
    check(ka_evaluate(&config, &player, &entity) == KA_NONE, "negative radius matches nothing");
    config.radius_m = NAN;
    check(ka_evaluate(&config, &player, &entity) == KA_NONE, "NaN radius matches nothing");
    config.radius_m = 10; entity.position_cm.x = NAN;
    check(ka_evaluate(&config, &player, &entity) == KA_NONE, "NaN distance matches nothing");
    check(strcmp(ka_action_name(KA_DROP_MATCH), "drop_match") == 0,
          "action naming is stable for fixture comparison");
    printf("RESULT tests=%u failures=%u\n", tests, failures);
    return failures ? 1 : 0;
}
