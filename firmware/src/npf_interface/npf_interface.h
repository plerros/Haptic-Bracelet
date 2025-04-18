/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef HAPTIC_BRACELET_NPF_INTERFACE_H
#define HAPTIC_BRACELET_NPF_INTERFACE_H

#include <stdarg.h>

int npf_vsnprintf(char *buffer, size_t bufsz, char const *format, va_list vlist);
int nanoprintf(const char *str, ...);

#endif /* HAPTIC_BRACELET_NPF_INTERFACE_H */