/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "webexport_module.hpp"

#include "interactive_svg_format.hpp"

std::vector<glaxnimate::module::ExternalComponent> glaxnimate::webexport::Module::components() const
{
    return {};
}

void glaxnimate::webexport::Module::initialize()
{
    register_io_classes<
        InteractiveSvgFormat
    >();
}
