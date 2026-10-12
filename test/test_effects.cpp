/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QDomDocument>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "glaxnimate/io/glaxnimate/glaxnimate_format.hpp"
#include "glaxnimate/io/lottie/lottie_format.hpp"
#include "glaxnimate/io/svg/svg_format.hpp"
#include "glaxnimate/model/assets/assets.hpp"
#include "glaxnimate/model/shapes/composable/group.hpp"
#include "glaxnimate/model/shapes/composable/layer.hpp"
#include "glaxnimate/model/shapes/effects/effects.hpp"
#include "glaxnimate/module/module.hpp"

using namespace glaxnimate;

namespace {

// A 20x20 red square centered in a 100x100 canvas
const char* square = R"({
    "v": "5.7.0", "fr": 30, "ip": 0, "op": 30, "w": 100, "h": 100,
    "layers": [{"ty": 4, "ind": 1, "ip": 0, "op": 30, "st": 0, "ks": {}, "shapes": [
        {"ty": "rc", "s": {"a": 0, "k": [20, 20]}, "p": {"a": 0, "k": [50, 50]}, "r": {"a": 0, "k": 0}},
        {"ty": "fl", "o": {"a": 0, "k": 100}, "c": {"a": 0, "k": [1, 0, 0]}}
    ]}]
})";

} // namespace

class TestEffects : public QObject
{
    Q_OBJECT

    std::unique_ptr<model::Document> document;

    model::Composition* load()
    {
        document = std::make_unique<model::Document>();
        io::lottie::LottieFormat lottie;
        if ( !lottie.load(document.get(), square) )
            return nullptr;
        return document->assets()->compositions->values[0];
    }

    model::Layer* layer(model::Composition* comp)
    {
        return static_cast<model::Layer*>(comp->shapes[0]);
    }

    template<class T>
    T* add_effect(model::Composition* comp)
    {
        return static_cast<T*>(layer(comp)->shapes.insert(std::make_unique<T>(document.get())));
    }

private Q_SLOTS:
    void initTestCase()
    {
        module::initialize();
    }

    void test_effects_do_not_change_geometry()
    {
        auto comp = load();
        QVERIFY(comp);
        QRectF before = layer(comp)->local_bounding_rect(0);
        add_effect<model::GaussianBlurEffect>(comp);
        QCOMPARE(layer(comp)->local_bounding_rect(0), before);
        QCOMPARE(layer(comp)->shapes.size(), 3);
    }

    void test_gaussian_blur_spreads_edges()
    {
        auto comp = load();
        QImage sharp = comp->render_image(0);
        QCOMPARE(sharp.size(), QSize(100, 100));
        QCOMPARE(qAlpha(sharp.pixel(36, 50)), 0);

        add_effect<model::GaussianBlurEffect>(comp)->blurriness.set(20);
        QImage blurred = comp->render_image(0);
        // Beyond the square's edge, now partly covered by the blur
        QVERIFY(qAlpha(blurred.pixel(36, 50)) > 0);
        // The center stays mostly opaque and red
        QVERIFY(qAlpha(blurred.pixel(50, 50)) > 200);
        QVERIFY(qRed(blurred.pixel(50, 50)) > qBlue(blurred.pixel(50, 50)));
    }

    void test_drop_shadow_offsets_a_dark_copy()
    {
        auto comp = load();
        auto shadow = add_effect<model::DropShadowEffect>(comp);
        shadow->opacity.set(1);
        shadow->distance.set(15);
        shadow->blurriness.set(0.1);
        QImage image = comp->render_image(0);
        // 135 degrees points down and to the right
        QRgb below_right = image.pixel(68, 68);
        QVERIFY(qAlpha(below_right) > 200);
        QVERIFY(qRed(below_right) < 60);
        QCOMPARE(qAlpha(image.pixel(32, 32)), 0);
        QVERIFY(qRed(image.pixel(50, 50)) > 200);
    }

    void test_fill_replaces_color()
    {
        auto comp = load();
        add_effect<model::FillEffect>(comp)->color.set(QColor(0, 0, 255));
        QRgb center = comp->render_image(0).pixel(50, 50);
        QVERIFY(qBlue(center) > 200);
        QVERIFY(qRed(center) < 60);
    }

    void test_tint_and_tritone_change_colors()
    {
        auto comp = load();
        auto tint = add_effect<model::TintEffect>(comp);
        tint->black.set(QColor(0, 0, 255));
        tint->white.set(QColor(0, 255, 0));
        QRgb tinted = comp->render_image(0).pixel(50, 50);
        QVERIFY(qRed(tinted) < 60);

        comp = load();
        auto tritone = add_effect<model::TritoneEffect>(comp);
        tritone->shadows.set(QColor(0, 0, 255));
        tritone->midtones.set(QColor(0, 255, 0));
        tritone->highlights.set(QColor(255, 255, 0));
        QRgb toned = comp->render_image(0).pixel(50, 50);
        QVERIFY(qRed(toned) < 200 || qGreen(toned) > 60);
    }

    void test_hidden_effect_is_ignored()
    {
        auto comp = load();
        add_effect<model::FillEffect>(comp)->visible.set(false);
        QVERIFY(qRed(comp->render_image(0).pixel(50, 50)) > 200);
    }

    void test_lottie_export_and_import()
    {
        auto comp = load();
        add_effect<model::GaussianBlurEffect>(comp)->blurriness.set(12);
        auto shadow = add_effect<model::DropShadowEffect>(comp);
        shadow->opacity.set(0.5);
        shadow->angle.set(90);
        shadow->distance.set_keyframe(0, 4);
        shadow->distance.set_keyframe(30, 16);
        auto fill = add_effect<model::FillEffect>(comp);
        fill->color.set(QColor(0, 0, 255));
        fill->opacity.set(0.25);
        add_effect<model::TintEffect>(comp)->amount.set(0.4);
        auto tritone = add_effect<model::TritoneEffect>(comp);
        tritone->blend.set(0.3);
        tritone->visible.set(false);

        io::lottie::LottieFormat lottie;
        QByteArray data = lottie.save(comp);
        QJsonArray effects = QJsonDocument::fromJson(data).object()["layers"].toArray()[0].toObject()["ef"].toArray();
        QCOMPARE(effects.size(), 5);

        auto param = [&effects](int effect, int index) {
            return effects[effect].toObject()["ef"].toArray()[index].toObject()["v"].toObject()["k"];
        };
        QCOMPARE(effects[0].toObject()["ty"].toInt(), 29);
        QCOMPARE(param(0, 0).toDouble(), 12.);
        QCOMPARE(effects[1].toObject()["ty"].toInt(), 25);
        QCOMPARE(param(1, 1).toDouble(), 127.5);
        QCOMPARE(param(1, 2).toDouble(), 90.);
        QCOMPARE(effects[1].toObject()["ef"].toArray()[3].toObject()["v"].toObject()["a"].toInt(), 1);
        QCOMPARE(effects[2].toObject()["ty"].toInt(), 21);
        QCOMPARE(param(2, 2).toArray()[2].toDouble(), 1.);
        QCOMPARE(param(2, 6).toDouble(), 0.25);
        QCOMPARE(effects[3].toObject()["ty"].toInt(), 20);
        QCOMPARE(param(3, 2).toDouble(), 40.);
        QCOMPARE(effects[4].toObject()["ty"].toInt(), 23);
        QCOMPARE(param(4, 3).toDouble(), 30.);
        QCOMPARE(effects[4].toObject()["en"].toInt(), 0);
        // Effects are not exported as shapes
        QCOMPARE(QJsonDocument::fromJson(data).object()["layers"].toArray()[0].toObject()["shapes"].toArray().size(), 2);

        model::Document loaded;
        QVERIFY(lottie.load(&loaded, data));
        auto loaded_layer = static_cast<model::Layer*>(loaded.assets()->compositions->values[0]->shapes[0]);
        std::vector<model::Effect*> loaded_effects;
        for ( const auto& shape : loaded_layer->shapes )
            if ( auto effect = qobject_cast<model::Effect*>(shape.get()) )
                loaded_effects.push_back(effect);
        QCOMPARE(int(loaded_effects.size()), 5);
        auto loaded_shadow = qobject_cast<model::DropShadowEffect*>(loaded_effects[1]);
        QVERIFY(loaded_shadow);
        QCOMPARE(loaded_shadow->opacity.get(), 0.5f);
        QCOMPARE(loaded_shadow->distance.keyframe_count(), 2);
        QCOMPARE(loaded_shadow->distance.get_at(30), 16.f);
        auto loaded_fill = qobject_cast<model::FillEffect*>(loaded_effects[2]);
        QVERIFY(loaded_fill);
        QCOMPARE(loaded_fill->color.get().blue(), 255);
        QCOMPARE(loaded_fill->opacity.get(), 0.25f);
        QCOMPARE(qobject_cast<model::TintEffect*>(loaded_effects[3])->amount.get(), 0.4f);
        QCOMPARE(loaded_effects[4]->visible.get(), false);
    }

    void test_lottie_warns_about_effects_in_groups()
    {
        auto comp = load();
        auto group = static_cast<model::Group*>(layer(comp)->shapes.insert(std::make_unique<model::Group>(document.get())));
        group->shapes.insert(std::make_unique<model::GaussianBlurEffect>(document.get()));

        io::lottie::LottieFormat lottie;
        QStringList warnings;
        connect(&lottie, &io::ImportExport::message, this, [&warnings](const QString& message, log::Severity severity){
            if ( severity == log::Warning )
                warnings.push_back(message);
        });
        QVERIFY(!lottie.save(comp).isEmpty());
        QVERIFY(warnings.join(" ").contains("layers only"));
    }

    void test_svg_filters()
    {
        auto comp = load();
        auto blur = add_effect<model::GaussianBlurEffect>(comp);
        blur->blurriness.set_keyframe(0, 10);
        blur->blurriness.set_keyframe(30, 20);
        auto shadow = add_effect<model::DropShadowEffect>(comp);
        shadow->angle.set(90);
        shadow->distance.set(10);
        add_effect<model::FillEffect>(comp)->color.set(QColor(0, 0, 255));
        add_effect<model::TintEffect>(comp)->amount.set(0.5);
        add_effect<model::TritoneEffect>(comp)->blend.set(0.25);
        add_effect<model::GaussianBlurEffect>(comp)->visible.set(false);

        io::svg::SvgFormat svg;
        QDomDocument dom;
        QVERIFY(dom.setContent(svg.save(comp)));

        auto filters = dom.elementsByTagName("filter");
        QCOMPARE(filters.size(), 1);
        QDomElement filter = filters.at(0).toElement();
        QCOMPARE(filter.attribute("color-interpolation-filters"), QStringLiteral("sRGB"));

        // The layer that owns the effects uses the filter
        bool referenced = false;
        auto groups = dom.elementsByTagName("g");
        for ( int i = 0; i < groups.size(); i++ )
            referenced |= groups.at(i).toElement().attribute("filter") == "url(#" + filter.attribute("id") + ")";
        QVERIFY(referenced);

        QStringList tags;
        for ( auto child = filter.firstChildElement(); !child.isNull(); child = child.nextSiblingElement() )
            tags.push_back(child.tagName());
        // The hidden blur is left out
        QCOMPARE(tags, (QStringList{
            "feGaussianBlur", "feDropShadow", "feFlood", "feComposite",
            "feColorMatrix", "feColorMatrix", "feComponentTransfer", "feComposite"
        }));

        QDomElement gaussian = filter.firstChildElement("feGaussianBlur");
        QCOMPARE(gaussian.attribute("stdDeviation").toDouble(), 3.);
        QCOMPARE(gaussian.attribute("in"), QStringLiteral("SourceGraphic"));
        QVERIFY(!gaussian.firstChildElement("animate").isNull());
        QCOMPARE(gaussian.firstChildElement("animate").attribute("attributeName"), QStringLiteral("stdDeviation"));

        QDomElement drop = filter.firstChildElement("feDropShadow");
        QCOMPARE(drop.attribute("in"), gaussian.attribute("result"));
        QVERIFY(qAbs(drop.attribute("dx").toDouble() - 10) < 0.001);
        QVERIFY(qAbs(drop.attribute("dy").toDouble()) < 0.001);
        QCOMPARE(drop.attribute("flood-opacity").toDouble(), 0.5);

        QDomElement flood = filter.firstChildElement("feFlood");
        QCOMPARE(flood.attribute("flood-color"), QStringLiteral("#0000ff"));

        QDomElement mix = filter.lastChildElement("feComposite");
        QCOMPARE(mix.attribute("operator"), QStringLiteral("arithmetic"));
        QCOMPARE(mix.attribute("k2").toDouble(), 0.75);
        QCOMPARE(mix.attribute("k3").toDouble(), 0.25);
    }

    void test_native_format_round_trip()
    {
        auto comp = load();
        auto shadow = add_effect<model::DropShadowEffect>(comp);
        shadow->angle.set(45);
        shadow->distance.set_keyframe(0, 5);
        shadow->distance.set_keyframe(30, 25);

        io::glaxnimate::GlaxnimateFormat format;
        QByteArray data = format.save(comp);
        QVERIFY(!data.isEmpty());

        model::Document loaded;
        QVERIFY(format.load(&loaded, data));
        auto loaded_layer = static_cast<model::Layer*>(loaded.assets()->compositions->values[0]->shapes[0]);
        model::DropShadowEffect* loaded_shadow = nullptr;
        for ( const auto& shape : loaded_layer->shapes )
            if ( auto effect = qobject_cast<model::DropShadowEffect*>(shape.get()) )
                loaded_shadow = effect;
        QVERIFY(loaded_shadow);
        QCOMPARE(loaded_shadow->angle.get(), 45.f);
        QCOMPARE(loaded_shadow->distance.keyframe_count(), 2);
        QCOMPARE(loaded_shadow->distance.get_at(30), 25.f);
    }
};

QTEST_MAIN(TestEffects)
#include "test_effects.moc"
