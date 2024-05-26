/*
 Copyright (C) 2024 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TextureImage.h"

#include "Macros.h"

#include "kdl/overload.h"
#include "kdl/reflection_impl.h"
#include "kdl/vector_utils.h"

#include "vm/vec_io.h"

namespace TrenchBroom::Assets
{

namespace
{

auto makeTextureLoadedState(
  const size_t width,
  const size_t height,
  const GLenum format,
  std::vector<TextureBuffer> buffers)
{
  const auto compressed = isCompressedFormat(format);
  [[maybe_unused]] const auto bytesPerPixel =
    compressed ? 0U : bytesPerPixelForFormat(format);
  [[maybe_unused]] const auto blockSize = compressed ? blockSizeForFormat(format) : 0U;

  for (size_t level = 0; level < buffers.size(); ++level)
  {
    [[maybe_unused]] const auto mipSize = sizeAtMipLevel(width, height, level);
    [[maybe_unused]] const auto numBytes =
      compressed ? (
        blockSize * std::max(size_t(1), mipSize.x() / 4)
        * std::max(size_t(1), mipSize.y() / 4))
                 : (bytesPerPixel * mipSize.x() * mipSize.y());
    assert(buffers[level].size() >= numBytes);
  }

  return TextureLoadedState{std::move(buffers)};
}

auto uploadTexture(
  const GLenum format,
  const TextureMask mask,
  const std::vector<TextureBuffer>& buffers,
  const size_t width,
  const size_t height)
{
  const auto compressed = isCompressedFormat(format);

  auto textureId = GLuint(0);
  glAssert(glGenTextures(1, &textureId));

  glAssert(glPixelStorei(GL_UNPACK_SWAP_BYTES, false));
  glAssert(glPixelStorei(GL_UNPACK_LSB_FIRST, false));
  glAssert(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
  glAssert(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
  glAssert(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
  glAssert(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

  glAssert(glBindTexture(GL_TEXTURE_2D, textureId));
  glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

  if (mask == TextureMask::On)
  {
    // masked textures don't work well with automatic mipmaps, so we force
    // GL_NEAREST filtering and don't generate any
    glAssert(glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE));
    glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  }
  else if (buffers.size() == 1)
  {
    // generate mipmaps if we don't have any
    glAssert(glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE));
  }
  else
  {
    glAssert(
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, GLint(buffers.size() - 1)));
  }

  // Upload only the first mipmap for masked textures.
  const auto mipmapsToUpload = (mask == TextureMask::On) ? 1u : buffers.size();

  for (size_t j = 0; j < mipmapsToUpload; ++j)
  {
    const auto mipSize = sizeAtMipLevel(width, height, j);

    const auto* data = reinterpret_cast<const GLvoid*>(buffers[j].data());
    if (compressed)
    {
      const auto dataSize = GLsizei(buffers[j].size());

      glAssert(glCompressedTexImage2D(
        GL_TEXTURE_2D,
        GLint(j),
        format,
        GLsizei(mipSize.x()),
        GLsizei(mipSize.y()),
        0,
        dataSize,
        data));
    }
    else
    {
      glAssert(glTexImage2D(
        GL_TEXTURE_2D,
        GLint(j),
        GL_RGBA,
        GLsizei(mipSize.x()),
        GLsizei(mipSize.y()),
        0,
        format,
        GL_UNSIGNED_BYTE,
        data));
    }
  }

  return textureId;
}

void dropTexture(GLuint textureId)
{
  glAssert(glDeleteTextures(1, &textureId));
}

} // namespace

std::ostream& operator<<(std::ostream& lhs, const TextureMask& rhs)
{
  switch (rhs)
  {
  case TextureMask::On:
    lhs << "On";
    break;
  case TextureMask::Off:
    lhs << "Off";
    break;
    switchDefault();
  }
  return lhs;
}

std::ostream& operator<<(std::ostream& lhs, const EmbeddedDefaults& rhs)
{
  std::visit([&](const auto& x) { lhs << x; }, rhs);
  return lhs;
}

kdl_reflect_impl(NoEmbeddedDefaults);
kdl_reflect_impl(Q2EmbeddedDefaults);

kdl_reflect_impl(TextureLoadedState);
kdl_reflect_impl(TextureReadyState);
kdl_reflect_impl(TextureDroppedState);

kdl_reflect_impl(TextureImage);

TextureImage::TextureImage(
  const size_t width,
  const size_t height,
  const Color averageColor,
  const GLenum format,
  const TextureMask mask,
  EmbeddedDefaults embeddedDefaults,
  std::vector<TextureBuffer> buffers)
  : m_width{width}
  , m_height{height}
  , m_averageColor{averageColor}
  , m_format{format}
  , m_mask{mask}
  , m_embeddedDefaults{std::move(embeddedDefaults)}
  , m_state{makeTextureLoadedState(m_width, m_height, m_format, std::move(buffers))}
{
  assert(m_width > 0);
  assert(m_height > 0);
}

TextureImage::TextureImage(
  const size_t width,
  const size_t height,
  const Color averageColor,
  const GLenum format,
  const TextureMask mask,
  EmbeddedDefaults embeddedDefaults,
  TextureBuffer buffer)
  : TextureImage{
    width,
    height,
    averageColor,
    format,
    mask,
    std::move(embeddedDefaults),
    kdl::vec_from(std::move(buffer))}
{
}

TextureImage::TextureImage(size_t width, size_t height)
  : TextureImage{
    width,
    height,
    Color{0, 0, 0, 0},
    GL_RGBA,
    TextureMask::Off,
    NoEmbeddedDefaults{},
    std::vector<TextureBuffer>{}}
{
}

size_t TextureImage::width() const
{
  return m_width;
}

size_t TextureImage::height() const
{
  return m_height;
}

const Color& TextureImage::averageColor() const
{
  return m_averageColor;
}

GLenum TextureImage::format() const
{
  return m_format;
}

TextureMask TextureImage::mask() const
{
  return m_mask;
}

void TextureImage::setMask(const TextureMask mask)
{
  m_mask = mask;
}

const EmbeddedDefaults& TextureImage::embeddedDefaults() const
{
  return m_embeddedDefaults;
}

bool TextureImage::isReady() const
{
  return std::holds_alternative<TextureReadyState>(m_state);
}

void TextureImage::setMode(int minFilter, int magFilter)
{
  if (isReady())
  {
    activate();
    if (m_mask == TextureMask::On)
    {
      // Force GL_NEAREST filtering for masked textures.
      glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
      glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    }
    else
    {
      glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter));
      glAssert(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter));
    }
    deactivate();
  }
}

bool TextureImage::activate() const
{
  return std::visit(
    kdl::overload(
      [](const TextureLoadedState&) { return false; },
      [](const TextureReadyState& readyState) {
        glAssert(glBindTexture(GL_TEXTURE_2D, readyState.textureId));
        return true;
      },
      [](const TextureDroppedState&) { return false; }),
    m_state);
}

bool TextureImage::deactivate() const
{
  if (isReady())
  {
    glAssert(glBindTexture(GL_TEXTURE_2D, 0));
    return true;
  }

  return false;
}

void TextureImage::upload()
{
  m_state = std::visit(
    kdl::overload(
      [&](const TextureLoadedState& textureLoadedState) -> TextureState {
        const auto textureId =
          uploadTexture(m_format, m_mask, textureLoadedState.buffers, m_width, m_height);
        return TextureReadyState{textureId};
      },
      [](TextureReadyState textureReadyState) -> TextureState {
        return textureReadyState;
      },
      [](TextureDroppedState textureDroppedState) -> TextureState {
        return textureDroppedState;
      }),
    std::move(m_state));
}

void TextureImage::drop()
{
  m_state = std::visit(
    kdl::overload(
      [&](const TextureLoadedState&) { return TextureDroppedState{}; },
      [](const TextureReadyState& textureReadyState) {
        dropTexture(textureReadyState.textureId);
        return TextureDroppedState{};
      },
      [](TextureDroppedState textureDroppedState) { return textureDroppedState; }),
    std::move(m_state));
}

const std::vector<TextureBuffer>& TextureImage::buffersIfLoaded() const
{
  static const auto empty = std::vector<TextureBuffer>{};
  return std::visit(
    kdl::overload(
      [](const TextureLoadedState& state) -> const std::vector<TextureBuffer>& {
        return state.buffers;
      },
      [](const TextureReadyState&) -> const std::vector<TextureBuffer>& { return empty; },
      [](const TextureDroppedState&) -> const std::vector<TextureBuffer>& {
        return empty;
      }),
    m_state);
}

} // namespace TrenchBroom::Assets
