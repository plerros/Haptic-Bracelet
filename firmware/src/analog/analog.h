/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef HAPTIC_BRACELET_FIRMWARE_ANALOG_H
#define HAPTIC_BRACELET_FIRMWARE_ANALOG_H

#include "config_adv.h"

struct analog_t;

void analog_new(struct analog_t **ptr, uint pin, uint adc_id);
void analog_update(struct analog_t *ptr);

bool analog_active(struct analog_t *ptr, adc_t threshold_percent);

/*
 * analog_active2:
 * 
 * Second activation, delta ms after the first.
 */
bool analog_active2(struct analog_t *ptr, ms_t delta);

#endif /* HAPTIC_BRACELET_FIRMWARE_ANALOG_H */