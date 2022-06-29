#ifndef __WDMP_C_H__
#define __WDMP_C_H__

#include <stdbool.h>
#include <stdint.h>


typedef enum
{
	WDMP_SUCCESS = 0,
	WDMP_ERR_DEFAULT_VALUE
}WDMP_STATUS;

typedef enum
{
    WDMP_STRING = 0,
    WDMP_INT,
    WDMP_UINT,
    WDMP_BOOLEAN,
    WDMP_DATETIME,
    WDMP_BASE64,
    WDMP_LONG,
    WDMP_ULONG,
    WDMP_FLOAT,
    WDMP_DOUBLE,
    WDMP_BYTE,
    WDMP_NONE,
    WDMP_BLOB
} DATA_TYPE;

#endif
