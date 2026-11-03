/* SPDX-License-Identifier: AGPL-3.0-or-later */
#pragma once
#include "api.h"
enum Feature { INSTANT_BREAK, GOD_MODE, MULTI_HIT, NO_COOLDOWN, UNLIMITED_ENERGY, MOB_VACUUM, FEATURE_COUNT };
extern volatile LONG feature_flags[FEATURE_COUNT], capabilities[FEATURE_COUNT];
extern volatile LONG multi_count, vacuum_distance;
bool initialize_backend(void);
void report_observed_calls(void);
int observed_target_count(void);
LONG total_entry_calls(void);
LONG read_atomic(volatile LONG *value);
bool install_interpreter_probe(HMODULE game);
bool install_frame_backend(HMODULE game);
extern volatile LONG event_flags[3],event_ready[3],event_hits[3];
