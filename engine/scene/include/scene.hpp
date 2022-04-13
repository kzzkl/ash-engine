#pragma once

#include "context.hpp"
#include "transform.hpp"
#include "view.hpp"
#include <memory>

namespace ash::scene
{
class scene : public ash::core::system_base
{
public:
    scene();
    scene(const scene&) = delete;

    virtual bool initialize(const dictionary& config) override;

    void sync_local();
    void sync_local(ecs::entity root);
    void sync_world();
    void sync_world(ecs::entity root);

    scene& operator=(const scene&) = delete;

    void reset_sync_counter();

    void link(ecs::entity entity);
    void link(ecs::entity child_entity, ecs::entity parent_entity);
    void unlink(ecs::entity entity, bool send_event = true);

    template <typename Functor>
    void each_children(ecs::entity entity, Functor&& functor)
    {
        auto& world = system<ecs::world>();

        std::queue<ecs::entity> bfs;
        bfs.push(entity);

        while (!bfs.empty())
        {
            ecs::entity node = bfs.front();
            bfs.pop();

            if (!world.has_component<transform>(node))
                continue;

            functor(node);

            auto& t = world.component<transform>(node);
            for (auto child : t.children)
                bfs.push(child);
        }
    }

private:
    std::queue<ecs::entity> find_dirty_node(ecs::entity root) const;

    ash::ecs::view<transform>* m_view;
    ash::ecs::entity m_root;
};
} // namespace ash::scene