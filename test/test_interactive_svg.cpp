/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QBuffer>
#include <QDomDocument>
#include <QJsonDocument>
#include <QJsonObject>

#include "glaxnimate/io/lottie/lottie_format.hpp"
#include "glaxnimate/model/assets/assets.hpp"
#include "glaxnimate/module/webexport/interactive_svg_format.hpp"

using namespace glaxnimate;

namespace {

// One rectangle that moves across a 100x100 canvas over 30 frames at 30 fps
const char* animated_lottie = R"({
    "v": "5.7.0", "fr": 30, "ip": 0, "op": 30, "w": 100, "h": 100,
    "layers": [{
        "ty": 4, "ind": 1, "ip": 0, "op": 30, "st": 0,
        "ks": {},
        "shapes": [
            {"ty": "rc", "s": {"a": 0, "k": [10, 10]}, "r": {"a": 0, "k": 0},
             "p": {"a": 1, "k": [
                {"t": 0, "s": [10, 10], "o": {"x": [0], "y": [0]}, "i": {"x": [1], "y": [1]}},
                {"t": 30, "s": [90, 10]}
             ]}},
            {"ty": "fl", "c": {"a": 0, "k": [1, 0, 0]}, "o": {"a": 0, "k": 100}}
        ]
    }]
})";

} // namespace

class TestInteractiveSvg : public QObject
{
    Q_OBJECT

    std::unique_ptr<model::Document> load(const QByteArray& lottie_json = animated_lottie)
    {
        auto document = std::make_unique<model::Document>();
        io::lottie::LottieFormat lottie;
        if ( !lottie.load(document.get(), lottie_json) )
            return {};
        return document;
    }

    model::Composition* composition(model::Document* document)
    {
        return document->assets()->compositions->values[0];
    }

    // Returns the root element and fills the parsed player options
    QDomElement export_svg(model::Composition* comp, const QVariantMap& settings, QJsonObject& player_options)
    {
        webexport::InteractiveSvgFormat format;
        QByteArray data = format.save(comp, settings);
        QDomDocument dom;
        if ( data.isEmpty() || !dom.setContent(data) )
            return {};

        QDomElement script = dom.documentElement().lastChildElement("script");
        const QString code = script.text();
        const QString marker = "const options = ";
        const int start = code.indexOf(marker) + marker.size();
        const int end = code.indexOf(";\n", start);
        player_options = QJsonDocument::fromJson(code.mid(start, end - start).toUtf8()).object();
        return dom.documentElement();
    }

    bool save_raw(model::Composition* comp, const QVariantMap& settings)
    {
        QByteArray data;
        QBuffer file(&data);
        webexport::InteractiveSvgFormat format;
        return format.save(file, "test.svg", comp, settings);
    }

private Q_SLOTS:
    void test_default_export()
    {
        auto document = load();
        QVERIFY(document);
        QJsonObject options;
        QDomElement svg = export_svg(composition(document.get()), {}, options);
        QVERIFY(!svg.isNull());

        QCOMPARE(svg.attribute("id"), QStringLiteral("glaxnimate-player"));
        QCOMPARE(svg.lastChildElement("script").attribute("type"), QStringLiteral("application/ecmascript"));
        QVERIFY(svg.elementsByTagName("animateTransform").size() + svg.elementsByTagName("animate").size() > 0);

        QCOMPARE(options["root_id"].toString(), QStringLiteral("glaxnimate-player"));
        QCOMPARE(options["trigger"].toString(), QStringLiteral("load"));
        QCOMPARE(options["direction"].toInt(), 1);
        QCOMPARE(options["iterations"].toInt(), 1);
        QCOMPARE(options["offset_ms"].toDouble(), 0.);
        QCOMPARE(options["duration_ms"].toDouble(), 1000.);
    }

    void test_custom_settings()
    {
        auto document = load();
        auto comp = composition(document.get());
        comp->animation->first_frame.set(15);
        QJsonObject options;
        QDomElement svg = export_svg(comp, {
            {"trigger", "hover"},
            {"direction", -1},
            {"alternate", true},
            {"iterations", 0},
            {"speed", 2.5},
            {"hover_out", "reverse"},
            {"click_again", "freeze"},
            {"scroll_threshold", 60},
            {"root_id", "hero"},
        }, options);

        QCOMPARE(svg.attribute("id"), QStringLiteral("hero"));
        QCOMPARE(options["root_id"].toString(), QStringLiteral("hero"));
        QCOMPARE(options["trigger"].toString(), QStringLiteral("hover"));
        QCOMPARE(options["direction"].toInt(), -1);
        QCOMPARE(options["alternate"].toBool(), true);
        QCOMPARE(options["iterations"].toInt(), 0);
        QCOMPARE(options["speed"].toDouble(), 2.5);
        QCOMPARE(options["hover_out"].toString(), QStringLiteral("reverse"));
        QCOMPARE(options["click_again"].toString(), QStringLiteral("freeze"));
        QCOMPARE(options["scroll_threshold"].toInt(), 60);
        QCOMPARE(options["offset_ms"].toDouble(), 500.);
        QCOMPARE(options["duration_ms"].toDouble(), 500.);
    }

    void test_rejects_invalid_settings_data()
    {
        QTest::addColumn<QVariantMap>("settings");
        QTest::newRow("id starts with digit") << QVariantMap{{"root_id", "1bad"}};
        QTest::newRow("id with quote") << QVariantMap{{"root_id", "a\"b"}};
        QTest::newRow("unknown trigger") << QVariantMap{{"trigger", "bogus"}};
        QTest::newRow("unknown hover end") << QVariantMap{{"hover_out", "restart"}};
        QTest::newRow("unknown second click") << QVariantMap{{"click_again", "reset"}};
        QTest::newRow("negative iterations") << QVariantMap{{"iterations", -1}};
        QTest::newRow("zero speed") << QVariantMap{{"speed", 0}};
        QTest::newRow("scroll threshold") << QVariantMap{{"scroll_threshold", 0}};
    }

    void test_rejects_invalid_settings()
    {
        QFETCH(QVariantMap, settings);
        auto document = load();
        QVERIFY(!save_raw(composition(document.get()), settings));
    }

    void test_rejects_static_composition()
    {
        // An out point of 0 leaves the composition without a valid last frame
        auto document = load(QByteArray(animated_lottie).replace("\"op\": 30, \"w\"", "\"op\": 0, \"w\""));
        QVERIFY(document);
        QVERIFY(!save_raw(composition(document.get()), {}));
    }
};

QTEST_GUILESS_MAIN(TestInteractiveSvg)
#include "test_interactive_svg.moc"
