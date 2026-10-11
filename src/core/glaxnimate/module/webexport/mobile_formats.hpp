/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/io/base.hpp"

namespace glaxnimate::webexport {

/**
 * \brief Source code for a mobile component that plays the animation as Lottie
 *
 * The Lottie data is embedded in the source, and playback uses the platform's
 * maintained Lottie library.
 */
class MobileComponentFormat : public io::ImportExport
{
public:
    bool can_save() const override { return true; }
    bool can_open() const override { return false; }
    std::unique_ptr<settings::SettingsGroup> save_settings(model::Composition* comp) const override;

protected:
    bool on_save(QIODevice& file, const QString& filename, model::Composition* comp, const QVariantMap& setting_values) override;

    /**
     * \brief Writes the component source
     * \param name Component name, starting with an uppercase letter
     * \param json Lottie JSON for the animation
     */
    virtual QByteArray component(const QString& name, const QByteArray& json, bool autoplay, bool loop) const = 0;
};

class ReactNativeFormat : public MobileComponentFormat
{
public:
    QString slug() const override { return "react_native"; }
    QString name() const override { return i18n("React Native Component"); }
    QStringList extensions(Direction) const override { return {"js", "jsx"}; }

protected:
    QByteArray component(const QString& name, const QByteArray& json, bool autoplay, bool loop) const override;
};

class FlutterFormat : public MobileComponentFormat
{
public:
    QString slug() const override { return "flutter_widget"; }
    QString name() const override { return i18n("Flutter Widget"); }
    QStringList extensions(Direction) const override { return {"dart"}; }

protected:
    QByteArray component(const QString& name, const QByteArray& json, bool autoplay, bool loop) const override;
};

} // namespace glaxnimate::webexport
