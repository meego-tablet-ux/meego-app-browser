// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "config.h"

#pragma warning(push, 0)
#include "csshelper.h"
#include "CString.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "FormData.h"
#include "FormDataList.h"
#include "FrameLoader.h"
#include "HTMLFormElement.h"
#include "HTMLOptionElement.h"
#include "HTMLGenericFormElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLOptionsCollection.h"
#include "HTMLSelectElement.h"
#include "ResourceRequest.h"
#include "String.h"
#include "TextEncoding.h"
#pragma warning(pop)

#undef LOG

#include "base/basictypes.h"
#include "webkit/glue/dom_operations.h"
#include "webkit/glue/glue_util.h"
#include "webkit/glue/searchable_form_data.h"
#include "webkit/glue/webframe_impl.h"

using WebCore::HTMLInputElement;
using WebCore::HTMLOptionElement;

namespace {

// TODO (sky): This comes straight out of HTMLFormElement, will work with 
// WebKit folks to make public.
WebCore::DeprecatedCString encodeCString(const WebCore::CString& cstr) {
    WebCore::DeprecatedCString e = cstr.deprecatedCString();

    // http://www.w3.org/TR/html4/interact/forms.html#h-17.13.4.1
    // same safe characters as Netscape for compatibility
    static const char *safe = "-._*";
    int elen = e.length();
    WebCore::DeprecatedCString encoded((elen + e.contains('\n')) * 3 + 1);
    int enclen = 0;

    for (int pos = 0; pos < elen; pos++) {
        unsigned char c = e[pos];

        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || strchr(safe, c))
            encoded[enclen++] = c;
        else if (c == ' ')
            encoded[enclen++] = '+';
        else if (c == '\n' || (c == '\r' && e[pos + 1] != '\n')) {
            encoded[enclen++] = '%';
            encoded[enclen++] = '0';
            encoded[enclen++] = 'D';
            encoded[enclen++] = '%';
            encoded[enclen++] = '0';
            encoded[enclen++] = 'A';
        } else if (c != '\r') {
            encoded[enclen++] = '%';
            unsigned int h = c / 16;
            h += (h > 9) ? ('A' - 10) : '0';
            encoded[enclen++] = h;

            unsigned int l = c % 16;
            l += (l > 9) ? ('A' - 10) : '0';
            encoded[enclen++] = l;
        }
    }
    encoded[enclen++] = '\0';
    encoded.truncate(enclen);

    return encoded;
}

// Returns true if the form element has an 'onsubmit' attribute.
bool FormHasOnSubmit(WebCore::HTMLFormElement* form) {
  const WebCore::AtomicString& attribute_value = 
    form->getAttribute(WebCore::HTMLNames::onsubmitAttr);
  return (!attribute_value.isNull() && !attribute_value.isEmpty());
}

// Returns true if the form element will submit the data using a GET.
bool IsFormMethodGet(WebCore::HTMLFormElement* form) {
  const WebCore::AtomicString& attribute_value = 
    form->getAttribute(WebCore::HTMLNames::methodAttr);
  return !equalIgnoringCase(attribute_value, "post");
}

// Gets the encoding for the form.
void GetFormEncoding(WebCore::HTMLFormElement* form, 
                     WebCore::TextEncoding* encoding) {
  WebCore::String str = 
    form->getAttribute(WebCore::HTMLNames::accept_charsetAttr);
  str.replace(',', ' ');
  Vector<WebCore::String> charsets = str.split(' ');
  Vector<WebCore::String>::const_iterator end = charsets.end();
  for (Vector<WebCore::String>::const_iterator it = charsets.begin(); it != end; 
       ++it) {
    *encoding = WebCore::TextEncoding(*it);
    if (encoding->isValid())
      return;
  }
  if (!encoding->isValid()) {
    WebCore::Frame* frame = form->document()->frame();
    if (frame)
      *encoding = WebCore::TextEncoding(frame->loader()->encoding());
    else
      *encoding = WebCore::Latin1Encoding();
  }
}

// Returns true if the submit request results in an HTTP URL.
bool IsHTTPFormSubmit(WebCore::HTMLFormElement* form) {
  WebCore::Frame* frame = form->document()->frame();
  WebCore::String action = WebCore::parseURL(form->action());
  WebCore::FrameLoader* loader = frame->loader();
  WebCore::KURL url = loader->completeURL(action.isNull() ? "" : action);
  return (url.protocol() == "http");
}

// If the form does not have an activated submit button, the first submit
// button is returned.
WebCore::HTMLGenericFormElement* GetButtonToActivate(
    WebCore::HTMLFormElement* form) {
  WTF::Vector<WebCore::HTMLGenericFormElement*> form_elements = 
      form->formElements;
  WebCore::HTMLGenericFormElement* first_submit_button = NULL;

  for (unsigned i = 0; i < form_elements.size(); ++i) {
    WebCore::HTMLGenericFormElement* current = form_elements[i];
    if (current->isActivatedSubmit()) {
      // There's a button that is already activated for submit, return NULL.
      return NULL;
    } else if (first_submit_button == NULL && 
               current->isSuccessfulSubmitButton()) {
      first_submit_button = current;
    }
  }
  return first_submit_button;
}

// Returns true if the selected state of all the options matches the default
// selected state.
bool IsSelectInDefaultState(WebCore::HTMLSelectElement* select) {
  RefPtr<WebCore::HTMLOptionsCollection> options = select->options();
  WebCore::Node* node = options->firstItem();

  if (!select->multiple() && select->size() <= 1) {
    // The select is rendered as a combobox (called menulist in WebKit). At
    // least one item is selected, determine which one.
    HTMLOptionElement* initial_selected = NULL;
    while (node) {
      HTMLOptionElement* option_element =
          webkit_glue::CastHTMLElement<HTMLOptionElement>(
              node, WebCore::HTMLNames::optionTag);
      if (option_element) {
        if (!initial_selected)
          initial_selected = option_element;
        if (option_element->defaultSelected()) {
          // The page specified the option to select.
          initial_selected = option_element;
          break;
        }
      }
      node = options->nextItem();
    }
    if (initial_selected)
      return initial_selected->selected();
  } else {
    while (node) {
      HTMLOptionElement* option_element =
          webkit_glue::CastHTMLElement<HTMLOptionElement>(
              node, WebCore::HTMLNames::optionTag);
      if (option_element &&
          option_element->selected() != option_element->defaultSelected()) {
        return false;
      }
      node = options->nextItem();
    }
  }
  return true;
}

bool IsCheckBoxOrRadioInDefaultState(HTMLInputElement* element) {
  return (element->checked() == element->defaultChecked());
}

// Returns true if the form element is in its default state, false otherwise.
// The default state is the state of the form element on initial load of the
// page, and varies depending upon the form element. For example, a checkbox is
// in its default state if the checked state matches the defaultChecked state.
bool IsInDefaultState(WebCore::HTMLGenericFormElement* form_element) {
  if (form_element->hasTagName(WebCore::HTMLNames::inputTag)) {
    HTMLInputElement* input_element =
        static_cast<HTMLInputElement*>(form_element);
    if (input_element->inputType() == HTMLInputElement::CHECKBOX ||
        input_element->inputType() == HTMLInputElement::RADIO) {
      return IsCheckBoxOrRadioInDefaultState(input_element);
    }
  } else if (form_element->hasTagName(WebCore::HTMLNames::selectTag)) {
    return IsSelectInDefaultState(
        static_cast<WebCore::HTMLSelectElement*>(form_element));
  }
  return true;
}

// If form has only one text input element, it is returned. If a valid input
// element is not found, NULL is returned. Additionally, the form data for all 
// elements is added to enc_string and the encoding used is set in
// encoding_name.
WebCore::HTMLInputElement* GetTextElement(
    WebCore::HTMLFormElement* form,
    WebCore::DeprecatedCString* enc_string,
    std::string* encoding_name) {
  WebCore::TextEncoding encoding;
  GetFormEncoding(form, &encoding);
  if (!encoding.isValid()) {
    // Need a valid encoding to encode the form elements.
    // If the encoding isn't found webkit ends up replacing the params with
    // empty strings. So, we don't try to do anything here.
    return NULL;
  }
  *encoding_name = encoding.name();
  WebCore::HTMLInputElement* text_element = NULL;
  WTF::Vector<WebCore::HTMLGenericFormElement*> form_elements = 
      form->formElements;
  for (unsigned i = 0; i < form_elements.size(); ++i) {
    WebCore::HTMLGenericFormElement* form_element = form_elements[i];
    if (!form_element->disabled() && !form_element->name().isNull()) {
      bool is_text_element = false;
      if (!IsInDefaultState(form_element)) {
        return NULL;
      }
      if (form_element->hasTagName(WebCore::HTMLNames::inputTag)) {
        WebCore::HTMLInputElement* input_element = 
            static_cast<WebCore::HTMLInputElement*>(form_element);
        switch (input_element->inputType()) {
          case WebCore::HTMLInputElement::TEXT:
          case WebCore::HTMLInputElement::ISINDEX:
            is_text_element = true;
            break;
          case WebCore::HTMLInputElement::PASSWORD:
            // Don't store passwords! This is most likely an https anyway.
            // Fall through.
          case WebCore::HTMLInputElement::FILE:
            // Too big, don't try to index this.
            return NULL;
            break;
          default:
            // All other input types are indexable.
            break;
        }
      } else if (form_element->hasTagName(WebCore::HTMLNames::textareaTag)) {
        // TextArea aren't use for search.
        return NULL;
      }
      WebCore::FormDataList lst(encoding);
      if (form_element->appendFormData(lst, false)) {
        if (is_text_element && lst.list().size() > 0) {
          if (text_element != NULL) {
            // The auto-complete bar only knows how to fill in one value.
            // This form has multiple fields; don't treat it as searchable.
            return NULL;
          }
          text_element = static_cast<WebCore::HTMLInputElement*>(form_element);
        }
        for (int j = 0, max = static_cast<int>(lst.list().size()); j < max; ++j) {
          const WebCore::FormDataListItem& item = lst.list()[j];
          // handle ISINDEX / <input name=isindex> special
          // but only if its the first entry
          if (enc_string->isEmpty() && item.m_data == "isindex") {
            if (form_element == text_element)
              *enc_string += "{searchTerms}";
            else
              *enc_string += encodeCString(lst.list()[j + 1].m_data);
            ++j;
          } else {
            if (!enc_string->isEmpty())
              *enc_string += '&';
            *enc_string += encodeCString(item.m_data);
            *enc_string += "=";
            if (form_element == text_element)
              *enc_string += "{searchTerms}";
            else
              *enc_string += encodeCString(lst.list()[j + 1].m_data);
            ++j;
          }
        }
      }
    }
  }
  return text_element;
}

} // namespace

SearchableFormData* SearchableFormData::Create(WebCore::Element* element) {
  if (!element->isHTMLElement() ||
      !static_cast<WebCore::HTMLElement*>(element)->isGenericFormElement()) {
    return NULL;
  }

  WebCore::Frame* frame = element->document()->frame();
  if (frame == NULL)
    return NULL;

  WebCore::HTMLGenericFormElement* input_element = 
    static_cast<WebCore::HTMLGenericFormElement*>(element);

  WebCore::HTMLFormElement* form = input_element->form();
  if (form == NULL)
    return NULL;

  return Create(form);
}

SearchableFormData* SearchableFormData::Create(WebCore::HTMLFormElement* form) {
  WebCore::Frame* frame = form->document()->frame();
  if (frame == NULL)
    return NULL;

  // Only consider forms that GET data, do not have script for onsubmit, and
  // the action targets an http page.
  if (!IsFormMethodGet(form) || FormHasOnSubmit(form) ||
      !IsHTTPFormSubmit(form))
    return NULL;

  WebCore::DeprecatedCString enc_string = "";
  WebCore::HTMLGenericFormElement* first_submit_button = 
    GetButtonToActivate(form);

  if (first_submit_button) {
    // The form does not have an active submit button, make the first button
    // active. We need to do this, otherwise the URL will not contain the
    // name of the submit button.
    first_submit_button->setActivatedSubmit(true);
  }

  std::string encoding;
  WebCore::HTMLInputElement* text_element =
      GetTextElement(form, &enc_string, &encoding);

  if (first_submit_button)
    first_submit_button->setActivatedSubmit(false);

  if (text_element == NULL) {
    // Not a searchable form.
    return NULL;
  }

  // It's a valid form.
  // Generate the URL and create a new SearchableFormData.
  RefPtr<WebCore::FormData> form_data = new WebCore::FormData;
  form_data->appendData(enc_string.data(), enc_string.length());
  WebCore::String action = WebCore::parseURL(form->action());
  WebCore::FrameLoader* loader = frame->loader();
  WebCore::KURL url = loader->completeURL(action.isNull() ? "" : action);
  url.setQuery(form_data->flattenToString().deprecatedString());
  std::wstring url_wstring = webkit_glue::StringToStdWString(url.string());
  std::wstring current_value = webkit_glue::StringToStdWString(
    static_cast<WebCore::HTMLInputElement*>(text_element)->value());
  std::wstring text_name = 
    webkit_glue::StringToStdWString(text_element->name());
  return
      new SearchableFormData(url_wstring, text_name, current_value, encoding);
}

// static 
bool SearchableFormData::Equals(const SearchableFormData* a, 
                                const SearchableFormData* b) {
  return ((a == b) ||
          (a != NULL && b != NULL &&
           a->url().spec() == b->url().spec() &&
           a->element_name() == b->element_name() &&
           a->element_value() == b->element_value() &&
           a->encoding() == b->encoding()));
}

SearchableFormData::SearchableFormData(const std::wstring& url, 
                                       const std::wstring& element_name,
                                       const std::wstring& element_value,
                                       const std::string& encoding)
    : url_(url),
      element_name_(element_name),
      element_value_(element_value),
      encoding_(encoding) {
}
