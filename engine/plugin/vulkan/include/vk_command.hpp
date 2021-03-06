#pragma once

#include "vk_common.hpp"
#include <vector>

namespace ash::graphics::vk
{
class vk_command : public render_command_interface
{
public:
    vk_command(VkCommandBuffer command_buffer);

    virtual void begin(
        render_pass_interface* render_pass,
        resource_interface* render_target,
        resource_interface* render_target_resolve,
        resource_interface* depth_stencil_buffer) override;
    virtual void end(render_pass_interface* render_pass) override;
    virtual void next(render_pass_interface* render_pass) override;

    virtual void scissor(const scissor_rect& rect) override;

    virtual void parameter(std::size_t i, pipeline_parameter_interface* parameter) override;
    virtual void draw(
        resource_interface* vertex,
        resource_interface* index,
        std::size_t index_start,
        std::size_t index_end,
        std::size_t vertex_base) override;

    void reset();
    VkCommandBuffer command_buffer() const noexcept { return m_command_buffer; }

private:
    VkCommandBuffer m_command_buffer;

    VkPipelineLayout m_pass_layout;
};

class vk_command_queue
{
public:
    vk_command_queue(const VkQueue& queue, std::size_t frame_resource_count);
    ~vk_command_queue();

    vk_command* allocate_command();

    void execute(vk_command* command);
    void execute_batch() {}

    VkCommandBuffer begin_dynamic_command();
    void end_dynamic_command(VkCommandBuffer command_buffer);

    VkQueue queue() const noexcept { return m_queue; }

    void switch_frame_resources();

private:
    VkQueue m_queue;
    VkCommandPool m_command_pool;

    std::size_t m_command_counter;
    std::vector<std::vector<vk_command>> m_commands;
};
} // namespace ash::graphics::vk