/*
 * Copyright (C) 2017 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

#define BTSTACK_FILE__ "hog_haptic.c"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "pico/stdlib.h"

#include "btstack.h"

#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"

#include "hog_haptic.h"

// Config Files
#include "config.h"
#include "config_adv.h"

#include "npf_interface.h"

static inline void print_timestamp()
{
	PRINTF("[%8ju] ", (uintmax_t)ms_now());
}

// TODO read USB HID Specification 1.1, Appendix B.2 and validate the following:
const uint8_t hid_descriptor_haptic[] = {
	0x05, 0x01,                   // USAGE_PAGE (Generic Desktop)
	0x09, 0x05,                   // USAGE (Game Pad)
	0xa1, 0x01,                   // COLLECTION (Application)
	0xa1, 0x02,                   //    COLLECTION (Logical)
	0x85, 0x30,                    //      REPORT_ID (48)

	0x75, 0x08,                   //      REPORT_SIZE (8)
	0x95, 0x02,                   //      REPORT_COUNT (2)
	0x05, 0x01,                   //      USAGE_PAGE (Generic Desktop)
	0x09, 0x30,                   //      USAGE (X)
	0x09, 0x31,                   //      USAGE (Y)
	0x15, 0x81,                   //      LOGICAL_MINIMUM (-127)
	0x25, 0x7f,                   //      LOGICAL_MAXIMUM (127)
	0x81, 0x02,                   //      INPUT (Data, Var, Abs)

	0x75, 0x01,                   //      REPORT_SIZE (1)
	0x95, 0x08,                   //      REPORT_COUNT (8)
	0x15, 0x00,                   //      LOGICAL_MINIMUM (0)
	0x25, 0x01,                   //      LOGICAL_MAXIMUM (1)
	0x05, 0x09,                   //      USAGE_PAGE (Button)
	0x19, 0x01,                   //      USAGE_MINIMUM (Button 1)
	0x29, 0x08,                   //      USAGE_MAXIMUM (Button 8)
	0x81, 0x02,                   //      INPUT (Data, Var, Abs)      
	0xc0,                         //   END_COLLECTION
	0xc0                          // END_COLLECTION
};

static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t l2cap_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static uint8_t battery = 100;
static hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
static uint8_t protocol_mode = 1;

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

const uint8_t adv_data[] = {
	// Flags general discoverable, BR/EDR not supported
	0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
	// Name
	0x0b, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'H', 'I', 'D', ' ', 'H', 'a', 'p', 't', 'i', 'c',
	// 16-bit Service UUIDs
	//0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
	// Appearance HID - Gamepad (Category 15, Sub-Category 4)
	0x03, BLUETOOTH_DATA_TYPE_APPEARANCE, 0xC4, 0x03,
};
const uint8_t adv_data_len = sizeof(adv_data);

static void hog_haptic_setup(void)
{
	print_timestamp();
	PRINTF("Reached hog_haptic_setup()\n");

	// setup l2cap and
	l2cap_init();

	// setup SM: Display only
	sm_init();
	sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
	sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

	// setup ATT server
	att_server_init(profile_data, NULL, NULL);

	// setup battery service
	battery_service_server_init(battery);

	// setup device information service
	device_information_service_server_init();

	// setup HID Device service
	hids_device_init(0, hid_descriptor_haptic, sizeof(hid_descriptor_haptic));

	// setup advertisements
	uint16_t adv_int_min = 0x0030;
	uint16_t adv_int_max = 0x0030;
	uint8_t adv_type = 0;
	bd_addr_t null_addr;
	memset(null_addr, 0, 6);
	gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
	gap_advertisements_set_data(adv_data_len, (uint8_t *)adv_data);
	gap_advertisements_enable(1);

	// register for HCI events
	hci_event_callback_registration.callback = &packet_handler;
	hci_add_event_handler(&hci_event_callback_registration);

	// register for connection parameter updates
	l2cap_event_callback_registration.callback = &packet_handler;
	l2cap_add_event_handler(&l2cap_event_callback_registration);

	// register for SM events
	sm_event_callback_registration.callback = &packet_handler;
	sm_add_event_handler(&sm_event_callback_registration);

	// register for HIDS
	hids_device_register_packet_handler(packet_handler);
}

// HID Report sending
static void send_report()
{
	uint8_t report [] = { 0 };
	switch (protocol_mode) {
		case 0:
			break;
		case 1:
			hids_device_send_input_report(con_handle, report, sizeof(report));
			break;
		default:
			break;
	}
}

static void haptics_can_send_now(void)
{
	send_report();
	hids_device_request_can_send_now_event(con_handle);
}

static inline void print_ble()
{
	PRINTF("[BLE] ");
}

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	UNUSED(channel);
	UNUSED(size);

	uint16_t conn_interval;

	if (packet_type != HCI_EVENT_PACKET) return;

	switch (hci_event_packet_get_type(packet)) {
		case HCI_EVENT_DISCONNECTION_COMPLETE:
			con_handle = HCI_CON_HANDLE_INVALID;
			print_timestamp();
			print_ble();
			PRINTF("Disconnected\n");
			break;
		case SM_EVENT_JUST_WORKS_REQUEST:
			print_timestamp();
			print_ble();
			PRINTF("Just Works requested\n");
			sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
			break;
		case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
			print_timestamp();
			print_ble();
			PRINTF("Confirming numeric comparison: %"PRIu32"\n", sm_event_numeric_comparison_request_get_passkey(packet));
			sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
			break;
		case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
			print_timestamp();
			print_ble();
			PRINTF("Display Passkey: %"PRIu32"\n", sm_event_passkey_display_number_get_passkey(packet));
			break;
		case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
			print_timestamp();
			print_ble();
			PRINTF("L2CAP Connection Parameter Update Complete, response: %x\n", l2cap_event_connection_parameter_update_response_get_result(packet));
			break;
		case HCI_EVENT_META_GAP:
			switch (hci_event_gap_meta_get_subevent_code(packet)) {
				case GAP_SUBEVENT_LE_CONNECTION_COMPLETE:
					// print connection parameters (without using float operations)
					conn_interval = gap_subevent_le_connection_complete_get_conn_interval(packet);
					print_timestamp();
					print_ble();
					PRINTF("LE Connection Complete:\n");
					PRINTF("- Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
					PRINTF("- Connection Latency: %u\n", gap_subevent_le_connection_complete_get_conn_latency(packet));
					break;
				default:
					break;
			}
			break;
		case HCI_EVENT_LE_META:
			switch (hci_event_le_meta_get_subevent_code(packet)) {
				case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
					// print connection parameters (without using float operations)
					conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
					print_timestamp();
					print_ble();
					PRINTF("LE Connection Update: \n");
					PRINTF("- Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
					PRINTF("- Connection Latency: %u\n", hci_subevent_le_connection_update_complete_get_conn_latency(packet));
					break;
				default:
					break;
			}
			break;
		case HCI_EVENT_HIDS_META:
			switch (hci_event_hids_meta_get_subevent_code(packet)) {
				case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
					con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
					print_timestamp();
					print_ble();
					PRINTF("Report Characteristic Subscribed %u\n", hids_subevent_input_report_enable_get_enable(packet));

					// request connection param update via L2CAP following Apple Bluetooth Design Guidelines
					// gap_request_connection_parameter_update(con_handle, 12, 12, 4, 100);    // 15 ms, 4, 1s

					// directly update connection params via HCI following Apple Bluetooth Design Guidelines
					// gap_update_connection_parameters(con_handle, 12, 12, 4, 100);    // 60-75 ms, 4, 1s


					break;
				// ...
				case HIDS_SUBEVENT_PROTOCOL_MODE:
					protocol_mode = hids_subevent_protocol_mode_get_con_handle(packet);
					print_timestamp();
					print_ble();
					PRINTF("Protocol Mode: %s mode\n", hids_subevent_protocol_mode_get_protocol_mode(packet) ? "Report" : "Boot");
					break;
				case HIDS_SUBEVENT_CAN_SEND_NOW:
					haptics_can_send_now();
					break;
				default:
					break;
			}
			break;

		default:
			break;
	}
}

int btstack_main(void);
int btstack_main(void)
{
	print_timestamp();
	PRINTF("Reached btstack_main()\n");
	hog_haptic_setup();
	hci_power_control(HCI_POWER_ON);

	return 0;
}
