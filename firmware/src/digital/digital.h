/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef HAPTIC_BRACELET_FIRMWARE_DIGITAL_H
#define HAPTIC_BRACELET_FIRMWARE_DIGITAL_H

#include "pico/stdlib.h"

#include "config_adv.h"

enum digital_io_types {low_is_false, low_is_true};

struct digital_t;

void digital_new(struct digital_t **ptr, uint pin, int type);
void digital_update(struct digital_t *ptr);

bool digital_now(struct digital_t *ptr);

/*
 * digital_trap:
 *
 * Normally false. True if signal was ever set to true.
 */
bool digital_trap(struct digital_t *ptr);

/*
 * digital_activated:
 *
 * Is there an activation of this pin that I haven't consumed?
 */
bool digital_active(struct digital_t *ptr);

ms_t digital_ms(struct digital_t *ptr);

#endif /* HAPTIC_BRACELET_FIRMWARE_DIGITAL_H */