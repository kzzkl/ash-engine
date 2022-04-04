#include "render_pipeline.hpp"

namespace ash::graphics
{
render_pipeline::render_pipeline(layout_type* layout, pipeline_type* pipeline)
    : m_layout(layout),
      m_pipeline(pipeline),
      m_unit_parameter_count(0),
      m_pass_parameter_count(0)
{
}

void render_pipeline::render(resource* target, render_command* command, render_parameter* pass)
{
    command->pipeline(m_pipeline.get());
    command->layout(m_layout.get());

    command->parameter(m_unit_parameter_count, pass->parameter());

    for (auto& unit : m_units)
    {
        for (std::size_t i = 0; i < m_unit_parameter_count; ++i)
            command->parameter(i, unit->parameters[i]->parameter());

        command->draw(
            unit->vertex_buffer,
            unit->index_buffer,
            unit->index_start,
            unit->index_end,
            primitive_topology_type::TRIANGLE_LIST,
            target);
    }
}
} // namespace ash::graphics