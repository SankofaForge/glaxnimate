/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "css_svg_format.hpp"

#include <map>
#include <set>
#include <vector>

#include <QRegularExpression>

#include "glaxnimate/io/svg/svg_renderer.hpp"
#include "glaxnimate/model/assets/composition.hpp"

using namespace glaxnimate;

namespace {

QStringList split_list(const QString& list)
{
    QStringList items = list.split(';', Qt::SkipEmptyParts);
    for ( auto& item : items )
        item = item.trimmed();
    items.removeAll(QString());
    return items;
}

QString number(double value)
{
    return QString::number(value, 'g', 8);
}

/**
 * \brief Converts one SMIL value to a CSS declaration
 * \return An empty string when CSS cannot animate the attribute on this element
 */
QString declaration(const QString& tag, const QString& attribute, const QString& value)
{
    static const std::set<QString> plain = {
        "opacity", "fill-opacity", "stroke-opacity", "stop-opacity",
        "fill", "stroke", "stop-color", "display",
    };
    // SVG 2 geometry properties, with the elements they apply to
    static const std::map<QString, std::set<QString>> geometry = {
        {"x", {"rect", "image", "use", "svg"}},
        {"y", {"rect", "image", "use", "svg"}},
        {"width", {"rect", "image", "use", "svg"}},
        {"height", {"rect", "image", "use", "svg"}},
        {"cx", {"circle", "ellipse"}},
        {"cy", {"circle", "ellipse"}},
        {"r", {"circle"}},
        {"rx", {"rect", "ellipse"}},
        {"ry", {"rect", "ellipse"}},
    };

    if ( plain.count(attribute) )
        return attribute + ": " + value + ";";
    if ( attribute == "stroke-width" )
        return attribute + ": " + value + "px;";
    if ( attribute == "d" && tag == "path" )
        return "d: path(\"" + value + "\");";

    auto it = geometry.find(attribute);
    if ( it != geometry.end() && it->second.count(tag) )
        return attribute + ": " + value + "px;";

    return {};
}

QString transform_declaration(const QString& type, const QString& value)
{
    QStringList args = value.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
    if ( args.isEmpty() )
        return {};

    if ( type == "translate" )
        return QString("transform: translate(%1px, %2px);").arg(args[0], args.value(1, "0"));
    if ( type == "scale" )
        return QString("transform: scale(%1, %2);").arg(args[0], args.value(1, args[0]));
    if ( type == "rotate" && args.size() == 1 )
        return QString("transform: rotate(%1deg);").arg(args[0]);
    if ( type == "rotate" && args.size() == 3 )
        return QString("transform: translate(%2px, %3px) rotate(%1deg) translate(-%2px, -%3px);").arg(args[0], args[1], args[2]);
    if ( type == "skewX" || type == "skewY" )
        return QString("transform: %1(%2deg);").arg(type, args[0]);
    return {};
}

struct CssAnimation
{
    QString target;
    QString keyframes;
    QString shorthand;
    QString static_rules;
};

} // namespace

std::unique_ptr<settings::SettingsGroup> webexport::CssSvgFormat::save_settings(model::Composition*) const
{
    QVariantMap triggers;
    triggers[i18n("On load")] = "load";
    triggers[i18n("While hovered")] = "hover";

    return std::make_unique<settings::SettingsGroup>(settings::SettingList{
        settings::Setting("trigger", i18n("Play"), i18n("When the animation plays"),
            settings::Setting::String, QString("load"), triggers),
        settings::Setting("root_id", i18n("Element ID"), i18n("ID for the root SVG element"), QString("glaxnimate-css")),
    });
}

bool webexport::CssSvgFormat::on_save(QIODevice& file, const QString&, model::Composition* comp, const QVariantMap& options)
{
    const QString trigger = options.value("trigger", QString("load")).toString();
    if ( trigger != "load" && trigger != "hover" )
    {
        error(i18n("Unknown playback trigger"));
        return false;
    }

    const QString root_id = options.value("root_id", QString("glaxnimate-css")).toString();
    static const QRegularExpression valid_id("^[A-Za-z_][A-Za-z0-9_.-]*$");
    if ( !valid_id.match(root_id).hasMatch() )
    {
        error(i18n("The element ID must start with a letter or underscore and contain only letters, digits, '_', '.', or '-'"));
        return false;
    }

    io::svg::SvgRenderer renderer(io::svg::SMIL, io::svg::CssFontType::FontFace);
    renderer.write_main(comp, comp->document()->current_time());
    QDomDocument dom = renderer.dom();
    QDomElement svg = dom.documentElement();
    svg.setAttribute("id", root_id);

    // Copy the live node lists before removing elements from the document
    std::vector<QDomElement> smil;
    for ( const char* tag : {"animate", "animateTransform", "animateMotion"} )
    {
        QDomNodeList nodes = dom.elementsByTagName(tag);
        for ( int i = 0; i < nodes.size(); i++ )
            smil.push_back(nodes.at(i).toElement());
    }

    std::vector<CssAnimation> animations;
    std::set<QString> unsupported;
    int generated_id = 0;

    for ( QDomElement& animation : smil )
    {
        QDomElement target = animation.parentNode().toElement();
        const QString kind = animation.tagName();
        const QString attribute = kind == "animateMotion" ? QString("motion") :
                                  kind == "animateTransform" ? QString("transform") :
                                  animation.attribute("attributeName");
        QStringList times = split_list(animation.attribute("keyTimes"));
        QStringList values = kind == "animateMotion" ? split_list(animation.attribute("keyPoints")) : split_list(animation.attribute("values"));
        // Discrete SMIL lists may stop before the end, holding the last value.
        // CSS would otherwise fill the gap from the underlying value.
        if ( values.size() == times.size() && !times.isEmpty() && times.back().toDouble() < 1 )
        {
            times.push_back("1");
            values.push_back(values.back());
        }
        const QStringList splines = split_list(animation.attribute("keySplines"));
        const bool discrete = animation.attribute("calcMode") == "discrete" || attribute == "display";
        const double duration = animation.attribute("dur").toDouble();

        QStringList declarations;
        for ( const QString& value : values )
        {
            if ( kind == "animateMotion" )
                declarations.push_back(QString("offset-distance: %1%;").arg(number(value.toDouble() * 100)));
            else if ( kind == "animateTransform" )
                declarations.push_back(transform_declaration(animation.attribute("type"), value));
            else
                declarations.push_back(declaration(target.tagName(), attribute, value));
        }

        if ( duration <= 0 || values.size() != times.size() || declarations.contains(QString()) ||
            (!discrete && splines.size() + 1 != times.size()) )
        {
            unsupported.insert(i18n("%1 on <%2>", attribute, target.tagName()));
            continue;
        }

        if ( target.attribute("id").isEmpty() )
            target.setAttribute("id", QString("%1-%2").arg(root_id).arg(++generated_id));

        CssAnimation css;
        css.target = target.attribute("id");
        const QString name = QString("%1-a%2").arg(root_id).arg(animations.size() + 1);
        css.keyframes = "@keyframes " + name + " {\n";
        for ( int i = 0; i < times.size(); i++ )
        {
            QString timing;
            if ( i + 1 < times.size() )
            {
                timing = discrete ? QString(" animation-timing-function: step-end;") :
                    QString(" animation-timing-function: cubic-bezier(%1);").arg(splines[i].simplified().replace(' ', ", "));
            }
            css.keyframes += QString("    %1% { %2%3 }\n").arg(number(times[i].toDouble() * 100), declarations[i], timing);
        }
        css.keyframes += "}\n";
        css.shorthand = QString("%1 %2s linear %3s infinite both").arg(name, number(duration), number(animation.attribute("begin").toDouble()));

        if ( kind == "animateTransform" || kind == "animateMotion" )
            css.static_rules = "transform-box: view-box; transform-origin: 0 0;";
        if ( kind == "animateMotion" )
        {
            css.static_rules += QString(" offset-path: path(\"%1\"); offset-rotate: %2;")
                .arg(animation.attribute("path"), animation.attribute("rotate") == "auto" ? "auto" : "0deg");
        }

        animations.push_back(css);
        animation.parentNode().removeChild(animation);
    }

    // Group animations by element, keeping document order
    QString style;
    std::vector<QString> order;
    std::map<QString, std::pair<QStringList, QString>> per_target;
    for ( const auto& css : animations )
    {
        style += css.keyframes;
        auto& entry = per_target[css.target];
        if ( entry.first.isEmpty() )
            order.push_back(css.target);
        entry.first.push_back(css.shorthand);
        if ( !css.static_rules.isEmpty() )
            entry.second = css.static_rules;
    }
    for ( const auto& target : order )
    {
        const auto& entry = per_target[target];
        style += QString("#%1 { animation: %2; %3}\n").arg(target, entry.first.join(", "), entry.second.isEmpty() ? QString() : entry.second + " ");
    }
    if ( trigger == "hover" )
        style += QString("#%1:not(:hover) * { animation-play-state: paused; }\n").arg(root_id);
    style += QString("@media (prefers-reduced-motion: reduce) {\n    #%1 * { animation: none !important; }\n}\n").arg(root_id);

    QDomElement style_element = dom.createElement("style");
    style_element.appendChild(dom.createCDATASection(style));
    svg.insertBefore(style_element, svg.firstChild());

    for ( const auto& item : unsupported )
        warning(i18n("CSS cannot animate %1; it keeps its SMIL animation", item));

    file.write(dom.toByteArray(4));
    return true;
}
