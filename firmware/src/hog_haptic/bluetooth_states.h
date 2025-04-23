#ifndef HAPTIC_BRACELET_FIRMWARE_BLUETOOTH_STATES_H
#define HAPTIC_BRACELET_FIRMWARE_BLUETOOTH_STATES_H

enum bluetooth_states {
	bt_disconnected   = 0,
	bt_connected      = 1,
	bt_vibrate_ms_min = 10 // values greater than 10 are interpreted as milliseconds to vibrate
};


#endif /* HAPTIC_BRACELET_FIRMWARE_BLUETOOTH_STATES_H */