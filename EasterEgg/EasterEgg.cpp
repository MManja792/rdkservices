/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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

#include "EasterEgg.h"
#include <string>
#include <iostream>
#include <mutex>
#include <thread>

const short WPEFramework::Plugin::EasterEggs::API_VERSION_NUMBER_MAJOR = 1;
const short WPEFramework::Plugin::EasterEggs::API_VERSION_NUMBER_MINOR = 0;
const string WPEFramework::Plugin::EasterEggs::SERVICE_NAME = "org.rdk.EasterEgg";
//methods
const string WPEFramework::Plugin::EasterEggs::EASTEREGG_METHOD_LAUNCH_FACTORY_APP = "launchFactoryApp";
const string WPEFramework::Plugin::EasterEggs::EASTEREGG_METHOD_LAUNCH_RESIDENT_APP = "launchResidentApp";
const string WPEFramework::Plugin::EasterEggs::EASTEREGG_METHOD_TOGGLE_FACTORY_APP = "toggleFactoryApp";
const string WPEFramework::Plugin::EasterEggs::EASTEREGG_METHOD_LAUNCH_FACTORY_APP_SHORTCUT = "launchFactoryAppShortcut";
const string WPEFramework::Plugin::EasterEggs::EASTEREGG_METHOD_SHOW_EASTEREGGS_MENU = "showEasterEggsMenu";
const string WPEFramework::Plugin::EasterEggs::EASTEREGG_EVENT_ON_WILL_DESTROY = "onWillDestroy";

using namespace std;

#define EASTEREGG_THUNDER_TIMEOUT 20000
#define EASTER_EGGS_MENU_URL "http://localhost:50050/easterEggsMenu/index.html"
#define EASTEREGG_WILLDESTROY_EVENT_WAITTIME 1

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

static uint32_t gWillDestroyEventWaitTime = EASTEREGG_WILLDESTROY_EVENT_WAITTIME;

namespace WPEFramework {

    namespace {
        static Plugin::Metadata<Plugin::EasterEggs> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    namespace Plugin {

        static std::string sEasterEggsSecurityToken;

        EasterEggs* EasterEggs::_instance = nullptr;

        EasterEggs::EasterEggs()
                : AbstractPlugin(), mFactoryAppRunning(false), mEasterEggsMenuUrl(EASTER_EGGS_MENU_URL), mEasterEggsMenuEnabled(false)
        {
            LOGINFO("ctor");
            EasterEggs::_instance = this;

            registerMethod(EASTEREGG_METHOD_LAUNCH_FACTORY_APP, &EasterEggs::launchFactoryAppWrapper, this);
            registerMethod(EASTEREGG_METHOD_LAUNCH_RESIDENT_APP, &EasterEggs::launchResidentAppWrapper, this);
            registerMethod(EASTEREGG_METHOD_TOGGLE_FACTORY_APP, &EasterEggs::toggleFactoryAppWrapper, this);
            registerMethod(EASTEREGG_METHOD_LAUNCH_FACTORY_APP_SHORTCUT, &EasterEggs::launchFactoryAppShortcutWrapper, this);
            registerMethod(EASTEREGG_METHOD_SHOW_EASTEREGGS_MENU, &EasterEggs::showEasterEggsMenuWrapper, this);
        }

        EasterEggs::~EasterEggs()
        {
        }

        string EasterEggs::Information() const
        {
            return(string("{\"service\": \"") + SERVICE_NAME + string("\"}"));
        }

        const string EasterEggs::Initialize(PluginHost::IShell* service )
        {
            std::cout << "initializing\n";
            Utils::SecurityToken::getSecurityToken(sEasterEggsSecurityToken);
            string query = "token=" + sEasterEggsSecurityToken;
            Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:9998")));
            char* willDestroyWaitTimeValue = getenv("RDKSHELL_WILLDESTROY_EVENT_WAITTIME");
            if (NULL != willDestroyWaitTimeValue)
            {
                gWillDestroyEventWaitTime = atoi(willDestroyWaitTimeValue);
            }
#ifdef RFC_ENABLED
            RFC_ParamData_t param;
            bool ret = Utils::getRFCConfig("DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.EasterEggMenu.Enable", param);
            if (true == ret && param.type == WDMP_BOOLEAN && (strncasecmp(param.value,"true",4) == 0))
            {
              mEasterEggsMenuEnabled = true;
              std::cout << "Eastereggs menu feature is enabled" << std::endl;
            }
#else
	    mEasterEggsMenuEnabled = true;
#endif
            return "";
        }

        void EasterEggs::Deinitialize(PluginHost::IShell* service)
        {
            LOGINFO("Deinitialize");
            mRdkShellPlugin = nullptr;
            EasterEggs::_instance = nullptr;
        }

        std::shared_ptr<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> > EasterEggs::getThunderControllerClient(std::string callsign)
        {
            string query = "token=" + sEasterEggsSecurityToken;
            Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:9998")));
            std::shared_ptr<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> > thunderClient = make_shared<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> >(callsign.c_str(), "", sEasterEggsSecurityToken);
            return thunderClient;
        }

        std::shared_ptr<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>> EasterEggs::getRDKShellPlugin()
        {
            if (nullptr == mRdkShellPlugin)
            {
                string query = "token=" + sEasterEggsSecurityToken;
                Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:9998")));
                mRdkShellPlugin = make_shared<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>>("org.rdk.RDKShell.1", "", false, sEasterEggsSecurityToken);
            }
            return mRdkShellPlugin;
        }

        void EasterEggs::getAllApps(std::vector<std::string>& callsigns)
        {
            auto rdkshellPlugin = getRDKShellPlugin();
            if (!rdkshellPlugin)
            {
                return;
            }

            uint32_t status;
            JsonObject getStateResult;
            JsonObject param;

            status = rdkshellPlugin->Invoke<JsonObject, JsonObject>(EASTEREGG_THUNDER_TIMEOUT, "getState", param, getStateResult);
            if (status > 0)
            {
                std::cout << "failed to get state " << std::endl;
            }
            
            // If success is false, the container isn't running so nothing to do
            if (getStateResult["success"].Boolean())
            {
                const JsonArray stateList = getStateResult.HasLabel("state")?getStateResult["state"].Array():JsonArray();
                for (int i=0; i<stateList.Length(); i++)
                {
                    const JsonObject& stateInfo = stateList[i].Object();
                    if (stateInfo.HasLabel("callsign"))
                    {
                       std::string callsign = stateInfo["callsign"].String();
                       if (mFactoryAppRunning && (callsign == "factoryapp"))
                       {
                           continue;
                       }
                       callsigns.push_back(callsign);
                    }
                }
            }
        }

        void EasterEggs::killAllApps()
        {
            auto rdkshellPlugin = getRDKShellPlugin();
            if (!rdkshellPlugin)
            {
                return;
            }

            uint32_t status;
            JsonObject getStateResult;
            JsonObject param;

            status = rdkshellPlugin->Invoke<JsonObject, JsonObject>(EASTEREGG_THUNDER_TIMEOUT, "getState", param, getStateResult);
            if (status > 0)
            {
                std::cout << "failed to get state " << std::endl;
            }
            
            // If success is false, the container isn't running so nothing to do
            if (getStateResult["success"].Boolean())
            {
                const JsonArray stateList = getStateResult.HasLabel("state")?getStateResult["state"].Array():JsonArray();
                for (int i=0; i<stateList.Length(); i++)
                {
                    const JsonObject& stateInfo = stateList[i].Object();
                    if (stateInfo.HasLabel("callsign"))
                    {
                       JsonObject destroyRequest, destroyResponse;
                       destroyRequest["callsign"] = stateInfo["callsign"].String();
                       status = rdkshellPlugin->Invoke<JsonObject, JsonObject>(EASTEREGG_THUNDER_TIMEOUT, "destroy", destroyRequest, destroyResponse);
                       if (status > 0)
                       {
                           std::cout << "failed to destroy client " << destroyRequest["callsign"].String() << std::endl;
                       }
                    }
                }
            }
        }

        void EasterEggs::displayAllApps(std::vector<std::string>& callsigns, bool hide)
        {
            auto rdkshellPlugin = getRDKShellPlugin();
            if (!rdkshellPlugin)
            {
                return;
            }

            uint32_t status;
            for (int i=0; i<callsigns.size(); i++)
            {
                   std::string callsign = callsigns[i];
                   JsonObject hideRequest,hideResponse;
                   hideRequest["callsign"] = callsign;
                   hideRequest["visible"] = hide?false:true;
                   status = rdkshellPlugin->Invoke<JsonObject, JsonObject>(EASTEREGG_THUNDER_TIMEOUT, "setVisibility", hideRequest, hideResponse);
                   if (status > 0)
                   {
                       std::cout << "failed to hide client " << callsign << std::endl;
                   }
             }
        }

        void EasterEggs::destroyAllApps(std::vector<std::string>& callsigns)
        {
            auto rdkshellPlugin = getRDKShellPlugin();
            if (!rdkshellPlugin)
            {
                return;
            }

            int32_t status = 0;
            for(int i=0; i<callsigns.size(); i++)
            {
                JsonObject destroyRequest, destroyResponse;
                destroyRequest["callsign"] = callsigns[i];
                status = rdkshellPlugin->Invoke<JsonObject, JsonObject>(EASTEREGG_THUNDER_TIMEOUT, "destroy", destroyRequest, destroyResponse);
                if (status > 0)
                {
                    std::cout << "failed to destroy client " << callsigns[i] << std::endl;
                }
            }
        }

      
        bool EasterEggs::isFactoryAppRunning()
        {
            bool ret = false;
            JsonObject joFactoryModeParams;
            JsonObject joFactoryModeResult;
            joFactoryModeParams.Set("namespace","FactoryTest");
            joFactoryModeParams.Set("key","FactoryMode");
            std::string factoryModeGetInvoke = "org.rdk.PersistentStore.1.getValue";

            std::cout << "attempting to check factory mode \n";
            int32_t status = getThunderControllerClient()->Invoke(EASTEREGG_THUNDER_TIMEOUT, factoryModeGetInvoke.c_str(), joFactoryModeParams, joFactoryModeResult);
            std::cout << "get status: " << status << std::endl;

            if (status == 0 && joFactoryModeResult.HasLabel("value"))
            {
                if (joFactoryModeResult.HasLabel("value"))
                {
                    const std::string valueString = joFactoryModeResult["value"].String();
                    if (valueString == "true")
                    {
                        ret = true;
                    }
                }
            }
            return ret;
        }

        uint32_t EasterEggs::launchFactoryAppWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            if (isFactoryAppRunning())
            {
                std::cout << "factory app is already running, do nothing";
                response["message"] = " factory mode already running";
                returnResponse(false);
            }
          
            bool ret = true;
            char* factoryAppUrl = getenv("RDKSHELL_FACTORY_APP_URL");
            if (NULL == factoryAppUrl)
            {
                std::cout << "factory app url is empty " << std::endl;
                response["message"] = "factory application url is empty";
                returnResponse(false);
            }

            if (parameters.HasLabel("startup"))
            {
                bool startup = parameters["startup"].Boolean();
                if (startup)
                {
                    JsonObject joAgingParams;
                    JsonObject joAgingResult;
                    joAgingParams.Set("namespace","FactoryTest");
                    joAgingParams.Set("key","AgingState");
                    std::string agingGetInvoke = "org.rdk.PersistentStore.1.getValue";

                    std::cout << "attempting to check aging flag \n";
                    int32_t status = getThunderControllerClient()->Invoke(EASTEREGG_THUNDER_TIMEOUT, agingGetInvoke.c_str(), joAgingParams, joAgingResult);
                    std::cout << "get status: " << status << std::endl;

                    if (status > 0)
                    {
                        response["message"] = " unable to check aging flag";
                        returnResponse(false);
                    }

                    if (!joAgingResult.HasLabel("value"))
                    {
                        response["message"] = " aging value not found";
                        returnResponse(false);
                    }

                    const std::string valueString = joAgingResult["value"].String();
                    if (valueString != "true")
                    {
                        std::cout << "aging value is " << valueString << std::endl;
                        response["message"] = " aging is not set for startup";
                        returnResponse(false);
                    }
                }
            }

            auto rdkshellPlugin = getRDKShellPlugin();
            int32_t status = 0;

            if (!rdkshellPlugin)
            {
                response["message"] = "rdkshell plugin initialization failed";
                returnResponse(false);
            }


            std::vector<std::string> callsigns;
            getAllApps(callsigns);
            callsigns.push_back("ResidentApp");
            displayAllApps(callsigns, true);
            /*
            killAllApps();
            JsonObject destroyRequest, destroyResponse;
            destroyRequest["callsign"] = "ResidentApp";
            status = rdkshellPlugin->Invoke<JsonObject, JsonObject>(EASTEREGG_THUNDER_TIMEOUT, "destroy", destroyRequest, destroyResponse);
            if (status > 0)
            {
                std::cout << "failed to destroy ResidentApp"  << std::endl;
            }
            */
            JsonObject launchRequest, launchResponse;
            launchRequest["callsign"] = "factoryapp";
            launchRequest["type"] = "ResidentApp";
            launchRequest["uri"] = std::string(factoryAppUrl);
            launchRequest["focused"] = true;
            status = rdkshellPlugin->Invoke<JsonObject, JsonObject>(EASTEREGG_THUNDER_TIMEOUT, "launch", launchRequest, launchResponse);
            bool launchFactoryResult = launchResponse.HasLabel("success")?launchResponse["success"].Boolean():false;
            if ((status > 0) || (false == launchFactoryResult))
            {
                std::cout << "Launching factory application failed " << std::endl;
                response["message"] = "launch factory app failed";
                ret = false;
                displayAllApps(callsigns, false);
            }
            else
            {
                mFactoryAppRunning = true;
                std::cout << "Launching factory application succeeded " << std::endl;
                destroyAllApps(callsigns);
                JsonObject joFactoryModeParams;
                JsonObject joFactoryModeResult;
                joFactoryModeParams.Set("namespace","FactoryTest");
                joFactoryModeParams.Set("key","FactoryMode");
                joFactoryModeParams.Set("value","true");
                std::string factoryModeSetInvoke = "org.rdk.PersistentStore.1.setValue";

                std::cout << "attempting to set factory mode flag \n";
                uint32_t setStatus = getThunderControllerClient()->Invoke(EASTEREGG_THUNDER_TIMEOUT, factoryModeSetInvoke.c_str(), joFactoryModeParams, joFactoryModeResult);
                std::cout << "set status: " << setStatus << std::endl;
            }

            callsigns.clear();

            returnResponse(ret);
        }

        uint32_t EasterEggs::launchFactoryAppShortcutWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            JsonObject joToFacParams;
            JsonObject joToFacResult;
            joToFacParams.Set("namespace","FactoryTest");
            joToFacParams.Set("key","ToFacFlag");
            std::string toFacGetInvoke = "org.rdk.PersistentStore.1.getValue";

            std::cout << "attempting to check flag \n";
            auto thunderController = getThunderControllerClient();
            int32_t status = thunderController->Invoke(EASTEREGG_THUNDER_TIMEOUT, toFacGetInvoke.c_str(), joToFacParams, joToFacResult);
            std::cout << "get status: " << status << std::endl;

            if (status > 0)
            {
                response["message"] = " unable to check toFac flag";
                returnResponse(false);
            }

            if (!joToFacResult.HasLabel("value"))
            {
                response["message"] = " toFac value not found";
                returnResponse(false);
            }

            const std::string valueString = joToFacResult["value"].String();
            if (valueString == "true")
            {
                std::cout << "factory app is arleady running, do nothing";
                response["message"] = " factory mode already running";
                returnResponse(false);
            }

            if (valueString != "M" && valueString != "m")
            {
                std::cout << "toFac value is " << valueString << std::endl;
                response["message"] = " toFac not in the correct mode";
                returnResponse(false);
            }

            return launchFactoryAppWrapper(parameters, response);
        }

        uint32_t EasterEggs::launchResidentAppWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();

            auto thunderController = getThunderControllerClient();
            bool factoryAppRunning = mFactoryAppRunning?true:isFactoryAppRunning();
            mFactoryAppRunning = false;
            if (factoryAppRunning)
            {
                JsonObject joAgingParams;
                JsonObject joAgingResult;
                joAgingParams.Set("namespace","FactoryTest");
                joAgingParams.Set("key","AgingState");
                std::string agingGetInvoke = "org.rdk.PersistentStore.1.getValue";

                std::cout << "attempting to check aging flag \n";
                int32_t agingStatus = thunderController->Invoke(EASTEREGG_THUNDER_TIMEOUT, agingGetInvoke.c_str(), joAgingParams, joAgingResult);
                std::cout << "aging get status: " << agingStatus << std::endl;

                if (agingStatus == 0 && joAgingResult.HasLabel("value"))
                {
                    const std::string valueString = joAgingResult["value"].String();
                    std::cout << "aging value is " << valueString << std::endl;
                    if (valueString == "true")
                    {
                        std::cout << "do not exit the factory app\n";
                        response["message"] = " aging is true, do not exit the factory app";
                        returnResponse(false);
                    }
                }
                else
                {
                    std::cout << "aging value is not set\n";
                }
            }

            std::vector<std::string> callsigns;
            getAllApps(callsigns);
            callsigns.push_back("factoryapp");

            std::cout << "EasterEgg sending onWillDestroyEvent for factoryapp" << std::endl;
            JsonObject params;
            params["callsign"] = "factoryapp";
            sendNotify(EASTEREGG_EVENT_ON_WILL_DESTROY, params);
            sleep(gWillDestroyEventWaitTime);

            destroyAllApps(callsigns);

            std::cout << "attempting to stop hdmi input...\n";
            JsonObject joStopHdmiParams;
            JsonObject joStopHdmiResult;
            std::string stopHdmiInvoke = "org.rdk.HdmiInput.1.stopHdmiInput";

            std::cout << "attempting to stop hdmi input \n";
            int32_t stopHdmiStatus = thunderController->Invoke(EASTEREGG_THUNDER_TIMEOUT, stopHdmiInvoke.c_str(), joStopHdmiParams, joStopHdmiResult);
            std::cout << "stopHdmiStatus status: " << stopHdmiStatus << std::endl;

            bool ret = true;
            std::string callsign("ResidentApp");
            JsonObject activateParams;
            activateParams.Set("callsign",callsign.c_str());
            JsonObject activateResult;
            int32_t status = thunderController->Invoke(3500, "activate", activateParams, activateResult);

            std::cout << "activate resident app status: " << status << std::endl;
            if (status > 0)
            {
                std::cout << "trying status one more time...\n";
                status = thunderController->Invoke(3500, "activate", activateParams, activateResult);
                std::cout << "activate resident app status: " << status << std::endl;
                if (status > 0)
                {
                    response["message"] = "resident app launch failed";
                    ret = false;
                }
                else
                {
                    ret = true;
                }
            }
            JsonObject joFactoryModeParams;
            JsonObject joFactoryModeResult;
            joFactoryModeParams.Set("namespace","FactoryTest");
            joFactoryModeParams.Set("key","FactoryMode");
            joFactoryModeParams.Set("value","false");
            std::string factoryModeSetInvoke = "org.rdk.PersistentStore.1.setValue";

            std::cout << "attempting to set factory mode flag \n";
            int32_t setStatus = thunderController->Invoke(EASTEREGG_THUNDER_TIMEOUT, factoryModeSetInvoke.c_str(), joFactoryModeParams, joFactoryModeResult);
            std::cout << "set status: " << setStatus << std::endl;
            returnResponse(ret);
        }

        uint32_t EasterEggs::showEasterEggsMenuWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();
            if (false == mEasterEggsMenuEnabled)
            {
                std::cout << "easter eggs menu feature is not enabled" << std::endl;
                response["message"] = "easter eggs menu feature is not enabled";
                returnResponse(false);
            }
            bool ret = true;
            auto rdkshellPlugin = getRDKShellPlugin();
            if (!rdkshellPlugin)
            {
                response["message"] = "rdkshell service is not ready to launch app";
                ret = false;
                returnResponse(ret);
            }

            JsonObject launchRequest, launchResponse;
            launchRequest["callsign"] ="eastereggsmenu";
            launchRequest["type"] = "ResidentApp";
            launchRequest["uri"] = mEasterEggsMenuUrl;
            launchRequest["focused"] = true;
            int32_t status = rdkshellPlugin->Invoke<JsonObject, JsonObject>(EASTEREGG_THUNDER_TIMEOUT, "launch", launchRequest, launchResponse);
            bool launchEasterEggsResult = launchResponse.HasLabel("success")?launchResponse["success"].Boolean():false;
            if ((status > 0) || (false == launchEasterEggsResult))
            {
                std::cout << "Launching easter eggs menu failed " << std::endl;
                response["message"] = "launch easter eggs menu failed";
                ret = false;
            }
            else
            {
                std::cout << "Launching eastereggs menu application succeeded " << std::endl;
            }
            returnResponse(ret);
        }

        uint32_t EasterEggs::toggleFactoryAppWrapper(const JsonObject& parameters, JsonObject& response)
        {
            LOGINFOMETHOD();
            bool ret = true;
            if (isFactoryAppRunning())
            {
                mFactoryAppRunning = true;
                launchResidentAppWrapper(parameters, response);
            }
            else
            {
                launchFactoryAppWrapper(parameters, response);
            }
            returnResponse(ret);
        }

    } // namespace Plugin
} // namespace WPEFramework
