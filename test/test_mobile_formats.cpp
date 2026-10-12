/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

#include "glaxnimate/io/lottie/lottie_format.hpp"
#include "glaxnimate/model/assets/assets.hpp"
#include "glaxnimate/module/webexport/mobile_formats.hpp"

using namespace glaxnimate;

namespace {

const char* lottie_json = R"({
    "v": "5.7.0", "fr": 30, "ip": 0, "op": 30, "w": 100, "h": 100, "nm": "Badge",
    "layers": [{"ty": 4, "ind": 1, "ip": 0, "op": 30, "st": 0, "ks": {}, "shapes": [
        {"ty": "rc", "s": {"a": 0, "k": [10, 10]}, "r": {"a": 0, "k": 0}, "p": {"a": 1, "k": [
            {"t": 0, "s": [10, 50], "o": {"x": [0], "y": [0]}, "i": {"x": [1], "y": [1]}},
            {"t": 30, "s": [90, 50]}
        ]}},
        {"ty": "fl", "o": {"a": 0, "k": 100}, "c": {"a": 0, "k": [1, 0, 0]}}
    ]}]
})";

} // namespace

class TestMobileFormats : public QObject
{
    Q_OBJECT

    std::unique_ptr<model::Document> document;

    model::Composition* composition()
    {
        document = std::make_unique<model::Document>();
        io::lottie::LottieFormat lottie;
        if ( !lottie.load(document.get(), lottie_json) )
            return nullptr;
        return document->assets()->compositions->values[0];
    }

    // Checks that the embedded data is the composition's Lottie JSON
    void verify_lottie(const QByteArray& json)
    {
        QJsonParseError error;
        QJsonObject lottie = QJsonDocument::fromJson(json, &error).object();
        QCOMPARE(error.error, QJsonParseError::NoError);
        QCOMPARE(lottie["w"].toInt(), 100);
        QCOMPARE(lottie["layers"].toArray().size(), 1);
    }

private Q_SLOTS:
    void test_react_native()
    {
        webexport::ReactNativeFormat format;
        QString source = QString::fromUtf8(format.save(composition(), {{"component_name", "SpinningBadge"}, {"loop", false}}));
        QVERIFY(source.contains("import LottieView from 'lottie-react-native';"));
        QVERIFY(source.contains("export default function SpinningBadge(props)"));
        QVERIFY(source.contains("autoPlay: true, loop: false, ...props"));

        const QString start = "const animationData = ";
        const int begin = source.indexOf(start) + start.size();
        verify_lottie(source.mid(begin, source.indexOf(";\n", begin) - begin).toUtf8());
    }

    void test_flutter()
    {
        webexport::FlutterFormat format;
        QString source = QString::fromUtf8(format.save(composition(), {{"autoplay", false}}));
        QVERIFY(source.contains("import 'package:lottie/lottie.dart';"));
        QVERIFY(source.contains("class GlaxnimateAnimation extends StatelessWidget"));
        QVERIFY(source.contains("this.animate = false, this.repeat = true"));

        auto match = QRegularExpression("base64Decode\\('([A-Za-z0-9+/=]+)'\\)").match(source);
        QVERIFY(match.hasMatch());
        verify_lottie(QByteArray::fromBase64(match.captured(1).toLatin1()));
    }

    void test_rejects_invalid_names_data()
    {
        QTest::addColumn<QString>("name");
        QTest::newRow("lowercase") << "badge";
        QTest::newRow("digit first") << "1Badge";
        QTest::newRow("punctuation") << "Bad-ge";
        QTest::newRow("code") << "A(){}";
        QTest::newRow("too long") << QString("A").repeated(65);
    }

    void test_rejects_invalid_names()
    {
        QFETCH(QString, name);
        QByteArray data;
        QBuffer file(&data);
        webexport::ReactNativeFormat format;
        QVERIFY(!format.save(file, "test.js", composition(), {{"component_name", name}}));
    }
};

QTEST_GUILESS_MAIN(TestMobileFormats)
#include "test_mobile_formats.moc"
