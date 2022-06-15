#include "image_page.hpp"
#include "graphics/graphics.hpp"

namespace ash::sample
{
image_page::image_page() : page("Image")
{
    add_description("You can use image control to display image.");

    initialize_sample_image();
}

void image_page::initialize_sample_image()
{
    // Image file.
    add_subtitle("Image loaded from file");

    auto display_1 = add_display_panel();
    display_1->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
    display_1->align_items(ui::LAYOUT_ALIGN_CENTER);

    m_cat_image = system<graphics::graphics>().make_texture("resource/image/huhu.jpg");
    m_file_image = std::make_unique<ui::image>(m_cat_image.get());
    m_file_image->link(display_1);
}
} // namespace ash::sample