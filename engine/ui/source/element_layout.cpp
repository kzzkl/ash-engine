#include "ui/element_layout.hpp"
#include "assert.hpp"
#include <yoga/Yoga.h>

namespace ash::ui
{
class layout_node_impl_yoga : public layout_node_impl
{
private:
    static constexpr YGDirection INTERNAL_YOGA_DIRECTION_MAP[] = {
        YGDirectionInherit,
        YGDirectionLTR,
        YGDirectionRTL};
    static constexpr YGFlexDirection INTERNAL_YOGA_FLEX_DIRECTION_MAP[] = {
        YGFlexDirectionColumn,
        YGFlexDirectionColumnReverse,
        YGFlexDirectionRow,
        YGFlexDirectionRowReverse};
    static constexpr YGWrap INTERNAL_YOGA_FLEX_WRAP_MAP[] = {
        YGWrapNoWrap,
        YGWrapWrap,
        YGWrapWrapReverse};
    static constexpr YGJustify INTERNAL_YOGA_JUSTIFY_MAP[] = {
        YGJustifyFlexStart,
        YGJustifyCenter,
        YGJustifyFlexEnd,
        YGJustifySpaceBetween,
        YGJustifySpaceAround,
        YGJustifySpaceEvenly};
    static constexpr YGAlign INTERNAL_YOGA_ALIGN_MAP[] = {
        YGAlignAuto,
        YGAlignFlexStart,
        YGAlignCenter,
        YGAlignFlexEnd,
        YGAlignStretch,
        YGAlignBaseline,
        YGAlignSpaceBetween,
        YGAlignSpaceAround};
    static constexpr YGEdge INTERNAL_YOGA_EDGE_MAP[] = {
        YGEdgeLeft,
        YGEdgeTop,
        YGEdgeRight,
        YGEdgeBottom,
        YGEdgeStart,
        YGEdgeEnd,
        YGEdgeHorizontal,
        YGEdgeVertical,
        YGEdgeAll};
    static constexpr YGPositionType INTERNAL_YOGA_POSITION_TYPE_MAP[] = {
        YGPositionTypeStatic,
        YGPositionTypeRelative,
        YGPositionTypeAbsolute};

    static constexpr layout_direction YOGA_INTERNAL_DIRECTION_MAP[] = {
        LAYOUT_DIRECTION_INHERIT,
        LAYOUT_DIRECTION_LTR,
        LAYOUT_DIRECTION_RTL};
    static constexpr layout_flex_direction YOGA_INTERNAL_FLEX_DIRECTION_MAP[] = {
        LAYOUT_FLEX_DIRECTION_COLUMN,
        LAYOUT_FLEX_DIRECTION_COLUMN_REVERSE,
        LAYOUT_FLEX_DIRECTION_ROW,
        LAYOUT_FLEX_DIRECTION_ROW_REVERSE};
    static constexpr layout_flex_wrap YOGA_INTERNAL_FLEX_WRAP_MAP[] = {
        LAYOUT_FLEX_WRAP_NOWRAP,
        LAYOUT_FLEX_WRAP_WRAP,
        LAYOUT_FLEX_WRAP_WRAP_REVERSE};
    static constexpr layout_justify YOGA_INTERNAL_JUSTIFY_MAP[] = {
        LAYOUT_JUSTIFY_FLEX_START,
        LAYOUT_JUSTIFY_CENTER,
        LAYOUT_JUSTIFY_FLEX_END,
        LAYOUT_JUSTIFY_SPACE_BETWEEN,
        LAYOUT_JUSTIFY_SPACE_AROUND,
        LAYOUT_JUSTIFY_SPACE_EVENLY};
    static constexpr layout_align YOGA_INTERNAL_ALIGN_MAP[] = {
        LAYOUT_ALIGN_AUTO,
        LAYOUT_ALIGN_FLEX_START,
        LAYOUT_ALIGN_CENTER,
        LAYOUT_ALIGN_FLEX_END,
        LAYOUT_ALIGN_STRETCH,
        LAYOUT_ALIGN_BASELINE,
        LAYOUT_ALIGN_SPACE_BETWEEN,
        LAYOUT_ALIGN_SPACE_AROUND};
    static constexpr layout_edge YOGA_INTERNAL_EDGE_MAP[] = {
        LAYOUT_EDGE_LEFT,
        LAYOUT_EDGE_TOP,
        LAYOUT_EDGE_RIGHT,
        LAYOUT_EDGE_BOTTOM,
        LAYOUT_EDGE_START,
        LAYOUT_EDGE_END,
        LAYOUT_EDGE_HORIZONTAL,
        LAYOUT_EDGE_VERTICAL,
        LAYOUT_EDGE_ALL};
    static constexpr layout_position_type YOGA_INTERNAL_POSITION_TYPE_MAP[] = {
        LAYOUT_POSITION_TYPE_STATIC,
        LAYOUT_POSITION_TYPE_RELATIVE,
        LAYOUT_POSITION_TYPE_ABSOLUTE};

public:
    layout_node_impl_yoga(bool is_root) : m_config(nullptr), m_absolute_x(0.0f), m_absolute_y(0.0f)
    {
        if (is_root)
        {
            m_config = YGConfigNew();
            m_node = YGNodeNewWithConfig(m_config);
        }
        else
        {
            m_node = YGNodeNew();
        }
    }

    virtual void direction(layout_direction direction) override
    {
        YGNodeStyleSetDirection(m_node, INTERNAL_YOGA_DIRECTION_MAP[direction]);
    }
    virtual void flex_direction(layout_flex_direction flex_direction) override
    {
        YGNodeStyleSetFlexDirection(m_node, INTERNAL_YOGA_FLEX_DIRECTION_MAP[flex_direction]);
    }
    virtual void flex_basis(float basis) override { YGNodeStyleSetFlexBasis(m_node, basis); }
    virtual void flex_grow(float grow) override { YGNodeStyleSetFlexGrow(m_node, grow); }
    virtual void flex_shrink(float shrink) override { YGNodeStyleSetFlexShrink(m_node, shrink); }
    virtual void flex_wrap(layout_flex_wrap wrap) override
    {
        YGNodeStyleSetFlexWrap(m_node, INTERNAL_YOGA_FLEX_WRAP_MAP[wrap]);
    }
    virtual void justify_content(layout_justify justify) override
    {
        YGNodeStyleSetJustifyContent(m_node, INTERNAL_YOGA_JUSTIFY_MAP[justify]);
    }
    virtual void align_items(layout_align align) override
    {
        YGNodeStyleSetAlignItems(m_node, INTERNAL_YOGA_ALIGN_MAP[align]);
    }
    virtual void align_self(layout_align align) override
    {
        YGNodeStyleSetAlignSelf(m_node, INTERNAL_YOGA_ALIGN_MAP[align]);
    }
    virtual void align_content(layout_align align) override
    {
        YGNodeStyleSetAlignContent(m_node, INTERNAL_YOGA_ALIGN_MAP[align]);
    }
    virtual void padding(float padding, layout_edge edge) override
    {
        YGNodeStyleSetPadding(m_node, INTERNAL_YOGA_EDGE_MAP[edge], padding);
    }
    virtual void border(float border, layout_edge edge) override
    {
        YGNodeStyleSetBorder(m_node, INTERNAL_YOGA_EDGE_MAP[edge], border);
    }
    virtual void margin(float margin, layout_edge edge) override
    {
        YGNodeStyleSetMargin(m_node, INTERNAL_YOGA_EDGE_MAP[edge], margin);
    }
    virtual void display(bool display) override
    {
        if (display)
            YGNodeStyleSetDisplay(m_node, YGDisplayFlex);
        else
            YGNodeStyleSetDisplay(m_node, YGDisplayNone);
    }
    virtual void position_type(layout_position_type position_type) override
    {
        YGNodeStyleSetPositionType(m_node, INTERNAL_YOGA_POSITION_TYPE_MAP[position_type]);
    }
    virtual void position(float position, layout_edge edge, bool percent) override
    {
        if (percent)
            YGNodeStyleSetPositionPercent(m_node, INTERNAL_YOGA_EDGE_MAP[edge], position);
        else
            YGNodeStyleSetPosition(m_node, INTERNAL_YOGA_EDGE_MAP[edge], position);
    }

    virtual layout_direction direction() const override
    {
        return YOGA_INTERNAL_DIRECTION_MAP[YGNodeStyleGetDirection(m_node)];
    }
    virtual layout_flex_direction flex_direction() const override
    {
        return YOGA_INTERNAL_FLEX_DIRECTION_MAP[YGNodeStyleGetFlexDirection(m_node)];
    }
    virtual float flex_basis() const override { return YGNodeStyleGetFlexBasis(m_node).value; }
    virtual float flex_grow() const override { return YGNodeStyleGetFlexGrow(m_node); }
    virtual float flex_shrink() const override { return YGNodeStyleGetFlexShrink(m_node); }
    virtual layout_flex_wrap flex_wrap() const override
    {
        return YOGA_INTERNAL_FLEX_WRAP_MAP[YGNodeStyleGetFlexWrap(m_node)];
    }
    virtual layout_justify justify_content() const override
    {
        return YOGA_INTERNAL_JUSTIFY_MAP[YGNodeStyleGetJustifyContent(m_node)];
    }
    virtual layout_align align_items() const override
    {
        return YOGA_INTERNAL_ALIGN_MAP[YGNodeStyleGetAlignItems(m_node)];
    }
    virtual layout_align align_self() const override
    {
        return YOGA_INTERNAL_ALIGN_MAP[YGNodeStyleGetAlignSelf(m_node)];
    }
    virtual layout_align align_content() const override
    {
        return YOGA_INTERNAL_ALIGN_MAP[YGNodeStyleGetAlignContent(m_node)];
    }
    virtual float padding(layout_edge edge) const override
    {
        return YGNodeStyleGetPadding(m_node, INTERNAL_YOGA_EDGE_MAP[edge]).value;
    }
    virtual float border(layout_edge edge) const override
    {
        return YGNodeStyleGetBorder(m_node, INTERNAL_YOGA_EDGE_MAP[edge]);
    }
    virtual float margin(layout_edge edge) const override
    {
        return YGNodeStyleGetMargin(m_node, INTERNAL_YOGA_EDGE_MAP[edge]).value;
    }

    virtual void add_child(layout_node_impl* child, std::size_t index) override
    {
        auto yoga_child = static_cast<layout_node_impl_yoga*>(child);
        YGNodeInsertChild(m_node, yoga_child->m_node, static_cast<std::uint32_t>(index));
    }

    virtual void remove_child(layout_node_impl* child) override
    {
        auto yoga_child = static_cast<layout_node_impl_yoga*>(child);
        ASH_ASSERT(YGNodeGetParent(yoga_child->m_node) == m_node);
        YGNodeRemoveChild(m_node, yoga_child->m_node);
    }

    void calculate(float width, float height) override
    {
        YGNodeCalculateLayout(m_node, width, height, YGDirectionLTR);
    }

    virtual void calculate_absolute_position(float parent_x, float parent_y) override
    {
        m_absolute_x = parent_x + YGNodeLayoutGetLeft(m_node);
        m_absolute_y = parent_y + YGNodeLayoutGetTop(m_node);
    }

    virtual void width(float value) override { YGNodeStyleSetWidth(m_node, value); }
    virtual void width_auto() override { YGNodeStyleSetWidthAuto(m_node); }
    virtual void width_percent(float value) override { YGNodeStyleSetWidthPercent(m_node, value); }
    virtual void width_min(float value) override { YGNodeStyleSetMinWidth(m_node, value); }
    virtual void width_max(float value) override { YGNodeStyleSetMaxWidth(m_node, value); }

    virtual void height(float value) override { YGNodeStyleSetHeight(m_node, value); }
    virtual void height_auto() override { YGNodeStyleSetHeightAuto(m_node); }
    virtual void height_percent(float value) override
    {
        YGNodeStyleSetHeightPercent(m_node, value);
    }
    virtual void height_min(float value) override { YGNodeStyleSetMinHeight(m_node, value); }
    virtual void height_max(float value) override { YGNodeStyleSetMaxHeight(m_node, value); }

    virtual void copy_style(layout_node_impl* target)
    {
        auto target_node = static_cast<layout_node_impl_yoga*>(target)->m_node;
        YGNodeCopyStyle(target_node, m_node);
    }

    virtual element_extent extent() const override
    {
        return element_extent{
            .x = m_absolute_x,
            .y = m_absolute_y,
            .width = YGNodeLayoutGetWidth(m_node),
            .height = YGNodeLayoutGetHeight(m_node)};
    }

    virtual bool dirty() const override { return YGNodeIsDirty(m_node); }

private:
    YGNodeRef m_node;
    YGConfigRef m_config;

    float m_absolute_x;
    float m_absolute_y;
};

element_layout::element_layout(bool is_root)
    : m_impl(std::make_unique<layout_node_impl_yoga>(is_root))
{
}
} // namespace ash::ui