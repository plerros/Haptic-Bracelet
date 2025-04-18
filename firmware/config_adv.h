/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef HAPTIC_BRACELET_CONFIG_ADV_H
#define HAPTIC_BRACELET_CONFIG_ADV_H

#include "config.h"

typedef uint     pwm_t;
typedef uint32_t ms_t;
typedef uint64_t us_t;
typedef uint16_t adc_t;

typedef uint     percent_t;

#define ADC_MAX ((1 << 12) - 1)
#define ADC_PERCENT (ADC_MAX / 100)

static inline ms_t ms_now()
{
	return to_ms_since_boot(get_absolute_time());
}

static inline us_t us_now()
{
	return to_us_since_boot(get_absolute_time());
}

#if ANALOG_AVERAGING_WINDOW < 1
#error ANALOG_AVERAGING_WINDOW must be >= 1
#endif

#endif /* HAPTIC_BRACELET_CONFIG_ADV_H */