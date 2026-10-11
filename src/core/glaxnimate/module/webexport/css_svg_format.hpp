/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/io/base.hpp"

namespace glaxnimate::webexport {

/**
 * \brief Animated SVG driven by CSS keyframes instead of SMIL
 *
 * Each SMIL animation from the SVG renderer becomes a CSS animation with the
 * same timing and easing. Animations that CSS cannot express keep their SMIL
 * element and produce a warning.
 */
class CssSvgFormat : public io::ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "svg_css"; }
    QString name() const override { return i18n("CSS Animated SVG"); }
    QStringList extensions(Direction) const override { return {"svg"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }
    // Plain SVG stays the default for the .svg extension
    int priority() const override { return 40; }
    std::unique_ptr<settings::SettingsGroup> save_settings(model::Composition* comp) const override;

protected:
    bool on_save(QIODevice& file, const QString& filename, model::Composition* comp, const QVariantMap& setting_values) override;
};

} // namespace glaxnimate::webexport
