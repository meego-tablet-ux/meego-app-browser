// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/transport_security_state.h"

#include "base/base64.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/sha1.h"
#include "base/string_number_conversions.h"
#include "base/string_split.h"
#include "base/string_tokenizer.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "crypto/sha2.h"
#include "googleurl/src/gurl.h"
#include "net/base/dns_util.h"
#include "net/base/net_switches.h"

namespace net {

const long int TransportSecurityState::kMaxHSTSAgeSecs = 86400 * 365;  // 1 year

TransportSecurityState::TransportSecurityState()
    : delegate_(NULL) {
}

static std::string HashHost(const std::string& canonicalized_host) {
  char hashed[crypto::SHA256_LENGTH];
  crypto::SHA256HashString(canonicalized_host, hashed, sizeof(hashed));
  return std::string(hashed, sizeof(hashed));
}

void TransportSecurityState::EnableHost(const std::string& host,
                                        const DomainState& state) {
  const std::string canonicalized_host = CanonicalizeHost(host);
  if (canonicalized_host.empty())
    return;

  // TODO(cevans) -- we likely want to permit a host to override a built-in,
  // for at least the case where the override is stricter (i.e. includes
  // subdomains, or includes certificate pinning).
  DomainState temp;
  if (IsPreloadedSTS(canonicalized_host, true, &temp))
    return;

  // Use the original creation date if we already have this host.
  DomainState state_copy(state);
  DomainState existing_state;
  if (IsEnabledForHost(&existing_state, host, true))
    state_copy.created = existing_state.created;

  // We don't store these values.
  state_copy.preloaded = false;
  state_copy.domain.clear();

  enabled_hosts_[HashHost(canonicalized_host)] = state_copy;
  DirtyNotify();
}

bool TransportSecurityState::DeleteHost(const std::string& host) {
  const std::string canonicalized_host = CanonicalizeHost(host);
  if (canonicalized_host.empty())
    return false;

  std::map<std::string, DomainState>::iterator i = enabled_hosts_.find(
      HashHost(canonicalized_host));
  if (i != enabled_hosts_.end()) {
    enabled_hosts_.erase(i);
    DirtyNotify();
    return true;
  }
  return false;
}

// IncludeNUL converts a char* to a std::string and includes the terminating
// NUL in the result.
static std::string IncludeNUL(const char* in) {
  return std::string(in, strlen(in) + 1);
}

bool TransportSecurityState::IsEnabledForHost(DomainState* result,
                                              const std::string& host,
                                              bool sni_available) {
  const std::string canonicalized_host = CanonicalizeHost(host);
  if (canonicalized_host.empty())
    return false;

  if (IsPreloadedSTS(canonicalized_host, sni_available, result))
    return result->mode != DomainState::MODE_NONE;

  *result = DomainState();

  base::Time current_time(base::Time::Now());

  for (size_t i = 0; canonicalized_host[i]; i += canonicalized_host[i] + 1) {
    std::string hashed_domain(HashHost(IncludeNUL(&canonicalized_host[i])));

    std::map<std::string, DomainState>::iterator j =
        enabled_hosts_.find(hashed_domain);
    if (j == enabled_hosts_.end())
      continue;

    if (current_time > j->second.expiry) {
      enabled_hosts_.erase(j);
      DirtyNotify();
      continue;
    }

    *result = j->second;
    result->domain = DNSDomainToString(
        canonicalized_host.substr(i, canonicalized_host.size() - i));

    // If we matched the domain exactly, it doesn't matter what the value of
    // include_subdomains is.
    if (i == 0)
      return true;

    return j->second.include_subdomains;
  }

  return false;
}

void TransportSecurityState::DeleteSince(const base::Time& time) {
  bool dirtied = false;

  std::map<std::string, DomainState>::iterator i = enabled_hosts_.begin();
  while (i != enabled_hosts_.end()) {
    if (i->second.created >= time) {
      dirtied = true;
      enabled_hosts_.erase(i++);
    } else {
      i++;
    }
  }

  if (dirtied)
    DirtyNotify();
}

// MaxAgeToInt converts a string representation of a number of seconds into a
// int. We use strtol in order to handle overflow correctly. The string may
// contain an arbitary number which we should truncate correctly rather than
// throwing a parse failure.
static bool MaxAgeToInt(std::string::const_iterator begin,
                        std::string::const_iterator end,
                        int* result) {
  const std::string s(begin, end);
  char* endptr;
  long int i = strtol(s.data(), &endptr, 10 /* base */);
  if (*endptr || i < 0)
    return false;
  if (i > TransportSecurityState::kMaxHSTSAgeSecs)
    i = TransportSecurityState::kMaxHSTSAgeSecs;
  *result = i;
  return true;
}

// "Strict-Transport-Security" ":"
//     "max-age" "=" delta-seconds [ ";" "includeSubDomains" ]
bool TransportSecurityState::ParseHeader(const std::string& value,
                                         int* max_age,
                                         bool* include_subdomains) {
  DCHECK(max_age);
  DCHECK(include_subdomains);

  int max_age_candidate = 0;

  enum ParserState {
    START,
    AFTER_MAX_AGE_LABEL,
    AFTER_MAX_AGE_EQUALS,
    AFTER_MAX_AGE,
    AFTER_MAX_AGE_INCLUDE_SUB_DOMAINS_DELIMITER,
    AFTER_INCLUDE_SUBDOMAINS,
  } state = START;

  StringTokenizer tokenizer(value, " \t=;");
  tokenizer.set_options(StringTokenizer::RETURN_DELIMS);
  while (tokenizer.GetNext()) {
    DCHECK(!tokenizer.token_is_delim() || tokenizer.token().length() == 1);
    switch (state) {
      case START:
        if (IsAsciiWhitespace(*tokenizer.token_begin()))
          continue;
        if (!LowerCaseEqualsASCII(tokenizer.token(), "max-age"))
          return false;
        state = AFTER_MAX_AGE_LABEL;
        break;

      case AFTER_MAX_AGE_LABEL:
        if (IsAsciiWhitespace(*tokenizer.token_begin()))
          continue;
        if (*tokenizer.token_begin() != '=')
          return false;
        DCHECK(tokenizer.token().length() ==  1);
        state = AFTER_MAX_AGE_EQUALS;
        break;

      case AFTER_MAX_AGE_EQUALS:
        if (IsAsciiWhitespace(*tokenizer.token_begin()))
          continue;
        if (!MaxAgeToInt(tokenizer.token_begin(),
                         tokenizer.token_end(),
                         &max_age_candidate))
          return false;
        state = AFTER_MAX_AGE;
        break;

      case AFTER_MAX_AGE:
        if (IsAsciiWhitespace(*tokenizer.token_begin()))
          continue;
        if (*tokenizer.token_begin() != ';')
          return false;
        state = AFTER_MAX_AGE_INCLUDE_SUB_DOMAINS_DELIMITER;
        break;

      case AFTER_MAX_AGE_INCLUDE_SUB_DOMAINS_DELIMITER:
        if (IsAsciiWhitespace(*tokenizer.token_begin()))
          continue;
        if (!LowerCaseEqualsASCII(tokenizer.token(), "includesubdomains"))
          return false;
        state = AFTER_INCLUDE_SUBDOMAINS;
        break;

      case AFTER_INCLUDE_SUBDOMAINS:
        if (!IsAsciiWhitespace(*tokenizer.token_begin()))
          return false;
        break;

      default:
        NOTREACHED();
    }
  }

  // We've consumed all the input.  Let's see what state we ended up in.
  switch (state) {
    case START:
    case AFTER_MAX_AGE_LABEL:
    case AFTER_MAX_AGE_EQUALS:
      return false;
    case AFTER_MAX_AGE:
      *max_age = max_age_candidate;
      *include_subdomains = false;
      return true;
    case AFTER_MAX_AGE_INCLUDE_SUB_DOMAINS_DELIMITER:
      return false;
    case AFTER_INCLUDE_SUBDOMAINS:
      *max_age = max_age_candidate;
      *include_subdomains = true;
      return true;
    default:
      NOTREACHED();
      return false;
  }
}

void TransportSecurityState::SetDelegate(
    TransportSecurityState::Delegate* delegate) {
  delegate_ = delegate;
}

// This function converts the binary hashes, which we store in
// |enabled_hosts_|, to a base64 string which we can include in a JSON file.
static std::string HashedDomainToExternalString(const std::string& hashed) {
  std::string out;
  CHECK(base::Base64Encode(hashed, &out));
  return out;
}

// This inverts |HashedDomainToExternalString|, above. It turns an external
// string (from a JSON file) into an internal (binary) string.
static std::string ExternalStringToHashedDomain(const std::string& external) {
  std::string out;
  if (!base::Base64Decode(external, &out) ||
      out.size() != crypto::SHA256_LENGTH) {
    return std::string();
  }

  return out;
}

bool TransportSecurityState::Serialise(std::string* output) {
  DictionaryValue toplevel;
  for (std::map<std::string, DomainState>::const_iterator
       i = enabled_hosts_.begin(); i != enabled_hosts_.end(); ++i) {
    DictionaryValue* state = new DictionaryValue;
    state->SetBoolean("include_subdomains", i->second.include_subdomains);
    state->SetDouble("created", i->second.created.ToDoubleT());
    state->SetDouble("expiry", i->second.expiry.ToDoubleT());

    switch (i->second.mode) {
      case DomainState::MODE_STRICT:
        state->SetString("mode", "strict");
        break;
      case DomainState::MODE_OPPORTUNISTIC:
        state->SetString("mode", "opportunistic");
        break;
      case DomainState::MODE_SPDY_ONLY:
        state->SetString("mode", "spdy-only");
        break;
      default:
        NOTREACHED() << "DomainState with unknown mode";
        delete state;
        continue;
    }

    ListValue* pins = new ListValue;
    for (std::vector<SHA1Fingerprint>::const_iterator
         j = i->second.public_key_hashes.begin();
         j != i->second.public_key_hashes.end(); ++j) {
      std::string hash_str(reinterpret_cast<const char*>(j->data),
                           sizeof(j->data));
      std::string b64;
      base::Base64Encode(hash_str, &b64);
      pins->Append(new StringValue("sha1/" + b64));
    }
    state->Set("public_key_hashes", pins);

    toplevel.Set(HashedDomainToExternalString(i->first), state);
  }

  base::JSONWriter::Write(&toplevel, true /* pretty print */, output);
  return true;
}

bool TransportSecurityState::LoadEntries(const std::string& input,
                                         bool* dirty) {
  enabled_hosts_.clear();
  return Deserialise(input, dirty, &enabled_hosts_);
}

// static
bool TransportSecurityState::Deserialise(
    const std::string& input,
    bool* dirty,
    std::map<std::string, DomainState>* out) {
  scoped_ptr<Value> value(
      base::JSONReader::Read(input, false /* do not allow trailing commas */));
  if (!value.get() || !value->IsType(Value::TYPE_DICTIONARY))
    return false;

  DictionaryValue* dict_value = reinterpret_cast<DictionaryValue*>(value.get());
  const base::Time current_time(base::Time::Now());
  bool dirtied = false;

  for (DictionaryValue::key_iterator i = dict_value->begin_keys();
       i != dict_value->end_keys(); ++i) {
    DictionaryValue* state;
    if (!dict_value->GetDictionaryWithoutPathExpansion(*i, &state))
      continue;

    bool include_subdomains;
    std::string mode_string;
    double created;
    double expiry;

    if (!state->GetBoolean("include_subdomains", &include_subdomains) ||
        !state->GetString("mode", &mode_string) ||
        !state->GetDouble("expiry", &expiry)) {
      continue;
    }

    ListValue* pins_list = NULL;
    std::vector<SHA1Fingerprint> public_key_hashes;
    if (state->GetList("public_key_hashes", &pins_list)) {
      size_t num_pins = pins_list->GetSize();
      for (size_t i = 0; i < num_pins; ++i) {
        std::string type_and_base64;
        std::string hash_str;
        SHA1Fingerprint hash;
        if (pins_list->GetString(i, &type_and_base64) &&
            type_and_base64.find("sha1/") == 0 &&
            base::Base64Decode(
                type_and_base64.substr(5, type_and_base64.size() - 5),
                &hash_str) &&
            hash_str.size() == base::SHA1_LENGTH) {
          memcpy(hash.data, hash_str.data(), sizeof(hash.data));
          public_key_hashes.push_back(hash);
        }
      }
    }

    DomainState::Mode mode;
    if (mode_string == "strict") {
      mode = DomainState::MODE_STRICT;
    } else if (mode_string == "opportunistic") {
      mode = DomainState::MODE_OPPORTUNISTIC;
    } else if (mode_string == "spdy-only") {
      mode = DomainState::MODE_SPDY_ONLY;
    } else if (mode_string == "none") {
      mode = DomainState::MODE_NONE;
    } else {
      LOG(WARNING) << "Unknown TransportSecurityState mode string found: "
                   << mode_string;
      continue;
    }

    base::Time expiry_time = base::Time::FromDoubleT(expiry);
    base::Time created_time;
    if (state->GetDouble("created", &created)) {
      created_time = base::Time::FromDoubleT(created);
    } else {
      // We're migrating an old entry with no creation date. Make sure we
      // write the new date back in a reasonable time frame.
      dirtied = true;
      created_time = base::Time::Now();
    }

    if (expiry_time <= current_time) {
      // Make sure we dirty the state if we drop an entry.
      dirtied = true;
      continue;
    }

    std::string hashed = ExternalStringToHashedDomain(*i);
    if (hashed.empty()) {
      dirtied = true;
      continue;
    }

    DomainState new_state;
    new_state.mode = mode;
    new_state.created = created_time;
    new_state.expiry = expiry_time;
    new_state.include_subdomains = include_subdomains;
    new_state.public_key_hashes = public_key_hashes;
    (*out)[hashed] = new_state;
  }

  *dirty = dirtied;
  return true;
}

TransportSecurityState::~TransportSecurityState() {
}

void TransportSecurityState::DirtyNotify() {
  if (delegate_)
    delegate_->StateIsDirty(this);
}

// static
std::string TransportSecurityState::CanonicalizeHost(const std::string& host) {
  // We cannot perform the operations as detailed in the spec here as |host|
  // has already undergone IDN processing before it reached us. Thus, we check
  // that there are no invalid characters in the host and lowercase the result.

  std::string new_host;
  if (!DNSDomainFromDot(host, &new_host)) {
    // DNSDomainFromDot can fail if any label is > 63 bytes or if the whole
    // name is >255 bytes. However, search terms can have those properties.
    return std::string();
  }

  for (size_t i = 0; new_host[i]; i += new_host[i] + 1) {
    const unsigned label_length = static_cast<unsigned>(new_host[i]);
    if (!label_length)
      break;

    for (size_t j = 0; j < label_length; ++j) {
      // RFC 3490, 4.1, step 3
      if (!IsSTD3ASCIIValidCharacter(new_host[i + 1 + j]))
        return std::string();

      new_host[i + 1 + j] = tolower(new_host[i + 1 + j]);
    }

    // step 3(b)
    if (new_host[i + 1] == '-' ||
        new_host[i + label_length] == '-') {
      return std::string();
    }
  }

  return new_host;
}

// IsPreloadedSTS returns true if the canonicalized hostname should always be
// considered to have STS enabled.
// static
bool TransportSecurityState::IsPreloadedSTS(
    const std::string& canonicalized_host,
    bool sni_available,
    DomainState* out) {
  out->preloaded = true;
  out->mode = DomainState::MODE_STRICT;
  out->created = base::Time::FromTimeT(0);
  out->expiry = out->created;
  out->include_subdomains = false;

  std::map<std::string, DomainState> hosts;
  std::string cmd_line_hsts =
      CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kHstsHosts);
  if (!cmd_line_hsts.empty()) {
    bool dirty;
    Deserialise(cmd_line_hsts, &dirty, &hosts);
  }

  // In the medium term this list is likely to just be hardcoded here. This,
  // slightly odd, form removes the need for additional relocations records.
  static const struct {
    uint8 length;
    bool include_subdomains;
    char dns_name[30];
  } kPreloadedSTS[] = {
    {16, false, "\003www\006paypal\003com"},
    {16, false, "\003www\006elanex\003biz"},
    {12, true,  "\006jottit\003com"},
    {19, true,  "\015sunshinepress\003org"},
    {21, false, "\003www\013noisebridge\003net"},
    {10, false, "\004neg9\003org"},
    {12, true, "\006riseup\003net"},
    {11, false, "\006factor\002cc"},
    {22, false, "\007members\010mayfirst\003org"},
    {22, false, "\007support\010mayfirst\003org"},
    {17, false, "\002id\010mayfirst\003org"},
    {20, false, "\005lists\010mayfirst\003org"},
    {19, true, "\015splendidbacon\003com"},
    {19, true, "\006health\006google\003com"},
    {21, true, "\010checkout\006google\003com"},
    {19, true, "\006chrome\006google\003com"},
    {26, false, "\006latest\006chrome\006google\003com"},
    {28, false, "\016aladdinschools\007appspot\003com"},
    {14, true, "\011ottospora\002nl"},
    {17, true, "\004docs\006google\003com"},
    {18, true, "\005sites\006google\003com"},
    {25, true, "\014spreadsheets\006google\003com"},
    {22, false, "\011appengine\006google\003com"},
    {25, false, "\003www\017paycheckrecords\003com"},
    {20, true, "\006market\007android\003com"},
    {14, false, "\010lastpass\003com"},
    {18, false, "\003www\010lastpass\003com"},
    {14, true, "\010keyerror\003com"},
    {22, true, "\011encrypted\006google\003com"},
    {13, false, "\010entropia\002de"},
    {17, false, "\003www\010entropia\002de"},
    {21, true, "\010accounts\006google\003com"},
#if defined(OS_CHROMEOS)
    {17, true, "\004mail\006google\003com"},
    {13, false, "\007twitter\003com"},
    {17, false, "\003www\007twitter\003com"},
    {17, false, "\003api\007twitter\003com"},
    {17, false, "\003dev\007twitter\003com"},
    {22, false, "\010business\007twitter\003com"},
#endif
  };
  static const size_t kNumPreloadedSTS = ARRAYSIZE_UNSAFE(kPreloadedSTS);

  static const struct {
    uint8 length;
    bool include_subdomains;
    char dns_name[30];
  } kPreloadedSNISTS[] = {
    {11, false, "\005gmail\003com"},
    {16, false, "\012googlemail\003com"},
    {15, false, "\003www\005gmail\003com"},
    {20, false, "\003www\012googlemail\003com"},
  };
  static const size_t kNumPreloadedSNISTS = ARRAYSIZE_UNSAFE(kPreloadedSNISTS);

  for (size_t i = 0; canonicalized_host[i]; i += canonicalized_host[i] + 1) {
    std::string host_sub_chunk(&canonicalized_host[i],
                               canonicalized_host.size() - i);
    out->domain = DNSDomainToString(host_sub_chunk);
    std::string hashed_host(HashHost(host_sub_chunk));
    if (hosts.find(hashed_host) != hosts.end()) {
      *out = hosts[hashed_host];
      out->domain = DNSDomainToString(host_sub_chunk);
      out->preloaded = true;
      return true;
    }
    for (size_t j = 0; j < kNumPreloadedSTS; j++) {
      if (kPreloadedSTS[j].length == canonicalized_host.size() - i &&
          memcmp(kPreloadedSTS[j].dns_name, &canonicalized_host[i],
                 kPreloadedSTS[j].length) == 0) {
        if (!kPreloadedSTS[j].include_subdomains && i != 0)
          return false;
        out->include_subdomains = kPreloadedSTS[j].include_subdomains;
        return true;
      }
    }
    if (sni_available) {
      for (size_t j = 0; j < kNumPreloadedSNISTS; j++) {
        if (kPreloadedSNISTS[j].length == canonicalized_host.size() - i &&
            memcmp(kPreloadedSNISTS[j].dns_name, &canonicalized_host[i],
                   kPreloadedSNISTS[j].length) == 0) {
          if (!kPreloadedSNISTS[j].include_subdomains && i != 0)
            return false;
          out->include_subdomains = kPreloadedSNISTS[j].include_subdomains;
          return true;
        }
      }
    }
  }

  return false;
}

static std::string HashesToBase64String(
    const std::vector<net::SHA1Fingerprint>& hashes) {
  std::vector<std::string> hashes_strs;
  for (std::vector<net::SHA1Fingerprint>::const_iterator
       i = hashes.begin(); i != hashes.end(); i++) {
    std::string s;
    const std::string hash_str(reinterpret_cast<const char*>(i->data),
                               sizeof(i->data));
    base::Base64Encode(hash_str, &s);
    hashes_strs.push_back(s);
  }

  return JoinString(hashes_strs, ',');
}

TransportSecurityState::DomainState::DomainState()
    : mode(MODE_STRICT),
      created(base::Time::Now()),
      include_subdomains(false),
      preloaded(false) {
}

TransportSecurityState::DomainState::~DomainState() {
}

bool TransportSecurityState::DomainState::IsChainOfPublicKeysPermitted(
    const std::vector<net::SHA1Fingerprint>& hashes) {
  if (public_key_hashes.empty())
    return true;

  for (std::vector<net::SHA1Fingerprint>::const_iterator
       i = hashes.begin(); i != hashes.end(); ++i) {
    for (std::vector<net::SHA1Fingerprint>::const_iterator
         j = public_key_hashes.begin(); j != public_key_hashes.end(); ++j) {
      if (i->Equals(*j))
        return true;
    }
  }

  LOG(ERROR) << "Rejecting public key chain for domain " << domain
             << ". Validated chain: " << HashesToBase64String(hashes)
             << ", expected: " << HashesToBase64String(public_key_hashes);

  return false;
}

}  // namespace
