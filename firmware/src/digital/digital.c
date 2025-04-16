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
#include "digital.h"

struct digital_t {
	uint pin;
	bool invert;
	volatile bool prev;
	volatile ms_t held_since;

	// External
	volatile bool trap;
	volatile bool went_true;
	volatile bool went_false;
	volatile ms_t held_for;
};

void digital_new(struct digital_t **ptr, uint pin, int type)
{
	// TODO error check ptr

	struct digital_t *new = malloc(sizeof(struct digital_t));
	if (new == NULL) {
		// error
	}

	new->pin = pin;
	gpio_init(new->pin);
	gpio_set_dir(new->pin, GPIO_IN);

	new->invert = false;
	if (type == low_is_true) {
		new->invert = true;
		gpio_pull_up(pin);
	}

	new->prev = false;
	new->trap = false;
	new->went_true = false;
	new->went_false = false;
	new->held_for = 0;

	*ptr = new;
}

void digital_free(struct digital_t *ptr)
{
	free(ptr);
}

bool digital_now(struct digital_t *ptr)
{
	bool now = gpio_get(ptr->pin);
	if (ptr->invert)
		now = !now;
	return now;
}

void digital_update(struct digital_t *ptr)
{
	bool now = digital_now(ptr);

	// Update trap
	if (now == true)
		ptr->trap = now;

	// If we transition {false -> true}
	if (ptr->prev == false && now == true) {
		ptr->went_true = true;
		ptr->held_since = ms_now();
	}

	// If we transition {true -> false}
	if (ptr->prev == true && now == false) {
		ptr->went_false = true;
		ms_t tmp = ms_now() - ptr->held_since;
		if (tmp >= 1000)
			ptr->held_for = tmp;
	}

	ptr->prev = now;
}

bool digital_trap(struct digital_t *ptr)
{
	return ptr->trap;
}

bool digital_went_true(struct digital_t *ptr)
{
	bool ret = ptr->went_true;
	ptr->went_true = false;
	return ret;
}

bool digital_went_false(struct digital_t *ptr)
{
	bool ret = ptr->went_false;
	ptr->went_false = false;
	return ret;
}

ms_t digital_ms(struct digital_t *ptr)
{
	ms_t ret = ptr->held_for;
	ptr->held_for = 0;
	return ret;
}