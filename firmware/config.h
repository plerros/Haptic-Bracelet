/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef HAPTIC_BRACELET_CONFIG_H
#define HAPTIC_BRACELET_CONFIG_H
 
// Pins
#define PIN_LED         19
#define PIN_PAIR        18

#define PIN_MOTOR_A1    21
#define PIN_MOTOR_A2    20
#define PIN_MOTOR_FAULT 22

#define PIN_AUX_DETECT  14
#define PIN_AUX_DIGITAL 15
#define PIN_AUX_ANALOG  26

#define ADC_CHANNEL_AUX_ANALOG 0

/*
 * PRINTF
 *
 * Can be either: 'printf' or 'nanoprintf'
 */
#define PRINTF nanoprintf

#define ANALOG_AVERAGING_WINDOW 64

#endif /* HAPTIC_BRACELET_CONFIG_H */