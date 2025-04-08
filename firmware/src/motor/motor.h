/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef HAPTIC_BRACELET_FIRMWARE_MOTOR_H
#define HAPTIC_BRACELET_FIRMWARE_MOTOR_H

#include "config_adv.h"
enum motor_states {motor_asleep, motor_forward, motor_reverse, motor_brake};

struct motor_t;
struct motor_parameters_t {
	uint pwm;
	ms_t reverse_denominator;
	ms_t reverse_ms_max;
	ms_t brake_denominator;
	ms_t brake_ms_max;
};

void motor_new(
	struct motor_t **ptr,
	uint pin_motorA_1,
	uint pin_motorA_2,
	uint pin_fault);

void motor_free(struct motor_t *ptr);
int  motor_get_state(struct motor_t *ptr);
void motor_set_parameters(struct motor_t *ptr, struct motor_parameters_t parameters);

void motor_update(struct motor_t *ptr);
void motor_pulse(struct motor_t *ptr, ms_t ms);

#endif /* HAPTIC_BRACELET_FIRMWARE_MOTOR_H */