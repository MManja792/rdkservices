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

#pragma once

#include "Module.h"
#include "utils.h"
#include "AbstractPlugin.h"

namespace WPEFramework {

    namespace Plugin {

        class EasterEggs :  public AbstractPlugin {
        public:
            EasterEggs();
            virtual ~EasterEggs();
            virtual const string Initialize(PluginHost::IShell* service) override;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override;

        public/*members*/:
            static EasterEggs* _instance;

        public /*constants*/:
            static const short API_VERSION_NUMBER_MAJOR;
            static const short API_VERSION_NUMBER_MINOR;
            static const string SERVICE_NAME;

            //methods
            static const string EASTEREGG_METHOD_LAUNCH_FACTORY_APP;
            static const string EASTEREGG_METHOD_LAUNCH_FACTORY_APP_SHORTCUT;
            static const string EASTEREGG_METHOD_LAUNCH_RESIDENT_APP;
            static const string EASTEREGG_METHOD_TOGGLE_FACTORY_APP;
            static const string EASTEREGG_METHOD_SHOW_EASTEREGGS_MENU;

            //events
            static const string EASTEREGG_EVENT_ON_WILL_DESTROY;

        private/*registered methods (wrappers)*/:

            //methods ("parameters" here is "params" from the curl request)
            uint32_t launchFactoryAppWrapper(const JsonObject& parameters, JsonObject& response);
            uint32_t launchFactoryAppShortcutWrapper(const JsonObject& parameters, JsonObject& response);
            uint32_t launchResidentAppWrapper(const JsonObject& parameters, JsonObject& response);
            uint32_t toggleFactoryAppWrapper(const JsonObject& parameters, JsonObject& response);
            uint32_t showEasterEggsMenuWrapper(const JsonObject& parameters, JsonObject& response);
            std::shared_ptr<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> > getThunderControllerClient(std::string callsign="");
            std::shared_ptr<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> > getRDKShellPlugin();

        private/*internal methods*/:
            EasterEggs(const EasterEggs&) = delete;
            EasterEggs& operator=(const EasterEggs&) = delete;
            void getAllApps(std::vector<std::string>& callsigns);
            void killAllApps();
            void displayAllApps(std::vector<std::string>& callsigns, bool hide=false);
            void destroyAllApps(std::vector<std::string>& callsigns);
            bool isFactoryAppRunning();

        private/*members*/:
            std::shared_ptr<WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement>> mRdkShellPlugin;
            bool mFactoryAppRunning;
            std::string mEasterEggsMenuUrl;
            bool mEasterEggsMenuEnabled;
        };
    } // namespace Plugin
} // namespace WPEFramework
