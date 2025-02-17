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
#include "RustAdapter.h"
#include "LocalPlugin.h"
#include "RemotePlugin.h"
#include "Logger.h"
#include <plugins/Channel.h>

namespace WPEFramework {
namespace Plugin {
  SERVICE_REGISTRATION(RustAdapter, 1, 0);
} }

WPEFramework::Plugin::RustAdapter::RustAdapter()
  : m_refcount(1)
{
}

const string
WPEFramework::Plugin::RustAdapter::Initialize(PluginHost::IShell *shell)
{
  /* The RustAdapter plugin should always run in-process
     We run the rust plugin itself either in-process or out-of-process
     base the config setting */

  m_config.FromString(shell->ConfigLine());     

  LOGINFO("RustAdapter::Initialize Config=%s", shell->ConfigLine().c_str());

  if (m_config.OutOfProcess)
    m_impl.reset(new WPEFramework::Plugin::Rust::RemotePlugin(this));
  else
    m_impl.reset(new WPEFramework::Plugin::Rust::LocalPlugin());

  return m_impl->Initialize(shell);
}

void
WPEFramework::Plugin::RustAdapter::Deinitialize(PluginHost::IShell *shell)
{
  return m_impl->Deinitialize(shell);
}

string
WPEFramework::Plugin::RustAdapter::Information() const
{
  return m_impl->Information();
}

void
WPEFramework::Plugin::RustAdapter::AddRef() const
{
  m_refcount++;
}

uint32_t
WPEFramework::Plugin::RustAdapter::Release() const
{
  uint32_t n = m_refcount.fetch_sub(1);
  if (n == 1) {
    delete this;
    return Core::ERROR_DESTRUCTION_SUCCEEDED;
  }

  return Core::ERROR_NONE;
}

#if JSON_RPC_CONTEXT
WPEFramework::Core::ProxyType<WPEFramework::Core::JSONRPC::Message>
WPEFramework::Plugin::RustAdapter::Invoke(
  const WPEFramework::Core::JSONRPC::Context &ctx,
  const WPEFramework::Core::JSONRPC::Message &req)
{
  return m_impl->Invoke(ctx, req);
}
#else
WPEFramework::Core::ProxyType<WPEFramework::Core::JSONRPC::Message> 
  WPEFramework::Plugin::RustAdapter::Invoke(
    const string& token, const uint32_t channelId, const WPEFramework::Core::JSONRPC::Message& req)
{
  return m_impl->Invoke(token, channelId, req);
}
#endif

void
WPEFramework::Plugin::RustAdapter::Activate(
  WPEFramework::PluginHost::IShell *shell)
{
  m_impl->Activate(shell);
}

void
WPEFramework::Plugin::RustAdapter::Deactivate()
{
  m_impl->Deactivate();
}

bool
WPEFramework::Plugin::RustAdapter::Attach(PluginHost::Channel &channel)
{
  return m_impl->Attach(channel);
}

void
WPEFramework::Plugin::RustAdapter::Detach(PluginHost::Channel &channel)
{
  m_impl->Detach(channel);
}

WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>
WPEFramework::Plugin::RustAdapter::Inbound(const string &identifier)
{
  return m_impl->Inbound(identifier);
}

WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>
WPEFramework::Plugin::RustAdapter::Inbound(const uint32_t id,
    const Core::ProxyType<Core::JSON::IElement> &element)
{
  return m_impl->Inbound(id, element);
}