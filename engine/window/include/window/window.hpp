#pragma once

#include "core/context.hpp"
#include "window/window_impl.hpp"

namespace ash::window
{
class window : public ash::core::system_base
{
public:
    using mouse_type = mouse;
    using keyboard_type = keyboard;

public:
    window();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    void tick();

    mouse_type& mouse() { return m_mouse; }
    keyboard_type& keyboard() { return m_keyboard; }

    void* handle() const { return m_impl->handle(); }
    window_extent extent() const { return m_impl->extent(); }

    void title(std::string_view title);

private:
    std::unique_ptr<window_impl> m_impl;

    mouse_type m_mouse;
    keyboard_type m_keyboard;

    std::string m_title;

#if defined(ASH_WINDOW_SHOW_FPS)
    float m_average_duration;
    int m_fps;
#endif
};
} // namespace ash::window