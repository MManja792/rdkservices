#pragma once

#define IARM_BUS_SYSMGR_NAME "SYSMgr"

typedef enum _SYSMgr_EventId_t {
    IARM_BUS_SYSMGR_EVENT_USB_MOUNT_CHANGED,
    IARM_BUS_SYSMGR_EVENT_MAX
} IARM_Bus_SYSMgr_EventId_t;

typedef struct _IARM_BUS_SYSMgr_EventData_t {
    union {
        struct _USB_MOUNT {
            int mounted;
            char device[128];
            char dir[256];
        } usbMountData;
    } data;
} IARM_Bus_SYSMgr_EventData_t;

typedef struct _propertyValue
{
  int state;
  int error;
  char payload[128];
} state_property;
/*
 * Declare RPC API names and their arguments
 */
#define IARM_BUS_SYSMGR_API_GetSystemStates     "GetSystemStates" /*!< Gets the states of the system*/

/*
 * Declare RPC API names for HDCP Profile
 */
#define IARM_BUS_SYSMGR_API_SetHDCPProfile      "SetHDCPProfile"
#define IARM_BUS_SYSMGR_API_GetHDCPProfile      "GetHDCPProfile"

/*
 * Declare RPC API names for Key Code Logging Status
 */
#define IARM_BUS_SYSMGR_API_GetKeyCodeLoggingPref "GetKeyCodeLoggingPref"
#define IARM_BUS_SYSMGR_API_SetKeyCodeLoggingPref "SetKeyCodeLoggingPref"

/*! Parameter for Setpowerstate call*/
typedef struct _IARM_Bus_SYSMgr_GetSystemStates_Param_t {
        state_property channel_map;        /*!< [in] New powerstate to be set */
  state_property disconnect_mgr_state;
  state_property TuneReadyStatus;
  state_property exit_ok_key_sequence;
  state_property cmac;
  state_property card_moto_entitlements;
  state_property card_moto_hrv_rx;
  state_property dac_init_timestamp;
  state_property card_cisco_status;
  state_property video_presenting;
  state_property hdmi_out;
  state_property hdcp_enabled;
  state_property hdmi_edid_read;
  state_property firmware_download;
  state_property time_source;
  state_property time_zone_available;
  state_property ca_system;
  state_property estb_ip;
  state_property ecm_ip;
  state_property lan_ip;
  state_property moca;
  state_property docsis;
  state_property dsg_broadcast_tunnel;
  state_property dsg_ca_tunnel;
  state_property cable_card;
  state_property cable_card_download;
  state_property cvr_subsystem;
  state_property download;
  state_property vod_ad;
  state_property card_serial_no;
  state_property ecm_mac;
  state_property dac_id;
  state_property plant_id;
  state_property stb_serial_no;
  state_property bootup;
  state_property dst_offset;
  state_property rf_connected;
  state_property ip_mode;
  state_property qam_ready_status;
  state_property firmware_update_state;
} IARM_Bus_SYSMgr_GetSystemStates_Param_t;

#define IARM_BUS_SYSMGR_Intrusion_MaxLen 1024
