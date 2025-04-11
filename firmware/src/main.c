/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

// External Libraries
#include <limits.h>
#include <stdio.h>
#include "../../lib/nanoprintf.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include "pico/time.h"

// Config Files
#include "config.h"
#include "config_adv.h"

// Internal Libraries
#include "analog.h"
#include "digital.h"
#include "led.h"
#include "motor.h"

struct bracelet_t {
	// On board
	struct led_t     *status_led;
	struct digital_t *button_pair;

	// Motor
	struct motor_t   *motor;

	// Aux
	struct digital_t *aux_connected;
	struct digital_t *button_aux;
	struct analog_t  *radial_aux;
};

static inline void bracelet_init(struct bracelet_t *ptr, struct motor_parameters_t motor_parameters)
{
	ptr->status_led    = NULL;
	ptr->button_pair   = NULL;

	ptr->motor         = NULL;

	ptr->aux_connected = NULL;
	ptr->button_aux    = NULL;

	stdio_init_all();
	adc_init();
	sleep_ms(3000);
	fflush(stdout);

	PRINTF("[%6ju] Init led\n", (uintmax_t)ms_now());
	led_new(&(ptr->status_led), PIN_LED);
	led_set(ptr->status_led, true);

	PRINTF("[%6ju] Init pair button\n", (uintmax_t)ms_now());
	digital_new(&(ptr->button_pair), PIN_PAIR,        low_is_false);

	PRINTF("[%6ju] Init motor\n", (uintmax_t)ms_now());
	motor_new(&(ptr->motor), PIN_MOTOR_A1, PIN_MOTOR_A2, PIN_MOTOR_FAULT);
	motor_set_parameters(ptr->motor, motor_parameters);

	PRINTF("[%6ju] Init aux\n", (uintmax_t)ms_now());
	digital_new(&(ptr->aux_connected), PIN_AUX_DETECT,  low_is_false);
	digital_new(&(ptr->button_aux),    PIN_AUX_DIGITAL, low_is_false);
	analog_new( &(ptr->radial_aux),    PIN_AUX_ANALOG,  ADC_CHANNEL_AUX_ANALOG);

	PRINTF("[%6ju] Init done\n", (uintmax_t)ms_now());
}

static inline void calibrate__brake_ms_max(struct bracelet_t *bracelet)
{
	PRINTF("Calibration #2: brake_ms_max\n");
	struct motor_parameters_t parameters = {
		.reverse_denominator = 1,
		.reverse_ms_max = 0,
		.brake_denominator = 1,
		.brake_ms_max = 150
	};
	motor_set_parameters(bracelet->motor, parameters);

	while (!digital_trap(bracelet->button_pair));

	int pulses = 0;
	while (parameters.brake_ms_max > 10) {
		if (pulses == 0) {
			sleep_ms(1000);
			parameters.brake_ms_max -= 10;
			motor_set_parameters(bracelet->motor, parameters);
			PRINTF("brake ms %lu\n", parameters.brake_ms_max);
			pulses = 5;
		}

		if (motor_get_state(bracelet->motor) == motor_asleep) {
			motor_pulse(bracelet->motor, 1000);
			pulses--;
		}
	}
	return;
}

static inline void calibrate__reverse_ms_max(struct bracelet_t *bracelet)
{
	PRINTF("Calibration #3: reverse_ms_max\n");
	struct motor_parameters_t parameters = {
		.reverse_denominator = 1,
		.reverse_ms_max = 20,
		.brake_denominator = 1,
		.brake_ms_max = 150
	};
	motor_set_parameters(bracelet->motor, parameters);

	while (!digital_trap(bracelet->button_pair));

	int pulses = 0;
	while (parameters.reverse_ms_max > 2) {
		if (pulses == 0) {
			sleep_ms(1000);
			parameters.reverse_ms_max -= 2;
			motor_set_parameters(bracelet->motor, parameters);
			PRINTF("reverse ms %lu\n", parameters.reverse_ms_max);
			pulses = 5;
		}

		if (motor_get_state(bracelet->motor) == motor_asleep) {
			motor_pulse(bracelet->motor, 1000);
			pulses--;
		}
	}
	return;
}

static inline void calibrate_denominator(struct bracelet_t *bracelet, struct motor_parameters_t parameters)
{
	PRINTF("Calibration #4: denominator\n");
	while (!digital_trap(bracelet->button_pair));

	PRINTF("brake\trev\tms\t#");
	for (parameters.brake_denominator = 6; parameters.brake_denominator > 2; parameters.brake_denominator--) {
		for (parameters.reverse_denominator = 7; parameters.reverse_denominator > 3; parameters.reverse_denominator--) {
			for (ms_t duration = 100; duration > 10; duration -= 10) {
				for (int pulses = 5; pulses > 0; pulses--) {
					motor_set_parameters(bracelet->motor, parameters);
					PRINTF("%lu\t%lu\t%lu\t%d\n", parameters.brake_denominator, parameters.reverse_denominator, duration, pulses);
					motor_pulse(bracelet->motor, duration);
					while (motor_get_state(bracelet->motor) != motor_asleep);
				}
				sleep_ms(1000);
			}
		}
	}
	return;
}

static inline void test_pulse(struct bracelet_t *bracelet)
{
	int pulses = 0;
	while (true) {
		if (digital_active(bracelet->button_pair)) {
			PRINTF("+20 pulses\n");
			pulses = 20;
		}
		if (pulses > 0 && motor_get_state(bracelet->motor) == motor_asleep) {
			motor_pulse(bracelet->motor, 30);
			pulses--;
		}
	}
	return;
}

/*
 * test_battery:
 *
 * For pico-only battery life testing, simply unplug the motor.
 * It could be done in software. I implemented it with a compile flag, but ultimately decided
 * against it. Unplugging the motor on hardware is so simple and easy.
 */

static inline void test_battery(struct bracelet_t *bracelet)
{
	while (!digital_trap(bracelet->button_pair));

	int pulses = 0;
	while (true) {
		if (motor_get_state(bracelet->motor) != motor_asleep)
			continue;
		
		if (pulses == 0) {
			sleep_ms(1000);
			pulses = 2;
		}

		if (pulses > 0) {
			motor_pulse(bracelet->motor, 30);
			pulses--;
		}
	}

}

struct bracelet_t bracelet = {
	.button_pair = NULL,
	.button_aux = NULL,
	.motor = NULL
};

bool timer_callback(__unused repeating_timer_t *rt)
{
	// On Board
	led_update(bracelet.status_led);
	digital_update(bracelet.button_pair);

	// Motor
	motor_update(bracelet.motor);

	// Aux
	digital_update(bracelet.aux_connected);

	if (digital_now(bracelet.aux_connected)) {
		digital_update(bracelet.button_aux);
		analog_update(bracelet.radial_aux);
	}

	if (digital_active(bracelet.button_aux)) {
		motor_pulse(bracelet.motor, 30);
	}

	if (analog_active(bracelet.radial_aux, 9)) {
		motor_pulse(bracelet.motor, 30);
	}

	return true;
}

int main()
{
	struct motor_parameters_t motor_parameters = {
		.pwm = 254,
		.reverse_denominator = 5,
		.reverse_ms_max = 8,
		.brake_denominator = 3,
		.brake_ms_max = 90
	};

	// Motor tuned values
	bracelet_init(&bracelet, motor_parameters);

	repeating_timer_t timer;
	int rc = add_repeating_timer_us(1000, timer_callback, NULL, &timer);
	if (!rc)
		return 1;
	
	calibrate_denominator(&bracelet, motor_parameters);
}
