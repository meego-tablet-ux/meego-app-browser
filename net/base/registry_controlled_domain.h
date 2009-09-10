//* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla TLD Service
 *
 * The Initial Developer of the Original Code is
 * Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pamela Greene <pamg.bugs@gmail.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

// NB: Modelled after Mozilla's code (originally written by Pamela Greene,
// later modified by others), but almost entirely rewritten for Chrome.

/*
  (Documentation based on the Mozilla documentation currently at
  http://wiki.mozilla.org/Gecko:Effective_TLD_Service, written by the same
  author.)

  The RegistryControlledDomainService examines the hostname of a GURL passed to
  it and determines the longest portion that is controlled by a registrar.
  Although technically the top-level domain (TLD) for a hostname is the last
  dot-portion of the name (such as .com or .org), many domains (such as co.uk)
  function as though they were TLDs, allocating any number of more specific,
  essentially unrelated names beneath them.  For example, .uk is a TLD, but
  nobody is allowed to register a domain directly under .uk; the "effective"
  TLDs are ac.uk, co.uk, and so on.  We wouldn't want to allow any site in
  *.co.uk to set a cookie for the entire co.uk domain, so it's important to be
  able to identify which higher-level domains function as effective TLDs and
  which can be registered.

  The service obtains its information about effective TLDs from a text resource
  that must be in the following format:

  * It should use plain ASCII.
  * It should contain one domain rule per line, terminated with \n, with nothing
    else on the line.  (The last rule in the file may omit the ending \n.)
  * Rules should have been normalized using the same canonicalization that GURL
    applies.  For ASCII, that means they're not case-sensitive, among other
    things; other normalizations are applied for other characters.
  * Each rule should list the entire TLD-like domain name, with any subdomain
    portions separated by dots (.) as usual.
  * Rules should neither begin nor end with a dot.
  * If a hostname matches more than one rule, the most specific rule (that is,
    the one with more dot-levels) will be used.
  * Other than in the case of wildcards (see below), rules do not implicitly
    include their subcomponents.  For example, "bar.baz.uk" does not imply
    "baz.uk", and if "bar.baz.uk" is the only rule in the list, "foo.bar.baz.uk"
    will match, but "baz.uk" and "qux.baz.uk" won't.
  * The wildcard character '*' will match any valid sequence of characters.
  * Wildcards may only appear as the entire most specific level of a rule.  That
    is, a wildcard must come at the beginning of a line and must be followed by
    a dot.  (You may not use a wildcard as the entire rule.)
  * A wildcard rule implies a rule for the entire non-wildcard portion.  For
    example, the rule "*.foo.bar" implies the rule "foo.bar" (but not the rule
    "bar").  This is typically important in the case of exceptions (see below).
  * The exception character '!' before a rule marks an exception to a wildcard
    rule.  If your rules are "*.tokyo.jp" and "!pref.tokyo.jp", then
    "a.b.tokyo.jp" has an effective TLD of "b.tokyo.jp", but "a.pref.tokyo.jp"
    has an effective TLD of "tokyo.jp" (the exception prevents the wildcard
    match, and we thus fall through to matching on the implied "tokyo.jp" rule
    from the wildcard).
  * If you use an exception rule without a corresponding wildcard rule, the
    behavior is undefined.

  Firefox has a very similar service, and it's their data file we use to
  construct our resource.  However, the data expected by this implementation
  differs from the Mozilla file in several important ways:
   (1) We require that all single-level TLDs (com, edu, etc.) be explicitly
       listed.  As of this writing, Mozilla's file includes the single-level
       TLDs too, but that might change.
   (2) Our data is expected be in pure ASCII: all UTF-8 or otherwise encoded
       items must already have been normalized.
   (3) We do not allow comments, rule notes, blank lines, or line endings other
       than LF.
  Rules are also expected to be syntactically valid.

  The utility application tld_cleanup.exe converts a Mozilla-style file into a
  Chrome one, making sure that single-level TLDs are explicitly listed, using
  GURL to normalize rules, and validating the rules.
*/

#ifndef NET_BASE_REGISTRY_CONTROLLED_DOMAIN_H_
#define NET_BASE_REGISTRY_CONTROLLED_DOMAIN_H_

#include <set>
#include <string>

#include "base/basictypes.h"
#include "base/string_piece.h"

class GURL;

template <typename T>
struct DefaultSingletonTraits;

namespace net {

struct RegistryControlledDomainServiceSingletonTraits;

// This class is a singleton.
class RegistryControlledDomainService {
 public:
   ~RegistryControlledDomainService() { }

  // Returns the registered, organization-identifying host and all its registry
  // information, but no subdomains, from the given GURL.  Returns an empty
  // string if the GURL is invalid, has no host (e.g. a file: URL), has multiple
  // trailing dots, is an IP address, has only one subcomponent (i.e. no dots
  // other than leading/trailing ones), or is itself a recognized registry
  // identifier.  If no matching rule is found in the effective-TLD data (or in
  // the default data, if the resource failed to load), the last subcomponent of
  // the host is assumed to be the registry.
  //
  // Examples:
  //   http://www.google.com/file.html -> "google.com"  (com)
  //   http://..google.com/file.html   -> "google.com"  (com)
  //   http://google.com./file.html    -> "google.com." (com)
  //   http://a.b.co.uk/file.html      -> "b.co.uk"     (co.uk)
  //   file:///C:/bar.html             -> ""            (no host)
  //   http://foo.com../file.html      -> ""            (multiple trailing dots)
  //   http://192.168.0.1/file.html    -> ""            (IP address)
  //   http://bar/file.html            -> ""            (no subcomponents)
  //   http://co.uk/file.html          -> ""            (host is a registry)
  //   http://foo.bar/file.html        -> "foo.bar"     (no rule; assume bar)
  static std::string GetDomainAndRegistry(const GURL& gurl);

  // Like the GURL version, but takes a host (which is canonicalized internally)
  // instead of a full GURL.
  static std::string GetDomainAndRegistry(const std::string& host);
  static std::string GetDomainAndRegistry(const std::wstring& host);

  // This convenience function returns true if the two GURLs both have hosts
  // and one of the following is true:
  // * They each have a known domain and registry, and it is the same for both
  //   URLs.  Note that this means the trailing dot, if any, must match too.
  // * They don't have known domains/registries, but the hosts are identical.
  // Effectively, callers can use this function to check whether the input URLs
  // represent hosts "on the same site".
  static bool SameDomainOrHost(const GURL& gurl1, const GURL& gurl2);

  // Finds the length in bytes of the registrar portion of the host in the
  // given GURL.  Returns std::string::npos if the GURL is invalid or has no
  // host (e.g. a file: URL).  Returns 0 if the GURL has multiple trailing dots,
  // is an IP address, has no subcomponents, or is itself a recognized registry
  // identifier.  If no matching rule is found in the effective-TLD data (or in
  // the default data, if the resource failed to load), returns 0 if
  // |allow_unknown_registries| is false, or the length of the last subcomponent
  // if |allow_unknown_registries| is true.
  //
  // Examples:
  //   http://www.google.com/file.html -> 3                 (com)
  //   http://..google.com/file.html   -> 3                 (com)
  //   http://google.com./file.html    -> 4                 (com)
  //   http://a.b.co.uk/file.html      -> 5                 (co.uk)
  //   file:///C:/bar.html             -> std::string::npos (no host)
  //   http://foo.com../file.html      -> 0                 (multiple trailing
  //                                                         dots)
  //   http://192.168.0.1/file.html    -> 0                 (IP address)
  //   http://bar/file.html            -> 0                 (no subcomponents)
  //   http://co.uk/file.html          -> 0                 (host is a registry)
  //   http://foo.bar/file.html        -> 0 or 3, depending (no rule; assume
  //                                                         bar)
  static size_t GetRegistryLength(const GURL& gurl,
                                  bool allow_unknown_registries);

  // Like the GURL version, but takes a host (which is canonicalized internally)
  // instead of a full GURL.
  static size_t GetRegistryLength(const std::string& host,
                                  bool allow_unknown_registries);
  static size_t GetRegistryLength(const std::wstring& host,
                                  bool allow_unknown_registries);

 protected:
  // The entire protected API is only for unit testing.  I mean it.  Don't make
  // me come over there!
  RegistryControlledDomainService() { Init(); }

  // Set the RegistryControledDomainService instance to be used internally.
  // |instance| will supersede the singleton instance normally used.  If
  // |instance| is NULL, normal behavior is restored, and internal operations
  // will return to using the singleton.  This function always returns the
  // instance set by the most recent call to SetInstance.
  static RegistryControlledDomainService* SetInstance(
      RegistryControlledDomainService* instance);

  // Sets the copied_domain_data_ of the current instance (creating one,
  // if necessary), then parses it.
  static void UseDomainData(const std::string& data);

 private:
  // To allow construction of the internal singleton instance.
  friend struct DefaultSingletonTraits<RegistryControlledDomainService>;

  void Init();

  // A DomainEntry is a combination of the domain name (as a StringPiece, so
  // that we can reference external memory without copying), and two bits of
  // information, if it's an exception and/or wildcard entry.  Note: we don't
  // consider the attributes when doing comparisons, so as far as any data
  // structures our concerned (ex our set), two DomainEntry's are equal as long
  // as their StringPiece (the domain) is equal.  This is the behavior we want.
  class DomainEntry : public base::StringPiece {
   public:
    struct DomainEntryAttributes {
      DomainEntryAttributes() : exception(false), wildcard(false) { }
      ~DomainEntryAttributes() { }

      void Combine(const DomainEntryAttributes& other) {
        if (other.exception) exception = true;
        if (other.wildcard) wildcard = true;
      }

      bool exception;
      bool wildcard;
    };

    DomainEntry() : base::StringPiece() { }
    DomainEntry(const char* ptr, size_type size)
        : base::StringPiece(ptr, size) { }
    ~DomainEntry() { }

    // We override StringPiece's operator < to make it more efficent, since we
    // don't care that it's sorted lexigraphically and we want to ignore the
    // attributes when we are doing the comparisons.
    bool operator<(const DomainEntry& other) const {
      // If we are the same size, call up to StringPiece's real less than.
      if (size() == other.size())
        return *static_cast<const base::StringPiece*>(this) < other;
      // Consider ourselves less if we are smaller
      return size() < other.size();
    }

    DomainEntryAttributes attributes;
  };

  // An entry in the set of domain specifications, describing the properties
  // that apply to that domain rule.
  typedef std::set<DomainEntry> DomainSet;

  // Parses a list of effective-TLD rules, building the domain_set_.  Rules are
  // assumed to be syntactically valid.  We operate on a StringPiece.  If we
  // were populated from an embedded resource, we will reference the embedded
  // resource directly.  If we were populated through UseDomainData, then our
  // StringPiece will reference our local copy in copied_domain_data_.
  void ParseDomainData(const base::StringPiece& data);

  // Returns the singleton instance, after attempting to initialize it.
  // NOTE that if the effective-TLD data resource can't be found, the instance
  // will be initialized and continue operation with simple default TLD data.
  static RegistryControlledDomainService* GetInstance();

  // Adds one rule, assumed to be valid, to the domain_set_.
  void AddRule(const base::StringPiece& rule_str);

  // Internal workings of the static public methods.  See above.
  static std::string GetDomainAndRegistryImpl(const std::string& host);
  size_t GetRegistryLengthImpl(const std::string& host,
                               bool allow_unknown_registries);

  // A set of our DomainEntry's.
  DomainSet domain_set_;

  // An optional copy of the full domain rule data.  If we're loaded from a
  // resource, then we just reference the resource directly without copying,
  // and copied_domain_data_ is not used.  If we are populated through
  // UseDomainData() then we copy that data here and reference it.
  std::string copied_domain_data_;

  // The actual domain data that we parse on startup.
  static const char kDomainData[];

  DISALLOW_COPY_AND_ASSIGN(RegistryControlledDomainService);
};

}  // namespace net

#endif  // NET_BASE_REGISTRY_CONTROLLED_DOMAIN_H_
