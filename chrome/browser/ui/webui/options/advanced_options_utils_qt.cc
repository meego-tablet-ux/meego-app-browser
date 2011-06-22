#include "chrome/browser/ui/webui/options/advanced_options_utils.h"

#include "base/file_util.h"
#include "base/environment.h"
#include "base/process_util.h"
#include "base/string_tokenizer.h"
#include "base/nix/xdg_util.h"
#include "chrome/browser/ui/browser_list.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/common/process_watcher.h"

struct ProxyConfigCommand {
  std::string binary;
  const char** argv;
};

static bool SearchPATH(ProxyConfigCommand* commands, size_t ncommands,
                       size_t* index) {
  NOTIMPLEMENTED();
  return false;
}

static void StartProxyConfigUtil(const ProxyConfigCommand& command) {
  NOTIMPLEMENTED();
}

void AdvancedOptionsUtilities::ShowNetworkProxySettings(
      TabContents* tab_contents) {
  NOTIMPLEMENTED();
}

void AdvancedOptionsUtilities::ShowManageSSLCertificates(
      TabContents* tab_contents) {
  NOTIMPLEMENTED();
}
