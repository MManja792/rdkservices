/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2019 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>
#include <cstdint>

#include "Module.h"
#include "utils.h"
#include "AbstractPlugin.h"
#include <interfaces/IProvisioning.h>
#include "sec_security.h"
#include "tptimer.h"
#include "libIBus.h"
#include "libIBusDaemon.h"
#include "pwrMgr.h"

class Reg;
namespace WPEFramework {
    namespace Plugin {

        // This is a server for a JSONRPC communication channel.
        // For a plugin to be capable to handle JSONRPC, inherit from PluginHost::JSONRPC.
        // By inheriting from this class, the plugin realizes the interface PluginHost::IDispatcher.
        // This realization of this interface implements, by default, the following methods on this plugin
        // - exists
        // - register
        // - unregister
        // Any other methood to be handled by this plugin  can be added can be added by using the
        // templated methods Register on the PluginHost::JSONRPC class.
        // As the registration/unregistration of notifications is realized by the class PluginHost::JSONRPC,
        // this class exposes a public method called, Notify(), using this methods, all subscribed clients
        // will receive a JSONRPC message as a notification, in case this method is called.
        class DeviceProvisioning : public AbstractPlugin, public Exchange::IProvisioning {

        private:
            // We do not allow this plugin to be copied !!
            DeviceProvisioning(const DeviceProvisioning&) = delete;
            DeviceProvisioning& operator=(const DeviceProvisioning&) = delete;

            // Registered methods begin
            // Note: `JsonObject& parameters` corresponds to `params` in JSON RPC call
            // Note 2: Either Sync or Async method is registered in ctor.
            // Async returns immediately, then sends an event
            // Sync returns after the job is done; this may take a while, depending on the network

            // Convenience methods
            uint32_t getApiVersionNumber(const JsonObject& parameters, JsonObject& response);

            // v1 methods
            uint32_t provisionCheckAsync(const JsonObject& parameters, JsonObject& response);
            uint32_t provisionCheckSync(const JsonObject& parameters, JsonObject& response);
            uint32_t reprovisionAsync(const JsonObject& parameters, JsonObject& response);
            uint32_t reprovisionSync(const JsonObject& parameters, JsonObject& response);

            // v2 methods
            uint32_t notifyProvisioningSync(const JsonObject& parameters, JsonObject& response);
            uint32_t getNetflixEsnSync(const JsonObject& parameters, JsonObject& response);

            // v3 methods
            uint32_t getSOCIdSync(const JsonObject& parameters, JsonObject& response);
            uint32_t notifyRuntimeInformationSync(const JsonObject& parameters, JsonObject& response);
            // RFU methods
            uint32_t checkForUpdatesSync(const JsonObject& parameters, JsonObject& response);
            uint32_t setAutoProvisionMode(const JsonObject& parameters, JsonObject& response);
            uint32_t getAutoProvisionMode(const JsonObject& parameters, JsonObject& response);

            // Registered methods end
            //      IProvisioning Methods
            // -------------------------------------------------------------------------------------------------------
            virtual void Register(IProvisioning::INotification* provisioning) override;
            virtual void Unregister(IProvisioning::INotification* provisioning) override;

        private: /*internal methods*/
            static void powerModeChange(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static void _iarmEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            void iarmEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
            static bool isProcessRunning(const std::string& processName);
            static int execCmd(const char* cmd, int* prcRet = nullptr, string* output = nullptr);
            static void provisionCheckProcess(DeviceProvisioning* dp, JsonObject* rp);
            static bool reprovisionProcess(const std::string& provisionType, DeviceProvisioning* dp, JsonObject* rp);
            void provisionRefreshProcess(const std::string appName);
            static void getPrimaryDrmDir(std::string& drmDir);
            static bool getDrmDir(const std::string& provisionType, std::string& drmDir);
            static bool deviceProvisioned(const std::string& provisionType = "HARDWARE");
            static std::vector<std::string>& getSupportedProvisionTypes();
            static void getProvisionedTypes(std::vector<std::string>& provisionTypes);
            inline bool supported(const std::string& provisionType);
            void fulfillPrecondition(const std::string& provisionType, const std::string& reason = "");
            void fulfillPrecondition(/*const*/ std::vector<string>& provisionTypes, const std::string& reason = "");
            uint64_t getRefreshEpoch(const std::string& provisionType);
            void setRefreshEpoch(uint64_t refreshEpoch, const std::string& provisionType);
            int getBackoffInfo(const std::string& provisionType, uint64_t& prefetchFailCount, uint64_t& prefetchFailEpoch);
            /*timers*/
            void onOCDMTimer();
            void onSocProvTimer(const std::string& provisionType, TpTimer* timer);

        private:

        public:
            DeviceProvisioning();
            virtual ~DeviceProvisioning();
            virtual const string Initialize(PluginHost::IShell* service) override;
            virtual void Deinitialize(PluginHost::IShell* service) override;

        public:
            static string PROVISION_PROPERTIES_FILTER;
            static string GET_DRM_DIR_CMD;
            static string GET_PROVISION_TYPES_CMD;
            static string GET_NETFLIX_ESN_CMD;
            static string RT_REFRESH_EPOCH_FILE_NAME;
            static string RT_BACKOFF_PERSISTENT_FILE_NAME;

        private:
            static DeviceProvisioning* _instance;
            static IARM_Bus_PWRMgr_PowerState_t _powerState;
            static uint64_t _wakeupTime;

        private:
            uint32_t _apiVersionNumber;
            // Assuming that there will be only one threaded call at a time (which is the case for DeviceProvisioning)
            // Otherwise we might need a thread for each async command for better performance
            std::thread _executionThread;
            std::string _primaryDrmDir;
            Core::CriticalSection _eventLock;
            std::list<Exchange::IProvisioning::INotification*> _observers;
            PluginHost::IShell* _service;
            PluginHost::ISubSystem* _subSystem;
            TpTimer _ocdmTimer;
            std::mutex _callMutex;
            std::shared_ptr <Sec_ProcessorHandle> _sec;
            std::unordered_map<std::string, TpTimer*> _socprovTimers;
            std::unordered_map<std::string, std::thread*> _socprovThreads;
            Reg* _reg;
        };
	} // Plugin
} // WPEFramework
