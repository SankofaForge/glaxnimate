/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/io/base.hpp"

namespace glaxnimate::webexport {

/**
 * \brief Animated SVG with an embedded script that controls playback
 *
 * The script pauses the SMIL timeline and plays it on load, hover, click,
 * scroll, or through a JavaScript API on the root element.
 */
class InteractiveSvgFormat : public io::ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "svg_interactive"; }
    QString name() const override { return i18n("Interactive SVG"); }
    QStringList extensions(Direction) const override { return {"svg"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }
    // Plain SVG stays the default for the .svg extension
    int priority() const override { return 50; }
    std::unique_ptr<settings::SettingsGroup> save_settings(model::Composition* comp) const override;

protected:
    bool on_save(QIODevice& file, const QString& filename, model::Composition* comp, const QVariantMap& setting_values) override;
};

} // namespace glaxnimate::webexport
