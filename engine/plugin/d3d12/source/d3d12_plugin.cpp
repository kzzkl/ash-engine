#include "d3d12_context.hpp"
#include "d3d12_pipeline.hpp"
#include "graphics_interface.hpp"
#include <cstring>

namespace ash::graphics::d3d12
{
class d3d12_factory : public factory
{
public:
    virtual pass_parameter* make_pass_parameter(
        const pass_parameter_desc& desc) override
    {
        return new d3d12_pass_parameter(desc);
    }

    virtual pass_layout* make_pass_layout(const pass_layout_desc& desc) override
    {
        return new d3d12_parameter_layout(desc);
    }

    virtual pipeline* make_pipeline(const pass_desc& desc) override
    {
        return new d3d12_pipeline(desc);
    }

    virtual resource* make_upload_buffer(std::size_t size) override
    {
        return new d3d12_upload_buffer(nullptr, size);
    }

    virtual resource* make_vertex_buffer(const vertex_buffer_desc& desc) override
    {
        if (desc.dynamic)
        {
            return new d3d12_vertex_buffer<d3d12_upload_buffer>(desc, nullptr);
        }
        else
        {
            auto command_list = d3d12_context::command()->allocate_dynamic_command();
            auto result = new d3d12_vertex_buffer<d3d12_default_buffer>(desc, command_list.get());
            d3d12_context::command()->execute_command(command_list);
            return result;
        }
    }

    virtual resource* make_index_buffer(const index_buffer_desc& desc) override
    {
        if (desc.dynamic)
        {
            return new d3d12_index_buffer<d3d12_upload_buffer>(desc, nullptr);
        }
        else
        {
            auto command_list = d3d12_context::command()->allocate_dynamic_command();
            auto result = new d3d12_index_buffer<d3d12_default_buffer>(desc, command_list.get());
            d3d12_context::command()->execute_command(command_list);
            return result;
        }
    }

    virtual resource* make_texture(const std::uint8_t* data, std::size_t size) override
    {
        auto command_list = d3d12_context::command()->allocate_dynamic_command();
        d3d12_texture* result = new d3d12_texture(data, size, command_list.get());
        d3d12_context::command()->execute_command(command_list);
        return result;
    }

    virtual resource* make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height) override
    {
        auto command_list = d3d12_context::command()->allocate_dynamic_command();
        d3d12_texture* result = new d3d12_texture(data, width, height, command_list.get());
        d3d12_context::command()->execute_command(command_list);
        return result;
    }

    virtual resource* make_render_target(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling) override
    {
        if (multiple_sampling == 1)
            return new d3d12_render_target(
                width,
                height,
                RENDER_TARGET_FORMAT,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        else
            return new d3d12_render_target_mutlisample(
                width,
                height,
                RENDER_TARGET_FORMAT,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                multiple_sampling,
                true);
    }

    virtual resource* make_depth_stencil(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling) override
    {
        return new d3d12_depth_stencil_buffer(
            width,
            height,
            DEPTH_STENCIL_FORMAT,
            multiple_sampling);
    }
};

class d3d12_context_wrapper : public context
{
public:
    virtual bool initialize(const context_config& config) override
    {
        return d3d12_context::initialize(config);
    }

    virtual renderer_type* renderer() override { return d3d12_context::renderer(); }
    virtual factory_type* factory() { return &m_factory; }

private:
    d3d12_factory m_factory;
};
} // namespace ash::graphics::d3d12

extern "C"
{
    PLUGIN_API ash::core::plugin_info get_plugin_info()
    {
        ash::core::plugin_info info = {};

        char name[] = "graphics-d3d12";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API ash::graphics::context* make_context()
    {
        return new ash::graphics::d3d12::d3d12_context_wrapper();
    }
}