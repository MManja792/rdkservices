<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.DeviceProvisioning_Plugin"></a>
# DeviceProvisioning Plugin

**Version: [1.1.0](https://github.com/rdkcentral/rdkservices/blob/main/DeviceProvisioning/CHANGELOG.md)**

A com.comcast.DeviceProvisioning plugin for Thunder framework.

### Table of Contents

- [Abbreviation, Acronyms and Terms](#head.Abbreviation,_Acronyms_and_Terms)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)

<a name="head.Abbreviation,_Acronyms_and_Terms"></a>
# Abbreviation, Acronyms and Terms

[[Refer to this link](userguide/aat.md)]

<a name="head.Description"></a>
# Description

The `DeviceProvisioning` plugin is used to determine the status of the provisioned objects that are stored on the device. A device may contain one or more provision types depending upon the types of security APIs that are enabled on the device.  This API allows the client to check on all provision types and specifically provision or reprovision a given provision type.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *com.comcast.DeviceProvisioning*) |
| classname | string | Class name: *com.comcast.DeviceProvisioning* |
| locator | string | Library name: *libWPEFrameworkDeviceProvisioning.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the com.comcast.DeviceProvisioning plugin:

DeviceProvisioning interface methods:

| Method | Description |
| :-------- | :-------- |
| [getApiVersionNumber](#method.getApiVersionNumber) | Gets the current API version number |
| [getNetflixESN](#method.getNetflixESN) | (Version 2) Returns the Netflix Electronic Serial Number (ESN) |
| [getSOCId](#method.getSOCId) | (Version 3) Returns the SOC ID based on the provisioning type |
| [notifyProvisioning](#method.notifyProvisioning) | (Version 2) Notify the valid provisionType of the provisioned device |
| [provisionCheck](#method.provisionCheck) | Checks the provisioning condition of the selected device |
| [reprovision](#method.reprovision) | Forces the device to reprovision |


<a name="method.getApiVersionNumber"></a>
## *getApiVersionNumber [<sup>method</sup>](#head.Methods)*

Gets the current API version number.
 
### Events 

 No Events.

### Events

No Events

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.version | string | The current API version number |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "com.comcast.DeviceProvisioning.getApiVersionNumber"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "version": "1",
        "success": true
    }
}
```

<a name="method.getNetflixESN"></a>
## *getNetflixESN [<sup>method</sup>](#head.Methods)*

(Version 2) Returns the Netflix Electronic Serial Number (ESN). In order to get the Netflix ESN, the device must be provisioned. If the device is not provisioned or if there is an error retrieving the ESN, then the ESN parameter must be empty.
 
### Events 

 No Events.

### Events

No Events

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.esn | string | The Netflix ESN |
| result.success | boolean | Whether the request succeeded |
| result?.error | string | <sup>*(optional)*</sup> A human readable string that states the cause of the failure. The `success` parameter in this case must be `false` |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "com.comcast.DeviceProvisioning.getNetflixESN"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "esn": "CMCSTX100000000000000000000000000001000000",
        "success": true,
        "error": "..."
    }
}
```

<a name="method.getSOCId"></a>
## *getSOCId [<sup>method</sup>](#head.Methods)*

(Version 3) Returns the SOC ID based on the provisioning type.
 
### Events 

 No Events.

### Events

No Events

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.socid | string | The SOC ID. If the hardware `provisionType` is supported on the device, then the SOC ID is the Chip ID of the SOC on the device |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "com.comcast.DeviceProvisioning.getSOCId"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "socid": "001b08011000082a",
        "success": true
    }
}
```

<a name="method.notifyProvisioning"></a>
## *notifyProvisioning [<sup>method</sup>](#head.Methods)*

(Version 2) Notify the valid provisionType of the provisioned device.
 
### Events 

 No Events.

> This API is **deprecated** and may be removed in the future. It is no longer recommended for use in new implementations.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.provisionType | string | The provision type (must be one of the following: *HARDWARE*, *CRYPTANIUM*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "com.comcast.DeviceProvisioning.notifyProvisioning",
    "params": {
        "provisionType": "HARDWARE"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.provisionCheck"></a>
## *provisionCheck [<sup>method</sup>](#head.Methods)*

Checks the provisioning condition of the selected device.
 
### Events 

 No Events.

### Events

No Events

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.provisionTypes | array | Information for each provision type that is supported on the device |
| result.provisionTypes[#] | object |  |
| result.provisionTypes[#].provisionType | string | The provision type (must be one of the following: *HARDWARE*, *CRYPTANIUM*) |
| result.provisionTypes[#].provisionKind | string | The provision kind |
| result.provisionTypes[#].state | string | The provision state (must be one of the following: *NOT_PROVISIONED*, *IN_PROGRESS*, *PROVISIONED*, *CORRUPTED*) |
| result.provisionTypes[#].reason | string | The provision reason |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "com.comcast.DeviceProvisioning.provisionCheck"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "provisionTypes": [
            {
                "provisionType": "HARDWARE",
                "provisionKind": "RT.2",
                "state": "PROVISIONED",
                "reason": "RT_GOOD_ACTIVE_PREFETCH_CREDS. Active device and NOT NEW prefetch creds are valid"
            }
        ],
        "success": true
    }
}
```

<a name="method.reprovision"></a>
## *reprovision [<sup>method</sup>](#head.Methods)*

Forces the device to reprovision.
 
### Events 

 No Events.

### Events

No Events

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.provisionType | string | The provision type (must be one of the following: *HARDWARE*, *CRYPTANIUM*) |
| params?.fetchCredentials | boolean | <sup>*(optional)*</sup> fetch the credentials |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.provisionTypes | array | Information for each provision type that is supported on the device |
| result.provisionTypes[#] | object |  |
| result.provisionTypes[#].provisionType | string | The provision type (must be one of the following: *HARDWARE*, *CRYPTANIUM*) |
| result.provisionTypes[#].provisionKind | string | The provision kind |
| result.provisionTypes[#].state | string | The provision state (must be one of the following: *NOT_PROVISIONED*, *IN_PROGRESS*, *PROVISIONED*, *CORRUPTED*) |
| result.provisionTypes[#].reason | string | The provision reason |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "com.comcast.DeviceProvisioning.reprovision",
    "params": {
        "provisionType": "HARDWARE",
        "fetchCredentials": true
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "provisionTypes": [
            {
                "provisionType": "HARDWARE",
                "provisionKind": "RT.2",
                "state": "PROVISIONED",
                "reason": "RT_GOOD_ACTIVE_PREFETCH_CREDS. Active device and NOT NEW prefetch creds are valid"
            }
        ],
        "success": true
    }
}
```
