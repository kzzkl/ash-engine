#pragma once

#include "pipeline_parameter.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
class skin_pipeline;
struct skinned_mesh
{
    std::vector<std::unique_ptr<resource_interface>> skinned_vertex_buffers;

    skin_pipeline* pipeline;
    std::unique_ptr<pipeline_parameter> parameter;

    std::size_t vertex_count;
};
} // namespace ash::graphics