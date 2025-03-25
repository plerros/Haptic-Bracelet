/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <limits.h>
#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#define PIN_LED         19
#define PIN_PAIR        18

#define PIN_MOTOR_A1    21
#define PIN_MOTOR_A2    20
#define PIN_MOTOR_FAULT 22

#define PIN_AUX_DETECT  14
#define PIN_AUX_DIGITAL 15
#define PIN_AUX_ANALOG  26

typedef uint32_t ms_t;
typedef uint     pwm_t;

static inline ms_t ms_now()
{
	return to_ms_since_boot(get_absolute_time());
}

enum button_types {button_normal, button_inverted};

struct button_t {
	uint pin;
	bool invert;
	bool prev;
	bool trap;
};

static inline bool button_get(struct button_t *ptr)
{
	bool ret = gpio_get(ptr->pin);
	if (ptr->invert)
		ret = !ret;
	
	if (ret == true)
		ptr->trap = ret;
	
	return ret;
}

static inline void button_init (struct button_t *ptr, uint pin, int type)
{
	ptr->pin = pin;
	gpio_init(ptr->pin);
	gpio_set_dir(ptr->pin, GPIO_IN);

	ptr->invert = false;
	if (type == button_inverted) {
		ptr->invert = true;
		gpio_pull_up(pin);
	}

	ptr->trap = false;
	ptr->prev = button_get(ptr);
}

static inline bool button_clicked (struct button_t *ptr)
{
	bool ret = false;
	bool now = button_get(ptr);
	if (now == true && ptr->prev == false)
		ret = true;

	ptr->prev = now;
	return ret;
}

static inline bool button_trap (struct button_t *ptr)
{
	button_get(ptr);
	return ptr->trap;
}

enum motor_states {motor_asleep, motor_forward, motor_reverse, motor_brake};

struct motor_t {
	// Hardware
	uint pwm_slice;
	struct button_t fault;

	pwm_t pwm;	// Running pwm
	int state;	// Current state

	// Constants
	ms_t reverse_denominator;
	ms_t reverse_ms_max;
	ms_t brake_denominator;
	ms_t brake_ms_max;

	ms_t time_next;		// Timestamp to next state
	ms_t reverse_ms;	// ms to run reverse
	ms_t brake_ms;		// ms to brake
};

static inline void motor_pwm(
	struct motor_t *ptr,
	pwm_t channel_A,
	pwm_t channel_B)
{
	if (button_trap(&(ptr->fault))) {
		channel_A = 0;
		channel_B = 0;
	}

	pwm_set_chan_level(ptr->pwm_slice, PWM_CHAN_A, channel_A);
	pwm_set_chan_level(ptr->pwm_slice, PWM_CHAN_B, channel_B);
}

static inline void motor_init(
	struct motor_t *ptr,
	uint pin_A,
	uint pin_B,
	uint pin_fault)
{

	// Initialize pwm_slice
	gpio_set_function(pin_A, GPIO_FUNC_PWM);
	gpio_set_function(pin_B, GPIO_FUNC_PWM);
	ptr->pwm_slice = pwm_gpio_to_slice_num(pin_A);

	pwm_set_wrap(ptr->pwm_slice, 255);
	pwm_set_enabled(ptr->pwm_slice, true);

	// Initialize fault pin (it's inverted)
	button_init(&(ptr->fault), pin_fault, button_inverted);

	motor_pwm(ptr, 0, 0);
	ptr->state = motor_asleep;

	ptr->time_next  = ms_now();
	ptr->reverse_ms = 0;
	ptr->brake_ms   = 0;
}

static inline void motor_update(struct motor_t *ptr)
{
	if (button_clicked(&(ptr->fault))) {
		printf("Motor driver fault detected.\n");
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
			ptr->state = motor_asleep;
			break;

		default:
			break;
	}
	//printf("%u\t%u\n", channel_A, channel_B);
	motor_pwm(ptr, channel_A, channel_B);
}

static inline void motor_pulse(struct motor_t *ptr, ms_t ms)
{
	ptr->brake_ms = ms / ptr->brake_denominator;
	if (ptr->brake_ms > ptr->brake_ms_max)
		ptr->brake_ms = ptr->brake_ms_max;
	ms -= ptr->brake_ms;

	ptr->reverse_ms = ms / ptr->reverse_denominator;
	if (ptr->reverse_ms > ptr->reverse_ms_max)
		ptr->reverse_ms = ptr->reverse_ms_max;
	ms -= ptr->reverse_ms;

	ptr->time_next = ms_now() + ms;

	motor_pwm(ptr, ptr->pwm, 0);
	ptr->state++;
}

struct bracelet_t {
	struct button_t button_pair;
	struct button_t button_aux;
	struct motor_t  motor;
};

static inline void bracelet_init (struct bracelet_t *ptr)
{
	printf("Initializing board\n");
	gpio_init(PIN_LED);
	gpio_set_dir(PIN_LED, GPIO_OUT);

	button_init(&(ptr->button_pair), PIN_PAIR,        button_normal);
	button_init(&(ptr->button_aux),  PIN_AUX_DIGITAL, button_normal);

	printf("Initializing motor driver\n");
	motor_init(&(ptr->motor), PIN_MOTOR_A1, PIN_MOTOR_A2, PIN_MOTOR_FAULT);

	printf("Initializing aux\n");
	// todo later

	printf("Initiated all\n");
}

static inline void calibrate__brake_ms_max(struct bracelet_t bracelet)
{
	printf("Calibration #2: brake_ms_max\n");
	bracelet.motor.reverse_denominator = 1;
	bracelet.motor.reverse_ms_max = 0;
	bracelet.motor.brake_denominator = 1;
	bracelet.motor.brake_ms_max = 150;

	while (!button_trap(&(bracelet.button_pair)));

	int pulses = 0;
	while (true) {
		if (bracelet.motor.brake_ms_max <= 10) {
			motor_pwm(&(bracelet.motor), 0, 0);
			break;
		}

		motor_update(&(bracelet.motor));

		if (pulses == 0) {
			sleep_ms(1000);
			bracelet.motor.brake_ms_max -= 10;
			printf("brake ms %u\n", bracelet.motor.brake_ms_max);
			pulses = 5;
		}

		if (bracelet.motor.state == motor_asleep) {
			motor_pulse(&(bracelet.motor), 1000);
			pulses--;
		}
	}
	return;
}

static inline void calibrate__reverse_ms_max(struct bracelet_t bracelet)
{
	printf("Calibration #3: reverse_ms_max\n");
	bracelet.motor.reverse_denominator = 1;
	bracelet.motor.reverse_ms_max = 20;
	bracelet.motor.brake_denominator = 1;
	bracelet.motor.brake_ms_max = 150;

	while (!button_trap(&(bracelet.button_pair)));

	int pulses = 0;
	while (true) {
		if (bracelet.motor.reverse_ms_max <= 2) {
			motor_pwm(&(bracelet.motor), 0, 0);
			break;
		}

		motor_update(&(bracelet.motor));

		if (pulses == 0) {
			sleep_ms(1000);
			bracelet.motor.reverse_ms_max -= 2;
			printf("reverse ms %u\n", bracelet.motor.reverse_ms_max);
			pulses = 5;
		}

		if (bracelet.motor.state == motor_asleep) {
			motor_pulse(&(bracelet.motor), 1000);
			pulses--;
		}
	}
	return;
}

static inline void calibrate_denominator(struct bracelet_t bracelet)
{
	printf("Calibration #4: denominator\n");
	bracelet.motor.reverse_denominator = 5;
	bracelet.motor.brake_denominator = 5;

	while (!button_trap(&(bracelet.button_pair)));

	printf("brake\trev\tms\t#");
	for (bracelet.motor.brake_denominator = 6; bracelet.motor.brake_denominator > 2; bracelet.motor.brake_denominator--) {
		for (bracelet.motor.reverse_denominator = 7; bracelet.motor.reverse_denominator > 3; bracelet.motor.reverse_denominator--) {
			for (ms_t duration = 100; duration > 10; duration -= 10) {
				for (int pulses = 5; pulses > 0; pulses--) {
					printf("%u\t%u\t%u\t%d\n", bracelet.motor.brake_denominator, bracelet.motor.reverse_denominator, duration, pulses);
					motor_pulse(&(bracelet.motor), duration);
					while (bracelet.motor.state != motor_asleep) {
						motor_update(&(bracelet.motor));
					}
				}
				sleep_ms(1000);
			}
		}
	}
	return;
}

static inline void test_pulse(struct bracelet_t bracelet)
{
	int pulses = 0;
	while (true) {
		motor_update(&(bracelet.motor));

		if (button_clicked(&(bracelet.button_pair))) {
			printf("+20 pulses\n");
			pulses = 20;
		}

		if (pulses > 0 && bracelet.motor.state == motor_asleep) {
			motor_pulse(&(bracelet.motor), 30);
			pulses--;
		}
	}
	return;
}

int main()
{
	stdio_init_all();
	sleep_ms(1000);

	// Motor tuned values
	struct bracelet_t bracelet = {
		.motor = {
			.pwm = 254,
			.reverse_denominator = 5,
			.reverse_ms_max = 8,
			.brake_denominator = 3,
			.brake_ms_max = 90
		}
	};

	bracelet_init(&bracelet);
	test_pulse(bracelet);
}
