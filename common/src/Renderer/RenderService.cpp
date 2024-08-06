/*
 Copyright (C) 2010-2017 Kristian Duske

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

#include "RenderService.h"

#include "AttrString.h"
#include "PreferenceManager.h"
#include "Preferences.h"
#include "Renderer/Camera.h"
#include "Renderer/FontDescriptor.h"
#include "Renderer/IndexRangeMapBuilder.h"
#include "Renderer/IndexRangeRenderer.h"
#include "Renderer/PointHandleRenderer.h"
#include "Renderer/PrimitiveRenderer.h"
#include "Renderer/RenderBatch.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RenderUtils.h"
#include "Renderer/TextAnchor.h"
#include "Renderer/TextRenderer.h"

#include "vm/forward.h"
#include "vm/polygon.h"
#include "vm/segment.h"
#include "vm/vec.h"
#include "vm/vec_ext.h"

namespace TrenchBroom::Renderer
{
namespace
{

Renderer::FontDescriptor makeRenderServiceFont()
{
  return {
    pref(Preferences::RendererFontPath()),
    size_t(pref(Preferences::RendererFontSize)),
  };
}

class HeadsUpTextAnchor : public TextAnchor
{
private:
  vm::vec3f offset(const Camera& camera, const vm::vec2f& size) const override
  {
    return getOffset(camera) - vm::vec3f{size.x() / 2.0f, size.y(), 0};
  }

  vm::vec3f position(const Camera& camera) const override
  {
    return camera.unproject(getOffset(camera));
  }

  vm::vec3f getOffset(const Camera& camera) const
  {
    const auto w = float(camera.viewport().width);
    const auto h = float(camera.viewport().height);
    return vm::vec3f{w / 2.0f, h - 20.0f, 0.0f};
  }
};

} // namespace

RenderService::RenderService(RenderContext& renderContext, RenderBatch& renderBatch)
  : m_renderContext{renderContext}
  , m_renderBatch{renderBatch}
  , m_textRenderer{std::make_unique<TextRenderer>(makeRenderServiceFont())}
  , m_pointHandleRenderer{std::make_unique<PointHandleRenderer>()}
  , m_primitiveRenderer{std::make_unique<PrimitiveRenderer>()}
  , m_foregroundColor{1, 1, 1, 1}
  , m_backgroundColor{0, 0, 0, 1}
  , m_lineWidth{1.0f}
  , m_occlusionPolicy{PrimitiveRendererOcclusionPolicy::Transparent}
  , m_cullingPolicy{PrimitiveRendererCullingPolicy::CullBackfaces}
{
}

RenderService::~RenderService()
{
  flush();
}

void RenderService::setForegroundColor(const Color& foregroundColor)
{
  m_foregroundColor = foregroundColor;
}

void RenderService::setBackgroundColor(const Color& backgroundColor)
{
  m_backgroundColor = backgroundColor;
}

void RenderService::setLineWidth(const float lineWidth)
{
  m_lineWidth = lineWidth;
}

void RenderService::setShowOccludedObjects()
{
  m_occlusionPolicy = PrimitiveRendererOcclusionPolicy::Show;
}

void RenderService::setShowOccludedObjectsTransparent()
{
  m_occlusionPolicy = PrimitiveRendererOcclusionPolicy::Transparent;
}

void RenderService::setHideOccludedObjects()
{
  m_occlusionPolicy = PrimitiveRendererOcclusionPolicy::Hide;
}

void RenderService::setShowBackfaces()
{
  m_cullingPolicy = PrimitiveRendererCullingPolicy::ShowBackfaces;
}

void RenderService::setCullBackfaces()
{
  m_cullingPolicy = PrimitiveRendererCullingPolicy::CullBackfaces;
}

void RenderService::renderString(const AttrString& string, const vm::vec3f& position)
{
  renderString(
    string, SimpleTextAnchor{position, TextAlignment::Bottom, vm::vec2f{0, 16}});
}

void RenderService::renderString(const AttrString& string, const TextAnchor& position)
{
  if (m_occlusionPolicy != PrimitiveRendererOcclusionPolicy::Hide)
  {
    m_textRenderer->renderStringOnTop(
      m_renderContext, m_foregroundColor, m_backgroundColor, string, position);
  }
  else
  {
    m_textRenderer->renderString(
      m_renderContext, m_foregroundColor, m_backgroundColor, string, position);
  }
}

void RenderService::renderHeadsUp(const AttrString& string)
{
  m_textRenderer->renderStringOnTop(
    m_renderContext, m_foregroundColor, m_backgroundColor, string, HeadsUpTextAnchor{});
}

void RenderService::renderString(const std::string& string, const vm::vec3f& position)
{
  renderString(AttrString{string}, position);
}

void RenderService::renderString(const std::string& string, const TextAnchor& position)
{
  renderString(AttrString{string}, position);
}

void RenderService::renderHeadsUp(const std::string& string)
{
  renderHeadsUp(AttrString{string});
}

void RenderService::renderHandles(const std::vector<vm::vec3f>& positions)
{
  for (const auto& position : positions)
  {
    renderHandle(position);
  }
}

void RenderService::renderHandle(const vm::vec3f& position)
{
  m_pointHandleRenderer->addPoint(m_foregroundColor, position);
}

void RenderService::renderHandleHighlight(const vm::vec3f& position)
{
  m_pointHandleRenderer->addHighlight(m_foregroundColor, position);
}

void RenderService::renderHandles(const std::vector<vm::segment3f>& positions)
{
  for (const auto& position : positions)
  {
    renderHandle(position);
  }
}

void RenderService::renderHandle(const vm::segment3f& position)
{
  m_primitiveRenderer->renderLine(
    m_foregroundColor, m_lineWidth, m_occlusionPolicy, position.start(), position.end());
  renderHandle(position.center());
}

void RenderService::renderHandleHighlight(const vm::segment3f& position)
{
  m_primitiveRenderer->renderLine(
    m_foregroundColor,
    2.0f * m_lineWidth,
    m_occlusionPolicy,
    position.start(),
    position.end());
  renderHandleHighlight(position.center());
}

void RenderService::renderHandles(const std::vector<vm::polygon3f>& positions)
{
  for (const auto& position : positions)
  {
    renderHandle(position);
  }
}

void RenderService::renderHandle(const vm::polygon3f& position)
{
  setShowBackfaces();
  m_primitiveRenderer->renderFilledPolygon(
    mixAlpha(m_foregroundColor, 0.07f),
    m_occlusionPolicy,
    m_cullingPolicy,
    position.vertices());
  renderHandle(position.center());
  setCullBackfaces();
}

void RenderService::renderHandleHighlight(const vm::polygon3f& position)
{
  m_primitiveRenderer->renderPolygon(
    m_foregroundColor, 2.0f * m_lineWidth, m_occlusionPolicy, position.vertices());
  renderHandleHighlight(position.center());
}

void RenderService::renderLine(const vm::vec3f& start, const vm::vec3f& end)
{
  m_primitiveRenderer->renderLine(
    m_foregroundColor, m_lineWidth, m_occlusionPolicy, start, end);
}

void RenderService::renderLines(const std::vector<vm::vec3f>& positions)
{
  m_primitiveRenderer->renderLines(
    m_foregroundColor, m_lineWidth, m_occlusionPolicy, positions);
}

void RenderService::renderLineStrip(const std::vector<vm::vec3f>& positions)
{
  m_primitiveRenderer->renderLineStrip(
    m_foregroundColor, m_lineWidth, m_occlusionPolicy, positions);
}

void RenderService::renderCoordinateSystem(const vm::bbox3f& bounds)
{
  const auto& x = pref(Preferences::XAxisColor);
  const auto& y = pref(Preferences::YAxisColor);
  const auto& z = pref(Preferences::ZAxisColor);

  if (m_renderContext.render2D())
  {
    const auto& camera = m_renderContext.camera();
    switch (vm::find_abs_max_component(camera.direction()))
    {
    case vm::axis::x:
      m_primitiveRenderer->renderCoordinateSystemYZ(
        y, z, m_lineWidth, m_occlusionPolicy, bounds);
      break;
    case vm::axis::y:
      m_primitiveRenderer->renderCoordinateSystemXZ(
        x, z, m_lineWidth, m_occlusionPolicy, bounds);
      break;
    default:
      m_primitiveRenderer->renderCoordinateSystemXY(
        x, y, m_lineWidth, m_occlusionPolicy, bounds);
      break;
    }
  }
  else
  {
    m_primitiveRenderer->renderCoordinateSystem3D(
      x, y, z, m_lineWidth, m_occlusionPolicy, bounds);
  }
}

void RenderService::renderPolygonOutline(const std::vector<vm::vec3f>& positions)
{
  m_primitiveRenderer->renderPolygon(
    m_foregroundColor, m_lineWidth, m_occlusionPolicy, positions);
}

void RenderService::renderFilledPolygon(const std::vector<vm::vec3f>& positions)
{
  m_primitiveRenderer->renderFilledPolygon(
    m_foregroundColor, m_occlusionPolicy, m_cullingPolicy, positions);
}

void RenderService::renderBounds(const vm::bbox3f& bounds)
{
  const auto p1 = vm::vec3f{bounds.min.x(), bounds.min.y(), bounds.min.z()};
  const auto p2 = vm::vec3f{bounds.min.x(), bounds.min.y(), bounds.max.z()};
  const auto p3 = vm::vec3f{bounds.min.x(), bounds.max.y(), bounds.min.z()};
  const auto p4 = vm::vec3f{bounds.min.x(), bounds.max.y(), bounds.max.z()};
  const auto p5 = vm::vec3f{bounds.max.x(), bounds.min.y(), bounds.min.z()};
  const auto p6 = vm::vec3f{bounds.max.x(), bounds.min.y(), bounds.max.z()};
  const auto p7 = vm::vec3f{bounds.max.x(), bounds.max.y(), bounds.min.z()};
  const auto p8 = vm::vec3f{bounds.max.x(), bounds.max.y(), bounds.max.z()};

  const auto positions = std::vector{
    p1, p2, p1, p3, p1, p5, p2, p4, p2, p6, p3, p4,
    p3, p7, p4, p8, p5, p6, p5, p7, p6, p8, p7, p8,
  };

  renderLines(positions);
}

void RenderService::renderCircle(
  const vm::vec3f& position,
  const vm::axis::type normal,
  const size_t segments,
  const float radius,
  const vm::vec3f& startAxis,
  const vm::vec3f& endAxis)
{
  const auto [startAngle, angleLength] = startAngleAndLength(normal, startAxis, endAxis);
  renderCircle(position, normal, segments, radius, startAngle, angleLength);
}

void RenderService::renderCircle(
  const vm::vec3f& position,
  const vm::axis::type normal,
  const size_t segments,
  const float radius,
  const float startAngle,
  const float angleLength)
{
  const auto positions =
    circle2D(radius, normal, startAngle, angleLength, segments) + position;
  m_primitiveRenderer->renderLineStrip(
    m_foregroundColor, m_lineWidth, m_occlusionPolicy, positions);
}

void RenderService::renderFilledCircle(
  const vm::vec3f& position,
  const vm::axis::type normal,
  const size_t segments,
  const float radius,
  const vm::vec3f& startAxis,
  const vm::vec3f& endAxis)
{
  const auto [startAngle, angleLength] = startAngleAndLength(normal, startAxis, endAxis);
  renderFilledCircle(position, normal, segments, radius, startAngle, angleLength);
}

void RenderService::renderFilledCircle(
  const vm::vec3f& position,
  const vm::axis::type normal,
  const size_t segments,
  const float radius,
  const float startAngle,
  const float angleLength)
{
  const auto positions =
    circle2D(radius, normal, startAngle, angleLength, segments) + position;
  m_primitiveRenderer->renderFilledPolygon(
    m_foregroundColor, m_occlusionPolicy, m_cullingPolicy, positions);
}

void RenderService::flush()
{
  m_renderBatch.addOneShot(m_primitiveRenderer.release());
  m_renderBatch.addOneShot(m_pointHandleRenderer.release());
  m_renderBatch.addOneShot(m_textRenderer.release());
}
} // namespace TrenchBroom::Renderer
