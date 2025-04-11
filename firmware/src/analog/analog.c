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
	volatile adc_t prev[ap_size];

	// External
};

static inline adc_t analog_now(struct analog_t *ptr)
{
	adc_select_input(ptr->adc_id);
	uint32_t now = 0;
	for (int i = 0; i < 64; i++)
		now += adc_read();
	now /= 64;
	return now;
}

static inline void analog_reset(struct analog_t *ptr)
{
	adc_t now = analog_now(ptr);

	ptr->prev[ap_low]  = now;
	ptr->prev[ap_high] = now;
}

void analog_new(struct analog_t **ptr, uint pin, uint adc_id)
{
	// TODO error check ptr
	struct analog_t *new = malloc(sizeof(struct analog_t));
	if (new == NULL) {
		// error
	}

	new->pin = pin;
	adc_gpio_init(new->pin);

	new->adc_id = adc_id;
	analog_reset(new);

	*ptr = new;
}

void analog_free(struct analog_t *ptr)
{
	free(ptr);
}

void analog_update(struct analog_t *ptr)
{
	adc_t now = analog_now(ptr);

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

	adc_t current = ptr->prev[ap_high] - ptr->prev[ap_low];

	if ((current / ADC_PERCENT) < threshold_percent)
		return false;

	analog_reset(ptr);
	return true;
}