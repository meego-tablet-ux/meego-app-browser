// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/form_structure.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/sha1.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autofill/field_types.h"
#include "chrome/browser/autofill/form_field.h"
#include "third_party/libjingle/files/talk/xmllite/xmlelement.h"
#include "webkit/glue/form_data.h"
#include "webkit/glue/form_field.h"

using webkit_glue::FormData;

namespace {

const char* kFormMethodPost = "post";

// XML attribute names.
const char* const kAttributeClientVersion = "clientversion";
const char* const kAttributeAutoFillUsed = "autofillused";
const char* const kAttributeSignature = "signature";
const char* const kAttributeFormSignature = "formsignature";
const char* const kAttributeDataPresent = "datapresent";

const char* const kXMLElementForm = "form";
const char* const kXMLElementField = "field";
const char* const kAttributeAutoFillType = "autofilltype";

// The list of form control types we handle.
const char* const kControlTypeSelect = "select-one";
const char* const kControlTypeText = "text";

// The number of fillable fields necessary for a form to be fillable.
const size_t kRequiredFillableFields = 3;

static std::string Hash64Bit(const std::string& str) {
  std::string hash_bin = base::SHA1HashString(str);
  DCHECK_EQ(20U, hash_bin.length());

  uint64 hash64 = (((static_cast<uint64>(hash_bin[0])) & 0xFF) << 56) |
                  (((static_cast<uint64>(hash_bin[1])) & 0xFF) << 48) |
                  (((static_cast<uint64>(hash_bin[2])) & 0xFF) << 40) |
                  (((static_cast<uint64>(hash_bin[3])) & 0xFF) << 32) |
                  (((static_cast<uint64>(hash_bin[4])) & 0xFF) << 24) |
                  (((static_cast<uint64>(hash_bin[5])) & 0xFF) << 16) |
                  (((static_cast<uint64>(hash_bin[6])) & 0xFF) << 8) |
                   ((static_cast<uint64>(hash_bin[7])) & 0xFF);

  return Uint64ToString(hash64);
}

}  // namespace

FormStructure::FormStructure(const FormData& form)
    : form_name_(UTF16ToUTF8(form.name)),
      source_url_(form.origin),
      target_url_(form.action) {
  // Copy the form fields.
  std::vector<webkit_glue::FormField>::const_iterator field;
  for (field = form.fields.begin();
       field != form.fields.end(); field++) {
    // We currently only handle text and select fields.  This prevents us from
    // thinking we can autofill other types of controls, e.g., password, hidden,
    // submit.
    if (!LowerCaseEqualsASCII(field->form_control_type(), kControlTypeText) &&
        !LowerCaseEqualsASCII(field->form_control_type(), kControlTypeSelect))
      continue;

    // Add all form fields (including with empty names) to signature.
    // This is a requirement for AutoFill servers.
    form_signature_field_names_.append("&");
    form_signature_field_names_.append(UTF16ToUTF8(field->name()));

    // Generate a unique name for this field by appending a counter to the name.
    string16 unique_name = field->name() + IntToString16(fields_.size() + 1);
    fields_.push_back(new AutoFillField(*field, unique_name));
  }

  // Terminate the vector with a NULL item.
  fields_.push_back(NULL);

  std::string method = UTF16ToUTF8(form.method);
  if (method == kFormMethodPost) {
    method_ = POST;
  } else {
    // Either the method is 'get', or we don't know.  In this case we default
    // to GET.
    method_ = GET;
  }
}

bool FormStructure::EncodeUploadRequest(bool auto_fill_used,
                                        std::string* encoded_xml) const {
  bool auto_fillable = IsAutoFillable();
  DCHECK(auto_fillable);  // Caller should've checked for search pages.
  if (!auto_fillable)
    return false;

  buzz::XmlElement autofil_request_xml(buzz::QName("autofillupload"));

  // Attributes for the <autofillupload> element.
  //
  // TODO(jhawkins): Work with toolbar devs to make a spec for autofill clients.
  // For now these values are hacked from the toolbar code.
  autofil_request_xml.SetAttr(buzz::QName(kAttributeClientVersion),
                              "6.1.1715.1442/en (GGLL)");

  autofil_request_xml.SetAttr(buzz::QName(kAttributeFormSignature),
                              FormSignature());

  autofil_request_xml.SetAttr(buzz::QName(kAttributeAutoFillUsed),
                              auto_fill_used ? "true" : "false");

  // TODO(jhawkins): Hook this up to the personal data manager.
  // personaldata_manager_->GetDataPresent();
  autofil_request_xml.SetAttr(buzz::QName(kAttributeDataPresent), "");

  EncodeFormRequest(FormStructure::UPLOAD, &autofil_request_xml);

  // Obtain the XML structure as a string.
  *encoded_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  *encoded_xml += autofil_request_xml.Str().c_str();

  return true;
}

bool FormStructure::EncodeQueryRequest(const ScopedVector<FormStructure>& forms,
                                       std::string* encoded_xml) {
  buzz::XmlElement autofil_request_xml(buzz::QName("autofillquery"));
  // Attributes for the <autofillquery> element.
  //
  // TODO(jhawkins): Work with toolbar devs to make a spec for autofill clients.
  // For now these values are hacked from the toolbar code.
  autofil_request_xml.SetAttr(buzz::QName(kAttributeClientVersion),
                              "6.1.1715.1442/en (GGLL)");
  for (ScopedVector<FormStructure>::const_iterator it = forms.begin();
       it != forms.end();
       ++it) {
    buzz::XmlElement* encompassing_xml_element =
        new buzz::XmlElement(buzz::QName("form"));
    encompassing_xml_element->SetAttr(buzz::QName(kAttributeSignature),
                                      (*it)->FormSignature());

    (*it)->EncodeFormRequest(FormStructure::QUERY, encompassing_xml_element);

    autofil_request_xml.AddElement(encompassing_xml_element);
  }

  // Obtain the XML structure as a string.
  *encoded_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  *encoded_xml += autofil_request_xml.Str().c_str();

  return true;
}

void FormStructure::GetHeuristicAutoFillTypes() {
  has_credit_card_field_ = false;
  has_autofillable_field_ = false;

  FieldTypeMap field_type_map;
  GetHeuristicFieldInfo(&field_type_map);

  for (size_t index = 0; index < field_count(); index++) {
    AutoFillField* field = fields_[index];
    FieldTypeMap::iterator iter = field_type_map.find(field->unique_name());

    AutoFillFieldType heuristic_auto_fill_type;
    if (iter == field_type_map.end())
      heuristic_auto_fill_type = UNKNOWN_TYPE;
    else
      heuristic_auto_fill_type = iter->second;

    field->set_heuristic_type(heuristic_auto_fill_type);

    AutoFillType autofill_type(field->type());
    if (autofill_type.group() == AutoFillType::CREDIT_CARD)
      has_credit_card_field_ = true;
    if (autofill_type.field_type() != UNKNOWN_TYPE)
      has_autofillable_field_ = true;
  }
}

std::string FormStructure::FormSignature() const {
  std::string form_string = target_url_.scheme() +
                            "://" +
                            target_url_.host() +
                            "&" +
                            form_name_ +
                            form_signature_field_names_;

  return Hash64Bit(form_string);
}

bool FormStructure::IsAutoFillable() const {
  if (field_count() < kRequiredFillableFields)
    return false;

  // Rule out http(s)://*/search?...
  //  e.g. http://www.google.com/search?q=...
  //       http://search.yahoo.com/search?p=...
  if (target_url_.path() == "/search")
    return false;

  if (method_ == GET)
    return false;

  return true;
}

void FormStructure::set_possible_types(int index, const FieldTypeSet& types) {
  int num_fields = static_cast<int>(field_count());
  DCHECK(index >= 0 && index < num_fields);
  if (index >= 0 && index < num_fields)
    fields_[index]->set_possible_types(types);
}

const AutoFillField* FormStructure::field(int index) const {
  return fields_[index];
}

size_t FormStructure::field_count() const {
  // Don't count the NULL terminator.
  size_t field_size = fields_.size();
  return (field_size == 0) ? 0 : field_size - 1;
}

bool FormStructure::operator==(const FormData& form) const {
  // TODO(jhawkins): Is this enough to differentiate a form?
  if (UTF8ToUTF16(form_name_) == form.name &&
      source_url_ == form.origin &&
      target_url_ == form.action) {
    return true;
  }

  // TODO(jhawkins): Compare field names, IDs and labels once we have labels
  // set up.

  return false;
}

bool FormStructure::operator!=(const FormData& form) const {
  return !operator==(form);
}

void FormStructure::GetHeuristicFieldInfo(FieldTypeMap* field_type_map) {
  FormFieldSet fields = FormFieldSet(this);

  FormFieldSet::const_iterator field;
  for (field = fields.begin(); field != fields.end(); field++) {
    bool ok = (*field)->GetFieldInfo(field_type_map);
    DCHECK(ok);
  }
}

bool FormStructure::EncodeFormRequest(
    FormStructure::EncodeRequestType request_type,
    buzz::XmlElement* encompassing_xml_element) const {
  if (!field_count())  // Nothing to add.
    return false;
  // Add the child nodes for the form fields.
  for (size_t index = 0; index < field_count(); index++) {
    const AutoFillField* field = fields_[index];
    if (request_type == FormStructure::UPLOAD) {
      FieldTypeSet types = field->possible_types();
      for (FieldTypeSet::const_iterator type = types.begin();
           type != types.end(); type++) {
        buzz::XmlElement *field_element = new buzz::XmlElement(
            buzz::QName(kXMLElementField));

        field_element->SetAttr(buzz::QName(kAttributeSignature),
                               field->FieldSignature());
        field_element->SetAttr(buzz::QName(kAttributeAutoFillType),
                               IntToString(*type));
        encompassing_xml_element->AddElement(field_element);
      }
    } else {
      buzz::XmlElement *field_element = new buzz::XmlElement(
          buzz::QName(kXMLElementField));
      field_element->SetAttr(buzz::QName(kAttributeSignature),
                             field->FieldSignature());
      encompassing_xml_element->AddElement(field_element);
    }
  }
  return true;
}

