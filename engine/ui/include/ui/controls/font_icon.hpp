#pragma once

#include "ui/color.hpp"
#include "ui/element.hpp"

namespace ash::ui
{
class font;

struct font_icon_theme
{
    const font* icon_font;
    std::uint32_t icon_color;
    float icon_scale;
};

class font_icon : public element
{
public:
    font_icon(std::uint32_t index, const font_icon_theme& theme);

    void icon(std::uint32_t index);
    void icon_scale(float scale);
    void icon_color(std::uint32_t color);

    virtual const element_mesh* mesh() const noexcept override { return &m_mesh; }

protected:
    virtual void on_extent_change(float width, float height) override;

private:
    const font* m_font;

    float m_icon_scale;
    float m_icon_width;
    float m_icon_height;

    std::array<math::float2, 4> m_position;
    std::array<math::float2, 4> m_uv;
    std::array<std::uint32_t, 4> m_color;
    std::array<std::uint32_t, 6> m_indices;

    element_mesh m_mesh;
};
} // namespace ash::ui