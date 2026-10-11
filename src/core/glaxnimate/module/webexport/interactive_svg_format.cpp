/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interactive_svg_format.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include "glaxnimate/io/svg/svg_renderer.hpp"
#include "glaxnimate/model/assets/composition.hpp"
#include "svg_player_js.hpp"

using namespace glaxnimate;

std::unique_ptr<settings::SettingsGroup> webexport::InteractiveSvgFormat::save_settings(model::Composition*) const
{
    QVariantMap triggers;
    triggers[i18n("On load")] = "load";
    triggers[i18n("On hover")] = "hover";
    triggers[i18n("On click")] = "click";
    triggers[i18n("When scrolled into view")] = "scroll";
    triggers[i18n("From JavaScript only")] = "programmatic";

    QVariantMap directions;
    directions[i18n("Forward")] = 1;
    directions[i18n("Backward")] = -1;

    QVariantMap hover_out;
    hover_out[i18n("Pause")] = "freeze";
    hover_out[i18n("Return to start")] = "reset";
    hover_out[i18n("Play backward")] = "reverse";
    hover_out[i18n("Keep playing")] = "none";

    QVariantMap click_again;
    click_again[i18n("Pause")] = "freeze";
    click_again[i18n("Restart")] = "restart";
    click_again[i18n("Play backward")] = "reverse";
    click_again[i18n("Keep playing")] = "none";

    return std::make_unique<settings::SettingsGroup>(settings::SettingList{
        settings::Setting("trigger", i18n("Start"), i18n("What starts playback"),
            settings::Setting::String, QString("load"), triggers),
        settings::Setting("direction", i18n("Direction"), i18n("Initial playback direction"),
            settings::Setting::Int, 1, directions),
        settings::Setting("alternate", i18n("Alternate"), i18n("Reverse direction on each repeat"), false),
        settings::Setting("iterations", i18n("Iterations"), i18n("Number of times to play, 0 to loop forever"), 1, 0, 1000),
        settings::Setting("speed", i18n("Speed"), i18n("Playback speed multiplier"), 1.f, 0.01f, 100.f),
        settings::Setting("hover_out", i18n("Hover end"), i18n("What happens when the pointer leaves"),
            settings::Setting::String, QString("freeze"), hover_out),
        settings::Setting("click_again", i18n("Second click"), i18n("What happens on a click after playback started"),
            settings::Setting::String, QString("restart"), click_again),
        settings::Setting("scroll_threshold", i18n("Scroll threshold"), i18n("Percentage of the image that must be visible to start"), 25, 1, 100),
        settings::Setting("root_id", i18n("Element ID"), i18n("ID for the root SVG element, used to access the player"), QString("glaxnimate-player")),
    });
}

bool webexport::InteractiveSvgFormat::on_save(QIODevice& file, const QString&, model::Composition* comp, const QVariantMap& options)
{
    const float fps = comp->fps.get();
    const float first_frame = comp->animation->first_frame.get();
    const float last_frame = comp->animation->last_frame.get();
    if ( last_frame <= first_frame || fps <= 0 )
    {
        error(i18n("Interactive SVG requires an animation with at least one frame"));
        return false;
    }

    const QString root_id = options.value("root_id", QString("glaxnimate-player")).toString();
    static const QRegularExpression valid_id("^[A-Za-z_][A-Za-z0-9_.-]*$");
    if ( !valid_id.match(root_id).hasMatch() )
    {
        error(i18n("The element ID must start with a letter or underscore and contain only letters, digits, '_', '.', or '-'"));
        return false;
    }

    const QString trigger = options.value("trigger", QString("load")).toString();
    const QString hover_out = options.value("hover_out", QString("freeze")).toString();
    const QString click_again = options.value("click_again", QString("restart")).toString();
    const int iterations = options.value("iterations", 1).toInt();
    const double speed = options.value("speed", 1.).toDouble();
    const int scroll_threshold = options.value("scroll_threshold", 25).toInt();
    static const QStringList triggers = {"load", "hover", "click", "scroll", "programmatic"};
    static const QStringList end_actions = {"freeze", "reset", "reverse", "none"};
    static const QStringList click_actions = {"freeze", "restart", "reverse", "none"};
    if ( !triggers.contains(trigger) || !end_actions.contains(hover_out) || !click_actions.contains(click_again) )
    {
        error(i18n("Unknown playback trigger or action"));
        return false;
    }
    if ( iterations < 0 || iterations > 1000 || !(speed > 0 && speed <= 100) || scroll_threshold < 1 || scroll_threshold > 100 )
    {
        error(i18n("Playback iterations, speed, or scroll threshold are out of range"));
        return false;
    }

    QJsonObject player_options{
        {"root_id", root_id},
        {"trigger", trigger},
        {"direction", options.value("direction", 1).toInt() < 0 ? -1 : 1},
        {"alternate", options.value("alternate", false).toBool()},
        {"iterations", iterations},
        {"speed", speed},
        {"hover_out", hover_out},
        {"click_again", click_again},
        {"scroll_threshold", scroll_threshold},
        {"offset_ms", first_frame / fps * 1000},
        {"duration_ms", (last_frame - first_frame) / fps * 1000},
    };

    io::svg::SvgRenderer renderer(io::svg::SMIL, io::svg::CssFontType::FontFace);
    renderer.write_main(comp, comp->document()->current_time());
    QDomDocument dom = renderer.dom();
    QDomElement svg = dom.documentElement();
    svg.setAttribute("id", root_id);

    QString script = QString::fromUtf8(webexport::svg_player_js);
    script.replace("__OPTIONS__", QString::fromUtf8(QJsonDocument(player_options).toJson(QJsonDocument::Compact)));
    QDomElement script_element = dom.createElement("script");
    script_element.setAttribute("type", "application/ecmascript");
    script_element.appendChild(dom.createCDATASection(script));
    svg.appendChild(script_element);

    file.write(dom.toByteArray(4));
    return true;
}
