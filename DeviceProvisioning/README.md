# Build:
```
bitbake wpeframework-service-plugins
```
# Getting the security token

Execute this before invoking other commands
```
token=`/usr/bin/WPEFrameworkSecurityUtility | grep token | cut -f4 -d "\""`
```
# Activate/Deactivate:
```
curl -H "Content-Type: application/json"  -H "Authorization: Bearer $token"  -X POST -d '{"jsonrpc":"2.0","id":"3","method": "Controller.1.activate", "params":{"callsign":"com.comcast.DeviceProvisioning"}}' http://127.0.0.1:9998/jsonrpc
curl -H "Content-Type: application/json"  -H "Authorization: Bearer $token"  -X POST -d '{"jsonrpc":"2.0","id":"3","method": "Controller.1.deactivate", "params":{"callsign":"com.comcast.DeviceProvisioning"}}' http://127.0.0.1:9998/jsonrpc
```
# Provision check and reprovisioning:
```
curl -H "Content-Type: application/json"  -H "Authorization: Bearer $token" -X POST -d '{"jsonrpc":"2.0","id":"3","method": "com.comcast.DeviceProvisioning.1.provisionCheck"}' http://127.0.0.1:9998/jsonrpc
curl -H "Content-Type: application/json"  -H "Authorization: Bearer $token" -X POST -d '{"jsonrpc":"2.0","id":"3","method": "com.comcast.DeviceProvisioning.1.reprovision", "params": {"provisionType": "HARDWARE"}}' http://127.0.0.1:9998/jsonrpc

curl -H "Content-Type: application/json"  -H "Authorization: Bearer $token" -X POST -d '{"jsonrpc":"2.0","id":"3","method": "com.comcast.DeviceProvisioning.1.getNetflixESN"}' http://127.0.0.1:9998/jsonrpc
curl -H "Content-Type: application/json"  -H "Authorization: Bearer $token" -X POST -d '{"jsonrpc":"2.0","id":"3","method": "com.comcast.DeviceProvisioning.1.notifyProvisioning", "params": {"provisionType": "HARDWARE"}}' http://127.0.0.1:9998/jsonrpc

curl -H "Content-Type: application/json"  -H "Authorization: Bearer $token" -X POST -d '{"jsonrpc":"2.0","id":"3","method": "com.comcast.DeviceProvisioning.1.getSOCId"}' http://127.0.0.1:9998/jsonrpc
curl -H "Content-Type: application/json"  -H "Authorization: Bearer $token" -X POST -d '{"jsonrpc":"2.0","id":"3","method": "com.comcast.DeviceProvisioning.1.notifyRuntimeInformation", "params": {"provisionType": "HARDWARE", "executionPhase": "provisioning", "bootupMode": "true", "sessionStartEpoch": 1238890, "deviceIsConnected": "true", "clockSetReceived": "true", "failCount": 12, "refreshEpoch": 128294, "sessionDeadlineEpoch": 129444, "clientStatus": "RT_SUCCESS", "credStatus": "RT_SUCCESS", "controllerStatus": "RT_CTL_OK"}}' http://127.0.0.1:9998/jsonrpc
```
# Events
- didFinishProvisionCheck
- didFinishReprovision

# Reference
https://etwiki.sys.comcast.net/display/APPS/DeviceProvisioning