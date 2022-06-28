/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
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
 */

#include <gtest/gtest.h>
#include "SystemServices.h"
#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "source/SystemInfo.h"

using namespace WPEFramework;

namespace WPEFramework
{
    namespace plugin
    {
        class SystemServicesTest: public::testting::Test
        {
            protected:
            IarmBusImplMock iarmBusImplMock;
            IARM_EventHandler_t handlerOnTerritoryChanged;
            IARM_EventHandler_t handlerOnDSTTimeChanged;
        private:
            /* data */
        public:
            SystemServicesTest(/* args */):systemplugin(Core::ProxyType<Plugin::SystemServices>::Create()),
            , handler(*systemplugin)
            ,connection(1,0)
            ,handlerV2(*(systemplugin->GetHandler(2)))
        {
        }
        virtual void SetUp()
        {
            IarmBus::getInstance().impl = &iarmBusImplMock_;
            PluginHost::IFactories::Assign(&factoriesImplementation_);
        }

        virtual void TearDown()
        {
            IarmBus::getInstance().impl = nullptr;
            PluginHost::IFactories::Assign(nullptr);
        }

        ~SystemServicesTest()
        {
            PluginHost::IFactories::Assign(nullptr);
        }
    };
    } // namespace plugin
} // namespace WPEFramework

TEST_F(SystemServicesTest, RegisteredMethods)
{
    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Exists(_T("getWakeupReason")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getXconfParams")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getTerritory")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setTerritory")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("hasRebootBeenRequested");
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("isGzEnabled");

    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("isOptOutTelemetry")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("queryMocaStatus")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("requestSystemUptime");
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setCachedValue");

    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("removeCacheKey")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setDeepSleepTimer")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setFirmwareRebootDelay");
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setMode");

    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setNetworkStandbyMode")));

}

TEST_F(SystemServicesTest, Plugin)
{
       // called by FrameRate::InitializeIARM, FrameRate::DeinitializeIARM
    EXPECT_CALL(iarmBusImplMock, IARM_Bus_IsConnected(::testing::_, ::testing::_))
        .Times(3)
        .WillOnce(::testing::Invoke(
            [](const char* memberName, int* isRegistered) {
                if (iarmName == string(memberName)) {
                    *isRegistered = 0;
                    return IARM_RESULT_SUCCESS;
                }
                return IARM_RESULT_INVALID_PARAM;
            }))
        .WillOnce(::testing::Invoke(
            [](const char* memberName, int* isRegistered) {
                if (iarmName == string(memberName)) {
                    *isRegistered = 1;
                    return IARM_RESULT_SUCCESS;
                }
                return IARM_RESULT_INVALID_PARAM;
            }))
        .WillOnce(::testing::Invoke(
            [](const char* memberName, int* isRegistered) {
                if (iarmName == string(memberName)) {
                    *isRegistered = 1;
                    return IARM_RESULT_SUCCESS;
                }
                return IARM_RESULT_INVALID_PARAM;
            }));

    // called by FrameRate::InitializeIARM
    EXPECT_CALL(iarmBusImplMock, IARM_Bus_Init(::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [](const char* name) {
                if (iarmName == string(name)) {
                    return IARM_RESULT_SUCCESS;
                }
                return IARM_RESULT_INVALID_PARAM;
            }));

    // called by FrameRate::InitializeIARM
    EXPECT_CALL(iarmBusImplMock, IARM_Bus_Connect())
        .Times(1)
        .WillOnce(::testing::Return(IARM_RESULT_SUCCESS));

    // called by FrameRate::InitializeIARM
    EXPECT_CALL(iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke(
            [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE)) {
                    handlerOnTerritoryChanged = handler;
                    return IARM_RESULT_SUCCESS;
                }
                return IARM_RESULT_INVALID_PARAM;
            }))
        .WillOnce(::testing::Invoke(
            [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE)) {
                    handlerOnDSTTimeChanged = handler;
                    return IARM_RESULT_SUCCESS;
                }
                return IARM_RESULT_INVALID_PARAM;
            }));

    // called by FrameRate::DeinitializeIARM
    EXPECT_CALL(iarmBusImplMock, IARM_Bus_UnRegisterEventHandler(::testing::_, ::testing::_))
        .Times(2)
        .WillOnce(::testing::Invoke(
            [&](const char* ownerName, IARM_EventId_t eventId) {
                if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE)) {
                    handlerOnTerritoryChanged = nullptr;
                    return IARM_RESULT_SUCCESS;
                }
                return IARM_RESULT_INVALID_PARAM;
            }))
        .WillOnce(::testing::Invoke(
            [&](const char* ownerName, IARM_EventId_t eventId) {
                if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE)) {
                    handlerOnDSTTimeChanged = nullptr;
                    return IARM_RESULT_SUCCESS;
                }
                return IARM_RESULT_INVALID_PARAM;
            }));

    //it will fail as we don't have terrirtory text
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setTerritory"), _T("{\"territory\":USA}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

    //it will fail as we don't have terrirtory text
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getTerritory"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"territory\":\"USA\",\"success\":true}"));


    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Invoke(connection, _T("getWakeupReason"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"wakeupReason\":\"WAKEUP_REASON_UNKNOWN\",\"success\":false}"));

    EXPECT_EQ(Core::ERROR_NONE, handlerV2.Invoke(connection, _T("getXconfParams"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"wakeupReason\":\"WAKEUP_REASON_UNKNOWN\",\"success\":false}"));
    
    // check of deprecated api should be checked???
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("isGzEnabled"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"enabled\":\"false\",\"deprecated\":\"true\",\"success\":false}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("isOptOutTelemetry"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"Opt-Out\":\"false\",\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("queryMocaStatus"), _T("{}"), response));
    EXPECT_EQ(response, string("{\"mocaEnabled\":\"false\",\"success\":true}"));

    // check this call is correct as return value will current time , can't be compared.
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("requestSystemUptime"), _T("{\"key\":test}"), response));
    EXPECT_EQ(response, string("{\"systemUptime\":\"\",\"success\":true}"));

    // check of deprecated api should be checked???
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setCachedValue"), _T("{\"key\":test}",\"value\":1"), response));
    EXPECT_EQ(response, string("{\"deprecated\":\"true\",\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("removeCacheKey"), _T("{\"key\":test}"), response));
    EXPECT_EQ(response, string("{\"deprecated\":\"true\",\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setDeepSleepTimer"), _T("{\"seconds\":5}",\"value\":1}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setFirmwareRebootDelay"), _T("{\"delaySeconds\":5}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setMode"), _T("{\"modeInfo\":\"mode\":normal,\"duration\":1"}"), response));
    EXPECT_EQ(response, string("{\"success\":false}"));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setNetworkStandbyMode"), _T("{\"nwStandby\":standaby}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
    

    // JSON-RPC events

    auto dispatcher = static_cast<PluginHost::IDispatcher*>(
        plugin->QueryInterface(PluginHost::IDispatcher::ID));
    EXPECT_TRUE(dispatcher != nullptr);

    dispatcher->Activate(&service);

    EXPECT_TRUE(handlerOnTerritoryChanged != nullptr);
    EXPECT_TRUE(handlerOnDSTTimeChanged != nullptr);

      handler.Subscribe(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);
    handler.Subscribe(0, _T("onDisplayFrameRateChanged"), _T("org.rdk.FrameRate"), message);

    handlerOnTerritoryChanged(
        IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_PRECHANGE, nullptr, 0);

    handlerOnDisplayFrameRateChanged(
        IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_DISPLAY_FRAMRATE_POSTCHANGE, nullptr, 0);

    handler.Unsubscribe(0, _T("onDisplayFrameRateChanging"), _T("org.rdk.FrameRate"), message);
    handler.Unsubscribe(0, _T("onDisplayFrameRateChanged"), _T("org.rdk.FrameRate"), message);

    dispatcher->Deactivate();

    dispatcher->Release();

    // Deinitialize

    plugin->Deinitialize(nullptr);
}
