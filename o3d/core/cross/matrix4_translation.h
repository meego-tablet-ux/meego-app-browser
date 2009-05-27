/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
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
 *     * Neither the name of Google Inc. nor the names of its
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


// This file contains the declaration of class Matrix4Translation.

#ifndef O3D_CORE_CROSS_MATRIX4_TRANSLATION_H_
#define O3D_CORE_CROSS_MATRIX4_TRANSLATION_H_

#include "core/cross/param_object.h"
#include "core/cross/param.h"
#include "core/cross/param_cache.h"
#include "core/cross/types.h"

namespace o3d {

// Operation to build a translation matrix from a translation vector and (if
// bound) compose it with an input transformation matrix.
class Matrix4Translation : public ParamObject {
 public:
  typedef SmartPointer<Matrix4Translation> Ref;
  typedef WeakPointer<Matrix4Translation> WeakPointerType;

  static const char* kInputMatrixParamName;
  static const char* kTranslationParamName;
  static const char* kOutputMatrixParamName;

  Matrix4 input_matrix() const {
    return input_matrix_param_->value();
  }

  void set_input_matrix(const Matrix4& input_matrix) {
    input_matrix_param_->set_value(input_matrix);
  }

  Float3 translation() const {
    return translation_param_->value();
  }

  void set_translation(const Float3& axis) {
    translation_param_->set_value(axis);
  }

  Matrix4 output_matrix() const {
    return output_matrix_param_->value();
  }

  void UpdateOutputs();

 private:
  typedef SlaveParam<ParamMatrix4, Matrix4Translation> SlaveParamMatrix4;

  explicit Matrix4Translation(ServiceLocator* service_locator);

  friend class IClassManager;
  static ObjectBase::Ref Create(ServiceLocator* service_locator);

  ParamMatrix4::Ref input_matrix_param_;
  ParamFloat3::Ref translation_param_;
  SlaveParamMatrix4::Ref output_matrix_param_;

  O3D_DECL_CLASS(Matrix4Translation, ParamObject);
  DISALLOW_COPY_AND_ASSIGN(Matrix4Translation);
};
}  // namespace o3d

#endif  // O3D_CORE_CROSS_MATRIX4_TRANSLATION_H_
