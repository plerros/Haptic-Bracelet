//
// btstack_config.h for Raspberry Pi port
//
// Documentation: https://bluekitchen-gmbh.com/btstack/#how_to/
//

#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

// Port related features
#define HAVE_ASSERT
#define HAVE_BTSTACK_STDIN
#define HAVE_MALLOC
#define HAVE_POSIX_FILE_IO

// BTstack features that can be enabled
#define ENABLE_AVRCP_COVER_ART
//#define ENABLE_BLE           // supress warnings
//#define ENABLE_CLASSIC
#define ENABLE_GOEP_L2CAP
#define ENABLE_H5
#define ENABLE_HFP_WIDE_BAND_SPEECH
#define ENABLE_L2CAP_ENHANCED_RETRANSMISSION_MODE
#define ENABLE_L2CAP_LE_CREDIT_BASED_FLOW_CONTROL_MODE
#define ENABLE_LE_CENTRAL
#define ENABLE_LE_DATA_LENGTH_EXTENSION
#define ENABLE_LE_PERIPHERAL
#define ENABLE_LE_SECURE_CONNECTIONS
//#define ENABLE_LOG_ERROR
//#define ENABLE_LOG_INFO
#define ENABLE_MICRO_ECC_FOR_LE_SECURE_CONNECTIONS
#define ENABLE_PRINTF_HEXDUMP
#define ENABLE_SCO_OVER_HCI
#define ENABLE_SDP_DES_DUMP
#define ENABLE_SOFTWARE_AES128

// Warm Boot needed if connected via Wifi on Raspberry Pi 3A+ or 3B+
// #define ENABLE_CONTROLLER_WARM_BOOT

// BTstack configuration. buffers, sizes, ...
#define HCI_ACL_PAYLOAD_SIZE (1691 + 4)
#define HCI_INCOMING_PRE_BUFFER_SIZE 14 // sizeof BNEP header, avoid memcpy
#define HCI_OUTGOING_PRE_BUFFER_SIZE  4

#define NVM_NUM_DEVICE_DB_ENTRIES      16
#define NVM_NUM_LINK_KEYS              16

// Mesh Configuration
//#define ENABLE_MESH
//#define ENABLE_MESH_ADV_BEARER
//#define ENABLE_MESH_GATT_BEARER
//#define ENABLE_MESH_PB_ADV
//#define ENABLE_MESH_PB_GATT
//#define ENABLE_MESH_PROVISIONER
//#define ENABLE_MESH_PROXY_SERVER

//#define MAX_NR_MESH_SUBNETS            2
//#define MAX_NR_MESH_TRANSPORT_KEYS    16
//#define MAX_NR_MESH_VIRTUAL_ADDRESSES 16

// allow for one NetKey update
#define MAX_NR_MESH_NETWORK_KEYS      (MAX_NR_MESH_SUBNETS+1)

#define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4

//#define ENABLE_EXPLICIT_DEDICATED_BONDING_DISCONNECT // maybe?

#endif
