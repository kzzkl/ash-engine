#include "animation.hpp"

namespace ash::sample::mmd
{
bool animation::initialize(const ash::dictionary& config)
{
    module<ash::ecs::world>().register_component<skeleton>();
    m_view = module<ash::ecs::world>().make_view<skeleton, ash::graphics::visual>();

    return true;
}

void animation::update()
{
    m_view->each([](skeleton& s, ash::graphics::visual& v) {
        for (std::size_t i = 0; i < s.nodes.size(); ++i)
            s.offset[i] = s.nodes[i]->to_world;

        s.parameter->set(0, s.offset.data(), s.offset.size());

        /*for (auto& submesh : v.submesh)
            submesh.parameters[2] = s.parameter.get();*/
    });
}
} // namespace ash::sample::mmd