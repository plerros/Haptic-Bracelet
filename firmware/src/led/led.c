/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/malloc.h"

#include "config_adv.h"
#include "led.h"

struct led_t {
	uint  pin;
	volatile bool state; // true == ON, false == OFF
	volatile ms_t state_since;

	// External
	volatile bool pulse_mode;
	volatile ms_t pulse_half_period;
};
#include <stdio.h>

void led_new(struct led_t **ptr, uint pin)
{
	struct led_t *new = malloc(sizeof(struct led_t));
	if (new == NULL) {
		// error
	}

	new->pin = pin;
	new->state = false;
	new->state_since = ms_now();
	new->pulse_mode = false;
	new->pulse_half_period = 0;

	gpio_init(new->pin);
	gpio_set_dir(new->pin, GPIO_OUT);
	(*ptr) = new;
}

static inline void led_set_internal(struct led_t *ptr, bool value)
{
	ms_t now = ms_now();
	gpio_put(ptr->pin, value);
	ptr->state = value;
	ptr->state_since = now;
}

void led_set(struct led_t *ptr, bool value)
{
	led_set_internal(ptr, value);
	ptr->pulse_mode = false;
}

void led_update(struct led_t *ptr)
{
	if (ptr->pulse_mode != true)
		return;
	
	ms_t now = ms_now();
	if (now >= ptr->state_since + ptr->pulse_half_period)
		led_set_internal(ptr, !ptr->state);
}

void led_set_pulse(struct led_t *ptr, ms_t pulse_half_period)
{
	ptr->pulse_mode = true;
	ptr->pulse_half_period = pulse_half_period;
}