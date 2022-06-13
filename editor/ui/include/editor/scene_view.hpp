#pragma once

#include "ecs/entity.hpp"
#include "editor/editor_view.hpp"
#include "graphics_interface.hpp"
#include "ui/controls/image.hpp"
#include <memory>

namespace ash::editor
{
class scene_view : public editor_view
{
public:
    scene_view();

    void tick();

    ecs::entity scene_camera() const noexcept { return m_camera; }

private:
    void update_camera();
    void resize_camera();

    std::unique_ptr<ui::image> m_image;

    ecs::entity m_camera;
    std::unique_ptr<graphics::resource> m_render_target;
    std::unique_ptr<graphics::resource> m_render_target_resolve;
    std::unique_ptr<graphics::resource> m_depth_stencil_buffer;

    float m_camera_move_speed;
    float m_camera_rotate_speed;

    bool m_mouse_flag;
    math::float2 m_mouse_position;

    std::uint32_t m_image_width;
    std::uint32_t m_image_height;

    bool m_focused;
};
} // namespace ash::editor