/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef HAPTIC_BRACELET_CONFIG_ADV_H
#define HAPTIC_BRACELET_CONFIG_ADV_H

typedef uint     pwm_t;
typedef uint32_t ms_t;
typedef uint16_t adc_t;

#define ADC_MAX ((1 << 12) - 1)
#define ADC_PERCENT (ADC_MAX / 100)

static inline ms_t ms_now()
{
	return to_ms_since_boot(get_absolute_time());
}

#endif /* HAPTIC_BRACELET_CONFIG_ADV_H */