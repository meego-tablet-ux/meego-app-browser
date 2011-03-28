/*
 * Copyright (c) 2010, Intel Corporation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following disclaimer 
 * in the documentation and/or other materials provided with the 
 * distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "proxy_gconf_setting.h"
#include <gconf/gconf-client.h>
#include <string.h>
#include <stdlib.h>

void ProxyGconfSettingHelper::ReadProxySetting(ProxySetting* setting)
{
    // Inintial the GConfClient
  GConfClient* gConfClient = gconf_client_get_default();
  if(!gConfClient) return;

  // get the value of iradio
  gchar* mode = gconf_client_get_string(gConfClient, "/system/proxy/mode", NULL);

  if (mode) {
    if(!strcmp(mode, "none"))
      setting->enabled = false;
    else if(!strcmp(mode, "manual"))
      setting->enabled = true;

    g_free(mode);
  }
  
  // get the value of address and port
  gchar* host = gconf_client_get_string(gConfClient, "/system/http_proxy/host", NULL);
  gint port = gconf_client_get_int(gConfClient, "/system/http_proxy/port", NULL);

  if(host) {
    setting->host = host;
    g_free(host);
  }

  setting->port = port;

  // et the value of ignore-hosts
  GSList *list_ignore_host = gconf_client_get_list(gConfClient, 
                              "/system/http_proxy/ignore_hosts", 
                               GCONF_VALUE_STRING, NULL);

  if(list_ignore_host)  {
    for (GSList *it = list_ignore_host; it; it = it->next) {
      setting->ignore_hosts.push_back(std::string(static_cast<char*>(it->data)));
      g_free(it->data);
    }
    g_slist_free(list_ignore_host);
  }

  // lose the GConfClient
  g_object_unref(gConfClient);
}

void ProxyGconfSettingHelper::WriteProxySetting(ProxySetting* setting)
{
  // Initial the GConfClient
  GConfClient* gConfClient = gconf_client_get_default();
  if(!gConfClient) 
    return;

  GSList* list_ignore_hosts =  NULL;
  for (int i = 0; i < setting->ignore_hosts.size(); ++i)
  {
    GString* host = g_string_new(setting->ignore_hosts[i].c_str());
    list_ignore_hosts = g_slist_append(list_ignore_hosts, host->str);
  }

  // set the value
  if (!setting->enabled) {
    gconf_client_set_string(gConfClient, "/system/proxy/mode", "none", NULL);
  } else {
    gconf_client_set_bool(gConfClient, "/system/http_proxy/use_http_proxy", 
                          TRUE, NULL);
    gconf_client_set_string(gConfClient, "/system/proxy/mode", "manual", NULL);
  
    if(setting->host.size() != 0 && setting->port != 0 && setting->port <= 65535) {
      gconf_client_set_string(gConfClient, "/system/http_proxy/host", 
                              setting->host.c_str(), NULL);
      gconf_client_set_int(gConfClient, "/system/http_proxy/port", setting->port, NULL);
    }

    if(list_ignore_hosts)
    {
      gconf_client_set_list(gConfClient, "/system/http_proxy/ignore_hosts", 
                            GCONF_VALUE_STRING, list_ignore_hosts, NULL);
      g_slist_foreach(list_ignore_hosts, (GFunc) g_free, NULL);
      g_slist_free(list_ignore_hosts);
    }

    gconf_client_set_bool(gConfClient, "/system/http_proxy/use_same_proxy", 
                          TRUE, NULL);
  }
    // lose the GConfClient
  g_object_unref(gConfClient);
}
