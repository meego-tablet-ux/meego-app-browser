// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is auto-generated. DO NOT EDIT!

// It is included by gles2_cmd_decoder.cc
#ifndef GPU_COMMAND_BUFFER_SERVICE_GLES2_CMD_DECODER_AUTOGEN_H_
#define GPU_COMMAND_BUFFER_SERVICE_GLES2_CMD_DECODER_AUTOGEN_H_

error::Error GLES2DecoderImpl::HandleActiveTexture(
    uint32 immediate_data_size, const gles2::ActiveTexture& c) {
  GLenum texture = static_cast<GLenum>(c.texture);
  glActiveTexture(texture);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleAttachShader(
    uint32 immediate_data_size, const gles2::AttachShader& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLuint shader;
  if (!id_manager_->GetServiceId(c.shader, &shader)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  glAttachShader(program, shader);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBindAttribLocation(
    uint32 immediate_data_size, const gles2::BindAttribLocation& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLuint index = static_cast<GLuint>(c.index);
  uint32 name_size = c.data_size;
  const char* name = GetSharedMemoryAs<const char*>(
      c.name_shm_id, c.name_shm_offset, name_size);
  if (name == NULL) {
    return error::kOutOfBounds;
  }
  String name_str(name, name_size);
  glBindAttribLocation(program, index, name_str.c_str());
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBindAttribLocationImmediate(
    uint32 immediate_data_size, const gles2::BindAttribLocationImmediate& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLuint index = static_cast<GLuint>(c.index);
  uint32 name_size = c.data_size;
  const char* name = GetImmediateDataAs<const char*>(
      c, name_size, immediate_data_size);
  if (name == NULL) {
    return error::kOutOfBounds;
  }
  String name_str(name, name_size);
  glBindAttribLocation(program, index, name_str.c_str());
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBindBuffer(
    uint32 immediate_data_size, const gles2::BindBuffer& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLuint buffer;
  if (!id_manager_->GetServiceId(c.buffer, &buffer)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (!ValidateGLenumBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  DoBindBuffer(target, buffer);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBindFramebuffer(
    uint32 immediate_data_size, const gles2::BindFramebuffer& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLuint framebuffer;
  if (!id_manager_->GetServiceId(c.framebuffer, &framebuffer)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (!ValidateGLenumFrameBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glBindFramebufferEXT(target, framebuffer);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBindRenderbuffer(
    uint32 immediate_data_size, const gles2::BindRenderbuffer& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLuint renderbuffer;
  if (!id_manager_->GetServiceId(c.renderbuffer, &renderbuffer)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (!ValidateGLenumRenderBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glBindRenderbufferEXT(target, renderbuffer);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBindTexture(
    uint32 immediate_data_size, const gles2::BindTexture& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLuint texture;
  if (!id_manager_->GetServiceId(c.texture, &texture)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureBindTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glBindTexture(target, texture);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBlendColor(
    uint32 immediate_data_size, const gles2::BlendColor& c) {
  GLclampf red = static_cast<GLclampf>(c.red);
  GLclampf green = static_cast<GLclampf>(c.green);
  GLclampf blue = static_cast<GLclampf>(c.blue);
  GLclampf alpha = static_cast<GLclampf>(c.alpha);
  glBlendColor(red, green, blue, alpha);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBlendEquation(
    uint32 immediate_data_size, const gles2::BlendEquation& c) {
  GLenum mode = static_cast<GLenum>(c.mode);
  if (!ValidateGLenumEquation(mode)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glBlendEquation(mode);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBlendEquationSeparate(
    uint32 immediate_data_size, const gles2::BlendEquationSeparate& c) {
  GLenum modeRGB = static_cast<GLenum>(c.modeRGB);
  GLenum modeAlpha = static_cast<GLenum>(c.modeAlpha);
  if (!ValidateGLenumEquation(modeRGB)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumEquation(modeAlpha)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glBlendEquationSeparate(modeRGB, modeAlpha);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBlendFunc(
    uint32 immediate_data_size, const gles2::BlendFunc& c) {
  GLenum sfactor = static_cast<GLenum>(c.sfactor);
  GLenum dfactor = static_cast<GLenum>(c.dfactor);
  if (!ValidateGLenumSrcBlendFactor(sfactor)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumDstBlendFactor(dfactor)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glBlendFunc(sfactor, dfactor);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBlendFuncSeparate(
    uint32 immediate_data_size, const gles2::BlendFuncSeparate& c) {
  GLenum srcRGB = static_cast<GLenum>(c.srcRGB);
  GLenum dstRGB = static_cast<GLenum>(c.dstRGB);
  GLenum srcAlpha = static_cast<GLenum>(c.srcAlpha);
  GLenum dstAlpha = static_cast<GLenum>(c.dstAlpha);
  if (!ValidateGLenumSrcBlendFactor(srcRGB)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumDstBlendFactor(dstRGB)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumSrcBlendFactor(srcAlpha)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumDstBlendFactor(dstAlpha)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBufferSubData(
    uint32 immediate_data_size, const gles2::BufferSubData& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLintptr offset = static_cast<GLintptr>(c.offset);
  GLsizeiptr size = static_cast<GLsizeiptr>(c.size);
  uint32 data_size = size;
  const void* data = GetSharedMemoryAs<const void*>(
      c.data_shm_id, c.data_shm_offset, data_size);
  if (!ValidateGLenumBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (data == NULL) {
    return error::kOutOfBounds;
  }
  glBufferSubData(target, offset, size, data);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleBufferSubDataImmediate(
    uint32 immediate_data_size, const gles2::BufferSubDataImmediate& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLintptr offset = static_cast<GLintptr>(c.offset);
  GLsizeiptr size = static_cast<GLsizeiptr>(c.size);
  uint32 data_size = size;
  const void* data = GetImmediateDataAs<const void*>(
      c, data_size, immediate_data_size);
  if (!ValidateGLenumBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (data == NULL) {
    return error::kOutOfBounds;
  }
  glBufferSubData(target, offset, size, data);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCheckFramebufferStatus(
    uint32 immediate_data_size, const gles2::CheckFramebufferStatus& c) {
  GLenum target = static_cast<GLenum>(c.target);
  if (!ValidateGLenumFrameBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glCheckFramebufferStatusEXT(target);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleClear(
    uint32 immediate_data_size, const gles2::Clear& c) {
  GLbitfield mask = static_cast<GLbitfield>(c.mask);
  glClear(mask);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleClearColor(
    uint32 immediate_data_size, const gles2::ClearColor& c) {
  GLclampf red = static_cast<GLclampf>(c.red);
  GLclampf green = static_cast<GLclampf>(c.green);
  GLclampf blue = static_cast<GLclampf>(c.blue);
  GLclampf alpha = static_cast<GLclampf>(c.alpha);
  glClearColor(red, green, blue, alpha);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleClearDepthf(
    uint32 immediate_data_size, const gles2::ClearDepthf& c) {
  GLclampf depth = static_cast<GLclampf>(c.depth);
  glClearDepth(depth);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleClearStencil(
    uint32 immediate_data_size, const gles2::ClearStencil& c) {
  GLint s = static_cast<GLint>(c.s);
  glClearStencil(s);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleColorMask(
    uint32 immediate_data_size, const gles2::ColorMask& c) {
  GLboolean red = static_cast<GLboolean>(c.red);
  GLboolean green = static_cast<GLboolean>(c.green);
  GLboolean blue = static_cast<GLboolean>(c.blue);
  GLboolean alpha = static_cast<GLboolean>(c.alpha);
  glColorMask(red, green, blue, alpha);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCompileShader(
    uint32 immediate_data_size, const gles2::CompileShader& c) {
  GLuint shader;
  if (!id_manager_->GetServiceId(c.shader, &shader)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  DoCompileShader(shader);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCompressedTexSubImage2D(
    uint32 immediate_data_size, const gles2::CompressedTexSubImage2D& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLint xoffset = static_cast<GLint>(c.xoffset);
  GLint yoffset = static_cast<GLint>(c.yoffset);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLenum format = static_cast<GLenum>(c.format);
  GLsizei imageSize = static_cast<GLsizei>(c.imageSize);
  uint32 data_size = imageSize;
  const void* data = GetSharedMemoryAs<const void*>(
      c.data_shm_id, c.data_shm_offset, data_size);
  if (!ValidateGLenumTextureTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (data == NULL) {
    return error::kOutOfBounds;
  }
  glCompressedTexSubImage2D(
      target, level, xoffset, yoffset, width, height, format, imageSize, data);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCompressedTexSubImage2DImmediate(
    uint32 immediate_data_size,
    const gles2::CompressedTexSubImage2DImmediate& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLint xoffset = static_cast<GLint>(c.xoffset);
  GLint yoffset = static_cast<GLint>(c.yoffset);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLenum format = static_cast<GLenum>(c.format);
  GLsizei imageSize = static_cast<GLsizei>(c.imageSize);
  uint32 data_size = imageSize;
  const void* data = GetImmediateDataAs<const void*>(
      c, data_size, immediate_data_size);
  if (!ValidateGLenumTextureTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (data == NULL) {
    return error::kOutOfBounds;
  }
  glCompressedTexSubImage2D(
      target, level, xoffset, yoffset, width, height, format, imageSize, data);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCopyTexImage2D(
    uint32 immediate_data_size, const gles2::CopyTexImage2D& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLenum internalformat = static_cast<GLenum>(c.internalformat);
  GLint x = static_cast<GLint>(c.x);
  GLint y = static_cast<GLint>(c.y);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLint border = static_cast<GLint>(c.border);
  if (!ValidateGLenumTextureTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCopyTexSubImage2D(
    uint32 immediate_data_size, const gles2::CopyTexSubImage2D& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLint xoffset = static_cast<GLint>(c.xoffset);
  GLint yoffset = static_cast<GLint>(c.yoffset);
  GLint x = static_cast<GLint>(c.x);
  GLint y = static_cast<GLint>(c.y);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  if (!ValidateGLenumTextureTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCreateProgram(
    uint32 immediate_data_size, const gles2::CreateProgram& c) {
  uint32 client_id = c.client_id;
  CreateProgramHelper(client_id);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCreateShader(
    uint32 immediate_data_size, const gles2::CreateShader& c) {
  GLenum type = static_cast<GLenum>(c.type);
  if (!ValidateGLenumShaderType(type)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  uint32 client_id = c.client_id;
  CreateShaderHelper(type, client_id);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleCullFace(
    uint32 immediate_data_size, const gles2::CullFace& c) {
  GLenum mode = static_cast<GLenum>(c.mode);
  if (!ValidateGLenumFaceType(mode)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glCullFace(mode);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDeleteBuffers(
    uint32 immediate_data_size, const gles2::DeleteBuffers& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  const GLuint* buffers = GetSharedMemoryAs<const GLuint*>(
      c.buffers_shm_id, c.buffers_shm_offset, data_size);
  if (buffers == NULL) {
    return error::kOutOfBounds;
  }
  DeleteGLObjects<GLDeleteBuffersHelper>(n, buffers);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDeleteBuffersImmediate(
    uint32 immediate_data_size, const gles2::DeleteBuffersImmediate& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  const GLuint* buffers = GetImmediateDataAs<const GLuint*>(
      c, data_size, immediate_data_size);
  if (buffers == NULL) {
    return error::kOutOfBounds;
  }
  DeleteGLObjects<GLDeleteBuffersHelper>(n, buffers);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDeleteFramebuffers(
    uint32 immediate_data_size, const gles2::DeleteFramebuffers& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  const GLuint* framebuffers = GetSharedMemoryAs<const GLuint*>(
      c.framebuffers_shm_id, c.framebuffers_shm_offset, data_size);
  if (framebuffers == NULL) {
    return error::kOutOfBounds;
  }
  DeleteGLObjects<GLDeleteFramebuffersHelper>(n, framebuffers);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDeleteFramebuffersImmediate(
    uint32 immediate_data_size, const gles2::DeleteFramebuffersImmediate& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  const GLuint* framebuffers = GetImmediateDataAs<const GLuint*>(
      c, data_size, immediate_data_size);
  if (framebuffers == NULL) {
    return error::kOutOfBounds;
  }
  DeleteGLObjects<GLDeleteFramebuffersHelper>(n, framebuffers);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDeleteRenderbuffers(
    uint32 immediate_data_size, const gles2::DeleteRenderbuffers& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  const GLuint* renderbuffers = GetSharedMemoryAs<const GLuint*>(
      c.renderbuffers_shm_id, c.renderbuffers_shm_offset, data_size);
  if (renderbuffers == NULL) {
    return error::kOutOfBounds;
  }
  DeleteGLObjects<GLDeleteRenderbuffersHelper>(n, renderbuffers);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDeleteRenderbuffersImmediate(
    uint32 immediate_data_size, const gles2::DeleteRenderbuffersImmediate& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  const GLuint* renderbuffers = GetImmediateDataAs<const GLuint*>(
      c, data_size, immediate_data_size);
  if (renderbuffers == NULL) {
    return error::kOutOfBounds;
  }
  DeleteGLObjects<GLDeleteRenderbuffersHelper>(n, renderbuffers);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDeleteTextures(
    uint32 immediate_data_size, const gles2::DeleteTextures& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  const GLuint* textures = GetSharedMemoryAs<const GLuint*>(
      c.textures_shm_id, c.textures_shm_offset, data_size);
  if (textures == NULL) {
    return error::kOutOfBounds;
  }
  DeleteGLObjects<GLDeleteTexturesHelper>(n, textures);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDeleteTexturesImmediate(
    uint32 immediate_data_size, const gles2::DeleteTexturesImmediate& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  const GLuint* textures = GetImmediateDataAs<const GLuint*>(
      c, data_size, immediate_data_size);
  if (textures == NULL) {
    return error::kOutOfBounds;
  }
  DeleteGLObjects<GLDeleteTexturesHelper>(n, textures);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDepthFunc(
    uint32 immediate_data_size, const gles2::DepthFunc& c) {
  GLenum func = static_cast<GLenum>(c.func);
  if (!ValidateGLenumCmpFunction(func)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glDepthFunc(func);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDepthMask(
    uint32 immediate_data_size, const gles2::DepthMask& c) {
  GLboolean flag = static_cast<GLboolean>(c.flag);
  glDepthMask(flag);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDepthRangef(
    uint32 immediate_data_size, const gles2::DepthRangef& c) {
  GLclampf zNear = static_cast<GLclampf>(c.zNear);
  GLclampf zFar = static_cast<GLclampf>(c.zFar);
  glDepthRange(zNear, zFar);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDetachShader(
    uint32 immediate_data_size, const gles2::DetachShader& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLuint shader;
  if (!id_manager_->GetServiceId(c.shader, &shader)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  glDetachShader(program, shader);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDisable(
    uint32 immediate_data_size, const gles2::Disable& c) {
  GLenum cap = static_cast<GLenum>(c.cap);
  if (!ValidateGLenumCapability(cap)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glDisable(cap);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDisableVertexAttribArray(
    uint32 immediate_data_size, const gles2::DisableVertexAttribArray& c) {
  GLuint index = static_cast<GLuint>(c.index);
  DoDisableVertexAttribArray(index);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleDrawArrays(
    uint32 immediate_data_size, const gles2::DrawArrays& c) {
  GLenum mode = static_cast<GLenum>(c.mode);
  GLint first = static_cast<GLint>(c.first);
  GLsizei count = static_cast<GLsizei>(c.count);
  if (!ValidateGLenumDrawMode(mode)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  DoDrawArrays(mode, first, count);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleEnable(
    uint32 immediate_data_size, const gles2::Enable& c) {
  GLenum cap = static_cast<GLenum>(c.cap);
  if (!ValidateGLenumCapability(cap)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glEnable(cap);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleEnableVertexAttribArray(
    uint32 immediate_data_size, const gles2::EnableVertexAttribArray& c) {
  GLuint index = static_cast<GLuint>(c.index);
  DoEnableVertexAttribArray(index);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleFinish(
    uint32 immediate_data_size, const gles2::Finish& c) {
  glFinish();
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleFlush(
    uint32 immediate_data_size, const gles2::Flush& c) {
  glFlush();
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleFramebufferRenderbuffer(
    uint32 immediate_data_size, const gles2::FramebufferRenderbuffer& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum attachment = static_cast<GLenum>(c.attachment);
  GLenum renderbuffertarget = static_cast<GLenum>(c.renderbuffertarget);
  GLuint renderbuffer;
  if (!id_manager_->GetServiceId(c.renderbuffer, &renderbuffer)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (!ValidateGLenumFrameBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumAttachment(attachment)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumRenderBufferTarget(renderbuffertarget)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glFramebufferRenderbufferEXT(
      target, attachment, renderbuffertarget, renderbuffer);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleFramebufferTexture2D(
    uint32 immediate_data_size, const gles2::FramebufferTexture2D& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum attachment = static_cast<GLenum>(c.attachment);
  GLenum textarget = static_cast<GLenum>(c.textarget);
  GLuint texture;
  if (!id_manager_->GetServiceId(c.texture, &texture)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLint level = static_cast<GLint>(c.level);
  if (!ValidateGLenumFrameBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumAttachment(attachment)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureTarget(textarget)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glFramebufferTexture2DEXT(target, attachment, textarget, texture, level);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleFrontFace(
    uint32 immediate_data_size, const gles2::FrontFace& c) {
  GLenum mode = static_cast<GLenum>(c.mode);
  if (!ValidateGLenumFaceMode(mode)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glFrontFace(mode);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGenBuffers(
    uint32 immediate_data_size, const gles2::GenBuffers& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  GLuint* buffers = GetSharedMemoryAs<GLuint*>(
      c.buffers_shm_id, c.buffers_shm_offset, data_size);
  if (buffers == NULL) {
    return error::kOutOfBounds;
  }
  if (!GenGLObjects<GLGenBuffersHelper>(n, buffers)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGenBuffersImmediate(
    uint32 immediate_data_size, const gles2::GenBuffersImmediate& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  GLuint* buffers = GetImmediateDataAs<GLuint*>(
      c, data_size, immediate_data_size);
  if (buffers == NULL) {
    return error::kOutOfBounds;
  }
  if (!GenGLObjects<GLGenBuffersHelper>(n, buffers)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGenerateMipmap(
    uint32 immediate_data_size, const gles2::GenerateMipmap& c) {
  GLenum target = static_cast<GLenum>(c.target);
  if (!ValidateGLenumTextureBindTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glGenerateMipmapEXT(target);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGenFramebuffers(
    uint32 immediate_data_size, const gles2::GenFramebuffers& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  GLuint* framebuffers = GetSharedMemoryAs<GLuint*>(
      c.framebuffers_shm_id, c.framebuffers_shm_offset, data_size);
  if (framebuffers == NULL) {
    return error::kOutOfBounds;
  }
  if (!GenGLObjects<GLGenFramebuffersHelper>(n, framebuffers)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGenFramebuffersImmediate(
    uint32 immediate_data_size, const gles2::GenFramebuffersImmediate& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  GLuint* framebuffers = GetImmediateDataAs<GLuint*>(
      c, data_size, immediate_data_size);
  if (framebuffers == NULL) {
    return error::kOutOfBounds;
  }
  if (!GenGLObjects<GLGenFramebuffersHelper>(n, framebuffers)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGenRenderbuffers(
    uint32 immediate_data_size, const gles2::GenRenderbuffers& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  GLuint* renderbuffers = GetSharedMemoryAs<GLuint*>(
      c.renderbuffers_shm_id, c.renderbuffers_shm_offset, data_size);
  if (renderbuffers == NULL) {
    return error::kOutOfBounds;
  }
  if (!GenGLObjects<GLGenRenderbuffersHelper>(n, renderbuffers)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGenRenderbuffersImmediate(
    uint32 immediate_data_size, const gles2::GenRenderbuffersImmediate& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  GLuint* renderbuffers = GetImmediateDataAs<GLuint*>(
      c, data_size, immediate_data_size);
  if (renderbuffers == NULL) {
    return error::kOutOfBounds;
  }
  if (!GenGLObjects<GLGenRenderbuffersHelper>(n, renderbuffers)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGenTextures(
    uint32 immediate_data_size, const gles2::GenTextures& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  GLuint* textures = GetSharedMemoryAs<GLuint*>(
      c.textures_shm_id, c.textures_shm_offset, data_size);
  if (textures == NULL) {
    return error::kOutOfBounds;
  }
  if (!GenGLObjects<GLGenTexturesHelper>(n, textures)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGenTexturesImmediate(
    uint32 immediate_data_size, const gles2::GenTexturesImmediate& c) {
  GLsizei n = static_cast<GLsizei>(c.n);
  uint32 data_size = n * sizeof(GLuint);
  GLuint* textures = GetImmediateDataAs<GLuint*>(
      c, data_size, immediate_data_size);
  if (textures == NULL) {
    return error::kOutOfBounds;
  }
  if (!GenGLObjects<GLGenTexturesHelper>(n, textures)) {
    return error::kInvalidArguments;
  }
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetBooleanv(
    uint32 immediate_data_size, const gles2::GetBooleanv& c) {
  GLenum pname = static_cast<GLenum>(c.pname);
  GLboolean* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLboolean*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetBooleanv(pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetBufferParameteriv(
    uint32 immediate_data_size, const gles2::GetBufferParameteriv& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLint*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (!ValidateGLenumBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumBufferParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetBufferParameteriv(target, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetError(
    uint32 immediate_data_size, const gles2::GetError& c) {
  GLenum* result_dst = GetSharedMemoryAs<GLenum*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result_dst));
  *result_dst = GetGLError();
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetFloatv(
    uint32 immediate_data_size, const gles2::GetFloatv& c) {
  GLenum pname = static_cast<GLenum>(c.pname);
  GLfloat* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLfloat*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetFloatv(pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetFramebufferAttachmentParameteriv(
    uint32 immediate_data_size,
    const gles2::GetFramebufferAttachmentParameteriv& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum attachment = static_cast<GLenum>(c.attachment);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLint*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (!ValidateGLenumFrameBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumAttachment(attachment)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumFrameBufferParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetFramebufferAttachmentParameterivEXT(target, attachment, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetIntegerv(
    uint32 immediate_data_size, const gles2::GetIntegerv& c) {
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLint*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetIntegerv(pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetProgramiv(
    uint32 immediate_data_size, const gles2::GetProgramiv& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLint*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (!ValidateGLenumProgramParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetProgramiv(program, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetProgramInfoLog(
    uint32 immediate_data_size, const gles2::GetProgramInfoLog& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLsizei bufsize = static_cast<GLsizei>(c.bufsize);
  uint32 size_shm_id = c.length_shm_id;
  uint32 size_shm_offset = c.length_shm_offset;
  GLsizei* length = NULL;
  if (size_shm_id != 0 || size_shm_offset != 0) {
    length = GetSharedMemoryAs<GLsizei*>(
        size_shm_id, size_shm_offset, sizeof(*length));
    if (!length) {
      return error::kOutOfBounds;
    }
  }
  char* infolog = GetSharedMemoryAs<char*>(
      c.infolog_shm_id, c.infolog_shm_offset, bufsize);
  if (infolog == NULL) {
    return error::kOutOfBounds;
  }
  glGetProgramInfoLog(program, bufsize, length, infolog);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetRenderbufferParameteriv(
    uint32 immediate_data_size, const gles2::GetRenderbufferParameteriv& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLint*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (!ValidateGLenumRenderBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumRenderBufferParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetRenderbufferParameterivEXT(target, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetShaderiv(
    uint32 immediate_data_size, const gles2::GetShaderiv& c) {
  GLuint shader;
  if (!id_manager_->GetServiceId(c.shader, &shader)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLint*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (!ValidateGLenumShaderParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetShaderiv(shader, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetShaderInfoLog(
    uint32 immediate_data_size, const gles2::GetShaderInfoLog& c) {
  GLuint shader;
  if (!id_manager_->GetServiceId(c.shader, &shader)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLsizei bufsize = static_cast<GLsizei>(c.bufsize);
  uint32 size_shm_id = c.length_shm_id;
  uint32 size_shm_offset = c.length_shm_offset;
  GLsizei* length = NULL;
  if (size_shm_id != 0 || size_shm_offset != 0) {
    length = GetSharedMemoryAs<GLsizei*>(
        size_shm_id, size_shm_offset, sizeof(*length));
    if (!length) {
      return error::kOutOfBounds;
    }
  }
  char* infolog = GetSharedMemoryAs<char*>(
      c.infolog_shm_id, c.infolog_shm_offset, bufsize);
  if (infolog == NULL) {
    return error::kOutOfBounds;
  }
  glGetShaderInfoLog(shader, bufsize, length, infolog);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetShaderSource(
    uint32 immediate_data_size, const gles2::GetShaderSource& c) {
  GLuint shader;
  if (!id_manager_->GetServiceId(c.shader, &shader)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLsizei bufsize = static_cast<GLsizei>(c.bufsize);
  uint32 size_shm_id = c.length_shm_id;
  uint32 size_shm_offset = c.length_shm_offset;
  GLsizei* length = NULL;
  if (size_shm_id != 0 || size_shm_offset != 0) {
    length = GetSharedMemoryAs<GLsizei*>(
        size_shm_id, size_shm_offset, sizeof(*length));
    if (!length) {
      return error::kOutOfBounds;
    }
  }
  char* source = GetSharedMemoryAs<char*>(
      c.source_shm_id, c.source_shm_offset, bufsize);
  if (source == NULL) {
    return error::kOutOfBounds;
  }
  DoGetShaderSource(shader, bufsize, length, source);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetString(
    uint32 immediate_data_size, const gles2::GetString& c) {
  GLenum name = static_cast<GLenum>(c.name);
  if (!ValidateGLenumStringType(name)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glGetString(name);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetTexParameterfv(
    uint32 immediate_data_size, const gles2::GetTexParameterfv& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLfloat* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLfloat*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (!ValidateGLenumTextureTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetTexParameterfv(target, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetTexParameteriv(
    uint32 immediate_data_size, const gles2::GetTexParameteriv& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLint*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (!ValidateGLenumTextureTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetTexParameteriv(target, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetVertexAttribfv(
    uint32 immediate_data_size, const gles2::GetVertexAttribfv& c) {
  GLuint index = static_cast<GLuint>(c.index);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLfloat* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLfloat*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (!ValidateGLenumVertexAttribute(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetVertexAttribfv(index, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleGetVertexAttribiv(
    uint32 immediate_data_size, const gles2::GetVertexAttribiv& c) {
  GLuint index = static_cast<GLuint>(c.index);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint* params;
  GLsizei num_values = util_.GLGetNumValuesReturned(pname);
  uint32 params_size = num_values * sizeof(*params);
  params = GetSharedMemoryAs<GLint*>(
      c.params_shm_id, c.params_shm_offset, params_size);
  if (!ValidateGLenumVertexAttribute(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glGetVertexAttribiv(index, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleHint(
    uint32 immediate_data_size, const gles2::Hint& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum mode = static_cast<GLenum>(c.mode);
  if (!ValidateGLenumHintTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumHintMode(mode)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glHint(target, mode);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleIsBuffer(
    uint32 immediate_data_size, const gles2::IsBuffer& c) {
  GLuint buffer;
  if (!id_manager_->GetServiceId(c.buffer, &buffer)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLboolean* result_dst = GetSharedMemoryAs<GLboolean*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result_dst));
  *result_dst = glIsBuffer(buffer);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleIsEnabled(
    uint32 immediate_data_size, const gles2::IsEnabled& c) {
  GLenum cap = static_cast<GLenum>(c.cap);
  GLboolean* result_dst = GetSharedMemoryAs<GLboolean*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result_dst));
  if (!ValidateGLenumCapability(cap)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  *result_dst = glIsEnabled(cap);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleIsFramebuffer(
    uint32 immediate_data_size, const gles2::IsFramebuffer& c) {
  GLuint framebuffer;
  if (!id_manager_->GetServiceId(c.framebuffer, &framebuffer)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLboolean* result_dst = GetSharedMemoryAs<GLboolean*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result_dst));
  *result_dst = glIsFramebufferEXT(framebuffer);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleIsProgram(
    uint32 immediate_data_size, const gles2::IsProgram& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLboolean* result_dst = GetSharedMemoryAs<GLboolean*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result_dst));
  *result_dst = glIsProgram(program);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleIsRenderbuffer(
    uint32 immediate_data_size, const gles2::IsRenderbuffer& c) {
  GLuint renderbuffer;
  if (!id_manager_->GetServiceId(c.renderbuffer, &renderbuffer)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLboolean* result_dst = GetSharedMemoryAs<GLboolean*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result_dst));
  *result_dst = glIsRenderbufferEXT(renderbuffer);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleIsShader(
    uint32 immediate_data_size, const gles2::IsShader& c) {
  GLuint shader;
  if (!id_manager_->GetServiceId(c.shader, &shader)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLboolean* result_dst = GetSharedMemoryAs<GLboolean*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result_dst));
  *result_dst = glIsShader(shader);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleIsTexture(
    uint32 immediate_data_size, const gles2::IsTexture& c) {
  GLuint texture;
  if (!id_manager_->GetServiceId(c.texture, &texture)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  GLboolean* result_dst = GetSharedMemoryAs<GLboolean*>(
      c.result_shm_id, c.result_shm_offset, sizeof(*result_dst));
  *result_dst = glIsTexture(texture);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleLineWidth(
    uint32 immediate_data_size, const gles2::LineWidth& c) {
  GLfloat width = static_cast<GLfloat>(c.width);
  glLineWidth(width);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleLinkProgram(
    uint32 immediate_data_size, const gles2::LinkProgram& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  DoLinkProgram(program);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandlePolygonOffset(
    uint32 immediate_data_size, const gles2::PolygonOffset& c) {
  GLfloat factor = static_cast<GLfloat>(c.factor);
  GLfloat units = static_cast<GLfloat>(c.units);
  glPolygonOffset(factor, units);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleRenderbufferStorage(
    uint32 immediate_data_size, const gles2::RenderbufferStorage& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum internalformat = static_cast<GLenum>(c.internalformat);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  if (!ValidateGLenumRenderBufferTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumRenderBufferFormat(internalformat)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glRenderbufferStorageEXT(target, internalformat, width, height);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleSampleCoverage(
    uint32 immediate_data_size, const gles2::SampleCoverage& c) {
  GLclampf value = static_cast<GLclampf>(c.value);
  GLboolean invert = static_cast<GLboolean>(c.invert);
  glSampleCoverage(value, invert);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleScissor(
    uint32 immediate_data_size, const gles2::Scissor& c) {
  GLint x = static_cast<GLint>(c.x);
  GLint y = static_cast<GLint>(c.y);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  glScissor(x, y, width, height);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleStencilFunc(
    uint32 immediate_data_size, const gles2::StencilFunc& c) {
  GLenum func = static_cast<GLenum>(c.func);
  GLint ref = static_cast<GLint>(c.ref);
  GLuint mask = static_cast<GLuint>(c.mask);
  if (!ValidateGLenumCmpFunction(func)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glStencilFunc(func, ref, mask);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleStencilFuncSeparate(
    uint32 immediate_data_size, const gles2::StencilFuncSeparate& c) {
  GLenum face = static_cast<GLenum>(c.face);
  GLenum func = static_cast<GLenum>(c.func);
  GLint ref = static_cast<GLint>(c.ref);
  GLuint mask = static_cast<GLuint>(c.mask);
  if (!ValidateGLenumFaceType(face)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumCmpFunction(func)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glStencilFuncSeparate(face, func, ref, mask);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleStencilMask(
    uint32 immediate_data_size, const gles2::StencilMask& c) {
  GLuint mask = static_cast<GLuint>(c.mask);
  glStencilMask(mask);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleStencilMaskSeparate(
    uint32 immediate_data_size, const gles2::StencilMaskSeparate& c) {
  GLenum face = static_cast<GLenum>(c.face);
  GLuint mask = static_cast<GLuint>(c.mask);
  if (!ValidateGLenumFaceType(face)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glStencilMaskSeparate(face, mask);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleStencilOp(
    uint32 immediate_data_size, const gles2::StencilOp& c) {
  GLenum fail = static_cast<GLenum>(c.fail);
  GLenum zfail = static_cast<GLenum>(c.zfail);
  GLenum zpass = static_cast<GLenum>(c.zpass);
  if (!ValidateGLenumStencilOp(fail)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumStencilOp(zfail)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumStencilOp(zpass)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glStencilOp(fail, zfail, zpass);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleStencilOpSeparate(
    uint32 immediate_data_size, const gles2::StencilOpSeparate& c) {
  GLenum face = static_cast<GLenum>(c.face);
  GLenum fail = static_cast<GLenum>(c.fail);
  GLenum zfail = static_cast<GLenum>(c.zfail);
  GLenum zpass = static_cast<GLenum>(c.zpass);
  if (!ValidateGLenumFaceType(face)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumStencilOp(fail)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumStencilOp(zfail)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumStencilOp(zpass)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glStencilOpSeparate(face, fail, zfail, zpass);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleTexParameterf(
    uint32 immediate_data_size, const gles2::TexParameterf& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLfloat param = static_cast<GLfloat>(c.param);
  if (!ValidateGLenumTextureBindTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glTexParameterf(target, pname, param);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleTexParameterfv(
    uint32 immediate_data_size, const gles2::TexParameterfv& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 1);
  const GLfloat* params = GetSharedMemoryAs<const GLfloat*>(
      c.params_shm_id, c.params_shm_offset, data_size);
  if (!ValidateGLenumTextureBindTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glTexParameterfv(target, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleTexParameterfvImmediate(
    uint32 immediate_data_size, const gles2::TexParameterfvImmediate& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 1);
  const GLfloat* params = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (!ValidateGLenumTextureBindTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glTexParameterfv(target, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleTexParameteri(
    uint32 immediate_data_size, const gles2::TexParameteri& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  GLint param = static_cast<GLint>(c.param);
  if (!ValidateGLenumTextureBindTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  glTexParameteri(target, pname, param);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleTexParameteriv(
    uint32 immediate_data_size, const gles2::TexParameteriv& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 1);
  const GLint* params = GetSharedMemoryAs<const GLint*>(
      c.params_shm_id, c.params_shm_offset, data_size);
  if (!ValidateGLenumTextureBindTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glTexParameteriv(target, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleTexParameterivImmediate(
    uint32 immediate_data_size, const gles2::TexParameterivImmediate& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLenum pname = static_cast<GLenum>(c.pname);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 1);
  const GLint* params = GetImmediateDataAs<const GLint*>(
      c, data_size, immediate_data_size);
  if (!ValidateGLenumTextureBindTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureParameter(pname)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (params == NULL) {
    return error::kOutOfBounds;
  }
  glTexParameteriv(target, pname, params);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleTexSubImage2D(
    uint32 immediate_data_size, const gles2::TexSubImage2D& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLint xoffset = static_cast<GLint>(c.xoffset);
  GLint yoffset = static_cast<GLint>(c.yoffset);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLenum format = static_cast<GLenum>(c.format);
  GLenum type = static_cast<GLenum>(c.type);
  uint32 data_size = GLES2Util::ComputeImageDataSize(
      width, height, format, type, unpack_alignment_);
  const void* pixels = GetSharedMemoryAs<const void*>(
      c.pixels_shm_id, c.pixels_shm_offset, data_size);
  if (!ValidateGLenumTextureTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureFormat(format)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumPixelType(type)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (pixels == NULL) {
    return error::kOutOfBounds;
  }
  glTexSubImage2D(
      target, level, xoffset, yoffset, width, height, format, type, pixels);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleTexSubImage2DImmediate(
    uint32 immediate_data_size, const gles2::TexSubImage2DImmediate& c) {
  GLenum target = static_cast<GLenum>(c.target);
  GLint level = static_cast<GLint>(c.level);
  GLint xoffset = static_cast<GLint>(c.xoffset);
  GLint yoffset = static_cast<GLint>(c.yoffset);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  GLenum format = static_cast<GLenum>(c.format);
  GLenum type = static_cast<GLenum>(c.type);
  uint32 data_size = GLES2Util::ComputeImageDataSize(
      width, height, format, type, unpack_alignment_);
  const void* pixels = GetImmediateDataAs<const void*>(
      c, data_size, immediate_data_size);
  if (!ValidateGLenumTextureTarget(target)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumTextureFormat(format)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (!ValidateGLenumPixelType(type)) {
    SetGLError(GL_INVALID_ENUM);
    return error::kNoError;
  }
  if (pixels == NULL) {
    return error::kOutOfBounds;
  }
  glTexSubImage2D(
      target, level, xoffset, yoffset, width, height, format, type, pixels);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform1f(
    uint32 immediate_data_size, const gles2::Uniform1f& c) {
  GLint location = static_cast<GLint>(c.location);
  GLfloat x = static_cast<GLfloat>(c.x);
  glUniform1f(location, x);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform1fv(
    uint32 immediate_data_size, const gles2::Uniform1fv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 1);
  const GLfloat* v = GetSharedMemoryAs<const GLfloat*>(
      c.v_shm_id, c.v_shm_offset, data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform1fv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform1fvImmediate(
    uint32 immediate_data_size, const gles2::Uniform1fvImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 1);
  const GLfloat* v = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform1fv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform1i(
    uint32 immediate_data_size, const gles2::Uniform1i& c) {
  GLint location = static_cast<GLint>(c.location);
  GLint x = static_cast<GLint>(c.x);
  glUniform1i(location, x);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform1iv(
    uint32 immediate_data_size, const gles2::Uniform1iv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 1);
  const GLint* v = GetSharedMemoryAs<const GLint*>(
      c.v_shm_id, c.v_shm_offset, data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform1iv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform1ivImmediate(
    uint32 immediate_data_size, const gles2::Uniform1ivImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 1);
  const GLint* v = GetImmediateDataAs<const GLint*>(
      c, data_size, immediate_data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform1iv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform2f(
    uint32 immediate_data_size, const gles2::Uniform2f& c) {
  GLint location = static_cast<GLint>(c.location);
  GLfloat x = static_cast<GLfloat>(c.x);
  GLfloat y = static_cast<GLfloat>(c.y);
  glUniform2f(location, x, y);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform2fv(
    uint32 immediate_data_size, const gles2::Uniform2fv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 2);
  const GLfloat* v = GetSharedMemoryAs<const GLfloat*>(
      c.v_shm_id, c.v_shm_offset, data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform2fv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform2fvImmediate(
    uint32 immediate_data_size, const gles2::Uniform2fvImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 2);
  const GLfloat* v = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform2fv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform2i(
    uint32 immediate_data_size, const gles2::Uniform2i& c) {
  GLint location = static_cast<GLint>(c.location);
  GLint x = static_cast<GLint>(c.x);
  GLint y = static_cast<GLint>(c.y);
  glUniform2i(location, x, y);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform2iv(
    uint32 immediate_data_size, const gles2::Uniform2iv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 2);
  const GLint* v = GetSharedMemoryAs<const GLint*>(
      c.v_shm_id, c.v_shm_offset, data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform2iv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform2ivImmediate(
    uint32 immediate_data_size, const gles2::Uniform2ivImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 2);
  const GLint* v = GetImmediateDataAs<const GLint*>(
      c, data_size, immediate_data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform2iv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform3f(
    uint32 immediate_data_size, const gles2::Uniform3f& c) {
  GLint location = static_cast<GLint>(c.location);
  GLfloat x = static_cast<GLfloat>(c.x);
  GLfloat y = static_cast<GLfloat>(c.y);
  GLfloat z = static_cast<GLfloat>(c.z);
  glUniform3f(location, x, y, z);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform3fv(
    uint32 immediate_data_size, const gles2::Uniform3fv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 3);
  const GLfloat* v = GetSharedMemoryAs<const GLfloat*>(
      c.v_shm_id, c.v_shm_offset, data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform3fv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform3fvImmediate(
    uint32 immediate_data_size, const gles2::Uniform3fvImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 3);
  const GLfloat* v = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform3fv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform3i(
    uint32 immediate_data_size, const gles2::Uniform3i& c) {
  GLint location = static_cast<GLint>(c.location);
  GLint x = static_cast<GLint>(c.x);
  GLint y = static_cast<GLint>(c.y);
  GLint z = static_cast<GLint>(c.z);
  glUniform3i(location, x, y, z);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform3iv(
    uint32 immediate_data_size, const gles2::Uniform3iv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 3);
  const GLint* v = GetSharedMemoryAs<const GLint*>(
      c.v_shm_id, c.v_shm_offset, data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform3iv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform3ivImmediate(
    uint32 immediate_data_size, const gles2::Uniform3ivImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 3);
  const GLint* v = GetImmediateDataAs<const GLint*>(
      c, data_size, immediate_data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform3iv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform4f(
    uint32 immediate_data_size, const gles2::Uniform4f& c) {
  GLint location = static_cast<GLint>(c.location);
  GLfloat x = static_cast<GLfloat>(c.x);
  GLfloat y = static_cast<GLfloat>(c.y);
  GLfloat z = static_cast<GLfloat>(c.z);
  GLfloat w = static_cast<GLfloat>(c.w);
  glUniform4f(location, x, y, z, w);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform4fv(
    uint32 immediate_data_size, const gles2::Uniform4fv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 4);
  const GLfloat* v = GetSharedMemoryAs<const GLfloat*>(
      c.v_shm_id, c.v_shm_offset, data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform4fv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform4fvImmediate(
    uint32 immediate_data_size, const gles2::Uniform4fvImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 4);
  const GLfloat* v = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform4fv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform4i(
    uint32 immediate_data_size, const gles2::Uniform4i& c) {
  GLint location = static_cast<GLint>(c.location);
  GLint x = static_cast<GLint>(c.x);
  GLint y = static_cast<GLint>(c.y);
  GLint z = static_cast<GLint>(c.z);
  GLint w = static_cast<GLint>(c.w);
  glUniform4i(location, x, y, z, w);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform4iv(
    uint32 immediate_data_size, const gles2::Uniform4iv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 4);
  const GLint* v = GetSharedMemoryAs<const GLint*>(
      c.v_shm_id, c.v_shm_offset, data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform4iv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniform4ivImmediate(
    uint32 immediate_data_size, const gles2::Uniform4ivImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLint), 4);
  const GLint* v = GetImmediateDataAs<const GLint*>(
      c, data_size, immediate_data_size);
  if (v == NULL) {
    return error::kOutOfBounds;
  }
  glUniform4iv(location, count, v);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniformMatrix2fv(
    uint32 immediate_data_size, const gles2::UniformMatrix2fv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  GLboolean transpose = static_cast<GLboolean>(c.transpose);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 4);
  const GLfloat* value = GetSharedMemoryAs<const GLfloat*>(
      c.value_shm_id, c.value_shm_offset, data_size);
  if (!ValidateGLbooleanFalse(transpose)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (value == NULL) {
    return error::kOutOfBounds;
  }
  glUniformMatrix2fv(location, count, transpose, value);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniformMatrix2fvImmediate(
    uint32 immediate_data_size, const gles2::UniformMatrix2fvImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  GLboolean transpose = static_cast<GLboolean>(c.transpose);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 4);
  const GLfloat* value = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (!ValidateGLbooleanFalse(transpose)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (value == NULL) {
    return error::kOutOfBounds;
  }
  glUniformMatrix2fv(location, count, transpose, value);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniformMatrix3fv(
    uint32 immediate_data_size, const gles2::UniformMatrix3fv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  GLboolean transpose = static_cast<GLboolean>(c.transpose);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 9);
  const GLfloat* value = GetSharedMemoryAs<const GLfloat*>(
      c.value_shm_id, c.value_shm_offset, data_size);
  if (!ValidateGLbooleanFalse(transpose)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (value == NULL) {
    return error::kOutOfBounds;
  }
  glUniformMatrix3fv(location, count, transpose, value);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniformMatrix3fvImmediate(
    uint32 immediate_data_size, const gles2::UniformMatrix3fvImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  GLboolean transpose = static_cast<GLboolean>(c.transpose);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 9);
  const GLfloat* value = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (!ValidateGLbooleanFalse(transpose)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (value == NULL) {
    return error::kOutOfBounds;
  }
  glUniformMatrix3fv(location, count, transpose, value);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniformMatrix4fv(
    uint32 immediate_data_size, const gles2::UniformMatrix4fv& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  GLboolean transpose = static_cast<GLboolean>(c.transpose);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 16);
  const GLfloat* value = GetSharedMemoryAs<const GLfloat*>(
      c.value_shm_id, c.value_shm_offset, data_size);
  if (!ValidateGLbooleanFalse(transpose)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (value == NULL) {
    return error::kOutOfBounds;
  }
  glUniformMatrix4fv(location, count, transpose, value);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUniformMatrix4fvImmediate(
    uint32 immediate_data_size, const gles2::UniformMatrix4fvImmediate& c) {
  GLint location = static_cast<GLint>(c.location);
  GLsizei count = static_cast<GLsizei>(c.count);
  GLboolean transpose = static_cast<GLboolean>(c.transpose);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 16);
  const GLfloat* value = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (!ValidateGLbooleanFalse(transpose)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  if (value == NULL) {
    return error::kOutOfBounds;
  }
  glUniformMatrix4fv(location, count, transpose, value);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleUseProgram(
    uint32 immediate_data_size, const gles2::UseProgram& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  DoUseProgram(program);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleValidateProgram(
    uint32 immediate_data_size, const gles2::ValidateProgram& c) {
  GLuint program;
  if (!id_manager_->GetServiceId(c.program, &program)) {
    SetGLError(GL_INVALID_VALUE);
    return error::kNoError;
  }
  glValidateProgram(program);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib1f(
    uint32 immediate_data_size, const gles2::VertexAttrib1f& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  GLfloat x = static_cast<GLfloat>(c.x);
  glVertexAttrib1f(indx, x);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib1fv(
    uint32 immediate_data_size, const gles2::VertexAttrib1fv& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 1);
  const GLfloat* values = GetSharedMemoryAs<const GLfloat*>(
      c.values_shm_id, c.values_shm_offset, data_size);
  if (values == NULL) {
    return error::kOutOfBounds;
  }
  glVertexAttrib1fv(indx, values);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib1fvImmediate(
    uint32 immediate_data_size, const gles2::VertexAttrib1fvImmediate& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 1);
  const GLfloat* values = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (values == NULL) {
    return error::kOutOfBounds;
  }
  glVertexAttrib1fv(indx, values);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib2f(
    uint32 immediate_data_size, const gles2::VertexAttrib2f& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  GLfloat x = static_cast<GLfloat>(c.x);
  GLfloat y = static_cast<GLfloat>(c.y);
  glVertexAttrib2f(indx, x, y);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib2fv(
    uint32 immediate_data_size, const gles2::VertexAttrib2fv& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 2);
  const GLfloat* values = GetSharedMemoryAs<const GLfloat*>(
      c.values_shm_id, c.values_shm_offset, data_size);
  if (values == NULL) {
    return error::kOutOfBounds;
  }
  glVertexAttrib2fv(indx, values);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib2fvImmediate(
    uint32 immediate_data_size, const gles2::VertexAttrib2fvImmediate& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 2);
  const GLfloat* values = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (values == NULL) {
    return error::kOutOfBounds;
  }
  glVertexAttrib2fv(indx, values);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib3f(
    uint32 immediate_data_size, const gles2::VertexAttrib3f& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  GLfloat x = static_cast<GLfloat>(c.x);
  GLfloat y = static_cast<GLfloat>(c.y);
  GLfloat z = static_cast<GLfloat>(c.z);
  glVertexAttrib3f(indx, x, y, z);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib3fv(
    uint32 immediate_data_size, const gles2::VertexAttrib3fv& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 3);
  const GLfloat* values = GetSharedMemoryAs<const GLfloat*>(
      c.values_shm_id, c.values_shm_offset, data_size);
  if (values == NULL) {
    return error::kOutOfBounds;
  }
  glVertexAttrib3fv(indx, values);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib3fvImmediate(
    uint32 immediate_data_size, const gles2::VertexAttrib3fvImmediate& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 3);
  const GLfloat* values = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (values == NULL) {
    return error::kOutOfBounds;
  }
  glVertexAttrib3fv(indx, values);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib4f(
    uint32 immediate_data_size, const gles2::VertexAttrib4f& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  GLfloat x = static_cast<GLfloat>(c.x);
  GLfloat y = static_cast<GLfloat>(c.y);
  GLfloat z = static_cast<GLfloat>(c.z);
  GLfloat w = static_cast<GLfloat>(c.w);
  glVertexAttrib4f(indx, x, y, z, w);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib4fv(
    uint32 immediate_data_size, const gles2::VertexAttrib4fv& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 4);
  const GLfloat* values = GetSharedMemoryAs<const GLfloat*>(
      c.values_shm_id, c.values_shm_offset, data_size);
  if (values == NULL) {
    return error::kOutOfBounds;
  }
  glVertexAttrib4fv(indx, values);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleVertexAttrib4fvImmediate(
    uint32 immediate_data_size, const gles2::VertexAttrib4fvImmediate& c) {
  GLuint indx = static_cast<GLuint>(c.indx);
  uint32 data_size =
      ComputeImmediateDataSize(immediate_data_size, 1, sizeof(GLfloat), 4);
  const GLfloat* values = GetImmediateDataAs<const GLfloat*>(
      c, data_size, immediate_data_size);
  if (values == NULL) {
    return error::kOutOfBounds;
  }
  glVertexAttrib4fv(indx, values);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleViewport(
    uint32 immediate_data_size, const gles2::Viewport& c) {
  GLint x = static_cast<GLint>(c.x);
  GLint y = static_cast<GLint>(c.y);
  GLsizei width = static_cast<GLsizei>(c.width);
  GLsizei height = static_cast<GLsizei>(c.height);
  glViewport(x, y, width, height);
  return error::kNoError;
}

error::Error GLES2DecoderImpl::HandleSwapBuffers(
    uint32 immediate_data_size, const gles2::SwapBuffers& c) {
  DoSwapBuffers();
  return error::kNoError;
}

#endif  // GPU_COMMAND_BUFFER_SERVICE_GLES2_CMD_DECODER_AUTOGEN_H_

