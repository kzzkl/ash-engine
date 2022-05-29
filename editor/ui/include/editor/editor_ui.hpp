#pragma once

#include "editor/component_view.hpp"
#include "editor/hierarchy_view.hpp"
#include "editor/scene_view.hpp"
#include "ui/element.hpp"
#include <memory>

namespace ash::editor
{
class editor_ui
{
public:
    editor_ui();

    void tick();

private:
    std::unique_ptr<ui::element> m_root_container;
    std::unique_ptr<ui::element> m_left_container;
    std::unique_ptr<ui::element> m_right_container;

    std::unique_ptr<scene_view> m_scene_view;
    std::unique_ptr<hierarchy_view> m_hierarchy_view;
    std::unique_ptr<component_view> m_component_view;
};
} // namespace ash::editor