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

using namespace WPEFramework;
using namespace std;

namespace {
const string iarmName = _T("Thunder_Plugins");
}

class SystemServicesTest : public::testing::Test
{
    protected:
    Core::ProxyType<Plugin::SystemServices> systemplugin;
    Core::JSONRPC::Handler& handler;
    Core::JSONRPC::Handler& handlerV2;
    Core::JSONRPC::Connection connection;
    string response;
    
    
private:
    /* data */
public:
    SystemServicesTest()
    :systemplugin(Core::ProxyType<Plugin::SystemServices>::Create())
    ,handler(*systemplugin)
    ,handlerV2(*(systemplugin->GetHandler(2)))
    ,connection(1,0)
    {
    }
   
    virtual void SetUp()
    {
	}

    virtual void TearDown()
    {
    }

    ~SystemServicesTest()
    {
     
    }
};

TEST_F(SystemServicesTest, SystemUptime)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("requestSystemUptime"), _T("{}"), response));

    size_t pos= response.find(":");
    size_t pos1 = response.find(",");

    string systemUptime = "\"systemUptime\":"+response.substr(pos+1,pos1-pos-1);
    string result = response.substr(pos1+1, response.length()-pos1-2);
    EXPECT_EQ(result,string("\"success\":true"));
}
    