// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/cros/network_library.h"

#include <algorithm>

#include "app/l10n_util.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/chromeos/cros/cros_library.h"
#include "grit/generated_resources.h"

namespace chromeos {

// Helper function to wrap Html with <th> tag.
static std::string WrapWithTH(std::string text) {
  return "<th>" + text + "</th>";
}

// Helper function to wrap Html with <td> tag.
static std::string WrapWithTD(std::string text) {
  return "<td>" + text + "</td>";
}

// Helper function to create an Html table header for a Network.
static std::string ToHtmlTableHeader(Network* network) {
  std::string str;
  if (network->type() == TYPE_WIFI || network->type() == TYPE_CELLULAR) {
    str += WrapWithTH("Name") + WrapWithTH("Auto-Connect") +
        WrapWithTH("Strength");
    if (network->type() == TYPE_WIFI)
      str += WrapWithTH("Encryption") + WrapWithTH("Passphrase") +
          WrapWithTH("Identity") + WrapWithTH("Certificate");
  }
  str += WrapWithTH("State") + WrapWithTH("Error") + WrapWithTH("IP Address");
  return str;
}

// Helper function to create an Html table row for a Network.
static std::string ToHtmlTableRow(Network* network) {
  std::string str;
  if (network->type() == TYPE_WIFI || network->type() == TYPE_CELLULAR) {
    WirelessNetwork* wireless = static_cast<WirelessNetwork*>(network);
    str += WrapWithTD(wireless->name()) +
        WrapWithTD(base::IntToString(wireless->auto_connect())) +
        WrapWithTD(base::IntToString(wireless->strength()));
    if (network->type() == TYPE_WIFI) {
      WifiNetwork* wifi = static_cast<WifiNetwork*>(network);
      str += WrapWithTD(wifi->GetEncryptionString()) +
          WrapWithTD(std::string(wifi->passphrase().length(), '*')) +
          WrapWithTD(wifi->identity()) + WrapWithTD(wifi->cert_path());
    }
  }
  str += WrapWithTD(network->GetStateString()) +
      WrapWithTD(network->failed() ? network->GetErrorString() : "") +
      WrapWithTD(network->ip_address());
  return str;
}

// Safe string constructor since we can't rely on non NULL pointers
// for string values from libcros.
static std::string SafeString(const char* s) {
  return s ? std::string(s) : std::string();
}

////////////////////////////////////////////////////////////////////////////////
// Network

void Network::Clear() {
  type_ = TYPE_UNKNOWN;
  state_ = STATE_UNKNOWN;
  error_ = ERROR_UNKNOWN;
  service_path_.clear();
  device_path_.clear();
  ip_address_.clear();
}

void Network::ConfigureFromService(const ServiceInfo& service) {
  type_ = service.type;
  state_ = service.state;
  error_ = service.error;
  service_path_ = SafeString(service.service_path);
  device_path_ = SafeString(service.device_path);
  ip_address_.clear();
  // If connected, get ip config.
  if (connected() && service.device_path) {
    IPConfigStatus* ipconfig_status = ListIPConfigs(service.device_path);
    if (ipconfig_status) {
      for (int i = 0; i < ipconfig_status->size; i++) {
        IPConfig ipconfig = ipconfig_status->ips[i];
        if (strlen(ipconfig.address) > 0)
          ip_address_ = ipconfig.address;
      }
      FreeIPConfigStatus(ipconfig_status);
    }
  }
}

// Used by GetHtmlInfo() which is called from the about:network handler.
std::string Network::GetStateString() {
  switch (state_) {
    case STATE_UNKNOWN:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_UNKNOWN);
    case STATE_IDLE:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_IDLE);
    case STATE_CARRIER:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_CARRIER);
    case STATE_ASSOCIATION:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_ASSOCIATION);
    case STATE_CONFIGURATION:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_CONFIGURATION);
    case STATE_READY:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_READY);
    case STATE_DISCONNECT:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_DISCONNECT);
    case STATE_FAILURE:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_FAILURE);
    case STATE_ACTIVATION_FAILURE:
      return l10n_util::GetStringUTF8(
          IDS_CHROMEOS_NETWORK_STATE_ACTIVATION_FAILURE);
    default:
      // Usually no default, but changes to libcros may add states.
      break;
  }
  return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_UNRECOGNIZED);
}

std::string Network::GetErrorString() {
  switch (error_) {
    case ERROR_UNKNOWN:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_ERROR_UNKNOWN);
    case ERROR_OUT_OF_RANGE:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_ERROR_OUT_OF_RANGE);
    case ERROR_PIN_MISSING:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_ERROR_PIN_MISSING);
    case ERROR_DHCP_FAILED:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_ERROR_DHCP_FAILED);
    case ERROR_CONNECT_FAILED:
      return l10n_util::GetStringUTF8(
          IDS_CHROMEOS_NETWORK_ERROR_CONNECT_FAILED);
    case ERROR_BAD_PASSPHRASE:
      return l10n_util::GetStringUTF8(
          IDS_CHROMEOS_NETWORK_ERROR_BAD_PASSPHRASE);
    case ERROR_BAD_WEPKEY:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_ERROR_BAD_WEPKEY);
    case ERROR_ACTIVATION_FAILED:
      return l10n_util::GetStringUTF8(
          IDS_CHROMEOS_NETWORK_ERROR_ACTIVATION_FAILED);
    case ERROR_NEED_EVDO:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_ERROR_NEED_EVDO);
    case ERROR_NEED_HOME_NETWORK:
      return l10n_util::GetStringUTF8(
          IDS_CHROMEOS_NETWORK_ERROR_NEED_HOME_NETWORK);
    case ERROR_OTASP_FAILED:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_ERROR_OTASP_FAILED);
    case ERROR_AAA_FAILED:
      return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_ERROR_AAA_FAILED);
    default:
      // Usually no default, but changes to libcros may add errors.
      break;
  }
  return l10n_util::GetStringUTF8(IDS_CHROMEOS_NETWORK_STATE_UNRECOGNIZED);
}

////////////////////////////////////////////////////////////////////////////////
// WirelessNetwork

void WirelessNetwork::Clear() {
  Network::Clear();
  name_.clear();
  strength_ = 0;
  auto_connect_ = false;
  favorite_ = false;
}

void WirelessNetwork::ConfigureFromService(const ServiceInfo& service) {
  Network::ConfigureFromService(service);
  name_ = SafeString(service.name);
  strength_ = service.strength;
  auto_connect_ = service.auto_connect;
  favorite_ = service.favorite;
}

////////////////////////////////////////////////////////////////////////////////
// CellularNetwork

bool CellularNetwork::StartActivation() const {
  // TODO(ers, jglasgow): Kick of device activation process.
  return true;
}

void CellularNetwork::Clear() {
  WirelessNetwork::Clear();
}

void CellularNetwork::ConfigureFromService(const ServiceInfo& service) {
  WirelessNetwork::ConfigureFromService(service);
  activation_state_ = service.activation_state;
  network_technology_ = service.network_technology;
  roaming_state_ = service.roaming_state;
  restricted_pool_ = service.restricted_pool;
  // Carrier Info
  if (service.carrier_info) {
    operator_name_ = SafeString(service.carrier_info->operator_name);
    operator_code_ = SafeString(service.carrier_info->operator_code);
    payment_url_ = SafeString(service.carrier_info->payment_url);
  }
  // Device Info
  if (service.device_info) {
    meid_ = SafeString(service.device_info->MEID);
    imei_ = SafeString(service.device_info->IMEI);
    imsi_ = SafeString(service.device_info->IMSI);
    esn_ = SafeString(service.device_info->ESN);
    mdn_ = SafeString(service.device_info->MDN);
    min_ = SafeString(service.device_info->MIN);
    model_id_ = SafeString(service.device_info->model_id);
    manufacturer_ = SafeString(service.device_info->manufacturer);
    firmware_revision_ = SafeString(service.device_info->firmware_revision);
    hardware_revision_ = SafeString(service.device_info->hardware_revision);
    last_update_ = SafeString(service.device_info->last_update);
    prl_version_ = service.device_info->PRL_version;
  }
}

////////////////////////////////////////////////////////////////////////////////
// WifiNetwork

void WifiNetwork::Clear() {
  WirelessNetwork::Clear();
  encryption_ = SECURITY_NONE;
  passphrase_.clear();
  identity_.clear();
  cert_path_.clear();
}

void WifiNetwork::ConfigureFromService(const ServiceInfo& service) {
  WirelessNetwork::ConfigureFromService(service);
  encryption_ = service.security;
  passphrase_ = SafeString(service.passphrase);
  identity_ = SafeString(service.identity);
  cert_path_ = SafeString(service.cert_path);
}

std::string WifiNetwork::GetEncryptionString() {
  switch (encryption_) {
    case SECURITY_UNKNOWN:
      break;
    case SECURITY_NONE:
      return "";
    case SECURITY_WEP:
      return "WEP";
    case SECURITY_WPA:
      return "WPA";
    case SECURITY_RSN:
      return "RSN";
    case SECURITY_8021X:
      return "8021X";
  }
  return "Unknown";}

////////////////////////////////////////////////////////////////////////////////
// NetworkLibrary

class NetworkLibraryImpl : public NetworkLibrary  {
 public:
  NetworkLibraryImpl()
      : network_status_connection_(NULL),
        data_plan_monitor_(NULL),
        available_devices_(0),
        enabled_devices_(0),
        connected_devices_(0),
        offline_mode_(false) {
    if (CrosLibrary::Get()->EnsureLoaded()) {
      Init();
    } else {
      InitTestData();
    }
  }

  ~NetworkLibraryImpl() {
    if (network_status_connection_) {
      DisconnectMonitorNetwork(network_status_connection_);
    }
    if (data_plan_monitor_) {
      DisconnectDataPlanUpdateMonitor(data_plan_monitor_);
    }
  }

  void AddObserver(Observer* observer) {
    observers_.AddObserver(observer);
  }

  void RemoveObserver(Observer* observer) {
    observers_.RemoveObserver(observer);
  }

  virtual const EthernetNetwork& ethernet_network() const { return ethernet_; }
  virtual bool ethernet_connecting() const { return ethernet_.connecting(); }
  virtual bool ethernet_connected() const { return ethernet_.connected(); }

  virtual const std::string& wifi_name() const { return wifi_.name(); }
  virtual bool wifi_connecting() const { return wifi_.connecting(); }
  virtual bool wifi_connected() const { return wifi_.connected(); }
  virtual int wifi_strength() const { return wifi_.strength(); }

  virtual const std::string& cellular_name() const { return cellular_.name(); }
  virtual const std::string& cellular_service_path() const {
    return cellular_.service_path(); }
  virtual bool cellular_connecting() const { return cellular_.connecting(); }
  virtual bool cellular_connected() const { return cellular_.connected(); }
  virtual int cellular_strength() const { return cellular_.strength(); }

  bool Connected() const {
    return ethernet_connected() || wifi_connected() || cellular_connected();
  }

  bool Connecting() const {
    return ethernet_connecting() || wifi_connecting() || cellular_connecting();
  }

  const std::string& IPAddress() const {
    // Returns highest priority IP address.
    if (ethernet_connected())
      return ethernet_.ip_address();
    if (wifi_connected())
      return wifi_.ip_address();
    if (cellular_connected())
      return cellular_.ip_address();
    return ethernet_.ip_address();
  }

  virtual const WifiNetworkVector& wifi_networks() const {
    return wifi_networks_;
  }

  virtual const WifiNetworkVector& remembered_wifi_networks() const {
    return remembered_wifi_networks_;
  }

  virtual const CellularNetworkVector& cellular_networks() const {
    return cellular_networks_;
  }

  virtual const CellularNetworkVector& remembered_cellular_networks() const {
    return remembered_cellular_networks_;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool FindWifiNetworkByPath(
      const std::string& path, WifiNetwork* result) const {
    const WifiNetwork* wifi =
        GetWirelessNetworkByPath(wifi_networks_, path);
    if (wifi) {
      if (result)
        *result = *wifi;
      return true;
    }
    return false;
  }

  bool FindCellularNetworkByPath(
      const std::string& path, CellularNetwork* result) const {
    const CellularNetwork* cellular =
        GetWirelessNetworkByPath(cellular_networks_, path);
    if (cellular) {
      if (result)
        *result = *cellular;
      return true;
    }
    return false;
  }

  void RequestWifiScan() {
    if (CrosLibrary::Get()->EnsureLoaded()) {
      RequestScan(TYPE_WIFI);
    }
  }

  bool GetWifiAccessPoints(WifiAccessPointVector* result) {
    if (!CrosLibrary::Get()->EnsureLoaded())
      return false;
    DeviceNetworkList* network_list = GetDeviceNetworkList();
    if (network_list == NULL)
      return false;
    result->clear();
    result->reserve(network_list->network_size);
    const base::Time now = base::Time::Now();
    for (size_t i = 0; i < network_list->network_size; ++i) {
      DCHECK(network_list->networks[i].address);
      DCHECK(network_list->networks[i].name);
      WifiAccessPoint ap;
      ap.mac_address = SafeString(network_list->networks[i].address);
      ap.name = SafeString(network_list->networks[i].name);
      ap.timestamp = now -
          base::TimeDelta::FromSeconds(network_list->networks[i].age_seconds);
      ap.signal_strength = network_list->networks[i].strength;
      ap.channel = network_list->networks[i].channel;
      result->push_back(ap);
    }
    FreeDeviceNetworkList(network_list);
    return true;
  }

  void ConnectToWifiNetwork(WifiNetwork network,
                            const std::string& password,
                            const std::string& identity,
                            const std::string& certpath) {
    if (CrosLibrary::Get()->EnsureLoaded()) {
      if (ConnectToNetworkWithCertInfo(network.service_path().c_str(),
          password.empty() ? NULL : password.c_str(),
          identity.empty() ? NULL : identity.c_str(),
          certpath.empty() ? NULL : certpath.c_str())) {
        // Update local cache and notify listeners.
        WifiNetwork* wifi = GetWirelessNetworkByPath(
            wifi_networks_, network.service_path());
        if (wifi) {
          wifi->set_passphrase(password);
          wifi->set_identity(identity);
          wifi->set_cert_path(certpath);
          wifi->set_connecting(true);
          wifi_ = *wifi;
        }
        NotifyNetworkChanged();
      }
    }
  }

  void ConnectToWifiNetwork(const std::string& ssid,
                            const std::string& password,
                            const std::string& identity,
                            const std::string& certpath,
                            bool auto_connect) {
    if (CrosLibrary::Get()->EnsureLoaded()) {
      // First create a service from hidden network.
      ServiceInfo* service = GetWifiService(ssid.c_str(),
                                            SECURITY_UNKNOWN);
      if (service) {
        // Set auto-connect.
        SetAutoConnect(service->service_path, auto_connect);
        // Now connect to that service.
        ConnectToNetworkWithCertInfo(service->service_path,
            password.empty() ? NULL : password.c_str(),
            identity.empty() ? NULL : identity.c_str(),
            certpath.empty() ? NULL : certpath.c_str());

        // Clean up ServiceInfo object.
        FreeServiceInfo(service);
      } else {
        LOG(WARNING) << "Cannot find hidden network: " << ssid;
        // TODO(chocobo): Show error message.
      }
    }
  }

  void ConnectToCellularNetwork(CellularNetwork network) {
    if (CrosLibrary::Get()->EnsureLoaded()) {
      if (ConnectToNetwork(network.service_path().c_str(), NULL)) {
        // Update local cache and notify listeners.
        CellularNetwork* cellular = GetWirelessNetworkByPath(
            cellular_networks_, network.service_path());
        if (cellular) {
          cellular->set_connecting(true);
          cellular_ = *cellular;
        }
        NotifyNetworkChanged();
      }
    }
  }

  void DisconnectFromWirelessNetwork(const WirelessNetwork& network) {
    if (CrosLibrary::Get()->EnsureLoaded()) {
      if (DisconnectFromNetwork(network.service_path().c_str())) {
        // Update local cache and notify listeners.
        if (network.type() == TYPE_WIFI) {
          WifiNetwork* wifi = GetWirelessNetworkByPath(
              wifi_networks_, network.service_path());
          if (wifi) {
            wifi->set_connected(false);
            wifi_ = WifiNetwork();
          }
        } else if (network.type() == TYPE_CELLULAR) {
          CellularNetwork* cellular = GetWirelessNetworkByPath(
              cellular_networks_, network.service_path());
          if (cellular) {
            cellular->set_connected(false);
            cellular_ = CellularNetwork();
          }
        }
        NotifyNetworkChanged();
      }
    }
  }

  void SaveCellularNetwork(const CellularNetwork& network) {
    // Update the wifi network in the local cache.
    CellularNetwork* cellular = GetWirelessNetworkByPath(
        cellular_networks_, network.service_path());
    if (cellular)
      *cellular = network;

    // Update the cellular network with libcros.
    if (CrosLibrary::Get()->EnsureLoaded()) {
      SetAutoConnect(network.service_path().c_str(), network.auto_connect());
    }
  }

  void SaveWifiNetwork(const WifiNetwork& network) {
    // Update the wifi network in the local cache.
    WifiNetwork* wifi = GetWirelessNetworkByPath(
        wifi_networks_, network.service_path());
    if (wifi)
      *wifi = network;

    // Update the wifi network with libcros.
    if (CrosLibrary::Get()->EnsureLoaded()) {
      SetPassphrase(
          network.service_path().c_str(), network.passphrase().c_str());
      SetIdentity(network.service_path().c_str(), network.identity().c_str());
      SetCertPath(network.service_path().c_str(), network.cert_path().c_str());
      SetAutoConnect(network.service_path().c_str(), network.auto_connect());
    }
  }

  void ForgetWirelessNetwork(const std::string& service_path) {
    if (CrosLibrary::Get()->EnsureLoaded()) {
      if (DeleteRememberedService(service_path.c_str())) {
        // Update local cache and notify listeners.
        std::remove_if(remembered_wifi_networks_.begin(),
                       remembered_wifi_networks_.end(),
                       WirelessNetwork::ServicePathEq(service_path));
        std::remove_if(remembered_cellular_networks_.begin(),
                       remembered_cellular_networks_.end(),
                       WirelessNetwork::ServicePathEq(service_path));
        NotifyNetworkChanged();
      }
    }
  }

  virtual bool ethernet_available() const {
    return available_devices_ & (1 << TYPE_ETHERNET);
  }
  virtual bool wifi_available() const {
    return available_devices_ & (1 << TYPE_WIFI);
  }
  virtual bool cellular_available() const {
    return available_devices_ & (1 << TYPE_CELLULAR);
  }

  virtual bool ethernet_enabled() const {
    return enabled_devices_ & (1 << TYPE_ETHERNET);
  }
  virtual bool wifi_enabled() const {
    return enabled_devices_ & (1 << TYPE_WIFI);
  }
  virtual bool cellular_enabled() const {
    return enabled_devices_ & (1 << TYPE_CELLULAR);
  }

  virtual bool offline_mode() const { return offline_mode_; }

  void EnableEthernetNetworkDevice(bool enable) {
    EnableNetworkDeviceType(TYPE_ETHERNET, enable);
  }

  void EnableWifiNetworkDevice(bool enable) {
    EnableNetworkDeviceType(TYPE_WIFI, enable);
  }

  void EnableCellularNetworkDevice(bool enable) {
    EnableNetworkDeviceType(TYPE_CELLULAR, enable);
  }

  void EnableOfflineMode(bool enable) {
    if (!CrosLibrary::Get()->EnsureLoaded())
      return;

    // If network device is already enabled/disabled, then don't do anything.
    if (enable && offline_mode_) {
      LOG(INFO) << "Trying to enable offline mode when it's already enabled. ";
      return;
    }
    if (!enable && !offline_mode_) {
      LOG(INFO) <<
          "Trying to disable offline mode when it's already disabled. ";
      return;
    }

    if (SetOfflineMode(enable)) {
      offline_mode_ = enable;
    }
  }

  NetworkIPConfigVector GetIPConfigs(const std::string& device_path) {
    NetworkIPConfigVector ipconfig_vector;
    if (!device_path.empty()) {
      IPConfigStatus* ipconfig_status = ListIPConfigs(device_path.c_str());
      if (ipconfig_status) {
        for (int i = 0; i < ipconfig_status->size; i++) {
          IPConfig ipconfig = ipconfig_status->ips[i];
          ipconfig_vector.push_back(
              NetworkIPConfig(device_path, ipconfig.type, ipconfig.address,
                              ipconfig.netmask, ipconfig.gateway,
                              ipconfig.name_servers));
        }
        FreeIPConfigStatus(ipconfig_status);
        // Sort the list of ip configs by type.
        std::sort(ipconfig_vector.begin(), ipconfig_vector.end());
      }
    }
    return ipconfig_vector;
  }

  std::string GetHtmlInfo(int refresh) {
    std::string output;
    output.append("<html><head><title>About Network</title>");
    if (refresh > 0)
      output.append("<meta http-equiv=\"refresh\" content=\"" +
          base::IntToString(refresh) + "\"/>");
    output.append("</head><body>");
    if (refresh > 0) {
      output.append("(Auto-refreshing page every " +
                    base::IntToString(refresh) + "s)");
    } else {
      output.append("(To auto-refresh this page: about:network/&lt;secs&gt;)");
    }

    output.append("<h3>Ethernet:</h3><table border=1>");
    if (ethernet_enabled()) {
      output.append("<tr>" + ToHtmlTableHeader(&ethernet_) + "</tr>");
      output.append("<tr>" + ToHtmlTableRow(&ethernet_) + "</tr>");
    }

    output.append("</table><h3>Wifi:</h3><table border=1>");
    for (size_t i = 0; i < wifi_networks_.size(); ++i) {
      if (i == 0)
        output.append("<tr>" + ToHtmlTableHeader(&wifi_networks_[i]) + "</tr>");
      output.append("<tr>" + ToHtmlTableRow(&wifi_networks_[i]) + "</tr>");
    }

    output.append("</table><h3>Cellular:</h3><table border=1>");
    for (size_t i = 0; i < cellular_networks_.size(); ++i) {
      if (i == 0)
        output.append("<tr>" + ToHtmlTableHeader(&cellular_networks_[i]) +
            "</tr>");
      output.append("<tr>" + ToHtmlTableRow(&cellular_networks_[i]) + "</tr>");
    }

    output.append("</table><h3>Remembered Wifi:</h3><table border=1>");
    for (size_t i = 0; i < remembered_wifi_networks_.size(); ++i) {
      if (i == 0)
        output.append(
            "<tr>" + ToHtmlTableHeader(&remembered_wifi_networks_[i]) +
            "</tr>");
      output.append("<tr>" + ToHtmlTableRow(&remembered_wifi_networks_[i]) +
          "</tr>");
    }

    output.append("</table><h3>Remembered Cellular:</h3><table border=1>");
    for (size_t i = 0; i < remembered_cellular_networks_.size(); ++i) {
      if (i == 0)
        output.append("<tr>" +
            ToHtmlTableHeader(&remembered_cellular_networks_[i]) + "</tr>");
      output.append("<tr>" + ToHtmlTableRow(&remembered_cellular_networks_[i]) +
          "</tr>");
    }

    output.append("</table></body></html>");
    return output;
  }

 private:
  static void NetworkStatusChangedHandler(void* object) {
    NetworkLibraryImpl* network = static_cast<NetworkLibraryImpl*>(object);
    DCHECK(network);
    network->UpdateNetworkStatus();
  }

  static void DataPlanUpdateHandler(void* object,
                                    const char* modem_service_path,
                                    const CellularDataPlanList* dataplan) {
    NetworkLibraryImpl* network = static_cast<NetworkLibraryImpl*>(object);
    DCHECK(network);
    // Store data plan for currently connected cellular network.
    if (network->cellular_service_path().compare(modem_service_path) == 0) {
      if (dataplan != NULL) {
        network->UpdateCellularDataPlan(*dataplan);
      }
    }
  }

  static void ParseSystem(SystemInfo* system,
      EthernetNetwork* ethernet,
      WifiNetworkVector* wifi_networks,
      CellularNetworkVector* cellular_networks,
      WifiNetworkVector* remembered_wifi_networks,
      CellularNetworkVector* remembered_cellular_networks) {
    DLOG(INFO) << "ParseSystem:";
    ethernet->Clear();
    for (int i = 0; i < system->service_size; i++) {
      const ServiceInfo service = *system->GetServiceInfo(i);
      DLOG(INFO) << "  (" << service.type <<
                    ") " << service.name <<
                    " mode=" << service.mode <<
                    " state=" << service.state <<
                    " sec=" << service.security <<
                    " req=" << service.passphrase_required <<
                    " pass=" << service.passphrase <<
                    " id=" << service.identity <<
                    " certpath=" << service.cert_path <<
                    " str=" << service.strength <<
                    " fav=" << service.favorite <<
                    " auto=" << service.auto_connect <<
                    " error=" << service.error;
      // Once a connected ethernet service is found, disregard other ethernet
      // services that are also found
      if (service.type == TYPE_ETHERNET && !(ethernet->connected()))
        ethernet->ConfigureFromService(service);
      else if (service.type == TYPE_WIFI)
        wifi_networks->push_back(WifiNetwork(service));
      else if (service.type == TYPE_CELLULAR)
        cellular_networks->push_back(CellularNetwork(service));
    }
    DLOG(INFO) << "Remembered networks:";
    for (int i = 0; i < system->remembered_service_size; i++) {
      const ServiceInfo& service = *system->GetRememberedServiceInfo(i);
      // Only serices marked as auto_connect are considered remembered networks.
      // TODO(chocobo): Don't add to remembered service if currently available.
      if (service.auto_connect) {
        DLOG(INFO) << "  (" << service.type <<
                      ") " << service.name <<
                      " mode=" << service.mode <<
                      " sec=" << service.security <<
                      " pass=" << service.passphrase <<
                      " id=" << service.identity <<
                      " certpath=" << service.cert_path <<
                      " auto=" << service.auto_connect;
        if (service.type == TYPE_WIFI)
          remembered_wifi_networks->push_back(WifiNetwork(service));
        else if (service.type == TYPE_CELLULAR)
          remembered_cellular_networks->push_back(CellularNetwork(service));
      }
    }
  }

  void Init() {
    // First, get the currently available networks.  This data is cached
    // on the connman side, so the call should be quick.
    LOG(INFO) << "Getting initial CrOS network info.";
    UpdateSystemInfo();

    LOG(INFO) << "Registering for network status updates.";
    // Now, register to receive updates on network status.
    network_status_connection_ = MonitorNetwork(&NetworkStatusChangedHandler,
                                                this);
    LOG(INFO) << "Registering for cellular data plan updates.";
    data_plan_monitor_ = MonitorCellularDataPlan(&DataPlanUpdateHandler, this);
  }

  void InitTestData() {
    ethernet_.Clear();
    ethernet_.set_connected(true);

    wifi_networks_.clear();
    WifiNetwork wifi1 = WifiNetwork();
    wifi1.set_service_path("fw1");
    wifi1.set_name("Fake Wifi 1");
    wifi1.set_strength(90);
    wifi1.set_connected(false);
    wifi1.set_encryption(SECURITY_NONE);
    wifi_networks_.push_back(wifi1);

    WifiNetwork wifi2 = WifiNetwork();
    wifi2.set_service_path("fw2");
    wifi2.set_name("Fake Wifi 2");
    wifi2.set_strength(70);
    wifi2.set_connected(true);
    wifi2.set_encryption(SECURITY_WEP);
    wifi_networks_.push_back(wifi2);

    WifiNetwork wifi3 = WifiNetwork();
    wifi3.set_service_path("fw3");
    wifi3.set_name("Fake Wifi 3");
    wifi3.set_strength(50);
    wifi3.set_connected(false);
    wifi3.set_encryption(SECURITY_WEP);
    wifi_networks_.push_back(wifi3);

    wifi_ = wifi2;

    cellular_networks_.clear();

    cellular_networks_.clear();
    CellularNetwork cellular1 = CellularNetwork();
    cellular1.set_service_path("fc1");
    cellular1.set_name("Fake Cellular 1");
    cellular1.set_strength(90);
    cellular1.set_connected(false);
    cellular_networks_.push_back(cellular1);

    CellularNetwork cellular2 = CellularNetwork();
    cellular2.set_service_path("fc2");
    cellular2.set_name("Fake Cellular 2");
    cellular2.set_strength(70);
    cellular2.set_connected(true);
    cellular_networks_.push_back(cellular2);

    CellularNetwork cellular3 = CellularNetwork();
    cellular3.set_service_path("fc3");
    cellular3.set_name("Fake Cellular 3");
    cellular3.set_strength(50);
    cellular3.set_connected(false);
    cellular_networks_.push_back(cellular3);

    cellular_ = cellular2;

    remembered_wifi_networks_.clear();
    remembered_wifi_networks_.push_back(wifi2);

    remembered_cellular_networks_.clear();
    remembered_cellular_networks_.push_back(cellular2);

    int devices = (1 << TYPE_ETHERNET) | (1 << TYPE_WIFI) |
        (1 << TYPE_CELLULAR);
    available_devices_ = devices;
    enabled_devices_ = devices;
    connected_devices_ = devices;
    offline_mode_ = false;
  }

  void UpdateSystemInfo() {
    if (CrosLibrary::Get()->EnsureLoaded()) {
      UpdateNetworkStatus();
    }
  }

  WifiNetwork* GetWifiNetworkByName(const std::string& name) {
    for (size_t i = 0; i < wifi_networks_.size(); ++i) {
      if (wifi_networks_[i].name().compare(name) == 0) {
        return &wifi_networks_[i];
      }
    }
    return NULL;
  }

  template<typename T> T* GetWirelessNetworkByPath(
      std::vector<T>& networks, const std::string& path) {
    typedef typename std::vector<T>::iterator iter_t;
    iter_t iter = std::find_if(networks.begin(), networks.end(),
                               WirelessNetwork::ServicePathEq(path));
    return (iter != networks.end()) ? &(*iter) : NULL;
  }

  // const version
  template<typename T> const T* GetWirelessNetworkByPath(
      const std::vector<T>& networks, const std::string& path) const {
    typedef typename std::vector<T>::const_iterator iter_t;
    iter_t iter = std::find_if(networks.begin(), networks.end(),
                               WirelessNetwork::ServicePathEq(path));
    return (iter != networks.end()) ? &(*iter) : NULL;
  }

  void EnableNetworkDeviceType(ConnectionType device, bool enable) {
    if (!CrosLibrary::Get()->EnsureLoaded())
      return;

    // If network device is already enabled/disabled, then don't do anything.
    if (enable && (enabled_devices_ & (1 << device))) {
      LOG(WARNING) << "Trying to enable a device that's already enabled: "
                   << device;
      return;
    }
    if (!enable && !(enabled_devices_ & (1 << device))) {
      LOG(WARNING) << "Trying to disable a device that's already disabled: "
                   << device;
      return;
    }

    EnableNetworkDevice(device, enable);
  }

  void NotifyNetworkChanged() {
    FOR_EACH_OBSERVER(Observer, observers_, NetworkChanged(this));
  }

  void NotifyCellularDataPlanChanged() {
    FOR_EACH_OBSERVER(Observer, observers_,
        CellularDataPlanChanged(cellular_.service_path(), cellular_data_plan_));
  }

  void UpdateNetworkStatus() {
    // Make sure we run on UI thread.
    if (!ChromeThread::CurrentlyOn(ChromeThread::UI)) {
      ChromeThread::PostTask(
          ChromeThread::UI, FROM_HERE,
          NewRunnableMethod(this,
                            &NetworkLibraryImpl::UpdateNetworkStatus));
      return;
    }

    SystemInfo* system = GetSystemInfo();
    if (!system)
      return;

    wifi_networks_.clear();
    cellular_networks_.clear();
    remembered_wifi_networks_.clear();
    remembered_cellular_networks_.clear();
    ParseSystem(system, &ethernet_, &wifi_networks_, &cellular_networks_,
                &remembered_wifi_networks_, &remembered_cellular_networks_);

    wifi_ = WifiNetwork();
    for (size_t i = 0; i < wifi_networks_.size(); i++) {
      if (wifi_networks_[i].connecting_or_connected()) {
        wifi_ = wifi_networks_[i];
        break;  // There is only one connected or connecting wifi network.
      }
    }
    std::string prev_service_path = cellular_.service_path();
    cellular_ = CellularNetwork();
    for (size_t i = 0; i < cellular_networks_.size(); i++) {
      if (cellular_networks_[i].connecting_or_connected()) {
        // If new cellular, then update data plan list.
        if (cellular_networks_[i].service_path() != prev_service_path) {
          CellularDataPlanList list;
          RetrieveCellularDataPlans(cellular_.service_path().c_str(), &list);
          UpdateCellularDataPlan(list);
        }
        cellular_ = cellular_networks_[i];
        break;  // There is only one connected or connecting cellular network.
      }
    }

    available_devices_ = system->available_technologies;
    enabled_devices_ = system->enabled_technologies;
    connected_devices_ = system->connected_technologies;
    offline_mode_ = system->offline_mode;

    NotifyNetworkChanged();
    FreeSystemInfo(system);
  }

  void UpdateCellularDataPlan(const CellularDataPlanList& dataplan) {
    if (dataplan.empty()) {
      // Set to an empty data plan.
      cellular_data_plan_ = CellularDataPlan();
    } else {
      // The active data plan is the first one.
      cellular_data_plan_ = dataplan[0];
    }
    NotifyCellularDataPlanChanged();
  }

  ObserverList<Observer> observers_;

  // The network status connection for monitoring network status changes.
  MonitorNetworkConnection network_status_connection_;

  // For monitoring data plan changes to the connected cellular network.
  DataPlanUpdateMonitor data_plan_monitor_;

  // The ethernet network.
  EthernetNetwork ethernet_;

  // The list of available wifi networks.
  WifiNetworkVector wifi_networks_;

  // The current connected (or connecting) wifi network.
  WifiNetwork wifi_;

  // The remembered wifi networks.
  WifiNetworkVector remembered_wifi_networks_;

  // The list of available cellular networks.
  CellularNetworkVector cellular_networks_;

  // The current connected (or connecting) cellular network.
  CellularNetwork cellular_;

  // The data plan for the current cellular network.
  CellularDataPlan cellular_data_plan_;

  // The remembered cellular networks.
  CellularNetworkVector remembered_cellular_networks_;

  // The current available network devices. Bitwise flag of ConnectionTypes.
  int available_devices_;

  // The current enabled network devices. Bitwise flag of ConnectionTypes.
  int enabled_devices_;

  // The current connected network devices. Bitwise flag of ConnectionTypes.
  int connected_devices_;

  bool offline_mode_;

  DISALLOW_COPY_AND_ASSIGN(NetworkLibraryImpl);
};

class NetworkLibraryStubImpl : public NetworkLibrary {
 public:
  NetworkLibraryStubImpl() : ip_address_("1.1.1.1") {}
  ~NetworkLibraryStubImpl() {}
  void AddObserver(Observer* observer) {}
  void RemoveObserver(Observer* observer) {}
  virtual const EthernetNetwork& ethernet_network() const {
    return ethernet_;
  }
  virtual bool ethernet_connecting() const { return false; }
  virtual bool ethernet_connected() const { return true; }
  virtual const std::string& wifi_name() const { return EmptyString(); }
  virtual bool wifi_connecting() const { return false; }
  virtual bool wifi_connected() const { return false; }
  virtual int wifi_strength() const { return 0; }

  virtual const std::string& cellular_name() const { return EmptyString(); }
  virtual const std::string& cellular_service_path() const {
    return EmptyString(); }
  virtual bool cellular_connecting() const { return false; }
  virtual bool cellular_connected() const { return false; }
  virtual int cellular_strength() const { return false; }

  bool Connected() const { return true; }
  bool Connecting() const { return false; }
  const std::string& IPAddress() const { return ip_address_; }
  virtual const WifiNetworkVector& wifi_networks() const {
    return wifi_networks_;
  }
  virtual const WifiNetworkVector& remembered_wifi_networks() const {
    return wifi_networks_;
  }
  virtual const CellularNetworkVector& cellular_networks() const {
    return cellular_networks_;
  }
  virtual const CellularNetworkVector& remembered_cellular_networks() const {
    return cellular_networks_;
  }

  /////////////////////////////////////////////////////////////////////////////

  bool FindWifiNetworkByPath(
      const std::string& path, WifiNetwork* result) const { return false; }
  bool FindCellularNetworkByPath(
      const std::string& path, CellularNetwork* result) const { return false; }
  void RequestWifiScan() {}
  bool GetWifiAccessPoints(WifiAccessPointVector* result) { return false; }

  void ConnectToWifiNetwork(WifiNetwork network,
                            const std::string& password,
                            const std::string& identity,
                            const std::string& certpath) {}
  void ConnectToWifiNetwork(const std::string& ssid,
                            const std::string& password,
                            const std::string& identity,
                            const std::string& certpath,
                            bool auto_connect) {}
  void ConnectToCellularNetwork(CellularNetwork network) {}
  void DisconnectFromWirelessNetwork(const WirelessNetwork& network) {}
  void SaveCellularNetwork(const CellularNetwork& network) {}
  void SaveWifiNetwork(const WifiNetwork& network) {}
  void ForgetWirelessNetwork(const std::string& service_path) {}
  virtual bool ethernet_available() const { return true; }
  virtual bool wifi_available() const { return false; }
  virtual bool cellular_available() const { return false; }
  virtual bool ethernet_enabled() const { return true; }
  virtual bool wifi_enabled() const { return false; }
  virtual bool cellular_enabled() const { return false; }
  virtual bool offline_mode() const { return false; }
  void EnableEthernetNetworkDevice(bool enable) {}
  void EnableWifiNetworkDevice(bool enable) {}
  void EnableCellularNetworkDevice(bool enable) {}
  void EnableOfflineMode(bool enable) {}
  NetworkIPConfigVector GetIPConfigs(const std::string& device_path) {
    return NetworkIPConfigVector();
  }
  std::string GetHtmlInfo(int refresh) { return std::string(); }
  void UpdateSystemInfo() {}

 private:
  std::string ip_address_;
  EthernetNetwork ethernet_;
  WifiNetworkVector wifi_networks_;
  CellularNetworkVector cellular_networks_;
};

// static
NetworkLibrary* NetworkLibrary::GetImpl(bool stub) {
  if (stub)
    return new NetworkLibraryStubImpl();
  else
    return new NetworkLibraryImpl();
}

}  // namespace chromeos

// Allows InvokeLater without adding refcounting. This class is a Singleton and
// won't be deleted until it's last InvokeLater is run.
DISABLE_RUNNABLE_METHOD_REFCOUNT(chromeos::NetworkLibraryImpl);
