/*
 * Copyright (c) 2025 Pierro Zachareas
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#ifndef HAPTIC_BRACELET_BLUETOOTH
#define HAPTIC_BRACELET_BLUETOOTH

#include <stdbool.h>

struct bt_data_t {
	bool volatile _Atomic connected;
	int  volatile _Atomic command1;
	int  volatile _Atomic command2;
};

int btstack_main(struct bt_data_t *data);

#endif /* HAPTIC_BRACELET_BLUETOOTH */