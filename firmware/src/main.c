/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <limits.h>
#include <stdio.h>
#include "../../lib/nanoprintf.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "config.h"
#include "config_adv.h"

#include "analog.h"
#include "digital.h"
#include "motor.h"

struct bracelet_t {
	struct digital_t *button_pair;
	struct digital_t *button_aux;
	struct motor_t   *motor;
};

static inline void bracelet_init(struct bracelet_t *ptr, struct motor_parameters_t motor_parameters)
{
	PRINTF("Initializing board\n");
	gpio_init(PIN_LED);
	gpio_set_dir(PIN_LED, GPIO_OUT);

	digital_new(&(ptr->button_pair), PIN_PAIR,        low_is_false);
	digital_new(&(ptr->button_aux),  PIN_AUX_DIGITAL, low_is_false);

	PRINTF("Initializing motor driver\n");
	motor_new(&(ptr->motor), PIN_MOTOR_A1, PIN_MOTOR_A2, PIN_MOTOR_FAULT);
	motor_set_parameters(ptr->motor, motor_parameters);

	PRINTF("Initializing aux\n");
	// todo later

	PRINTF("Initiated all\n");
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
	digital_update(bracelet.button_pair);
	digital_update(bracelet.button_aux);
	motor_update(bracelet.motor);
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

	stdio_init_all();
	sleep_ms(1000);

	// Motor tuned values
	bracelet_init(&bracelet, motor_parameters);

	repeating_timer_t timer;
	int rc = add_repeating_timer_us(1000, timer_callback, NULL, &timer);
	if (!rc)
		return 1;
	
	calibrate_denominator(&bracelet, motor_parameters);
}
