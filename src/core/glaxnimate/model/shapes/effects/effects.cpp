/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/model/shapes/effects/effects.hpp"

GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::GaussianBlurEffect)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::DropShadowEffect)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::FillEffect)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::TintEffect)
GLAXNIMATE_OBJECT_IMPL(glaxnimate::model::TritoneEffect)

using namespace glaxnimate;

namespace {

QColor with_opacity(QColor color, qreal opacity)
{
    color.setAlphaF(color.alphaF() * opacity);
    return color;
}

} // namespace

void model::Effect::paint_effect(renderer::Renderer* painter, FrameTime time) const
{
    if ( visible.get() )
        painter->add_effect(effect_at(time));
}

QIcon model::GaussianBlurEffect::static_tree_icon()
{
    return QIcon::fromTheme("blurimage");
}

QString model::GaussianBlurEffect::static_type_name_human()
{
    return i18n("Gaussian Blur");
}

renderer::Effect model::GaussianBlurEffect::effect_at(FrameTime time) const
{
    renderer::Effect effect;
    effect.type = renderer::Effect::GaussianBlur;
    effect.sigma = blurriness.get_at(time) * blur_to_sigma;
    return effect;
}

QIcon model::DropShadowEffect::static_tree_icon()
{
    return QIcon::fromTheme("object-shadow");
}

QString model::DropShadowEffect::static_type_name_human()
{
    return i18n("Drop Shadow");
}

renderer::Effect model::DropShadowEffect::effect_at(FrameTime time) const
{
    renderer::Effect effect;
    effect.type = renderer::Effect::DropShadow;
    effect.colors[0] = with_opacity(color.get_at(time), opacity.get_at(time));
    effect.angle = angle.get_at(time);
    effect.distance = distance.get_at(time);
    effect.sigma = blurriness.get_at(time) * blur_to_sigma;
    return effect;
}

QIcon model::FillEffect::static_tree_icon()
{
    return QIcon::fromTheme("fill-color");
}

QString model::FillEffect::static_type_name_human()
{
    return i18n("Fill Effect");
}

renderer::Effect model::FillEffect::effect_at(FrameTime time) const
{
    renderer::Effect effect;
    effect.type = renderer::Effect::Fill;
    effect.colors[0] = with_opacity(color.get_at(time), opacity.get_at(time));
    return effect;
}

QIcon model::TintEffect::static_tree_icon()
{
    return QIcon::fromTheme("color-management");
}

QString model::TintEffect::static_type_name_human()
{
    return i18n("Tint");
}

renderer::Effect model::TintEffect::effect_at(FrameTime time) const
{
    renderer::Effect effect;
    effect.type = renderer::Effect::Tint;
    effect.colors[0] = black.get_at(time);
    effect.colors[1] = white.get_at(time);
    effect.amount = amount.get_at(time);
    return effect;
}

QIcon model::TritoneEffect::static_tree_icon()
{
    return QIcon::fromTheme("color-management");
}

QString model::TritoneEffect::static_type_name_human()
{
    return i18n("Tritone");
}

renderer::Effect model::TritoneEffect::effect_at(FrameTime time) const
{
    renderer::Effect effect;
    effect.type = renderer::Effect::Tritone;
    effect.colors[0] = shadows.get_at(time);
    effect.colors[1] = midtones.get_at(time);
    effect.colors[2] = highlights.get_at(time);
    effect.amount = blend.get_at(time);
    return effect;
}
