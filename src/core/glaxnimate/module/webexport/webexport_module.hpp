/*
 * SPDX-FileCopyrightText: 2026 Builder106
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/module/module.hpp"

namespace glaxnimate::webexport {

class Module : public module::Module
{
public:
    Module() : module::Module(i18n("Web Export")) {}

    std::vector<module::ExternalComponent> components() const override;

protected:
    void initialize() override;
};

} // namespace glaxnimate::webexport
