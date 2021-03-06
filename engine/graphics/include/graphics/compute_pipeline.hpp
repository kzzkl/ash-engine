#pragma once

#include "graphics_interface.hpp"

namespace ash::graphics
{
class compute_pipeline
{
public:
    virtual ~compute_pipeline() = default;
    virtual void compute(render_command_interface* command) = 0;
};
} // namespace ash::graphics