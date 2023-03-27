<!-- Generated automatically, DO NOT EDIT! -->
<a name="EasterEgg_Plugin"></a>
# EasterEgg Plugin

**Version: [1.0.0](https://github.com/rdkcentral/rdkservices/blob/main/EasterEgg/CHANGELOG.md)**

A org.rdk.EasterEgg plugin for Thunder framework.

### Table of Contents

- [Abbreviation, Acronyms and Terms](#Abbreviation,_Acronyms_and_Terms)
- [Description](#Description)
- [Configuration](#Configuration)
- [Methods](#Methods)
- [Notifications](#Notifications)

<a name="Abbreviation,_Acronyms_and_Terms"></a>
# Abbreviation, Acronyms and Terms

[[Refer to this link](userguide/aat.md)]

<a name="Description"></a>
# Description

The `EasterEgg` plugin is used to start the factory application and the resident application.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#Thunder)].

<a name="Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *org.rdk.EasterEgg*) |
| classname | string | Class name: *org.rdk.EasterEgg* |
| locator | string | Library name: *libWPEFrameworkEasterEgg.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |

<a name="Methods"></a>
# Methods

The following methods are provided by the org.rdk.EasterEgg plugin:

EasterEgg interface methods:

| Method | Description |
| :-------- | :-------- |
| [launchFactoryApp](#launchFactoryApp) | Launches the factory application and sets the `FactoryMode` and `AllowExit` parameters to `true` in the persistent store configuration |
| [launchResidentApp](#launchResidentApp) | Launches the resident application and sets the `FactoryMode` parameter to `false` in the persistent store configuration |
| [toggleFactoryApp](#toggleFactoryApp) | Launches the resident application if the factory application is currently running |
| [launchFactoryAppShortcut](#launchFactoryAppShortcut) | Launches the factory application shortcut using the `ToFacFlag` value that is in the persistent store configuration |
| [showEasterEggsMenu](#showEasterEggsMenu) | Launches the EasterEggs menu application |


<a name="launchFactoryApp"></a>
## *launchFactoryApp*

Launches the factory application and sets the `FactoryMode` and `AllowExit` parameters to `true` in the persistent store configuration.
 
### Events 

 No Events.

### Events

No Events

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
    "method": "org.rdk.EasterEgg.launchFactoryApp",
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

<a name="launchResidentApp"></a>
## *launchResidentApp*

Launches the resident application and sets the `FactoryMode` parameter to `false` in the persistent store configuration. This method attempts to exit the factory application before launching the resident application.
 
### Events 
| Event | Description | 
| :----------- | :----------- |
| `onWillDestroy`| Triggered when the factory application is forcibly closed |.

### Events

| Event | Description |
| :-------- | :-------- |
