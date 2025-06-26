/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico/malloc.h"

#include "config_adv.h"

#include "digital.h"
#include "motor.h"

struct motor_t {
	uint pwm_slice;
	struct digital_t *fault;
	struct motor_parameters_t parameters;

	ms_t reverse_ms;		// ms to run reverse
	ms_t brake_ms;			// ms to brake

	volatile _Atomic int state;		// Current state
	volatile _Atomic ms_t time_next;	// Timestamp to next state
	volatile _Atomic ms_t last_activation;	// Last time update function was called
};

static inline void motor_pwm(
	struct motor_t *ptr,
	pwm_t pico_pwm_channel_A,
	pwm_t pico_pwm_channel_B)
{
	if (digital_trap(ptr->fault)) {
		pico_pwm_channel_A = 0;
		pico_pwm_channel_B = 0;
	}

	pwm_set_chan_level(ptr->pwm_slice, PWM_CHAN_A, pico_pwm_channel_A);
	pwm_set_chan_level(ptr->pwm_slice, PWM_CHAN_B, pico_pwm_channel_B);
}

void motor_new(
	struct motor_t **ptr,
	uint pin_motorA_1,
	uint pin_motorA_2,
	uint pin_fault)
{
	struct motor_t *new = malloc(sizeof(struct motor_t));
	if (new == NULL) {
		// error
	}

	// Initialize pwm_slice
	gpio_set_function(pin_motorA_1, GPIO_FUNC_PWM);
	gpio_set_function(pin_motorA_2, GPIO_FUNC_PWM);
	new->pwm_slice = pwm_gpio_to_slice_num(pin_motorA_1);
	if (new->pwm_slice != pwm_gpio_to_slice_num(pin_motorA_2)) {
		// error
	}

	pwm_set_wrap(new->pwm_slice, 255);
	pwm_set_enabled(new->pwm_slice, true);

	// Initialize fault pin (it's inverted, so low_is_true)
	new->fault = NULL;
	digital_new(&(new->fault), pin_fault, low_is_true);

	motor_pwm(new, 0, 0);
	new->state = motor_asleep;

	new->time_next  = ms_now();
	new->last_activation = 0;
	new->reverse_ms = 0;
	new->brake_ms   = 0;

	*ptr = new;
}

void motor_free(struct motor_t *ptr)
{
	free(ptr);
}

int  motor_get_state(struct motor_t *ptr)
{
	return (ptr->state);
}

void motor_set_parameters(struct motor_t *ptr, struct motor_parameters_t parameters)
{
	ptr->parameters=parameters;
}

void motor_update(struct motor_t *ptr)
{
	digital_update(ptr->fault);
	if (digital_went_true(ptr->fault)) {
		//error
	}

	ms_t now = ms_now();
	if (ptr->state == motor_asleep)
		return;
	if (now < ptr->time_next)
		return;

	pwm_t channel_A = 0;
	pwm_t channel_B = 0;

	switch (ptr->state) {
		case motor_forward:
			channel_A = 0;
			channel_B = 255;
			ptr->time_next = now + ptr->reverse_ms;
			ptr->state++;
			break;

		case motor_reverse:
			channel_A = 255;
			channel_B = 255;
			ptr->time_next = now + ptr->brake_ms;
			ptr->state++;
			break;

		case motor_brake:
			ptr->last_activation = now;
			ptr->state = motor_asleep;
			break;

		default:
			break;
	}
	motor_pwm(ptr, channel_A, channel_B);
}

void motor_pulse(struct motor_t *ptr, ms_t ms)
{
	if (ptr->state != motor_asleep)
		return;

	ptr->state++;
	// Dampen activation if multiple happen consecutively.
	ms_t now = ms_now();
	if (ms > 10 && now - ptr->last_activation < 200)
		ms = ms / 2 + 1;

	ptr->brake_ms = ms / ptr->parameters.brake_denominator;
	if (ptr->brake_ms > ptr->parameters.brake_ms_max)
		ptr->brake_ms = ptr->parameters.brake_ms_max;
	ms -= ptr->brake_ms;

	ptr->reverse_ms = ms / ptr->parameters.reverse_denominator;
	if (ptr->reverse_ms > ptr->parameters.reverse_ms_max)
		ptr->reverse_ms = ptr->parameters.reverse_ms_max;
	ms -= ptr->reverse_ms;

	ptr->time_next = now + ms;

	motor_pwm(ptr, ptr->parameters.pwm, 0);
}