// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_TEXTURE_MANAGER_H_
#define GPU_COMMAND_BUFFER_SERVICE_TEXTURE_MANAGER_H_

#include <map>
#include <vector>
#include "base/basictypes.h"
#include "base/logging.h"
#include "base/ref_counted.h"
#include "gpu/command_buffer/service/gl_utils.h"

namespace gpu {
namespace gles2 {

// This class keeps track of the textures and their sizes so we can do NPOT and
// texture complete checking.
//
// NOTE: To support shared resources an instance of this class will need to be
// shared by multiple GLES2Decoders.
class TextureManager {
 public:
  // Info about Textures currently in the system.
  class TextureInfo : public base::RefCounted<TextureInfo> {
   public:
    typedef scoped_refptr<TextureInfo> Ref;

    explicit TextureInfo(GLuint service_id)
        : service_id_(service_id),
          deleted_(false),
          target_(0),
          min_filter_(GL_NEAREST_MIPMAP_LINEAR),
          mag_filter_(GL_LINEAR),
          wrap_s_(GL_REPEAT),
          wrap_t_(GL_REPEAT),
          max_level_set_(-1),
          texture_complete_(false),
          cube_complete_(false),
          npot_(false) {
    }

    // True if this texture meets all the GLES2 criteria for rendering.
    // See section 3.8.2 of the GLES2 spec.
    bool CanRender() const;

    // The service side OpenGL id of the texture.
    GLuint service_id() const {
      return service_id_;
    }

    // Returns the target this texure was first bound to or 0 if it has not
    // been bound. Once a texture is bound to a specific target it can never be
    // bound to a different target.
    GLenum target() const {
      return target_;
    }

    // In GLES2 "texture complete" means it has all required mips for filtering
    // down to a 1x1 pixel texture, they are in the correct order, they are all
    // the same format.
    bool texture_complete() const {
      return texture_complete_;
    }

    // In GLES2 "cube complete" means all 6 faces level 0 are defined, all the
    // same format, all the same dimensions and all width = height.
    bool cube_complete() const {
      return cube_complete_;
    }

    // Whether or not this texture is a non-power-of-two texture.
    bool npot() const {
      return npot_;
    }

    // Returns true if mipmaps can be generated by GL.
    bool CanGenerateMipmaps() const;

    // Get the width and height for a particular level. Returns false if level
    // does not exist.
    bool GetLevelSize(
        GLint face, GLint level, GLsizei* width, GLsizei* height) const;

    bool IsDeleted() const {
      return deleted_;
    }

   private:
    friend class TextureManager;
    friend class base::RefCounted<TextureInfo>;

    ~TextureInfo() { }

    struct LevelInfo {
      LevelInfo()
         : valid(false),
           internal_format(0),
           width(0),
           height(0),
           depth(0),
           border(0),
           format(0),
           type(0) {
      }

      bool valid;
      GLint internal_format;
      GLsizei width;
      GLsizei height;
      GLsizei depth;
      GLint border;
      GLenum format;
      GLenum type;
    };

    // Set the info for a particular level.
    void SetLevelInfo(
        GLenum target,
        GLint level,
        GLint internal_format,
        GLsizei width,
        GLsizei height,
        GLsizei depth,
        GLint border,
        GLenum format,
        GLenum type);

    // Sets a texture parameter.
    // TODO(gman): Expand to SetParameteri,f,iv,fv
    void SetParameter(GLenum pname, GLint param);

    // Makes each of the mip levels as though they were generated.
    bool MarkMipmapsGenerated();

    void MarkAsDeleted() {
      service_id_ = 0;
      deleted_ = true;
    }

    bool NeedsMips() const {
      return min_filter_ != GL_NEAREST && min_filter_ != GL_LINEAR;
    }

    // Sets the TextureInfo's target
    // Parameters:
    //   target: GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP
    //   max_levels: The maximum levels this type of target can have.
    void SetTarget(GLenum target, GLint max_levels) {
      DCHECK_EQ(0u, target_);  // you can only set this once.
      target_ = target;
      size_t num_faces = (target == GL_TEXTURE_2D) ? 1 : 6;
      level_infos_.resize(num_faces);
      for (size_t ii = 0; ii < num_faces; ++ii) {
        level_infos_[ii].resize(max_levels);
      }
    }

    // Update info about this texture.
    void Update();

    // Info about each face and level of texture.
    std::vector<std::vector<LevelInfo> > level_infos_;

    // The id of the texure
    GLuint service_id_;

    // Whether this texture has been deleted.
    bool deleted_;

    // The target. 0 if unset, otherwise GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP.
    GLenum target_;

    // Texture parameters.
    GLenum min_filter_;
    GLenum mag_filter_;
    GLenum wrap_s_;
    GLenum wrap_t_;

    // The maximum level that has been set.
    GLint max_level_set_;

    // Whether or not this texture is "texture complete"
    bool texture_complete_;

    // Whether or not this texture is "cube complete"
    bool cube_complete_;

    // Whether or not this texture is non-power-of-two
    bool npot_;

    DISALLOW_COPY_AND_ASSIGN(TextureInfo);
  };

  TextureManager(GLsizei max_texture_size,
                 GLsizei max_cube_map_texture_size);
  ~TextureManager();

  // Must call before destruction.
  void Destroy(bool have_context);

  // Returns the maximum number of levels.
  GLint MaxLevelsForTarget(GLenum target) const {
    return (target == GL_TEXTURE_2D) ? max_levels_ : max_cube_map_levels_;
  }

  // Returns the maximum size.
  GLsizei MaxSizeForTarget(GLenum target) const {
    return (target == GL_TEXTURE_2D) ? max_texture_size_ :
                                       max_cube_map_texture_size_;
  }

  // Checks if a dimensions are valid for a given target.
  bool ValidForTarget(
      GLenum target, GLint level,
      GLsizei width, GLsizei height, GLsizei depth);

  // Sets the TextureInfo's target
  // Parameters:
  //   target: GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP
  //   max_levels: The maximum levels this type of target can have.
  void SetInfoTarget(TextureInfo* info, GLenum target) {
    DCHECK(info);
    info->SetTarget(target, MaxLevelsForTarget(target));
  }

  // Set the info for a particular level in a TexureInfo.
  void SetLevelInfo(
      TextureInfo* info,
      GLenum target,
      GLint level,
      GLint internal_format,
      GLsizei width,
      GLsizei height,
      GLsizei depth,
      GLint border,
      GLenum format,
      GLenum type);

  // Sets a texture parameter of a TextureInfo
  // TODO(gman): Expand to SetParameteri,f,iv,fv
  void SetParameter(TextureInfo* info, GLenum pname, GLint param);

  // Makes each of the mip levels as though they were generated.
  bool MarkMipmapsGenerated(TextureInfo* info);

  // Creates a new texture info.
  TextureInfo* CreateTextureInfo(GLuint client_id, GLuint service_id);

  // Gets the texture info for the given texture.
  TextureInfo* GetTextureInfo(GLuint client_id);

  // Removes a texture info.
  void RemoveTextureInfo(GLuint client_id);

  // Gets a client id for a given service id.
  bool GetClientId(GLuint service_id, GLuint* client_id) const;

  TextureInfo* GetDefaultTextureInfo(GLenum target) {
    return target == GL_TEXTURE_2D ? default_texture_2d_ :
                                     default_texture_cube_map_;
  }

  bool HaveUnrenderableTextures() const {
    return num_unrenderable_textures_ > 0;
  }

 private:
  // Info for each texture in the system.
  // TODO(gman): Choose a faster container.
  typedef std::map<GLuint, TextureInfo::Ref> TextureInfoMap;
  TextureInfoMap texture_infos_;

  GLsizei max_texture_size_;
  GLsizei max_cube_map_texture_size_;
  GLint max_levels_;
  GLint max_cube_map_levels_;

  int num_unrenderable_textures_;

  // The default textures for each target (texture name = 0)
  TextureInfo::Ref default_texture_2d_;
  TextureInfo::Ref default_texture_cube_map_;

  DISALLOW_COPY_AND_ASSIGN(TextureManager);
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_TEXTURE_MANAGER_H_

