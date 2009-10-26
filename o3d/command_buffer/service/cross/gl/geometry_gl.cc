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


// This file contains the implementation of the VertexBufferGL, IndexBufferGL
// and VertexStructGL classes, as well as the geometry-related GAPI functions.

#include "command_buffer/service/cross/gl/gapi_gl.h"
#include "command_buffer/service/cross/gl/geometry_gl.h"

namespace o3d {
namespace command_buffer {
namespace o3d {

VertexBufferGL::~VertexBufferGL() {
  glDeleteBuffers(1, &gl_buffer_);
  CHECK_GL_ERROR();
}

// Creates the GL buffer object.
void VertexBufferGL::Create() {
  glGenBuffers(1, &gl_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, gl_buffer_);
  GLenum usage =
      (flags() & vertex_buffer::kDynamic) ?  GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
  glBufferData(GL_ARRAY_BUFFER, size(), NULL, usage);
  CHECK_GL_ERROR();
}

// Sets the data into the GL buffer object.
bool VertexBufferGL::SetData(unsigned int offset,
                             unsigned int size,
                             const void *data) {
  if (!gl_buffer_) {
    LOG(ERROR) << "Calling SetData on a non-initialized VertexBufferGL.";
    return false;
  }
  if ((offset >= this->size()) || (offset + size > this->size())) {
    LOG(ERROR) << "Invalid size or offset on VertexBufferGL::SetData.";
    return false;
  }
  glBindBuffer(GL_ARRAY_BUFFER, gl_buffer_);
  glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
  CHECK_GL_ERROR();
  return true;
}

// Gets the data from the GL buffer object.
bool VertexBufferGL::GetData(unsigned int offset,
                             unsigned int size,
                             void *data) {
  if (!gl_buffer_) {
    LOG(ERROR) << "Calling GetData on a non-initialized VertexBufferGL.";
    return false;
  }
  if ((offset >= this->size()) || (offset + size > this->size())) {
    LOG(ERROR) << "Invalid size or offset on VertexBufferGL::GetData.";
    return false;
  }
  glBindBuffer(GL_ARRAY_BUFFER, gl_buffer_);
  glGetBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
  CHECK_GL_ERROR();
  return true;
}

IndexBufferGL::~IndexBufferGL() {
  glDeleteBuffers(1, &gl_buffer_);
}

// Creates the GL buffer object.
void IndexBufferGL::Create() {
  glGenBuffers(1, &gl_buffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer_);
  GLenum usage =
      (flags() & vertex_buffer::kDynamic) ?  GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size(), NULL, usage);
  CHECK_GL_ERROR();
}

// Sets the data into the GL buffer object.
bool IndexBufferGL::SetData(unsigned int offset,
                            unsigned int size,
                            const void *data) {
  if (!gl_buffer_) {
    LOG(ERROR) << "Calling SetData on a non-initialized IndexBufferGL.";
    return false;
  }
  if ((offset >= this->size()) || (offset + size > this->size())) {
    LOG(ERROR) << "Invalid size or offset on IndexBufferGL::SetData.";
    return false;
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer_);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
  CHECK_GL_ERROR();
  return true;
}

// Gets the data from the GL buffer object.
bool IndexBufferGL::GetData(unsigned int offset,
                            unsigned int size,
                            void *data) {
  if (!gl_buffer_) {
    LOG(ERROR) << "Calling GetData on a non-initialized IndexBufferGL.";
    return false;
  }
  if ((offset >= this->size()) || (offset + size > this->size())) {
    LOG(ERROR) << "Invalid size or offset on IndexBufferGL::GetData.";
    return false;
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_buffer_);
  glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
  CHECK_GL_ERROR();
  return true;
}

// Sets the input element in the VertexStruct resource.
void VertexStructGL::SetInput(unsigned int input_index,
                              ResourceId vertex_buffer_id,
                              unsigned int offset,
                              unsigned int stride,
                              vertex_struct::Type type,
                              vertex_struct::Semantic semantic,
                              unsigned int semantic_index) {
  Element &element = GetElement(input_index);
  element.vertex_buffer = vertex_buffer_id;
  element.offset = offset;
  element.stride = stride;
  element.type = type;
  element.semantic = semantic;
  element.semantic_index = semantic_index;
  dirty_ = true;
}

namespace {

inline ptrdiff_t OffsetToPtrDiff(unsigned int offset) {
  return static_cast<ptrdiff_t>(offset);
}

inline void* OffsetToPtr(ptrdiff_t offset) {
  return reinterpret_cast<void*>(offset);
}

}  // anonymous namespace

unsigned int VertexStructGL::SetStreams(GAPIGL *gapi) {
  if (dirty_) Compile();
  unsigned int max_vertices = UINT_MAX;
  for (unsigned int i = 0; i < kMaxAttribs; ++i) {
    const AttribDesc &attrib = attribs_[i];
    if (attrib.vertex_buffer_id == kInvalidResource) {
      if (i == 0) {
        glDisableClientState(GL_VERTEX_ARRAY);
      } else {
        glDisableVertexAttribArray(i);
      }
    } else {
      glEnableVertexAttribArray(i);
      VertexBufferGL *vertex_buffer =
          gapi->GetVertexBuffer(attrib.vertex_buffer_id);
      if (!vertex_buffer) {
        glDisableVertexAttribArray(i);
        max_vertices = 0;
        continue;
      }
      DCHECK_NE(vertex_buffer->gl_buffer(), 0);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer->gl_buffer());
      glVertexAttribPointer(i, attrib.size, attrib.type, attrib.normalized,
                            attrib.stride,
                            OffsetToPtr(attrib.offset));
      max_vertices = std::min(max_vertices,
                              vertex_buffer->size() / attrib.stride);
    }
  }
  CHECK_GL_ERROR();
  return max_vertices;
}

namespace {

// from the ARB_vertex_program extension, at
// http://www.opengl.org/registry/specs/ARB/vertex_program.txt
//
//   Generic
//   Attribute   Conventional Attribute       Conventional Attribute Command
//   ---------   ------------------------     ------------------------------
//        0      vertex position              Vertex
//        1      vertex weights 0-3           WeightARB, VertexWeightEXT
//        2      normal                       Normal
//        3      primary color                Color
//        4      secondary color              SecondaryColorEXT
//        5      fog coordinate               FogCoordEXT
//        6      -                            -
//        7      -                            -
//        8      texture coordinate set 0     MultiTexCoord(TEXTURE0, ...)
//        9      texture coordinate set 1     MultiTexCoord(TEXTURE1, ...)
//       10      texture coordinate set 2     MultiTexCoord(TEXTURE2, ...)
//       11      texture coordinate set 3     MultiTexCoord(TEXTURE3, ...)
//       12      texture coordinate set 4     MultiTexCoord(TEXTURE4, ...)
//       13      texture coordinate set 5     MultiTexCoord(TEXTURE5, ...)
//       14      texture coordinate set 6     MultiTexCoord(TEXTURE6, ...)
//       15      texture coordinate set 7     MultiTexCoord(TEXTURE7, ...)
//      8+n      texture coordinate set n     MultiTexCoord(TEXTURE0+n, ...)
//
// Note: we only accept at most 8 texture coordinates for maximum compatibility
// with DirectX.

inline unsigned int GetAttribIndex(vertex_struct::Semantic semantic,
                                   unsigned int semantic_index) {
  switch (semantic) {
    case vertex_struct::kPosition:
      DCHECK_EQ(semantic_index, 0);
      return 0;
    case vertex_struct::kNormal:
      DCHECK_EQ(semantic_index, 0);
      return 2;
    case vertex_struct::kColor:
      DCHECK_LT(semantic_index, 2U);
      return 3 + semantic_index;
    case vertex_struct::kTexCoord:
      DCHECK_LT(semantic_index, 8U);
      return 8 + semantic_index;
    default:
      DLOG(FATAL) << "Not reached.";
      return 0;
  }
}

inline void ExtractSizeTypeNormalized(vertex_struct::Type type,
                                      GLint *size,
                                      GLenum *gl_type,
                                      GLboolean *normalized) {
  switch (type) {
    case vertex_struct::kFloat1:
    case vertex_struct::kFloat2:
    case vertex_struct::kFloat3:
    case vertex_struct::kFloat4:
      *size = type - vertex_struct::kFloat1 + 1;
      *gl_type = GL_FLOAT;
      *normalized = false;
      break;
    case vertex_struct::kUChar4N:
      *size = 4;
      *gl_type = GL_UNSIGNED_BYTE;
      *normalized = true;
      break;
    default:
      DLOG(FATAL) << "Not reached.";
      break;
  }
}

}  // anonymous namespace

#ifndef COMPILER_MSVC
// Although required by the spec, this causes problems on MSVC. It is needed by
// GCC.
const unsigned int VertexStructGL::kMaxAttribs;
#endif

void VertexStructGL::Compile() {
  DCHECK(dirty_);
  for (unsigned int i = 0; i < kMaxAttribs; ++i) {
    attribs_[i].vertex_buffer_id = kInvalidResource;
  }
  for (unsigned int i = 0; i < count_ ; ++i) {
    const Element &element = GetElement(i);
    unsigned int index = GetAttribIndex(element.semantic,
                                        element.semantic_index);
    DCHECK_LT(index, kMaxAttribs);
    AttribDesc &attrib = attribs_[index];
    attrib.vertex_buffer_id = element.vertex_buffer;
    ExtractSizeTypeNormalized(element.type, &attrib.size, &attrib.type,
                              &attrib.normalized);
    attrib.stride = element.stride;
    attrib.offset = OffsetToPtrDiff(element.offset);
  }
  dirty_ = false;
}

parse_error::ParseError GAPIGL::CreateVertexBuffer(ResourceId id,
                                                           unsigned int size,
                                                           unsigned int flags) {
  VertexBufferGL *vertex_buffer = new VertexBufferGL(size, flags);
  vertex_buffer->Create();
  vertex_buffers_.Assign(id, vertex_buffer);
  return parse_error::kParseNoError;
}

parse_error::ParseError GAPIGL::DestroyVertexBuffer(ResourceId id) {
  return vertex_buffers_.Destroy(id) ?
      parse_error::kParseNoError :
      parse_error::kParseInvalidArguments;
}

parse_error::ParseError GAPIGL::SetVertexBufferData(ResourceId id,
                                                            unsigned int offset,
                                                            unsigned int size,
                                                            const void *data) {
  VertexBufferGL *vertex_buffer = vertex_buffers_.Get(id);
  if (!vertex_buffer) return parse_error::kParseInvalidArguments;
  return vertex_buffer->SetData(offset, size, data) ?
      parse_error::kParseNoError :
      parse_error::kParseInvalidArguments;
}

parse_error::ParseError GAPIGL::GetVertexBufferData(ResourceId id,
                                                            unsigned int offset,
                                                            unsigned int size,
                                                            void *data) {
  VertexBufferGL *vertex_buffer = vertex_buffers_.Get(id);
  if (!vertex_buffer) return parse_error::kParseInvalidArguments;
  return vertex_buffer->GetData(offset, size, data) ?
      parse_error::kParseNoError :
      parse_error::kParseInvalidArguments;
}

parse_error::ParseError GAPIGL::CreateIndexBuffer(ResourceId id,
                                                          unsigned int size,
                                                          unsigned int flags) {
  IndexBufferGL *index_buffer = new IndexBufferGL(size, flags);
  index_buffer->Create();
  index_buffers_.Assign(id, index_buffer);
  return parse_error::kParseNoError;
}

parse_error::ParseError GAPIGL::DestroyIndexBuffer(ResourceId id) {
  return index_buffers_.Destroy(id) ?
      parse_error::kParseNoError :
      parse_error::kParseInvalidArguments;
}

parse_error::ParseError GAPIGL::SetIndexBufferData(ResourceId id,
                                                           unsigned int offset,
                                                           unsigned int size,
                                                           const void *data) {
  IndexBufferGL *index_buffer = index_buffers_.Get(id);
  if (!index_buffer) return parse_error::kParseInvalidArguments;
  return index_buffer->SetData(offset, size, data) ?
      parse_error::kParseNoError :
      parse_error::kParseInvalidArguments;
}

parse_error::ParseError GAPIGL::GetIndexBufferData(ResourceId id,
                                                           unsigned int offset,
                                                           unsigned int size,
                                                           void *data) {
  IndexBufferGL *index_buffer = index_buffers_.Get(id);
  if (!index_buffer) return parse_error::kParseInvalidArguments;
  return index_buffer->GetData(offset, size, data) ?
      parse_error::kParseNoError :
      parse_error::kParseInvalidArguments;
}

parse_error::ParseError GAPIGL::CreateVertexStruct(
    ResourceId id,
    unsigned int input_count) {
  if (id == current_vertex_struct_) validate_streams_ = true;
  VertexStructGL *vertex_struct = new VertexStructGL(input_count);
  vertex_structs_.Assign(id, vertex_struct);
  return parse_error::kParseNoError;
}

parse_error::ParseError GAPIGL::DestroyVertexStruct(ResourceId id) {
  if (id == current_vertex_struct_) validate_streams_ = true;
  return vertex_structs_.Destroy(id) ?
      parse_error::kParseNoError :
      parse_error::kParseInvalidArguments;
}

parse_error::ParseError GAPIGL::SetVertexInput(
    ResourceId vertex_struct_id,
    unsigned int input_index,
    ResourceId vertex_buffer_id,
    unsigned int offset,
    unsigned int stride,
    vertex_struct::Type type,
    vertex_struct::Semantic semantic,
    unsigned int semantic_index) {
  switch (semantic) {
    case vertex_struct::kPosition:
      if (semantic_index != 0) {
        return parse_error::kParseInvalidArguments;
      }
      break;
    case vertex_struct::kNormal:
      if (semantic_index != 0) {
        return parse_error::kParseInvalidArguments;
      }
      break;
    case vertex_struct::kColor:
      if (semantic_index >= 2) {
        return parse_error::kParseInvalidArguments;
      }
      break;
    case vertex_struct::kTexCoord:
      if (semantic_index >= 8) {
        return parse_error::kParseInvalidArguments;
      }
      break;
    default:
      DLOG(FATAL) << "Not reached.";
      break;
  }
  if (vertex_buffer_id == current_vertex_struct_) validate_streams_ = true;
  VertexStructGL *vertex_struct = vertex_structs_.Get(vertex_struct_id);
  if (!vertex_struct || input_index >= vertex_struct->count())
    return parse_error::kParseInvalidArguments;
  vertex_struct->SetInput(input_index, vertex_buffer_id, offset, stride, type,
                          semantic, semantic_index);
  return parse_error::kParseNoError;
}

parse_error::ParseError GAPIGL::SetVertexStruct(ResourceId id) {
  current_vertex_struct_ = id;
  validate_streams_ = true;
  return parse_error::kParseNoError;
}

bool GAPIGL::ValidateStreams() {
  DCHECK(validate_streams_);
  VertexStructGL *vertex_struct = vertex_structs_.Get(current_vertex_struct_);
  if (!vertex_struct) {
    LOG(ERROR) << "Drawing with invalid streams.";
    return false;
  }
  max_vertices_ = vertex_struct->SetStreams(this);
  validate_streams_ = false;
  return max_vertices_ > 0;
}

namespace {

void PrimitiveTypeToGL(o3d::PrimitiveType primitive_type,
                       GLenum *gl_mode,
                       unsigned int *count) {
  switch (primitive_type) {
    case o3d::kPoints:
      *gl_mode = GL_POINTS;
      break;
    case o3d::kLines:
      *gl_mode = GL_LINES;
      *count *= 2;
      break;
    case o3d::kLineStrips:
      *gl_mode = GL_LINE_STRIP;
      ++*count;
      break;
    case o3d::kTriangles:
      *gl_mode = GL_TRIANGLES;
      *count *= 3;
      break;
    case o3d::kTriangleStrips:
      *gl_mode = GL_TRIANGLE_STRIP;
      *count += 2;
      break;
    case o3d::kTriangleFans:
      *gl_mode = GL_TRIANGLE_FAN;
      *count += 2;
      break;
    default:
      LOG(FATAL) << "Invalid primitive type";
      break;
  }
}

}  // anonymous namespace

parse_error::ParseError GAPIGL::Draw(o3d::PrimitiveType primitive_type,
                                     unsigned int first,
                                     unsigned int count) {
  if (validate_effect_ && !ValidateEffect()) {
    return parse_error::kParseInvalidArguments;
  }
  DCHECK(current_effect_);
  if (validate_streams_ && !ValidateStreams()) {
    return parse_error::kParseInvalidArguments;
  }
  GLenum gl_mode = GL_POINTS;
  PrimitiveTypeToGL(primitive_type, &gl_mode, &count);
  if (first + count > max_vertices_) {
    return parse_error::kParseInvalidArguments;
  }
  glDrawArrays(gl_mode, first, count);
  CHECK_GL_ERROR();
  return parse_error::kParseNoError;
}

parse_error::ParseError GAPIGL::DrawIndexed(
    o3d::PrimitiveType primitive_type,
    ResourceId index_buffer_id,
    unsigned int first,
    unsigned int count,
    unsigned int min_index,
    unsigned int max_index) {
  IndexBufferGL *index_buffer = index_buffers_.Get(index_buffer_id);
  if (!index_buffer) return parse_error::kParseInvalidArguments;
  if (validate_effect_ && !ValidateEffect()) {
    return parse_error::kParseInvalidArguments;
  }
  DCHECK(current_effect_);
  if (validate_streams_ && !ValidateStreams()) {
    return parse_error::kParseInvalidArguments;
  }
  if ((min_index >= max_vertices_) || (max_index > max_vertices_)) {
    return parse_error::kParseInvalidArguments;
  }
  GLenum gl_mode = GL_POINTS;
  PrimitiveTypeToGL(primitive_type, &gl_mode, &count);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer->gl_buffer());
  GLenum index_type = (index_buffer->flags() & index_buffer::kIndex32Bit) ?
                      GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
  GLuint index_size = (index_buffer->flags() & index_buffer::kIndex32Bit) ?
                      sizeof(GLuint) : sizeof(GLushort);  // NOLINT
  GLuint offset = first * index_size;
  if (offset + count * index_size > index_buffer->size()) {
    return parse_error::kParseInvalidArguments;
  }
  glDrawRangeElements(gl_mode, min_index, max_index, count, index_type,
                      OffsetToPtr(offset));
  CHECK_GL_ERROR();
  return parse_error::kParseNoError;
}

}  // namespace o3d
}  // namespace command_buffer
}  // namespace o3d
