/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QDomDocument>

#include "glaxnimate/io/lottie/lottie_format.hpp"
#include "glaxnimate/io/svg/svg_format.hpp"
#include "glaxnimate/model/assets/assets.hpp"

using namespace glaxnimate;

namespace {

// A 10x10 rectangle whose center moves from x=10 at frame 0 to x=90 at frame 30
QByteArray lottie(int in_point, int out_point, const QString& easing = "\"o\": {\"x\": [0], \"y\": [0]}, \"i\": {\"x\": [1], \"y\": [1]}")
{
    return QStringLiteral(R"({
        "v": "5.7.0", "fr": 30, "ip": %1, "op": %2, "w": 100, "h": 100,
        "layers": [{
            "ty": 4, "ind": 1, "ip": 0, "op": 30, "st": 0,
            "ks": {},
            "shapes": [
                {"ty": "rc", "s": {"a": 0, "k": [10, 10]}, "r": {"a": 0, "k": 0},
                 "p": {"a": 1, "k": [
                    {"t": 0, "s": [10, 50], %3},
                    {"t": 30, "s": [90, 50]}
                 ]}},
                {"ty": "fl", "c": {"a": 0, "k": [1, 0, 0]}, "o": {"a": 0, "k": 100}}
            ]
        }]
    })").arg(in_point).arg(out_point).arg(easing).toUtf8();
}

} // namespace

class TestSvgRenderer : public QObject
{
    Q_OBJECT

    QDomDocument export_svg(const QByteArray& json)
    {
        model::Document document;
        io::lottie::LottieFormat lottie_format;
        QDomDocument dom;
        if ( !lottie_format.load(&document, json) )
            return dom;

        io::svg::SvgFormat svg_format;
        dom.setContent(svg_format.save(document.assets()->compositions->values[0], {}));
        return dom;
    }

    // Returns the animate element for the rectangle's x attribute
    QDomElement export_x(const QByteArray& json)
    {
        auto animations = export_svg(json).elementsByTagName("animate");
        for ( int i = 0; i < animations.size(); i++ )
        {
            QDomElement element = animations.at(i).toElement();
            if ( element.attribute("attributeName") == "x" )
                return element;
        }
        return {};
    }

    std::vector<double> numbers(const QString& list)
    {
        std::vector<double> result;
        for ( const auto& item : list.split(';', Qt::SkipEmptyParts) )
            result.push_back(item.trimmed().toDouble());
        return result;
    }

private Q_SLOTS:
    void test_window_data()
    {
        QTest::addColumn<int>("in_point");
        QTest::addColumn<int>("out_point");
        QTest::addColumn<double>("first_x");
        QTest::addColumn<double>("last_x");
        // Rectangle x is the center minus half the width
        QTest::newRow("full range") << 0 << 30 << 5. << 85.;
        QTest::newRow("starts after the first keyframe") << 15 << 30 << 45. << 85.;
        QTest::newRow("ends before the last keyframe") << 0 << 15 << 5. << 45.;
        QTest::newRow("cut on both sides") << 10 << 20 << 10 + 80 / 3. - 5 << 10 + 160 / 3. - 5;
    }

    void test_window()
    {
        QFETCH(int, in_point);
        QFETCH(int, out_point);
        QFETCH(double, first_x);
        QFETCH(double, last_x);

        QDomElement x = export_x(lottie(in_point, out_point));
        QVERIFY(!x.isNull());
        auto values = numbers(x.attribute("values"));
        auto times = numbers(x.attribute("keyTimes"));
        QCOMPARE(values.size(), times.size());
        QCOMPARE(numbers(x.attribute("keySplines").replace(' ', ';')).size(), 4 * (times.size() - 1));
        QCOMPARE(times.front(), 0.);
        QCOMPARE(times.back(), 1.);
        QVERIFY(qAbs(values.front() - first_x) < 0.01);
        QVERIFY(qAbs(values.back() - last_x) < 0.01);
    }

    void test_eased_start_keeps_curve_tail()
    {
        QString ease = "\"o\": {\"x\": [0.42], \"y\": [0]}, \"i\": {\"x\": [0.58], \"y\": [1]}";
        QDomElement x = export_x(lottie(15, 30, ease));
        QVERIFY(!x.isNull());
        auto values = numbers(x.attribute("values"));
        // Ease in-out is symmetric, so the midpoint value is the linear midpoint
        QVERIFY(qAbs(values.front() - 45) < 0.01);
        // The remaining half of the curve eases out instead of moving linearly
        QVERIFY(x.attribute("keySplines").split(';').front().trimmed() != "0 0 1 1");
    }

    void test_static_size_keeps_position_easing()
    {
        QString ease = "\"o\": {\"x\": [0.42], \"y\": [0]}, \"i\": {\"x\": [0.58], \"y\": [1]}";
        QDomElement x = export_x(lottie(0, 30, ease));
        QVERIFY(!x.isNull());
        auto spline = numbers(x.attribute("keySplines").replace(' ', ';'));
        std::vector<double> expected{0.42, 0, 0.58, 1};
        QCOMPARE(spline.size(), expected.size());
        for ( std::size_t i = 0; i < expected.size(); i++ )
            QVERIFY(qAbs(spline[i] - expected[i]) < 0.001);
    }

    void test_hold_before_start()
    {
        QDomElement x = export_x(lottie(15, 30, "\"h\": 1"));
        QVERIFY(!x.isNull());
        auto values = numbers(x.attribute("values"));
        // The held value lasts until the next keyframe, then jumps
        QVERIFY(qAbs(values.front() - 5) < 0.01);
        QVERIFY(qAbs(values.back() - 85) < 0.01);
    }
};

QTEST_GUILESS_MAIN(TestSvgRenderer)
#include "test_svg_renderer.moc"
