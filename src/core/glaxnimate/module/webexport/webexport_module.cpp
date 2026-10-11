/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "webexport_module.hpp"

#include "css_svg_format.hpp"
#include "interactive_svg_format.hpp"
#include "mobile_formats.hpp"

std::vector<glaxnimate::module::ExternalComponent> glaxnimate::webexport::Module::components() const
{
    return {};
}

void glaxnimate::webexport::Module::initialize()
{
    register_io_classes<
        InteractiveSvgFormat,
        CssSvgFormat,
        ReactNativeFormat,
        FlutterFormat
    >();
}
