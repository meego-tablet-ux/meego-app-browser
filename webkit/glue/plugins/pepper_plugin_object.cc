// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/plugins/pepper_plugin_object.h"

#include "base/logging.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "third_party/npapi/bindings/npapi.h"
#include "third_party/npapi/bindings/npruntime.h"
#include "ppapi/c/dev/ppb_var_deprecated.h"
#include "ppapi/c/dev/ppp_class_deprecated.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/ppb_class.h"
#include "third_party/WebKit/WebKit/chromium/public/WebBindings.h"
#include "webkit/glue/plugins/pepper_class.h"
#include "webkit/glue/plugins/pepper_plugin_module.h"
#include "webkit/glue/plugins/pepper_resource.h"
#include "webkit/glue/plugins/pepper_string.h"
#include "webkit/glue/plugins/pepper_var.h"

using WebKit::WebBindings;

namespace pepper {

namespace {

const char kInvalidValueException[] = "Error: Invalid value";
const char kInvalidPluginValue[] = "Error: Plugin returned invalid value.";

// -----------------------------------------------------------------------------
// Utilities

// Converts the given PP_Var to an NPVariant, returning true on success.
// False means that the given variant is invalid. In this case, the result
// NPVariant will be set to a void one.
//
// The contents of the PP_Var will be copied unless the PP_Var corresponds to
// an object.
bool PPVarToNPVariant(PP_Var var, NPVariant* result) {
  switch (var.type) {
    case PP_VARTYPE_UNDEFINED:
      VOID_TO_NPVARIANT(*result);
      break;
    case PP_VARTYPE_NULL:
      NULL_TO_NPVARIANT(*result);
      break;
    case PP_VARTYPE_BOOL:
      BOOLEAN_TO_NPVARIANT(var.value.as_bool, *result);
      break;
    case PP_VARTYPE_INT32:
      INT32_TO_NPVARIANT(var.value.as_int, *result);
      break;
    case PP_VARTYPE_DOUBLE:
      DOUBLE_TO_NPVARIANT(var.value.as_double, *result);
      break;
    case PP_VARTYPE_STRING: {
      scoped_refptr<StringVar> string(StringVar::FromPPVar(var));
      if (!string) {
        VOID_TO_NPVARIANT(*result);
        return false;
      }
      const std::string& value = string->value();
      STRINGN_TO_NPVARIANT(base::strdup(value.c_str()), value.size(), *result);
      break;
    }
    case PP_VARTYPE_OBJECT: {
      scoped_refptr<ObjectVar> object(ObjectVar::FromPPVar(var));
      if (!object) {
        VOID_TO_NPVARIANT(*result);
        return false;
      }
      OBJECT_TO_NPVARIANT(WebBindings::retainObject(object->np_object()),
                          *result);
      break;
    }
  }
  return true;
}

// PPVarArrayFromNPVariantArray ------------------------------------------------

// Converts an array of NPVariants to an array of PP_Var, and scopes the
// ownership of the PP_Var. This is used when converting argument lists from
// WebKit to the plugin.
class PPVarArrayFromNPVariantArray {
 public:
  PPVarArrayFromNPVariantArray(PluginModule* module,
                               size_t size,
                               const NPVariant* variants)
        : size_(size) {
    if (size_ > 0) {
      array_.reset(new PP_Var[size_]);
      for (size_t i = 0; i < size_; i++)
        array_[i] = Var::NPVariantToPPVar(module, &variants[i]);
    }
  }

  ~PPVarArrayFromNPVariantArray() {
    for (size_t i = 0; i < size_; i++)
      Var::PluginReleasePPVar(array_[i]);
  }

  PP_Var* array() { return array_.get(); }

 private:
  size_t size_;
  scoped_array<PP_Var> array_;

  DISALLOW_COPY_AND_ASSIGN(PPVarArrayFromNPVariantArray);
};

// PPVarFromNPObject -----------------------------------------------------------

// Converts an NPObject tp PP_Var, and scopes the ownership of the PP_Var. This
// is used when converting 'this' pointer from WebKit to the plugin.
class PPVarFromNPObject {
 public:
  PPVarFromNPObject(PluginModule* module, NPObject* object)
      : var_(ObjectVar::NPObjectToPPVar(module, object)) {
  }
  ~PPVarFromNPObject() {
    Var::PluginReleasePPVar(var_);
  }
  PP_Var var() const { return var_; }
 private:
  const PP_Var var_;

  DISALLOW_COPY_AND_ASSIGN(PPVarFromNPObject);
};

// PPResultAndExceptionToNPResult ----------------------------------------------

// Convenience object for converting a PPAPI call that can throw an exception
// and optionally return a value, back to the NPAPI layer which expects a
// NPVariant as a result.
//
// Normal usage is that you will pass the result of exception() to the
// PPAPI function as the exception output parameter. Then you will either
// call SetResult with the result of the PPAPI call, or
// CheckExceptionForNoResult if the PPAPI call doesn't return a PP_Var.
//
// Both SetResult and CheckExceptionForNoResult will throw an exception to
// the JavaScript library if the plugin reported an exception. SetResult
// will additionally convert the result to an NPVariant and write it to the
// output parameter given in the constructor.
class PPResultAndExceptionToNPResult {
 public:
  // The object_var parameter is the object to associate any exception with.
  // It may not be NULL.
  //
  // The np_result parameter is the NPAPI result output parameter. This may be
  // NULL if there is no NPVariant result (like for HasProperty). If this is
  // specified, you must call SetResult() to set it. If it is not, you must
  // call CheckExceptionForNoResult to do the exception checking with no result
  // conversion.
  PPResultAndExceptionToNPResult(NPObject* object_var,
                                 NPVariant* np_result)
      : object_var_(object_var),
        np_result_(np_result),
        exception_(PP_MakeUndefined()),
        success_(false),
        checked_exception_(false) {
  }

  ~PPResultAndExceptionToNPResult() {
    // The user should have called SetResult or CheckExceptionForNoResult
    // before letting this class go out of scope, or the exception will have
    // been lost.
    DCHECK(checked_exception_);

    ObjectVar::PluginReleasePPVar(exception_);
  }

  // Returns true if an exception has been set.
  bool has_exception() const { return exception_.type != PP_VARTYPE_UNDEFINED; }

  // Returns a pointer to the exception. You would pass this to the PPAPI
  // function as the exception parameter. If it is set to non-void, this object
  // will take ownership of destroying it.
  PP_Var* exception() { return &exception_; }

  // Returns true if everything succeeded with no exception. This is valid only
  // after calling SetResult/CheckExceptionForNoResult.
  bool success() const {
    DCHECK(checked_exception_);
    return success_;
  }

  // Call this with the return value of the PPAPI function. It will convert
  // the result to the NPVariant output parameter and pass any exception on to
  // the JS engine. It will update the success flag and return it.
  bool SetResult(PP_Var result) {
    DCHECK(!checked_exception_);  // Don't call more than once.
    DCHECK(np_result_);  // Should be expecting a result.

    checked_exception_ = true;

    if (has_exception()) {
      ThrowException();
      success_ = false;
    } else if (!PPVarToNPVariant(result, np_result_)) {
      WebBindings::setException(object_var_, kInvalidPluginValue);
      success_ = false;
    } else {
      success_ = true;
    }

    // No matter what happened, we need to release the reference to the
    // value passed in. On success, a reference to this value will be in
    // the np_result_.
    Var::PluginReleasePPVar(result);
    return success_;
  }

  // Call this after calling a PPAPI function that could have set the
  // exception. It will pass the exception on to the JS engine and update
  // the success flag.
  //
  // The success flag will be returned.
  bool CheckExceptionForNoResult() {
    DCHECK(!checked_exception_);  // Don't call more than once.
    DCHECK(!np_result_);  // Can't have a result when doing this.

    checked_exception_ = true;

    if (has_exception()) {
      ThrowException();
      success_ = false;
      return false;
    }
    success_ = true;
    return true;
  }

  // Call this to ignore any exception. This prevents the DCHECK from failing
  // in the destructor.
  void IgnoreException() {
    checked_exception_ = true;
  }

 private:
  // Throws the current exception to JS. The exception must be set.
  void ThrowException() {
    scoped_refptr<StringVar> string(StringVar::FromPPVar(exception_));
    if (string) {
      WebBindings::setException(object_var_, string->value().c_str());
    }
  }

  NPObject* object_var_;  // Non-owning ref (see constructor).
  NPVariant* np_result_;  // Output value, possibly NULL (see constructor).
  PP_Var exception_;  // Exception set by the PPAPI call. We own a ref to it.
  bool success_;  // See the success() function above.
  bool checked_exception_;  // SetResult/CheckExceptionForNoResult was called.

  DISALLOW_COPY_AND_ASSIGN(PPResultAndExceptionToNPResult);
};

// NPObjectAccessorWithIdentifier ----------------------------------------------

// Helper class for our NPObject wrapper. This converts a call from WebKit
// where it gives us an NPObject and an NPIdentifier to an easily-accessible
// ObjectVar (corresponding to the NPObject) and PP_Var (corresponding to the
// NPIdentifier).
//
// If the NPObject or identifier is invalid, we'll set is_valid() to false.
// The caller should check is_valid() before doing anything with the class.
//
// JS can't have integer functions, so when dealing with these, we don't want
// to allow integer identifiers. The calling code can decode if it wants to
// allow integer identifiers (like for property access) or prohibit them
// (like for method calling) by setting |allow_integer_identifier|. If this
// is false and the identifier is an integer, we'll set is_valid() to false.
//
// Getting an integer identifier in this case should be impossible. V8
// shouldn't be allowing this, and the Pepper Var calls from the plugin are
// supposed to error out before calling into V8 (which will then call us back).
// Aside from an egregious error, the only time this could happen is an NPAPI
// plugin calling us.
class NPObjectAccessorWithIdentifier {
 public:
  NPObjectAccessorWithIdentifier(NPObject* object,
                                 NPIdentifier identifier,
                                 bool allow_integer_identifier)
      : object_(PluginObject::FromNPObject(object)),
        identifier_(PP_MakeUndefined()) {
    if (object_) {
      identifier_ = Var::NPIdentifierToPPVar(object_->module(), identifier);
      if (identifier_.type == PP_VARTYPE_INT32 && !allow_integer_identifier)
        identifier_.type = PP_VARTYPE_UNDEFINED;  // Mark it invalid.
    }
  }

  ~NPObjectAccessorWithIdentifier() {
    Var::PluginReleasePPVar(identifier_);
  }

  // Returns true if both the object and identifier are valid.
  bool is_valid() const {
    return object_ && identifier_.type != PP_VARTYPE_UNDEFINED;
  }

  PluginObject* object() { return object_; }
  PP_Var identifier() const { return identifier_; }

 private:
  PluginObject* object_;
  PP_Var identifier_;

  DISALLOW_COPY_AND_ASSIGN(NPObjectAccessorWithIdentifier);
};

// NPObject implementation in terms of PPP_Class_Deprecated --------------------

NPObject* WrapperClass_Allocate(NPP npp, NPClass* unused) {
  return PluginObject::AllocateObjectWrapper();
}

void WrapperClass_Deallocate(NPObject* np_object) {
  PluginObject* plugin_object = PluginObject::FromNPObject(np_object);
  if (!plugin_object)
    return;
  plugin_object->ppp_class()->Deallocate(plugin_object->ppp_class_data());
  delete plugin_object;
}

void WrapperClass_Invalidate(NPObject* object) {
}

bool WrapperClass_HasMethod(NPObject* object, NPIdentifier method_name) {
  NPObjectAccessorWithIdentifier accessor(object, method_name, false);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), NULL);
  bool rv = accessor.object()->ppp_class()->HasMethod(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      result_converter.exception());
  result_converter.CheckExceptionForNoResult();
  return rv;
}

bool WrapperClass_Invoke(NPObject* object, NPIdentifier method_name,
                         const NPVariant* argv, uint32_t argc,
                         NPVariant* result) {
  NPObjectAccessorWithIdentifier accessor(object, method_name, false);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), result);
  PPVarArrayFromNPVariantArray args(accessor.object()->module(), argc, argv);

  return result_converter.SetResult(accessor.object()->ppp_class()->Call(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      argc, args.array(), result_converter.exception()));
}

bool WrapperClass_InvokeDefault(NPObject* np_object, const NPVariant* argv,
                                uint32_t argc, NPVariant* result) {
  PluginObject* obj = PluginObject::FromNPObject(np_object);
  if (!obj)
    return false;

  PPVarArrayFromNPVariantArray args(obj->module(), argc, argv);
  PPResultAndExceptionToNPResult result_converter(obj->GetNPObject(), result);

  result_converter.SetResult(obj->ppp_class()->Call(
      obj->ppp_class_data(), PP_MakeUndefined(), argc, args.array(),
      result_converter.exception()));
  return result_converter.success();
}

bool WrapperClass_HasProperty(NPObject* object, NPIdentifier property_name) {
  NPObjectAccessorWithIdentifier accessor(object, property_name, true);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), NULL);
  bool rv = accessor.object()->ppp_class()->HasProperty(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      result_converter.exception());
  result_converter.CheckExceptionForNoResult();
  return rv;
}

bool WrapperClass_GetProperty(NPObject* object, NPIdentifier property_name,
                              NPVariant* result) {
  NPObjectAccessorWithIdentifier accessor(object, property_name, true);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), result);
  return result_converter.SetResult(accessor.object()->ppp_class()->GetProperty(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      result_converter.exception()));
}

bool WrapperClass_SetProperty(NPObject* object, NPIdentifier property_name,
                              const NPVariant* value) {
  NPObjectAccessorWithIdentifier accessor(object, property_name, true);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), NULL);
  PP_Var value_var = Var::NPVariantToPPVar(accessor.object()->module(), value);
  accessor.object()->ppp_class()->SetProperty(
      accessor.object()->ppp_class_data(), accessor.identifier(), value_var,
      result_converter.exception());
  Var::PluginReleasePPVar(value_var);
  return result_converter.CheckExceptionForNoResult();
}

bool WrapperClass_RemoveProperty(NPObject* object, NPIdentifier property_name) {
  NPObjectAccessorWithIdentifier accessor(object, property_name, true);
  if (!accessor.is_valid())
    return false;

  PPResultAndExceptionToNPResult result_converter(
      accessor.object()->GetNPObject(), NULL);
  accessor.object()->ppp_class()->RemoveProperty(
      accessor.object()->ppp_class_data(), accessor.identifier(),
      result_converter.exception());
  return result_converter.CheckExceptionForNoResult();
}

bool WrapperClass_Enumerate(NPObject* object, NPIdentifier** values,
                            uint32_t* count) {
  *values = NULL;
  *count = 0;
  PluginObject* obj = PluginObject::FromNPObject(object);
  if (!obj)
    return false;

  uint32_t property_count = 0;
  PP_Var* properties = NULL;  // Must be freed!
  PPResultAndExceptionToNPResult result_converter(obj->GetNPObject(), NULL);
  obj->ppp_class()->GetAllPropertyNames(obj->ppp_class_data(),
                                        &property_count, &properties,
                                        result_converter.exception());

  // Convert the array of PP_Var to an array of NPIdentifiers. If any
  // conversions fail, we will set the exception.
  if (!result_converter.has_exception()) {
    if (property_count > 0) {
      *values = static_cast<NPIdentifier*>(
          malloc(sizeof(NPIdentifier) * property_count));
      *count = 0;  // Will be the number of items successfully converted.
      for (uint32_t i = 0; i < property_count; ++i) {
        if (!((*values)[i] = Var::PPVarToNPIdentifier(properties[i]))) {
          // Throw an exception for the failed convertion.
          *result_converter.exception() = StringVar::StringToPPVar(
              obj->module(), kInvalidValueException);
          break;
        }
        (*count)++;
      }

      if (result_converter.has_exception()) {
        // We don't actually have to free the identifiers we converted since
        // all identifiers leak anyway :( .
        free(*values);
        *values = NULL;
        *count = 0;
      }
    }
  }

  // This will actually throw the exception, either from GetAllPropertyNames,
  // or if anything was set during the conversion process.
  result_converter.CheckExceptionForNoResult();

  // Release the PP_Var that the plugin allocated. On success, they will all
  // be converted to NPVariants, and on failure, we want them to just go away.
  for (uint32_t i = 0; i < property_count; ++i)
    Var::PluginReleasePPVar(properties[i]);
  free(properties);
  return result_converter.success();
}

bool WrapperClass_Construct(NPObject* object, const NPVariant* argv,
                            uint32_t argc, NPVariant* result) {
  PluginObject* obj = PluginObject::FromNPObject(object);
  if (!obj)
    return false;

  PPVarArrayFromNPVariantArray args(obj->module(), argc, argv);
  PPResultAndExceptionToNPResult result_converter(obj->GetNPObject(), result);
  return result_converter.SetResult(obj->ppp_class()->Construct(
      obj->ppp_class_data(), argc, args.array(),
      result_converter.exception()));
}

const NPClass wrapper_class = {
  NP_CLASS_STRUCT_VERSION,
  WrapperClass_Allocate,
  WrapperClass_Deallocate,
  WrapperClass_Invalidate,
  WrapperClass_HasMethod,
  WrapperClass_Invoke,
  WrapperClass_InvokeDefault,
  WrapperClass_HasProperty,
  WrapperClass_GetProperty,
  WrapperClass_SetProperty,
  WrapperClass_RemoveProperty,
  WrapperClass_Enumerate,
  WrapperClass_Construct
};

}  // namespace

// PluginObject ----------------------------------------------------------------

struct PluginObject::NPObjectWrapper : public NPObject {
  // Points to the var object that owns this wrapper. This value may be NULL
  // if there is no var owning this wrapper. This can happen if the plugin
  // releases all references to the var, but a reference to the underlying
  // NPObject is still held by script on the page.
  PluginObject* obj;
};

PluginObject::PluginObject(PluginModule* module,
                           NPObjectWrapper* object_wrapper,
                           const PPP_Class_Deprecated* ppp_class,
                           void* ppp_class_data)
    : module_(module),
      object_wrapper_(object_wrapper),
      ppp_class_(ppp_class),
      ppp_class_data_(ppp_class_data) {
  // Make the object wrapper refer back to this class so our NPObject
  // implementation can call back into the Pepper layer.
  object_wrapper_->obj = this;
  module_->AddPluginObject(this);
}

PluginObject::~PluginObject() {
  // The wrapper we made for this NPObject may still have a reference to it
  // from JavaScript, so we clear out its ObjectVar back pointer which will
  // cause all calls "up" to the plugin to become NOPs. Our ObjectVar base
  // class will release our reference to the object, which may or may not
  // delete the NPObject.
  DCHECK(object_wrapper_->obj == this);
  object_wrapper_->obj = NULL;
  module_->RemovePluginObject(this);
}

PP_Var PluginObject::Create(PluginModule* module,
                            const PPP_Class_Deprecated* ppp_class,
                            void* ppp_class_data) {
  // This will internally end up calling our AllocateObjectWrapper via the
  // WrapperClass_Allocated function which will have created an object wrapper
  // appropriate for this class (derived from NPObject).
  NPObjectWrapper* wrapper = static_cast<NPObjectWrapper*>(
      WebBindings::createObject(NULL, const_cast<NPClass*>(&wrapper_class)));

  // This object will register itself both with the NPObject and with the
  // PluginModule. The NPObject will normally handle its lifetime, and it
  // will get deleted in the destroy method. It may also get deleted when the
  // plugin module is deallocated.
  new PluginObject(module, wrapper, ppp_class, ppp_class_data);

  // We can just use a normal ObjectVar to refer to this object from the
  // plugin. It will hold a ref to the underlying NPObject which will in turn
  // hold our pluginObject.
  return ObjectVar::NPObjectToPPVar(module, wrapper);
}

NPObject* PluginObject::GetNPObject() const {
  return object_wrapper_;
}

// static
bool PluginObject::IsInstanceOf(NPObject* np_object,
                                const PPP_Class_Deprecated* ppp_class,
                                void** ppp_class_data) {
  // Validate that this object is implemented by our wrapper class before
  // trying to get the PluginObject.
  if (np_object->_class != &wrapper_class)
    return false;

  PluginObject* plugin_object = FromNPObject(np_object);
  if (!plugin_object)
    return false;  // Object is no longer alive.

  if (plugin_object->ppp_class() != ppp_class)
    return false;
  if (ppp_class_data)
    *ppp_class_data = plugin_object->ppp_class_data();
  return true;
}

// static
PluginObject* PluginObject::FromNPObject(NPObject* object) {
  return static_cast<NPObjectWrapper*>(object)->obj;
}

// static
NPObject* PluginObject::AllocateObjectWrapper() {
  NPObjectWrapper* wrapper = new NPObjectWrapper;
  memset(wrapper, 0, sizeof(NPObjectWrapper));
  return wrapper;
}

// VarObjectClass::InstanceData  -----------------------------------------------

struct VarObjectClass::InstanceData : public NPObject {
  InstanceData() : native_data(NULL) {}

  scoped_refptr<VarObjectClass> object_class;
  void* native_data;
};

// VarObjectClass::Property  ---------------------------------------------------

VarObjectClass::Property::Property(const PP_ClassProperty& prop)
    : method(prop.method),
      getter(prop.getter),
      setter(prop.setter),
      writable(!(prop.modifiers & PP_OBJECTPROPERTY_MODIFIER_READONLY)),
      enumerable(!(prop.modifiers & PP_OBJECTPROPERTY_MODIFIER_DONTENUM)) {
}


// VarObjectAccessorWithIdentifier ---------------------------------------------

// Helper class for the new (PPB_Class) NPObject wrapper. This converts a call
// from WebKit where it gives us an NPObject and an NPIdentifier to an
// easily-accessible InstanceData (corresponding to the NPObject) and
// std::string and Property (corresponding to the NPIdentifier).
class VarObjectAccessorWithIdentifier {
 public:
  VarObjectAccessorWithIdentifier(NPObject* object, NPIdentifier identifier)
      : exists_(false),
        instance_(static_cast<VarObjectClass::InstanceData*>(object)),
        property_(NULL) {
    if (instance_) {
      const NPUTF8* string_value = NULL;
      int32_t int_value = 0;
      bool is_string = false;
      WebBindings::extractIdentifierData(identifier, string_value, int_value,
                                         is_string);
      if (is_string) {
        property_name_ = string_value;

        const VarObjectClass::PropertyMap& properties =
            instance_->object_class->properties();
        VarObjectClass::PropertyMap::const_iterator it =
            properties.find(property_name_);
        if (it != properties.end()) {
          property_ = &it->second;
          exists_ = true;
        }
      }
    }
  }

  // Return true if the object is valid, the identifier is valid, and the
  // property with said name exists.
  bool exists() const { return exists_; }
  bool is_method() const { return exists() && property_->method; }
  bool is_readable() const { return exists() && property_->getter; }
  bool is_writable() const {
    return exists() && property_->setter && property_->writable;
  }
  const VarObjectClass::InstanceData* instance() const { return instance_; }
  const VarObjectClass::Property* property() const { return property_; }
  PluginModule* module() const {
    return instance_ ? instance_->object_class->module() : NULL;
  }

 private:
  bool exists_;
  const VarObjectClass::InstanceData* instance_;
  std::string property_name_;
  const VarObjectClass::Property* property_;

  DISALLOW_COPY_AND_ASSIGN(VarObjectAccessorWithIdentifier);
};

// NPObject implementation in terms of PPB_Class -------------------------------

namespace {

NPObject* VarObjectClassAllocate(NPP npp, NPClass* the_class) {
  return new VarObjectClass::InstanceData;
}

void VarObjectClassDeallocate(NPObject* object) {
  VarObjectClass::InstanceData* instance =
      static_cast<VarObjectClass::InstanceData*>(object);
  if (instance->object_class->instance_native_destructor())
    instance->object_class->instance_native_destructor()(instance->native_data);
  delete instance;
}

bool VarObjectClassHasMethod(NPObject* np_obj, NPIdentifier name) {
  VarObjectAccessorWithIdentifier accessor(np_obj, name);
  return accessor.is_method();
}

bool VarObjectClassInvoke(NPObject* np_obj, NPIdentifier name,
                          const NPVariant* args, uint32 arg_count,
                          NPVariant* result) {
  VarObjectAccessorWithIdentifier accessor(np_obj, name);
  if (!accessor.is_method())
    return false;

  PPResultAndExceptionToNPResult result_converter(np_obj, result);
  PPVarArrayFromNPVariantArray arguments(accessor.module(), arg_count, args);
  PPVarFromNPObject self(accessor.module(), np_obj);

  return result_converter.SetResult(accessor.property()->method(
    accessor.instance()->native_data, self.var(), arguments.array(), arg_count,
    result_converter.exception()));
}

bool VarObjectClassInvokeDefault(NPObject* np_obj,
                                 const NPVariant* args,
                                 uint32 arg_count,
                                 NPVariant* result) {
  VarObjectClass::InstanceData* instance =
      static_cast<VarObjectClass::InstanceData*>(np_obj);
  if (!instance || !instance->object_class->instance_invoke())
    return false;

  PPResultAndExceptionToNPResult result_converter(np_obj, result);
  PPVarArrayFromNPVariantArray arguments(instance->object_class->module(),
                                         arg_count, args);
  PPVarFromNPObject self(instance->object_class->module(), np_obj);

  return result_converter.SetResult(instance->object_class->instance_invoke()(
      instance->native_data, self.var(), arguments.array(), arg_count,
      result_converter.exception()));
}

bool VarObjectClassHasProperty(NPObject* np_obj, NPIdentifier name) {
  VarObjectAccessorWithIdentifier accessor(np_obj, name);
  return accessor.is_readable();
}

bool VarObjectClassGetProperty(NPObject* np_obj, NPIdentifier name,
                               NPVariant* result) {
  VarObjectAccessorWithIdentifier accessor(np_obj, name);
  if (!accessor.is_readable()) {
    return false;
  }

  PPResultAndExceptionToNPResult result_converter(np_obj, result);
  PPVarFromNPObject self(accessor.module(), np_obj);

  return result_converter.SetResult(accessor.property()->getter(
    accessor.instance()->native_data, self.var(), 0, 0,
    result_converter.exception()));
}

bool VarObjectClassSetProperty(NPObject* np_obj, NPIdentifier name,
                     const NPVariant* variant) {
  VarObjectAccessorWithIdentifier accessor(np_obj, name);
  if (!accessor.is_writable()) {
    return false;
  }

  PPResultAndExceptionToNPResult result_converter(np_obj, NULL);
  PPVarArrayFromNPVariantArray arguments(accessor.module(), 1, variant);
  PPVarFromNPObject self(accessor.module(), np_obj);

  // Ignore return value.
  Var::PluginReleasePPVar(accessor.property()->setter(
      accessor.instance()->native_data, self.var(), arguments.array(), 1,
      result_converter.exception()));

  return result_converter.CheckExceptionForNoResult();
}

bool VarObjectClassEnumerate(NPObject *np_obj, NPIdentifier **value,
                             uint32_t *count) {
  VarObjectClass::InstanceData* instance =
      static_cast<VarObjectClass::InstanceData*>(np_obj);
  *count = 0;
  *value = NULL;
  if (!instance)
    return false;

  const VarObjectClass::PropertyMap& properties =
      instance->object_class->properties();

  // Don't bother calculating the size of enumerable properties, just allocate
  // enough for all and then fill it partially.
  *value = static_cast<NPIdentifier*>(
      malloc(sizeof(NPIdentifier) * properties.size()));

  NPIdentifier* inserter = *value;
  for (VarObjectClass::PropertyMap::const_iterator i = properties.begin();
      i != properties.end(); ++i)
    if (i->second.enumerable)
      *inserter++ = WebBindings::getStringIdentifier(i->first.c_str());

  *count = inserter - *value;
  return true;
}

NPClass objectclassvar_class = {
  NP_CLASS_STRUCT_VERSION,
  &VarObjectClassAllocate,
  &VarObjectClassDeallocate,
  NULL,
  &VarObjectClassHasMethod,
  &VarObjectClassInvoke,
  &VarObjectClassInvokeDefault,
  &VarObjectClassHasProperty,
  &VarObjectClassGetProperty,
  &VarObjectClassSetProperty,
  NULL,
  &VarObjectClassEnumerate,
};

// PPB_Class interface ---------------------------------------------------------

PP_Resource Create(PP_Module module, PP_ClassDestructor destruct,
                   PP_ClassFunction invoke, PP_ClassProperty* properties) {
  PluginModule* plugin_module = ResourceTracker::Get()->GetModule(module);
  if (!properties || !plugin_module)
    return 0;
  scoped_refptr<VarObjectClass> cls = new VarObjectClass(plugin_module,
                                                         destruct,
                                                         invoke,
                                                         properties);
  if (!cls)
    return 0;
  return cls->GetReference();
}

PP_Var Instantiate(PP_Resource class_object, void* native_data,
                   PP_Var* exception) {
  scoped_refptr<VarObjectClass> object_class =
      Resource::GetAs<VarObjectClass>(class_object);
  if (!object_class)
    return PP_MakeUndefined();
  NPObject* obj = WebBindings::createObject(NULL, &objectclassvar_class);
  VarObjectClass::InstanceData* instance_data =
      static_cast<VarObjectClass::InstanceData*>(obj);
  instance_data->object_class = object_class;
  instance_data->native_data = native_data;
  return ObjectVar::NPObjectToPPVar(object_class->module(), obj);
}

}  // namespace

// VarObjectClass --------------------------------------------------------------

VarObjectClass::VarObjectClass(PluginModule* module,
                               PP_ClassDestructor destruct,
                               PP_ClassFunction invoke,
                               PP_ClassProperty* properties)
    : Resource(module),
      instance_native_destructor_(destruct),
      instance_invoke_(invoke) {
  PP_ClassProperty* prop = properties;
  while (prop->name) {
    properties_.insert(std::make_pair(std::string(prop->name),
                                      Property(*prop)));
    ++prop;
  }
}

// static
const PPB_Class* VarObjectClass::GetInterface() {
  static PPB_Class interface = {
    &Create,
    &Instantiate
  };
  return &interface;
}

// virtual
VarObjectClass::~VarObjectClass() { }

VarObjectClass* VarObjectClass::AsVarObjectClass() {
  return this;
}

}  // namespace pepper
