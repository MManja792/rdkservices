/**
* @defgroup cpc
* @{
* @defgroup socprovisioning
* @{
* @defgroup include
* @{
**/

#ifndef SOCPROV_DATATYPE_H
#define SOCPROV_DATATYPE_H
#include <stdint.h>

/* enables debug prints */
//#define SOCPROV_DEBUG

#define SOCPROV_MAX_DETAILED_MESSAGE_LEN 4*512
#define IARM_BUS_SOCPROVISIONING_NAME "socprovisioning"
#define SIZE 256

/*****************************************************************************
 * EXPORTED TYPES
 *****************************************************************************/

// API 1.0.5
typedef uint8_t SOCPROV_BYTE;
typedef uint8_t SOCPROV_BOOL;
typedef uint32_t SOCPROV_SIZE;

#define SOCPROV_TRUE 1
#define SOCPROV_FALSE 0

typedef enum {
    SOCPROV_RESULT_SUCCESS = 0,
    //error indicating a failure of getting provisining status
    SOCPROV_RESULT_FAILURE_STATUS,
    SOCPROV_RESULT_FAILURE_PROVISIONING,
    SOCPROV_RESULT_FAILURE_UNINITIALIZED,
    SOCPROV_RESULT_FAILURE_FIFO_FILE,

    //error indicating that device is failed to execute a procedure for an unknown reason
    SOCPROV_RESULT_FAILURE_GENERIC
}SocProv_Result;

typedef enum {
    SOCPROV_TYPE_HARDWARE = 0,
    SOCPROV_TYPE_CRYPTANIUM,
    SOCPROV_TYPE_HYBRID_OPENSSL,
    SOCPROV_TYPE_UNKNOWN
}SocProv_Type;

typedef enum {
    SOCPROV_KIND_RT = 0,
    SOCPROV_KIND_RT2,
    SOCPROV_KIND_LEGACY,
    SOCPROV_KIND_UNKNOWN
}SocProv_Kind;

typedef enum {
    SOCPROV_STATE_PROVISIONED = 0,
    SOCPROV_STATE_NOT_PROVISIONED,
    SOCPROV_STATE_CORRUPTED,
    SOCPROV_STATE_UNKNOWN
}SocProv_State;

typedef struct SocProv_Status_{
    SocProv_Type  type;
    SocProv_Kind  kind;
    SocProv_State state;
    char message [SOCPROV_MAX_DETAILED_MESSAGE_LEN]; // empty if "PROVISIONED"
} SocProv_Status;

typedef enum ProvisionOptions_{
    PROVISION_OPTION_NONE = 0,
    PROVISION_OPTION_FORCE,
    PROVISION_OPTION_SKIP_PACKAGES //ignored if PROVISION_OPTION_FORCE is set
}ProvisionOptions;

typedef enum{
    RT_CTL_SUCCESS = 0
    , RT_CTL_UNKNOWN = 1
    , RT_CTL_CONFIG_ERROR = 2
    , RT_CTL_FETCH_CREDS_ERROR = 4
    , RT_CTL_PROVISION_CREDS_ERROR = 8
    , RT_CTL_FETCH_PROVISION_CREDS_ERROR = 12             // (RT_CTL_FETCH_CREDS_ERROR | RT_CTL_PROVISION_CREDS_ERROR)
    , RT_CTL_HEALTHCHECK_CREDS_ERROR = 16
    , RT_CTL_FETCH_HEALTHCHECK_CREDS_ERROR = 20           // (RT_CTL_FETCH_CREDS_ERROR | RT_CTL_HEALTHCHECK_CREDS_ERROR)
    , RT_CTL_PROVISION_HEALTHCHECK_CREDS_ERROR = 24       // (RT_CTL_PROVISION_CREDS_ERROR | RT_CTL_HEALTHCHECK_CREDS_ERROR)
    , RT_CTL_FETCH_PROVISION_HEALTHCHECK_CREDS_ERROR = 28 // (RT_CTL_FETCH_CREDS_ERROR | RT_CTL_PROVISION_CREDS_ERROR | RT_CTL_PROVISION_HEALTHCHECK_CREDS_ERROR)
    , RT_CTL_RESULT_UNEXPECTED = 128
}RT_CTL_Result;

typedef enum _SocProv_EventId_t {
        IARM_BUS_SOCPROVISIONING_EVENT_DEVICE_PROVISIONED,
        IARM_BUS_SOCPROVISIONING_EVENT_DEVICE_REPROVISIONED,
        IARM_BUS_SOCPROVISIONING_EVENT_MAX
} IARM_Bus_SocProv_EventId_t;

typedef struct _IARM_BUS_SocProv_EventData_t {
        char type[SIZE];
} IARM_BUS_SocProv_EventData_t;

#endif // SOCPROV_DATATYPE_H

/** @} */
/** @} */
/** @} */
