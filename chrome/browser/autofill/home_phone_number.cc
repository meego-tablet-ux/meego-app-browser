// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/home_phone_number.h"

FormGroup* HomePhoneNumber::Clone() const {
  return new HomePhoneNumber(*this);
}

AutofillFieldType HomePhoneNumber::GetNumberType() const {
  return PHONE_HOME_NUMBER;
}

AutofillFieldType HomePhoneNumber::GetCityCodeType() const {
  return PHONE_HOME_CITY_CODE;
}

AutofillFieldType HomePhoneNumber::GetCountryCodeType() const {
  return PHONE_HOME_COUNTRY_CODE;
}

AutofillFieldType HomePhoneNumber::GetCityAndNumberType() const {
  return PHONE_HOME_CITY_AND_NUMBER;
}

AutofillFieldType HomePhoneNumber::GetWholeNumberType() const {
  return PHONE_HOME_WHOLE_NUMBER;
}
