/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <stdlib.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/malloc.h"

#include "config_adv.h"
#include "analog.h"

enum analog_prev {ap_low, ap_high, ap_size};

struct analog_t {
	uint pin;
	uint adc_id;
	adc_t onepercent;
	volatile adc_t prev[ap_size];

	// External
};

void analog_new(struct analog_t **ptr, uint pin, uint adc_id, adc_t max)
{
	// TODO error check ptr
	struct analog_t *new = malloc(sizeof(struct analog_t));
	if (new == NULL) {
		// error
	}

	new->pin = pin;
	adc_gpio_init(new->pin);

	new->adc_id = adc_id;
	adc_select_input(new->adc_id);
	adc_t now = adc_read();

	new->onepercent = max/100;

	new->prev[ap_low]  = now;
	new->prev[ap_high] = now;

	*ptr = new;
}

void analof_free(struct analog_t *ptr)
{
	free(ptr);
}

void analog_update(struct analog_t *ptr)
{
	adc_select_input(ptr->adc_id);
	adc_t now = adc_read();

	if (now < ptr->prev[ap_low])
		ptr->prev[ap_low] = now;
	
	if (now > ptr->prev[ap_high])
		ptr->prev[ap_high] = now;
}

bool analog_active(struct analog_t *ptr, adc_t threshold_percent)
{
	// Cap the input
	if (threshold_percent > 100)
		threshold_percent = 100;

	adc_t current_percent = ptr->prev[ap_high] - ptr->prev[ap_low];
	current_percent /= ptr->onepercent;

	if (current_percent < threshold_percent)
		return false;

	adc_select_input(ptr->adc_id);
	adc_t now = adc_read();

	ptr->prev[ap_low]  = now;
	ptr->prev[ap_high] = now;

	return true;
}