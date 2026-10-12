/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QBuffer>
#include <QDomDocument>

#include "glaxnimate/io/lottie/lottie_format.hpp"
#include "glaxnimate/model/assets/assets.hpp"
#include "glaxnimate/module/webexport/css_svg_format.hpp"

using namespace glaxnimate;

namespace {

// A moving rectangle with an animated color on a layer with eased rotation,
// plus a gradient whose start point moves, which CSS cannot animate
const char* lottie_json = R"({
    "v": "5.7.0", "fr": 30, "ip": 0, "op": 30, "w": 100, "h": 100,
    "layers": [
        {
            "ty": 4, "ind": 1, "ip": 0, "op": 30, "st": 0,
            "ks": {"r": {"a": 1, "k": [
                {"t": 0, "s": [0], "o": {"x": [0.42], "y": [0]}, "i": {"x": [0.58], "y": [1]}},
                {"t": 30, "s": [90]}
            ]}},
            "shapes": [
                {"ty": "rc", "s": {"a": 0, "k": [10, 10]}, "r": {"a": 0, "k": 0},
                 "p": {"a": 1, "k": [
                    {"t": 0, "s": [10, 50], "o": {"x": [0], "y": [0]}, "i": {"x": [1], "y": [1]}},
                    {"t": 30, "s": [90, 50]}
                 ]}},
                {"ty": "fl", "o": {"a": 0, "k": 100}, "c": {"a": 1, "k": [
                    {"t": 0, "s": [1, 0, 0], "o": {"x": [0], "y": [0]}, "i": {"x": [1], "y": [1]}},
                    {"t": 30, "s": [0, 0, 1]}
                ]}}
            ]
        },
        {
            "ty": 4, "ind": 2, "ip": 0, "op": 30, "st": 0, "ks": {},
            "shapes": [
                {"ty": "rc", "s": {"a": 0, "k": [100, 10]}, "p": {"a": 0, "k": [50, 5]}, "r": {"a": 0, "k": 0}},
                {"ty": "gf", "o": {"a": 0, "k": 100}, "r": 1, "t": 1,
                 "s": {"a": 1, "k": [
                    {"t": 0, "s": [0, 0], "o": {"x": [0], "y": [0]}, "i": {"x": [1], "y": [1]}},
                    {"t": 30, "s": [50, 0]}
                 ]},
                 "e": {"a": 0, "k": [100, 0]},
                 "g": {"p": 2, "k": {"a": 0, "k": [0, 1, 0, 0, 1, 0, 0, 1]}}}
            ]
        }
    ]
})";

} // namespace

class TestCssSvg : public QObject
{
    Q_OBJECT

    struct Result
    {
        bool ok = false;
        QDomDocument dom;
        QStringList warnings;

        QString style() const
        {
            return dom.documentElement().firstChildElement("style").text();
        }

        QStringList smil_attributes() const
        {
            QStringList attributes;
            for ( const char* tag : {"animate", "animateTransform", "animateMotion"} )
            {
                auto nodes = dom.elementsByTagName(tag);
                for ( int i = 0; i < nodes.size(); i++ )
                    attributes.push_back(nodes.at(i).toElement().attribute("attributeName"));
            }
            return attributes;
        }
    };

    Result export_css(const QVariantMap& settings)
    {
        Result result;
        model::Document document;
        io::lottie::LottieFormat lottie;
        if ( !lottie.load(&document, lottie_json) )
            return result;

        webexport::CssSvgFormat format;
        connect(&format, &io::ImportExport::message, this, [&result](const QString& message, log::Severity severity){
            if ( severity == log::Warning )
                result.warnings.push_back(message);
        });

        QByteArray data;
        QBuffer file(&data);
        result.ok = format.save(file, "test.svg", document.assets()->compositions->values[0], settings);
        result.dom.setContent(data);
        return result;
    }

private Q_SLOTS:
    void test_converts_supported_animations()
    {
        auto result = export_css({});
        QVERIFY(result.ok);
        QCOMPARE(result.dom.documentElement().attribute("id"), QStringLiteral("glaxnimate-css"));

        const QString style = result.style();
        QVERIFY(style.contains("@keyframes glaxnimate-css-a1"));
        QVERIFY(style.contains("cubic-bezier(0.420, 0, 0.580, 1)"));
        QVERIFY(style.contains("transform: rotate("));
        QVERIFY(style.contains("fill: #"));
        QVERIFY(style.contains("x: 5px;"));
        QVERIFY(style.contains("infinite both"));
        QVERIFY(style.contains("prefers-reduced-motion: reduce"));
        QVERIFY(!style.contains(":hover"));

        // Only the gradient geometry keeps SMIL, and the export says so
        auto smil = result.smil_attributes();
        QVERIFY(!smil.isEmpty());
        for ( const auto& attribute : smil )
            QVERIFY2(attribute == "x1" || attribute == "y1", qPrintable(attribute));
        QVERIFY(!result.warnings.isEmpty());
        QVERIFY(result.warnings.join(" ").contains("x1"));
    }

    void test_hover_trigger()
    {
        auto result = export_css({{"trigger", "hover"}, {"root_id", "badge"}});
        QVERIFY(result.ok);
        QCOMPARE(result.dom.documentElement().attribute("id"), QStringLiteral("badge"));
        QVERIFY(result.style().contains("#badge:not(:hover) * { animation-play-state: paused; }"));
        QVERIFY(result.style().contains("@keyframes badge-a1"));
    }

    void test_layer_time_range_holds_last_value()
    {
        model::Document document;
        io::lottie::LottieFormat lottie;
        QVERIFY(lottie.load(&document, R"({
            "v": "5.7.0", "fr": 30, "ip": 0, "op": 30, "w": 100, "h": 100,
            "layers": [{"ty": 4, "ind": 1, "ip": 10, "op": 20, "st": 0, "ks": {}, "shapes": [
                {"ty": "rc", "s": {"a": 0, "k": [10, 10]}, "p": {"a": 0, "k": [50, 50]}, "r": {"a": 0, "k": 0}},
                {"ty": "fl", "o": {"a": 0, "k": 100}, "c": {"a": 0, "k": [1, 0, 0]}}
            ]}]
        })"));
        webexport::CssSvgFormat format;
        QString style = QString::fromUtf8(format.save(document.assets()->compositions->values[0], {}));
        // Hidden before frame 10 and after frame 20, holding each value until the next keyframe
        QVERIFY(style.contains("0% { display: none; animation-timing-function: step-end; }"));
        QVERIFY(style.contains("% { display: inline; animation-timing-function: step-end; }"));
        QVERIFY(style.contains("100% { display: none; }"));
    }

    void test_rejects_invalid_settings()
    {
        QVERIFY(!export_css({{"trigger", "click"}}).ok);
        QVERIFY(!export_css({{"root_id", "9lives"}}).ok);
    }
};

QTEST_GUILESS_MAIN(TestCssSvg)
#include "test_css_svg.moc"
