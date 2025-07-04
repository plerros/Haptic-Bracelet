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

#define BTSTACK_FILE__ "bluetooth.c"

// *****************************************************************************
/* EXAMPLE_START(spp_counter): SPP Server - Heartbeat Counter over RFCOMM
 *
 * @text The Serial port profile (SPP) is widely used as it provides a serial
 * port over Bluetooth. The SPP counter example demonstrates how to setup an SPP
 * service, and provide a periodic timer over RFCOMM.   
 *
 * @text Note: To test, please run the spp_counter example, and then pair from 
 * a remote device, and open the Virtual Serial Port.
 */
// *****************************************************************************

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
 
#include "btstack.h"

// Config Files
#include "config.h"
#include "config_adv.h"

#include "npf_interface.h"
#include "btstack_main.h"

static inline void print_timestamp()
{
	PRINTF("[%8ju] ", (uintmax_t)ms_now());
}

struct bt_data_t *bt_data = NULL;

#define RFCOMM_SERVER_CHANNEL 1
#define HEARTBEAT_PERIOD_MS 10

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static uint16_t rfcomm_channel_id;
static uint8_t  spp_service_buffer[150];
static btstack_packet_callback_registration_t hci_event_callback_registration;


/* @section SPP Service Setup 
 *s
 * @text To provide an SPP service, the L2CAP, RFCOMM, and SDP protocol layers 
 * are required. After setting up an RFCOMM service with channel nubmer
 * RFCOMM_SERVER_CHANNEL, an SDP record is created and registered with the SDP server.
 * Example code for SPP service setup is
 * provided in Listing SPPSetup. The SDP record created by function
 * spp_create_sdp_record consists of a basic SPP definition that uses the provided
 * RFCOMM channel ID and service name. For more details, please have a look at it
 * in \path{src/sdp_util.c}. 
 * The SDP record is created on the fly in RAM and is deterministic.
 * To preserve valuable RAM, the result could be stored as constant data inside the ROM.   
 */

/* LISTING_START(SPPSetup): SPP service setup */ 
static void spp_service_setup(void)
{
	print_timestamp();
	PRINTF("Reached bluetooth_setup()\n");

	l2cap_init();

#ifdef ENABLE_BLE
	// Initialize LE Security Manager. Needed for cross-transport key derivation
	sm_init();
	sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
	sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
#endif

	// register for HCI events
	hci_event_callback_registration.callback = &packet_handler;
	hci_add_event_handler(&hci_event_callback_registration);

	rfcomm_init();
	rfcomm_register_service(packet_handler, RFCOMM_SERVER_CHANNEL, 0xffff);  // reserved channel, mtu limited by l2cap

	// init SDP, create record for SPP and register with SDP
	sdp_init();
	memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
	spp_create_sdp_record(spp_service_buffer, sdp_create_service_record_handle(), RFCOMM_SERVER_CHANNEL, "SPP Counter");
	btstack_assert(de_get_len( spp_service_buffer) <= sizeof(spp_service_buffer));
	sdp_register_service(spp_service_buffer);
}
/* LISTING_END */

/* @section Periodic Timer Setup
 * 
 * @text The heartbeat handler increases the real counter every second, 
 * and sends a text string with the counter value, as shown in Listing PeriodicCounter. 
 */

/* LISTING_START(PeriodicCounter): Periodic Counter */ 
static btstack_timer_source_t heartbeat;
static void  heartbeat_handler(struct btstack_timer_source *ts){
	btstack_run_loop_set_timer(ts, HEARTBEAT_PERIOD_MS);
	btstack_run_loop_add_timer(ts);
} 

static void one_shot_timer_setup(void){
	// set one-shot timer
	heartbeat.process = &heartbeat_handler;
	btstack_run_loop_set_timer(&heartbeat, HEARTBEAT_PERIOD_MS);
	btstack_run_loop_add_timer(&heartbeat);
}


/* @section Bluetooth Logic 
 * @text The Bluetooth logic is implemented within the 
 * packet handler, see Listing SppServerPacketHandler. In this example, 
 * the following events are passed sequentially: 
 * - BTSTACK_EVENT_STATE,
 * - HCI_EVENT_PIN_CODE_REQUEST (Standard pairing) or 
 * - HCI_EVENT_USER_CONFIRMATION_REQUEST (Secure Simple Pairing),
 * - RFCOMM_EVENT_INCOMING_CONNECTION,
 * - RFCOMM_EVENT_CHANNEL_OPENED, 
* - RFCOMM_EVETN_CAN_SEND_NOW, and
 * - RFCOMM_EVENT_CHANNEL_CLOSED
 */

/* @text Upon receiving HCI_EVENT_PIN_CODE_REQUEST event, we need to handle
 * authentication. Here, we use a fixed PIN code "0000".
 *
 * When HCI_EVENT_USER_CONFIRMATION_REQUEST is received, the user will be 
 * asked to accept the pairing request. If the IO capability is set to 
 * SSP_IO_CAPABILITY_DISPLAY_YES_NO, the request will be automatically accepted.
 *
 * The RFCOMM_EVENT_INCOMING_CONNECTION event indicates an incoming connection.
 * Here, the connection is accepted. More logic is need, if you want to handle connections
 * from multiple clients. The incoming RFCOMM connection event contains the RFCOMM
 * channel number used during the SPP setup phase and the newly assigned RFCOMM
 * channel ID that is used by all BTstack commands and events.
 *
 * If RFCOMM_EVENT_CHANNEL_OPENED event returns status greater then 0,
 * then the channel establishment has failed (rare case, e.g., client crashes).
 * On successful connection, the RFCOMM channel ID and MTU for this
 * channel are made available to the heartbeat counter. After opening the RFCOMM channel, 
 * the communication between client and the application
 * takes place. In this example, the timer handler increases the real counter every
 * second. 
 *
 * RFCOMM_EVENT_CAN_SEND_NOW indicates that it's possible to send an RFCOMM packet
 * on the rfcomm_cid that is include

 */ 

/* LISTING_START(SppServerPacketHandler): SPP Server - Heartbeat Counter over RFCOMM */
static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
	UNUSED(channel);

	bd_addr_t event_addr;
	uint8_t   rfcomm_channel_nr;
	uint16_t  mtu;
	int i;

	switch (packet_type) {
		case HCI_EVENT_PACKET:
		switch (hci_event_packet_get_type(packet)) {
			case HCI_EVENT_PIN_CODE_REQUEST:
				// inform about pin code request
				printf("Pin code request - using '0000'\n");
				hci_event_pin_code_request_get_bd_addr(packet, event_addr);
				gap_pin_code_response(event_addr, "0000");
				break;

			case HCI_EVENT_CONNECTION_COMPLETE:
				PRINTF("Connected\n");
				bt_data->connected = true;
				break;

			case HCI_EVENT_DISCONNECTION_COMPLETE:
				//PRINTF("Disconnected\n");
				PRINTF("Disconnect, reason 0x%02x\n", hci_event_disconnection_complete_get_reason(packet));

				bt_data->connected = false;
				break;

			case HCI_EVENT_USER_CONFIRMATION_REQUEST:
				// ssp: inform about user confirmation request
				printf("SSP User Confirmation Request with numeric value '%06"PRIu32"'\n", little_endian_read_32(packet, 8));
				printf("SSP User Confirmation Auto accept\n");
				break;

			case SM_EVENT_IDENTITY_RESOLVING_STARTED:
				PRINTF("[SM] Identity resolving\n");
				break;
			case SM_EVENT_JUST_WORKS_REQUEST:
				PRINTF("[SM] Just Works requested\n");
				sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
				break;
			case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
				PRINTF("[SM] Confirming numeric comparison: %"PRIu32"\n", sm_event_numeric_comparison_request_get_passkey(packet));
				sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
				break;
			case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
				PRINTF("[SM] Display Passkey: %"PRIu32"\n", sm_event_passkey_display_number_get_passkey(packet));
				break;

			case RFCOMM_EVENT_INCOMING_CONNECTION:
				rfcomm_event_incoming_connection_get_bd_addr(packet, event_addr);
				rfcomm_channel_nr = rfcomm_event_incoming_connection_get_server_channel(packet);
				rfcomm_channel_id = rfcomm_event_incoming_connection_get_rfcomm_cid(packet);
				printf("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
				rfcomm_accept_connection(rfcomm_channel_id);
				break;

			case RFCOMM_EVENT_CHANNEL_OPENED:
				if (rfcomm_event_channel_opened_get_status(packet)) {
				printf("RFCOMM channel open failed, status 0x%02x\n", rfcomm_event_channel_opened_get_status(packet));
				} else {
				rfcomm_channel_id = rfcomm_event_channel_opened_get_rfcomm_cid(packet);
				mtu = rfcomm_event_channel_opened_get_max_frame_size(packet);
				printf("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_channel_id, mtu);
				}
				break;
			case RFCOMM_EVENT_CAN_SEND_NOW:
				//rfcomm_send(rfcomm_channel_id, (uint8_t*) lineBuffer, (uint16_t) strlen(lineBuffer));  
				break;

			case RFCOMM_EVENT_CHANNEL_CLOSED:
				printf("RFCOMM channel closed\n");
				rfcomm_channel_id = 0;
				break;

			default:
				break;
		}
		break;

		case RFCOMM_DATA_PACKET: {
			int tmp = 0;
			for (i = 0; i < size; i++) {
				if (! isdigit(packet[i]))
					break;

				if (tmp > 10000)
					break;
			
				tmp *= 10;
				tmp += packet[i] - '0';
			}
			printf("%d\n", tmp);
			bt_data->command1 = tmp;
			tmp = 0;
			if (i < size && packet[i] == ' ')
				i++;

			for (; i < size; i++) {
				if (! isdigit(packet[i]))
					break;

				if (tmp > 10000)
					break;
			
				tmp *= 10;
				tmp += packet[i] - '0';
			}
			printf("%d\n", tmp);
			bt_data->command2 = tmp;
			break;
		}

		default:
			break;
	}
}

int btstack_main(struct bt_data_t *data)
{
	bt_data = data;

	one_shot_timer_setup();
	spp_service_setup();

	gap_discoverable_control(1);

	gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
	gap_set_local_name("Haptic Serial");

	// turn on!
	hci_power_control(HCI_POWER_ON);

	return 0;
}