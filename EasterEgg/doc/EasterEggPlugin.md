<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.EasterEgg_Plugin"></a>
# EasterEgg Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

A org.rdk.EasterEgg plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the org.rdk.EasterEgg plugin. It includes detailed specification about its configuration, methods provided and notifications sent.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |

The table below provides and overview of terms and abbreviations used in this document and their definitions.

| Term | Description |
| :-------- | :-------- |
| <a name="term.callsign">callsign</a> | The name given to an instance of a plugin. One plugin can be instantiated multiple times, but each instance the instance name, callsign, must be unique. |

<a name="head.References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The `EasterEgg` plugin is used to start the factory application and the resident application.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *org.rdk.EasterEgg*) |
| classname | string | Class name: *org.rdk.EasterEgg* |
| locator | string | Library name: *libWPEFrameworkEasterEgg.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the org.rdk.EasterEgg plugin:

EasterEgg interface methods:

| Method | Description |
| :-------- | :-------- |
| [launchFactoryApp](#method.launchFactoryApp) | Launches the factory application and sets the `FactoryMode` and `AllowExit` parameters to `true` in the persistent store configuration |
| [launchResidentApp](#method.launchResidentApp) | Launches the resident application and sets the `FactoryMode` parameter to `false` in the persistent store configuration |
| [toggleFactoryApp](#method.toggleFactoryApp) | Launches the resident application if the factory application is currently running |
| [launchFactoryAppShortcut](#method.launchFactoryAppShortcut) | Launches the factory application shortcut using the `ToFacFlag` value that is in the persistent store configuration |
| [showEasterEggsMenu](#method.showEasterEggsMenu) | Launches the EasterEggs menu application |


<a name="method.launchFactoryApp"></a>
## *launchFactoryApp [<sup>method</sup>](#head.Methods)*

Launches the factory application and sets the `FactoryMode` and `AllowExit` parameters to `true` in the persistent store configuration.
 
### Events 

 No Events.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.startup | boolean | <sup>*(optional)*</sup> Verify the factory application aging status in the persistent store before launching |
| params?.resetagingtime | boolean | <sup>*(optional)*</sup> Sets the factory application aging time to 0 in the persistent store configuration before launching |
| params?.nokillresapp | boolean | <sup>*(optional)*</sup> Whether to destroy the resident application before launching the factory application |

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
    "method": "org.rdk.EasterEgg.1.launchFactoryApp",
    "params": {
        "startup": true,
        "resetagingtime": true,
        "nokillresapp": true
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

<a name="method.launchResidentApp"></a>
## *launchResidentApp [<sup>method</sup>](#head.Methods)*

Launches the resident application and sets the `FactoryMode` parameter to `false` in the persistent store configuration. This method attempts to exit the factory application before launching the resident application.
 
### Events 
| Event | Description | 
| :----------- | :----------- |
| `onWillDestroy`| Triggered when the factory application is forcibly closed |.

Also see: [onWillDestroy](#event.onWillDestroy)

### Parameters

This method takes no parameters.

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
    "method": "org.rdk.EasterEgg.1.launchResidentApp"
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

<a name="method.toggleFactoryApp"></a>
## *toggleFactoryApp [<sup>method</sup>](#head.Methods)*

Launches the resident application if the factory application is currently running. The factory application is launched if it is not currently running.
 
### Events 

 No Events.

### Parameters

This method takes no parameters.

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
    "method": "org.rdk.EasterEgg.1.toggleFactoryApp"
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

<a name="method.launchFactoryAppShortcut"></a>
## *launchFactoryAppShortcut [<sup>method</sup>](#head.Methods)*

Launches the factory application shortcut using the `ToFacFlag` value that is in the persistent store configuration.
 
### Events 

 No Events.

### Parameters

This method takes no parameters.

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
    "method": "org.rdk.EasterEgg.1.launchFactoryAppShortcut"
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

<a name="method.showEasterEggsMenu"></a>
## *showEasterEggsMenu [<sup>method</sup>](#head.Methods)*

Launches the EasterEggs menu application. If the menu application cannot be launched, a `message` parameter is returned with the response and includes the error message.
 
### Events 

 No Events.

### Parameters

This method takes no parameters.

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
    "method": "org.rdk.EasterEgg.1.showEasterEggsMenu"
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

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the org.rdk.EasterEgg plugin:

EasterEgg interface events:

| Event | Description |
| :-------- | :-------- |
| [onWillDestroy](#event.onWillDestroy) | Triggered when the factory application is forcibly closed |


<a name="event.onWillDestroy"></a>
## *onWillDestroy [<sup>event</sup>](#head.Notifications)*

Triggered when the factory application is forcibly closed.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.callsign | string | The callsign of the factory application |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.onWillDestroy",
    "params": {
        "callsign": "factoryapp"
    }
}
```

