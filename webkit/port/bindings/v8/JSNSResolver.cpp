// Copyright (c) 2008, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
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
#include "JSNSResolver.h"

#include "v8_proxy.h"
#include "v8_binding.h"
#include "ExceptionContext.h"
#include "PlatformString.h"

namespace WebCore {

JSNSResolver::JSNSResolver(v8::Handle<v8::Object> resolver)
    : m_resolver(resolver)
{
}

JSNSResolver::~JSNSResolver()
{
}

String JSNSResolver::lookupNamespaceURI(ExceptionContext* exceptionContext,
                                        const String& prefix)
{
    v8::Handle<v8::Function> lookupNamespaceURIFunc;
    v8::Handle<v8::String> lookupNamespaceURIName = 
        v8::String::New("lookupNamespaceURI");

    // Check if the resolver has a function property named lookupNamespaceURI.
    if (m_resolver->Has(lookupNamespaceURIName)) {
        // In case the property is a getter that throws an error,
        // see LayoutTests/fast/dom/SelectorAPI/NSResolver-exceptions.xhtml
        ExceptionCatcher exceptionCatcher(exceptionContext);
        v8::Handle<v8::Value> lookupNamespaceURI = m_resolver->Get(
            lookupNamespaceURIName);
        if (exceptionContext->hadException())
            return String();
        if (lookupNamespaceURI->IsFunction()) {
            lookupNamespaceURIFunc = v8::Handle<v8::Function>::Cast(
                lookupNamespaceURI);
        }
    }

    if (lookupNamespaceURIFunc.IsEmpty() && !m_resolver->IsFunction()) {
        Frame* frame = ScriptController::retrieveActiveFrame();
        log_info(frame, "NSResolver does not have a lookupNamespaceURI method.",
                 String());
        return String();
    }

    // Catch exceptions from calling the namespace resolver.
    ExceptionCatcher exceptionCatcher(exceptionContext);

    const int argc = 1;
    v8::Handle<v8::Value> argv[argc] = { v8String(prefix) };
    v8::Handle<v8::Function> function = lookupNamespaceURIFunc.IsEmpty()
        ? v8::Handle<v8::Function>::Cast(m_resolver)
        : lookupNamespaceURIFunc;

    V8Proxy* proxy = V8Proxy::retrieve();
    v8::Handle<v8::Value> retval = proxy->CallFunction(function, m_resolver,
                                                       argc, argv);

    // Eat exceptions from namespace resolver and return an empty string. This
    // will cause NAMESPACE_ERR.
    if (exceptionContext->hadException())
        return String();

    return valueToStringWithNullOrUndefinedCheck(retval);
}

}
