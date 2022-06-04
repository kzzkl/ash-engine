#include "gallery.hpp"
#include "button_page.hpp"
#include "image_page.hpp"
#include "style.hpp"
#include "tree_page.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
gallery::gallery() : m_current_page(nullptr)
{
}

void gallery::initialize()
{
    auto& ui = system<ui::ui>();

    ui.load_font(style::title_1.font, style::title_1.font_path, style::title_1.font_size);
    ui.load_font(style::title_2.font, style::title_2.font_path, style::title_2.font_size);
    ui.load_font(style::content.font, style::content.font_path, style::content.font_size);

    m_left = std::make_unique<ui::panel>(style::background_color);
    m_left->link(ui.root());

    m_navigation_node_style = ui::tree_node::default_style;
    m_navigation_node_style.padding_top = 8.0f;
    m_navigation_node_style.padding_bottom = 8.0f;
    m_navigation_node_style.padding_increment = 10.0f;
    m_navigation_node_style.icon_scale = 0.7f;

    m_navigation_tree = std::make_unique<ui::tree>();
    m_navigation_tree->width_min(300.0f);
    m_navigation_tree->on_select = [this](ui::tree_node* node) {
        auto& page = m_pages[node->name()];
        if (page != nullptr && m_current_page != page.get())
        {
            if (m_current_page != nullptr)
                m_main_view->remove(m_current_page);

            m_main_view->add(page.get());
            m_current_page = page.get();
        }
    };
    m_navigation_tree->link(m_left.get());

    m_main_view = std::make_unique<ui::scroll_view>();
    m_main_view->flex_grow(1.0f);
    m_main_view->link(ui.root());

    initialize_basic();
    initialize_views();
}

void gallery::initialize_basic()
{
    auto& text_font = system<ui::ui>().font("content");
    auto& icon_font = system<ui::ui>().font("remixicon");

    m_nodes["Basic"] = std::make_unique<ui::tree_node>(
        "Basic",
        text_font,
        0xEB82,
        icon_font,
        m_navigation_node_style);
    m_navigation_tree->add(m_nodes["Basic"].get());

    // Button.
    m_pages["Button"] = std::make_unique<button_page>();
    m_nodes["Button"] = std::make_unique<ui::tree_node>(
        "Button",
        text_font,
        0xEB7E,
        icon_font,
        m_navigation_node_style);
    m_nodes["Basic"]->add(m_nodes["Button"].get());

    // Image.
    m_pages["Image"] = std::make_unique<image_page>();
    m_nodes["Image"] = std::make_unique<ui::tree_node>(
        "Image",
        text_font,
        0xEE4A,
        icon_font,
        m_navigation_node_style);
    m_nodes["Basic"]->add(m_nodes["Image"].get());
}

void gallery::initialize_views()
{
    auto& text_font = system<ui::ui>().font("content");
    auto& icon_font = system<ui::ui>().font("remixicon");

    m_nodes["Views"] = std::make_unique<ui::tree_node>(
        "Views",
        text_font,
        0xEE84,
        icon_font,
        m_navigation_node_style);
    m_navigation_tree->add(m_nodes["Views"].get());

    // Tree.
    m_pages["Tree"] = std::make_unique<tree_page>();
    m_nodes["Tree"] = std::make_unique<ui::tree_node>(
        "Tree",
        text_font,
        0xEEBA,
        icon_font,
        m_navigation_node_style);
    m_nodes["Views"]->add(m_nodes["Tree"].get());
}
} // namespace ash::sample