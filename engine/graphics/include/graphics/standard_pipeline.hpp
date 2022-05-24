#pragma once

#include "graphics/render_pipeline.hpp"

namespace ash::graphics
{
class standard_pipeline : public render_pipeline
{
public:
    standard_pipeline();

    virtual void render(const camera& camera, render_command_interface* command) override;

private:
    std::unique_ptr<render_pass_interface> m_interface;
};
} // namespace ash::graphics