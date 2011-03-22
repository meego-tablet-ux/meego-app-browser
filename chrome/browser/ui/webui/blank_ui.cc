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

#include "ui/base/l10n/l10n_util.h"
#include "base/callback.h"
#include "base/i18n/rtl.h"
#include "base/singleton.h"
#include "base/values.h"
#include "content/browser/webui/web_ui_util.h"
#include "chrome/browser/ui/webui/blank_ui.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "chrome/common/bindings_policy.h"
#include "chrome/common/url_constants.h"
#include "chrome/common/jstemplate_builder.h"
#include "content/browser/browser_thread.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/chrome_url_data_manager.h"
#include "grit/generated_resources.h"

class BlankUIHTMLSource : public ChromeURLDataManager::DataSource {
  public: 
    BlankUIHTMLSource()
	:DataSource(chrome::kChromeUINewTabHost, MessageLoop::current()) {}

    virtual void StartDataRequest(const std::string& path,
						bool is_off_the_record,
						int request_id) {
 	DictionaryValue localized_strings;
  	localized_strings.SetString("title",
      		l10n_util::GetStringUTF16(IDS_NEW_TAB_TITLE));

  	ChromeURLDataManager::DataSource::SetFontAndTextDirection(&localized_strings);

  	static const base::StringPiece blank_tab_html("<html><head><title i18n-content=\"title\"></title></head><body></body></html>");
	

  	std::string full_html = jstemplate_builder::GetI18nTemplateHtml(
      		blank_tab_html, &localized_strings);

	RefCountedBytes* html = new RefCountedBytes;
	html->data.resize(full_html.size());
	std::copy(full_html.begin(), full_html.end(), html->data.begin());

	SendResponse(request_id, html); 
    }

    virtual std::string GetMimeType(const std::string&) const {
       DLOG(INFO)<<__FUNCTION__;
       return "text/html";
    }

  private:
    ~BlankUIHTMLSource() {}

    DISALLOW_COPY_AND_ASSIGN(BlankUIHTMLSource);
};

BlankUI::BlankUI(TabContents* tab_contents) : WebUI(tab_contents) {
  BlankUIHTMLSource* html_source = new BlankUIHTMLSource();
  tab_contents->profile()->GetChromeURLDataManager()->AddDataSource(html_source);
}
