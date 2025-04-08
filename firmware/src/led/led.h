/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef HAPTIC_BRACELET_FIRMWARE_LED_H
#define HAPTIC_BRACELET_FIRMWARE_LED_H

#include "config_adv.h"

struct led_t;

void led_new(struct led_t **ptr, uint pin);
void led_update(struct led_t *ptr);
void led_set(struct led_t *ptr, bool value);
void led_set_pulse(struct led_t *ptr, ms_t pulse_half_period);

#endif /* HAPTIC_BRACELET_FIRMWARE_LED_H */