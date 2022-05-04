#pragma once

#include "graphics_interface.hpp"
#include <string>
#include <vector>

namespace ash::graphics
{
class vertex_layout_info
{
public:
    vertex_layout_desc convert() noexcept;
    std::vector<vertex_attribute_type> attributes;
};

class pass_parameter_layout_info
{
public:
    pass_parameter_layout_desc convert() noexcept;
    std::vector<pass_parameter_pair> parameters;
};

class pass_layout_info
{
public:
    pass_layout_desc convert() noexcept;
    std::vector<std::string> parameters;
};

class pass_blend_info : public pass_blend_desc
{
public:
    pass_blend_info();

    pass_blend_desc convert() noexcept { return *this; }
};

class pass_depth_stencil_info : public pass_depth_stencil_desc
{
public:
    pass_depth_stencil_desc convert() noexcept { return *this; }
};

class pass_info
{
public:
    pass_info();

    pass_desc convert() noexcept;

    std::string vertex_shader;
    std::string pixel_shader;

    vertex_layout_info vertex_layout;
    pass_layout_interface* pass_layout;
    pass_layout_info pass_layout_info;

    pass_blend_info blend;
    pass_depth_stencil_info depth_stencil;

    std::vector<std::size_t> input;
    std::vector<std::size_t> output;

    std::size_t depth;
    bool output_depth;

    primitive_topology_type primitive_topology;
};

class render_target_info : public render_target_desc
{
public:
    render_target_desc convert() noexcept { return *this; }
};

class technique_info
{
public:
    technique_desc convert() noexcept;

    std::vector<render_target_info> render_targets;
    std::vector<pass_info> subpasses;

private:
    std::vector<render_target_desc> m_render_target_desc;
    std::vector<pass_desc> m_pass_desc;
};

class render_target_set_info
{
public:
    render_target_set_desc convert() noexcept;

    std::vector<resource_interface*> render_targets;
    std::uint32_t width;
    std::uint32_t height;
    technique_interface* technique;
};

class renderer_info : public renderer_desc
{
public:
    renderer_desc convert() noexcept { return *this; }
};

class vertex_buffer_info : public vertex_buffer_desc
{
public:
    vertex_buffer_desc convert() noexcept { return *this; }
};

class index_buffer_info : public index_buffer_desc
{
public:
    index_buffer_desc convert() noexcept { return *this; }
};
} // namespace ash::graphics