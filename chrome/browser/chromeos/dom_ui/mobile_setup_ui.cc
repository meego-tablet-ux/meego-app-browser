// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dom_ui/mobile_setup_ui.h"

#include <map>
#include <string>

#include "app/l10n_util.h"
#include "app/resource_bundle.h"
#include "base/callback.h"
#include "base/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/string_piece.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "base/weak_ptr.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_thread.h"
#include "chrome/browser/chromeos/cros/cros_library.h"
#include "chrome/browser/chromeos/cros/network_library.h"
#include "chrome/browser/chromeos/cros/system_library.h"
#include "chrome/browser/dom_ui/chrome_url_data_manager.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/jstemplate_builder.h"
#include "chrome/common/url_constants.h"
#include "googleurl/src/gurl.h"
#include "grit/browser_resources.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"

namespace {

// Host page JS API function names.
const char kJsApiStartActivation[] = "startActivation";
const char kJsApiCloseTab[] = "closeTab";
const char kJsApiSetTransactionStatus[] = "setTransactionStatus";

const wchar_t kJsDeviceStatusChangedHandler[] =
    L"mobile.MobileSetup.deviceStateChanged";

// Collular device states that are reported to DOM UI layer.
const char kStateUnknown[] = "unknown";
const char kStateConnecting[] = "connecting";
const char kStateError[] = "error";
const char kStateNeedsPayment[] = "payment";
const char kStateActivating[] = "activating";
const char kStateDisconnected[] = "disconnected";
const char kStateConnected[] = "connected";
const char kFailedPayment[] = "failed_payment";

// Error codes matching codes defined in the cellular config file.
const char kErrorDefault[] = "default";
const char kErrorBadConnectionPartial[] = "bad_connection_partial";
const char kErrorBadConnectionActivated[] = "bad_connection_activated";
const char kErrorRoamingOnConnection[] = "roaming_connection";
const char kErrorNoEVDO[] = "no_evdo";
const char kErrorRoamingActivation[] = "roaming_activation";
const char kErrorRoamingPartiallyActivated[] = "roaming_partially_activated";
const char kErrorNoService[] = "no_service";
const char kFailedPaymentError[] = "failed_payment";

// Cellular configuration file path.
const char kCellularConfigPath[] =
    "/usr/share/chromeos-assets/mobile/mobile_config.json";

// Cellular config file field names.
const char kVersionField[] = "version";
const char kErrorsField[] = "errors";

}  // namespace

class CellularConfigDocument {
 public:
  CellularConfigDocument() {}

  // Return error message for a given code.
  std::string GetErrorMessage(const std::string& code);
  const std::string& version() { return version_; }

  bool LoadFromFile(const FilePath& config_path);

 private:
  std::string version_;
  std::map<std::string, std::string> error_map_;

  DISALLOW_COPY_AND_ASSIGN(CellularConfigDocument);
};

static std::map<std::string, std::string> error_messages_;

class MobileSetupUIHTMLSource : public ChromeURLDataManager::DataSource {
 public:
  MobileSetupUIHTMLSource();

  // Called when the network layer has requested a resource underneath
  // the path we registered.
  virtual void StartDataRequest(const std::string& path,
                                bool is_off_the_record,
                                int request_id);
  virtual std::string GetMimeType(const std::string&) const {
    return "text/html";
  }

 private:
  ~MobileSetupUIHTMLSource() {}

  DISALLOW_COPY_AND_ASSIGN(MobileSetupUIHTMLSource);
};

// The handler for Javascript messages related to the "register" view.
class MobileSetupHandler : public DOMMessageHandler,
                           public chromeos::NetworkLibrary::Observer,
                           public chromeos::NetworkLibrary::PropertyObserver,
                           public base::SupportsWeakPtr<MobileSetupHandler> {
 public:
  MobileSetupHandler();
  virtual ~MobileSetupHandler();

  // Init work after Attach.
  void Init(TabContents* contents);

  // DOMMessageHandler implementation.
  virtual DOMMessageHandler* Attach(DOMUI* dom_ui);
  virtual void RegisterMessages();

  // NetworkLibrary::Observer implementation.
  virtual void NetworkChanged(chromeos::NetworkLibrary* obj);
  // NetworkLibrary::PropertyObserver implementation.
  virtual void PropertyChanged(const char* service_path,
                               const char* key,
                               const Value* value);

  // Returns currently present cellular network, NULL if no network found.
  static const chromeos::CellularNetwork* GetNetwork();

 private:
  typedef enum PlanActivationState {
    PLAN_ACTIVATION_PAGE_LOADING = -1,
    PLAN_ACTIVATION_START = 0,
    PLAN_ACTIVATION_INITIATING_ACTIVATION = 1,
    PLAN_ACTIVATION_ACTIVATING = 2,
    PLAN_ACTIVATION_SHOWING_PAYMENT = 3,
    PLAN_ACTIVATION_DONE = 4,
    PLAN_ACTIVATION_ERROR = 5,
  } PlanActivationState;

  // Handlers for JS DOMUI messages.
  void HandleStartActivation(const ListValue* args);
  void HandleCloseTab(const ListValue* args);
  void HandleSetTransactionStatus(const ListValue* args);

  // Sends message to host registration page with system/user info data.
  void SendDeviceInfo();

  // Verify the state of cellular network and modify internal state.
  void EvaluateCellularNetwork();
  // Check the current cellular network for error conditions.
  bool GotActivationError(const chromeos::CellularNetwork* network,
                          std::string* error);
  void ChangeState(const chromeos::CellularNetwork* network,
                   PlanActivationState new_state,
                   const std::string& error_description);

  // Converts the currently active CellularNetwork device into a JS object.
  static void GetDeviceInfo(const chromeos::CellularNetwork* network,
                            DictionaryValue* value);
  static bool ShouldReportDeviceState(std::string* state, std::string* error);

  // Performs activation state cellular device evaluation.
  // Returns false if device activation failed. In this case |error|
  // will contain error message to be reported to DOM UI.
  static bool EvaluateCellularDeviceState(bool* report_status,
                                          std::string* state,
                                          std::string* error);

  // Return error message for a given code.
  static std::string GetErrorMessage(const std::string& code);
  static void LoadCellularConfig();

  static const char* GetStateDescription(PlanActivationState state);

  static scoped_ptr<CellularConfigDocument> cellular_config_;

  TabContents* tab_contents_;
  // Internal handler state.
  PlanActivationState state_;
  DISALLOW_COPY_AND_ASSIGN(MobileSetupHandler);
};

scoped_ptr<CellularConfigDocument> MobileSetupHandler::cellular_config_;

////////////////////////////////////////////////////////////////////////////////
//
// CellularConfigDocument
//
////////////////////////////////////////////////////////////////////////////////

std::string CellularConfigDocument::GetErrorMessage(const std::string& code) {
  std::map<std::string, std::string>::iterator iter = error_map_.find(code);
  if (iter == error_map_.end())
    return code;
  return iter->second;
}

bool CellularConfigDocument::LoadFromFile(const FilePath& config_path) {
  error_map_.clear();

  std::string config;
  if (!file_util::ReadFileToString(config_path, &config))
    return false;

  scoped_ptr<Value> root(base::JSONReader::Read(config, true));
  DCHECK(root.get() != NULL);
  if (!root.get() || root->GetType() != Value::TYPE_DICTIONARY) {
    VLOG(1) << "Bad cellular config file";
    return false;
  }

  DictionaryValue* root_dict = static_cast<DictionaryValue*>(root.get());
  if (!root_dict->GetString(kVersionField, &version_)) {
    VLOG(1) << "Cellular config file missing version";
    return false;
  }
  DictionaryValue* errors = NULL;
  if (!root_dict->GetDictionary(kErrorsField, &errors))
    return false;
  for (DictionaryValue::key_iterator keys = errors->begin_keys();
       keys != errors->end_keys();
       ++keys) {
    std::string value;
    if (!errors->GetString(*keys, &value)) {
      VLOG(1) << "Bad cellular config error value";
      error_map_.clear();
      return false;
    }

    error_map_.insert(std::pair<std::string, std::string>(*keys, value));
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// MobileSetupUIHTMLSource
//
////////////////////////////////////////////////////////////////////////////////

MobileSetupUIHTMLSource::MobileSetupUIHTMLSource()
    : DataSource(chrome::kChromeUIMobileSetupHost, MessageLoop::current()) {
}

void MobileSetupUIHTMLSource::StartDataRequest(const std::string& path,
                                                bool is_off_the_record,
                                                int request_id) {
  const chromeos::CellularNetwork* network = MobileSetupHandler::GetNetwork();

  DictionaryValue strings;
  strings.SetString("title", l10n_util::GetStringUTF16(IDS_MOBILE_SETUP_TITLE));
  strings.SetString("connecting_header",
                    l10n_util::GetStringFUTF16(IDS_MOBILE_CONNECTING_HEADER,
                    network ? UTF8ToUTF16(network->name()) : string16()));
  strings.SetString("error_header",
                    l10n_util::GetStringUTF16(IDS_MOBILE_ERROR_HEADER));
  strings.SetString("activating_header",
                    l10n_util::GetStringUTF16(IDS_MOBILE_ACTIVATING_HEADER));
  strings.SetString("completed_header",
                    l10n_util::GetStringUTF16(IDS_MOBILE_COMPLETED_HEADER));
  strings.SetString("completed_text",
                    l10n_util::GetStringUTF16(IDS_MOBILE_COMPLETED_TEXT));
  SetFontAndTextDirection(&strings);

  static const base::StringPiece html(
      ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_MOBILE_SETUP_PAGE_HTML));
  const std::string full_html = jstemplate_builder::GetTemplatesHtml(
      html, &strings, "t" /* template root node id */);

  scoped_refptr<RefCountedBytes> html_bytes(new RefCountedBytes);
  html_bytes->data.resize(full_html.size());
  std::copy(full_html.begin(), full_html.end(), html_bytes->data.begin());

  SendResponse(request_id, html_bytes);
}

////////////////////////////////////////////////////////////////////////////////
//
// MobileSetupHandler
//
////////////////////////////////////////////////////////////////////////////////
MobileSetupHandler::MobileSetupHandler()
    : tab_contents_(NULL), state_(PLAN_ACTIVATION_PAGE_LOADING) {
}

MobileSetupHandler::~MobileSetupHandler() {
  chromeos::NetworkLibrary* lib =
      chromeos::CrosLibrary::Get()->GetNetworkLibrary();
  lib->RemoveObserver(this);
  lib->RemoveProperyObserver(this);
}

DOMMessageHandler* MobileSetupHandler::Attach(DOMUI* dom_ui) {
  return DOMMessageHandler::Attach(dom_ui);
}

void MobileSetupHandler::Init(TabContents* contents) {
  tab_contents_ = contents;
  LoadCellularConfig();
}

void MobileSetupHandler::RegisterMessages() {
  dom_ui_->RegisterMessageCallback(kJsApiStartActivation,
      NewCallback(this, &MobileSetupHandler::HandleStartActivation));
  dom_ui_->RegisterMessageCallback(kJsApiCloseTab,
      NewCallback(this, &MobileSetupHandler::HandleCloseTab));
  dom_ui_->RegisterMessageCallback(kJsApiSetTransactionStatus,
      NewCallback(this, &MobileSetupHandler::HandleSetTransactionStatus));
}

void MobileSetupHandler::NetworkChanged(chromeos::NetworkLibrary* cros) {
  if (state_ == PLAN_ACTIVATION_PAGE_LOADING)
    return;
  EvaluateCellularNetwork();
}

void MobileSetupHandler::PropertyChanged(const char* service_path,
                                         const char* key,
                                         const Value* value) {
  if (state_ == PLAN_ACTIVATION_PAGE_LOADING)
    return;
  const chromeos::CellularNetwork* network = GetNetwork();
  if (network->service_path() != service_path) {
    NOTREACHED();
    return;
  }
  std::string value_string;
  VLOG(1) << "Cellular property change: " << key << " = " << value_string;
  // Force status updates.
  chromeos::CrosLibrary::Get()->GetNetworkLibrary()->UpdateSystemInfo();
  EvaluateCellularNetwork();
}

void MobileSetupHandler::HandleCloseTab(const ListValue* args) {
  if (!dom_ui_)
    return;
  Browser* browser = BrowserList::FindBrowserWithFeature(
      dom_ui_->GetProfile(), Browser::FEATURE_TABSTRIP);
  if (browser)
    browser->CloseTabContents(tab_contents_);
}

void MobileSetupHandler::HandleStartActivation(const ListValue* args) {
  const chromeos::CellularNetwork* network = GetNetwork();
  if (!network) {
    ChangeState(NULL, PLAN_ACTIVATION_ERROR, std::string());
    return;
  }

  // Start monitoring network and service property changes.
  chromeos::NetworkLibrary* lib =
      chromeos::CrosLibrary::Get()->GetNetworkLibrary();
  lib->AddObserver(this);
  lib->AddProperyObserver(network->service_path().c_str(),
                          this);
  state_ = PLAN_ACTIVATION_START;
  EvaluateCellularNetwork();
}

void MobileSetupHandler::HandleSetTransactionStatus(const ListValue* args) {
  const size_t kSetTransactionStatusParamCount = 1;
  if (args->GetSize() != kSetTransactionStatusParamCount)
    return;

  // Get change callback function name.
  std::string status;
  if (!args->GetString(0, &status))
    return;

  // The payment is received, try to reconnect and check the status all over
  // again.
  if (LowerCaseEqualsASCII(status, "OK")) {
    ChangeState(GetNetwork(), PLAN_ACTIVATION_START, std::string());
  }
}


void MobileSetupHandler::EvaluateCellularNetwork() {
  if (!dom_ui_)
    return;

  PlanActivationState new_state = state_;
  const chromeos::CellularNetwork* network = GetNetwork();
  if (network) {
    VLOG(1) << "Cellular:\n  service=" << network->GetStateString()
            << "\n  ui=" << GetStateDescription(state_)
            << "\n  activation=" << network->GetActivationStateString()
            << "\n  restricted=" << (network->restricted_pool() ? "yes" : "no")
            << "\n  error=" << network->GetErrorString()
            << "\n  setvice_path=" << network->service_path();
  } else {
    LOG(WARNING) << "Cellular service lost";
  }
  switch (state_) {
    case PLAN_ACTIVATION_START:
      if (network) {
        switch (network->activation_state()) {
          case chromeos::ACTIVATION_STATE_PARTIALLY_ACTIVATED:
          case chromeos::ACTIVATION_STATE_ACTIVATED:
            if (network->failed_or_disconnected()) {
              new_state = PLAN_ACTIVATION_ACTIVATING;
            } else if (network->connection_state() == chromeos::STATE_READY) {
              new_state = network->restricted_pool() ?
                  PLAN_ACTIVATION_SHOWING_PAYMENT : PLAN_ACTIVATION_DONE;
            }
            break;
          case chromeos::ACTIVATION_STATE_UNKNOWN:
          case chromeos::ACTIVATION_STATE_NOT_ACTIVATED:
            if (network->failed_or_disconnected()) {
              new_state = PLAN_ACTIVATION_INITIATING_ACTIVATION;
            } if (network->connected()) {
              VLOG(1) << "Disconnecting from " << network->service_path();
              chromeos::CrosLibrary::Get()->GetNetworkLibrary()->
                  DisconnectFromWirelessNetwork(*network);
            }
            break;
          default:
            new_state = PLAN_ACTIVATION_INITIATING_ACTIVATION;
            break;
        }
      }
      break;
    case PLAN_ACTIVATION_INITIATING_ACTIVATION:
      if (network) {
        switch (network->activation_state()) {
          case chromeos::ACTIVATION_STATE_ACTIVATED:
            if (network->failed_or_disconnected()) {
              new_state = PLAN_ACTIVATION_ACTIVATING;
            } else if (network->connection_state() == chromeos::STATE_READY) {
              if (network->restricted_pool()) {
                new_state = PLAN_ACTIVATION_SHOWING_PAYMENT;
              } else {
                new_state = PLAN_ACTIVATION_DONE;
              }
            }
            break;
          case chromeos::ACTIVATION_STATE_PARTIALLY_ACTIVATED:
            if (network->connected())
              new_state = PLAN_ACTIVATION_SHOWING_PAYMENT;
            else
              new_state = PLAN_ACTIVATION_ACTIVATING;
            break;
          case chromeos::ACTIVATION_STATE_NOT_ACTIVATED:
            // Wait in this state until activation state changes.
            break;
          default:
            NOTREACHED();
            break;
        }
      }
      break;
    case PLAN_ACTIVATION_ACTIVATING:
      // Wait until the service shows up and gets activated.
      if (network) {
        switch (network->activation_state()) {
          case chromeos::ACTIVATION_STATE_ACTIVATED:
            if (network->connection_state() == chromeos::STATE_READY) {
              if (network->restricted_pool()) {
                new_state = PLAN_ACTIVATION_SHOWING_PAYMENT;
              } else {
                new_state = PLAN_ACTIVATION_DONE;
              }
            }
            break;
          case chromeos::ACTIVATION_STATE_PARTIALLY_ACTIVATED:
            if (network->connected()) {
              if (network->restricted_pool())
                new_state = PLAN_ACTIVATION_SHOWING_PAYMENT;
            }
            break;
          default:
            NOTREACHED();
            break;
        }
      }
      break;
    case PLAN_ACTIVATION_PAGE_LOADING:
      break;
    // Just ignore all signals until the site confirms payment.
    case PLAN_ACTIVATION_SHOWING_PAYMENT:
      // Activation completed/failed, ignore network changes.
    case PLAN_ACTIVATION_DONE:
    case PLAN_ACTIVATION_ERROR:
      break;
  }

  std::string error_description;
  if (GotActivationError(network, &error_description)) {
    new_state = PLAN_ACTIVATION_ERROR;
  }
  ChangeState(network, new_state, error_description);
}

// Debugging helper function, will take it out at the end.
const char* MobileSetupHandler::GetStateDescription(
    PlanActivationState state) {
  switch (state) {
    case PLAN_ACTIVATION_PAGE_LOADING:
      return "PAGE_LOADING";
    case PLAN_ACTIVATION_START:
      return "ACTIVATION_START";
    case PLAN_ACTIVATION_INITIATING_ACTIVATION:
      return "INITIATING_ACTIVATION";
    case PLAN_ACTIVATION_ACTIVATING :
      return "ACTIVATING";
    case PLAN_ACTIVATION_SHOWING_PAYMENT:
      return "SHOWING_PAYMENT";
    case PLAN_ACTIVATION_DONE:
      return "DONE";
    case PLAN_ACTIVATION_ERROR:
      return "ERROR";
  }
  return "UNKNOWN";
}

void MobileSetupHandler::ChangeState(const chromeos::CellularNetwork* network,
                                     PlanActivationState new_state,
                                     const std::string& error_description) {
  static bool first_time = true;
  if (state_ == new_state && !first_time)
    return;
  VLOG(1) << "Activation state flip old = " << GetStateDescription(state_)
          << ", new = " << GetStateDescription(new_state);
  first_time = false;
  state_ = new_state;
  switch (new_state) {
    case PLAN_ACTIVATION_START:
      break;
    case PLAN_ACTIVATION_INITIATING_ACTIVATION:
      DCHECK(network);
      VLOG(1) << "Activating service " << network->service_path();
      if (!network->StartActivation())
        new_state = PLAN_ACTIVATION_ERROR;
      break;
    case PLAN_ACTIVATION_ACTIVATING:
      DCHECK(network);
      if (network) {
        chromeos::CrosLibrary::Get()->GetNetworkLibrary()->
            ConnectToCellularNetwork(*network);
      }
      break;
    case PLAN_ACTIVATION_PAGE_LOADING:
      return;
    case PLAN_ACTIVATION_SHOWING_PAYMENT:
    case PLAN_ACTIVATION_DONE:
    case PLAN_ACTIVATION_ERROR:
      break;
  }

  DictionaryValue device_dict;
  if (network)
    GetDeviceInfo(network, &device_dict);
  device_dict.SetInteger("state", new_state);
  if (error_description.length())
    device_dict.SetString("error", error_description);
  dom_ui_->CallJavascriptFunction(
      kJsDeviceStatusChangedHandler, device_dict);
}


bool MobileSetupHandler::GotActivationError(
    const chromeos::CellularNetwork* network, std::string* error) {
  if (network)
    return false;
  bool got_error = false;
  const char* error_code = kErrorDefault;

  // This is the magic for detection of errors in during activation process.
  if (network->connection_state() == chromeos::STATE_FAILURE &&
      network->error() == chromeos::ERROR_AAA_FAILED ) {
    if (network->activation_state() ==
            chromeos::ACTIVATION_STATE_PARTIALLY_ACTIVATED) {
      error_code = kErrorBadConnectionPartial;
    } else if (network->activation_state() ==
            chromeos::ACTIVATION_STATE_ACTIVATED) {
      if (network->roaming_state() == chromeos::ROAMING_STATE_HOME) {
        error_code = kErrorBadConnectionActivated;
      } else if (network->roaming_state() == chromeos::ROAMING_STATE_ROAMING) {
        error_code = kErrorRoamingOnConnection;
      }
    }
    got_error = true;
  } else if (network->connection_state() ==
                 chromeos::STATE_ACTIVATION_FAILURE) {
    if (network->error() == chromeos::ERROR_NEED_EVDO) {
      if (network->activation_state() ==
              chromeos::ACTIVATION_STATE_PARTIALLY_ACTIVATED)
        error_code = kErrorNoEVDO;
    } else if (network->error() == chromeos::ERROR_NEED_HOME_NETWORK) {
      if (network->activation_state() ==
              chromeos::ACTIVATION_STATE_NOT_ACTIVATED) {
        error_code = kErrorRoamingActivation;
      } else if (network->activation_state() ==
                    chromeos::ACTIVATION_STATE_PARTIALLY_ACTIVATED) {
        error_code = kErrorRoamingPartiallyActivated;
      }
    }
    got_error = true;
  }

  if (got_error)
    *error = GetErrorMessage(error_code);

  return got_error;
}

const chromeos::CellularNetwork* MobileSetupHandler::GetNetwork() {
  chromeos::NetworkLibrary* network_lib =
      chromeos::CrosLibrary::Get()->GetNetworkLibrary();

  const chromeos::CellularNetworkVector& cell_networks =
      network_lib->cellular_networks();
  if (!cell_networks.size())
    return NULL;
  return &(*(cell_networks.begin()));
}

void MobileSetupHandler::GetDeviceInfo(const chromeos::CellularNetwork* network,
          DictionaryValue* value) {
  if (!network)
    return;
  value->SetString("carrier", network->name());
  value->SetString("payment_url", network->payment_url());
  value->SetString("MEID", network->meid());
  value->SetString("IMEI", network->imei());
  value->SetString("MDN", network->mdn());
}

std::string MobileSetupHandler::GetErrorMessage(const std::string& code) {
  if (!cellular_config_.get())
    return "";
  return cellular_config_->GetErrorMessage(code);
}

void MobileSetupHandler::LoadCellularConfig() {
  static bool config_loaded = false;
  if (config_loaded)
    return;
  config_loaded = true;
  // Load partner customization startup manifest if it is available.
  FilePath config_path(kCellularConfigPath);
  if (file_util::PathExists(config_path)) {
    scoped_ptr<CellularConfigDocument> config(new CellularConfigDocument());
    bool config_loaded = config->LoadFromFile(config_path);
    if (config_loaded) {
      VLOG(1) << "Cellular config file loaded: " << kCellularConfigPath;
      // lock
      cellular_config_.reset(config.release());
    } else {
      LOG(ERROR) << "Error loading cellular config file: " <<
          kCellularConfigPath;
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
//
// MobileSetupUI
//
////////////////////////////////////////////////////////////////////////////////

MobileSetupUI::MobileSetupUI(TabContents* contents) : DOMUI(contents){
  MobileSetupHandler* handler = new MobileSetupHandler();
  AddMessageHandler((handler)->Attach(this));
  handler->Init(contents);
  MobileSetupUIHTMLSource* html_source = new MobileSetupUIHTMLSource();

  // Set up the chrome://mobilesetup/ source.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      NewRunnableMethod(
          Singleton<ChromeURLDataManager>::get(),
          &ChromeURLDataManager::AddDataSource,
          make_scoped_refptr(html_source)));
}
