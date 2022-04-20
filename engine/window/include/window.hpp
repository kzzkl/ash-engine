#pragma once

#include "context.hpp"
#include "window_impl.hpp"

namespace ash::window
{
class window : public ash::core::system_base
{
public:
    static constexpr const char* TASK_WINDOW_TICK = "window tick";

public:
    window();

    virtual bool initialize(const dictionary& config) override;

    mouse& mouse() { return m_impl->mouse(); }
    keyboard& keyboard() { return m_impl->keyboard(); }

    void* handle() const { return m_impl->handle(); }
    window_rect rect() const { return m_impl->rect(); }

    void title(std::string_view title) { m_impl->title(title); }

private:
    std::unique_ptr<window_impl> m_impl;
};
} // namespace ash::window