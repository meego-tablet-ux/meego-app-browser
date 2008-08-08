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

#ifndef V8_NPOBJECT_H__
#define V8_NPOBJECT_H__

#include <v8.h>
#include "third_party/npapi/bindings/npruntime.h"

// These functions can be replaced by normal JS operation.
// Getters
v8::Handle<v8::Value> NPObjectNamedPropertyGetter(v8::Local<v8::String> name,
                                                  const v8::AccessorInfo& info);
v8::Handle<v8::Value> NPObjectIndexedPropertyGetter(uint32_t index,
                                                    const v8::AccessorInfo& info);
v8::Handle<v8::Value> NPObjectGetNamedProperty(v8::Local<v8::Object> self,
                                               v8::Local<v8::String> name);
v8::Handle<v8::Value> NPObjectGetIndexedProperty(v8::Local<v8::Object> self,
                                                 uint32_t index);

// Setters
v8::Handle<v8::Value> NPObjectNamedPropertySetter(v8::Local<v8::String> name,
                                                  v8::Local<v8::Value> value,
                                                  const v8::AccessorInfo& info);
v8::Handle<v8::Value> NPObjectIndexedPropertySetter(uint32_t index,
                                                    const v8::AccessorInfo& info);
v8::Handle<v8::Value> NPObjectSetNamedProperty(v8::Local<v8::Object> self,
                                               v8::Local<v8::String> name,
                                               v8::Local<v8::Value> value);
v8::Handle<v8::Value> NPObjectSetIndexedProperty(v8::Local<v8::Object> self,
                                                 uint32_t index,
                                                 v8::Local<v8::Value> value);

v8::Handle<v8::Value> NPObjectInvokeDefaultHandler(const v8::Arguments& args);

// Get a wrapper for a NPObject.
// If the object is already wrapped, the pre-existing wrapper
// will be returned.
// If the object is not wrapped, wrap it, and give V8 a weak
// reference to the wrapper which will cleanup when there are
// no more JS references to the object.
v8::Local<v8::Object> CreateV8ObjectForNPObject(NPObject* object,
                                                NPObject *root);

// Tell V8 to forcibly remove an object.
// This is used at plugin teardown so that the caller can
// aggressively unload the plugin library.  After calling this
// function, the persistent handle to the wrapper will be
// gone, and the wrapped NPObject will be removed so that
// it cannot be referred to.
void ForgetV8ObjectForNPObject(NPObject*object);

#endif  // V8_NPOBJECT_H__
