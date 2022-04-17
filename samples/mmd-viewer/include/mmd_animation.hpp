#pragma once

#include "context.hpp"
#include "math.hpp"
#include "mmd_component.hpp"
#include "transform.hpp"
#include "view.hpp"
#include <vector>

namespace ash::sample::mmd
{
class mmd_animation : public ash::core::system_base
{
public:
    mmd_animation() : system_base("mmd_animation") {}

    virtual bool initialize(const dictionary& config) override;

    void evaluate(float t, float weight = 1.0f);
    void update(bool after_physics);

private:
    void evaluate_node(mmd_node& node, mmd_node_animation& animation, float t, float weight);
    void evaluate_ik(mmd_node& node, mmd_ik_solver& ik, float t, float weight);

    void update_local(mmd_skeleton& skeleton, bool after_physics);
    void update_world(mmd_skeleton& skeleton, bool after_physics);
    void update_transform(mmd_skeleton& skeleton, std::size_t index);

    void update_inherit(mmd_node& node, mmd_node_animation& animation, scene::transform& transform);
    void update_ik(mmd_skeleton& skeleton, mmd_node& node, mmd_ik_solver& ik);

    void ik_solve_core(
        mmd_skeleton& skeleton,
        mmd_node& node,
        mmd_ik_solver& ik,
        std::size_t iteration);
    // axis: 0: x, 1: y, 2: z
    void ik_solve_plane(
        mmd_skeleton& skeleton,
        mmd_node& node,
        mmd_ik_solver& ik,
        std::size_t iteration,
        std::size_t link_index,
        uint8_t axis);

    template <typename Key>
    auto bound_key(const std::vector<Key>& keys, std::int32_t t, std::size_t start)
    {
        if (keys.empty() || keys.size() < start)
            return keys.end();

        const auto& key0 = keys[start];
        if (key0.frame <= t)
        {
            if (start + 1 < keys.size())
            {
                const auto& key1 = keys[start + 1];
                if (key1.frame > t)
                {
                    return keys.begin() + start + 1;
                }
            }
            else
            {
                return keys.end();
            }
        }
        else
        {
            if (start != 0)
            {
                const auto& key1 = keys[start - 1];
                if (key1.frame <= t)
                {
                    return keys.begin() + start;
                }
            }
            else
            {
                return keys.begin();
            }
        }

        auto bundIt =
            std::upper_bound(keys.begin(), keys.end(), t, [](int32_t lhs, const Key& rhs) {
                return lhs < rhs.frame;
            });
        return bundIt;
    }

    ecs::view<mmd_skeleton>* m_view;

    ecs::view<mmd_node, mmd_node_animation>* m_node_view;
    ecs::view<mmd_node, mmd_ik_solver>* m_ik_view;

    ecs::view<mmd_node, scene::transform>* m_transform_view;
};
} // namespace ash::sample::mmd