/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/model/shapes/shape.hpp"
#include "glaxnimate/renderer/renderer.hpp"

namespace glaxnimate::model {

/**
 * \brief Base class for post-processing effects
 *
 * An effect draws nothing on its own. It applies to everything its parent
 * group or layer paints, after the parent's other children are drawn.
 * Parameters use the same units as Lottie and After Effects.
 */
class Effect : public ShapeElement
{
    Q_OBJECT

public:
    /// Converts Lottie blurriness to the standard deviation of a Gaussian
    static constexpr qreal blur_to_sigma = 0.3;

    using ShapeElement::ShapeElement;

    void add_shapes(FrameTime, math::bezier::MultiBezier&, const QTransform&) const override {}
    QRectF local_bounding_rect(FrameTime) const override { return {}; }
    void paint(renderer::Renderer*, FrameTime, PaintMode, model::Modifier*) const override {}
    void paint_effect(renderer::Renderer* painter, FrameTime time) const override;

    /**
     * \brief Renderer description of this effect at the given time
     */
    virtual renderer::Effect effect_at(FrameTime time) const = 0;

protected:
    math::bezier::MultiBezier to_painter_path_impl(FrameTime) const override { return {}; }
};

class GaussianBlurEffect : public StaticOverrides<GaussianBlurEffect, Effect>
{
    GLAXNIMATE_OBJECT(GaussianBlurEffect)
    GLAXNIMATE_ANIMATABLE(float, blurriness, 10, {}, 0, 10000)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();
    renderer::Effect effect_at(FrameTime time) const override;
};

class DropShadowEffect : public StaticOverrides<DropShadowEffect, Effect>
{
    GLAXNIMATE_OBJECT(DropShadowEffect)
    GLAXNIMATE_ANIMATABLE(QColor, color, QColor(0, 0, 0))
    GLAXNIMATE_ANIMATABLE(float, opacity, 0.5, {}, 0, 1, PropertyTraits::Percent)
    /// Direction in degrees, clockwise from up
    GLAXNIMATE_ANIMATABLE(float, angle, 135)
    GLAXNIMATE_ANIMATABLE(float, distance, 5, {}, 0, 10000)
    GLAXNIMATE_ANIMATABLE(float, blurriness, 10, {}, 0, 10000)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();
    renderer::Effect effect_at(FrameTime time) const override;
};

/**
 * \brief Replaces the color of everything painted, keeping its alpha
 */
class FillEffect : public StaticOverrides<FillEffect, Effect>
{
    GLAXNIMATE_OBJECT(FillEffect)
    GLAXNIMATE_ANIMATABLE(QColor, color, QColor(255, 0, 0))
    GLAXNIMATE_ANIMATABLE(float, opacity, 1, {}, 0, 1, PropertyTraits::Percent)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();
    renderer::Effect effect_at(FrameTime time) const override;
};

/**
 * \brief Maps dark colors toward one color and light colors toward another
 */
class TintEffect : public StaticOverrides<TintEffect, Effect>
{
    GLAXNIMATE_OBJECT(TintEffect)
    GLAXNIMATE_ANIMATABLE(QColor, black, QColor(0, 0, 0))
    GLAXNIMATE_ANIMATABLE(QColor, white, QColor(255, 255, 255))
    GLAXNIMATE_ANIMATABLE(float, amount, 1, {}, 0, 1, PropertyTraits::Percent)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();
    renderer::Effect effect_at(FrameTime time) const override;
};

/**
 * \brief Maps shadows, midtones, and highlights to three colors
 */
class TritoneEffect : public StaticOverrides<TritoneEffect, Effect>
{
    GLAXNIMATE_OBJECT(TritoneEffect)
    GLAXNIMATE_ANIMATABLE(QColor, highlights, QColor(255, 255, 255))
    GLAXNIMATE_ANIMATABLE(QColor, midtones, QColor(128, 128, 128))
    GLAXNIMATE_ANIMATABLE(QColor, shadows, QColor(0, 0, 0))
    /// How much of the original colors to keep
    GLAXNIMATE_ANIMATABLE(float, blend, 0, {}, 0, 1, PropertyTraits::Percent)

public:
    using Ctor::Ctor;

    static QIcon static_tree_icon();
    static QString static_type_name_human();
    renderer::Effect effect_at(FrameTime time) const override;
};

} // namespace glaxnimate::model
