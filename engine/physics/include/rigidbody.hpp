#pragma once

#include "ecs.hpp"
#include "physics_interface.hpp"
#include "transform.hpp"

namespace ash::physics
{
struct rigidbody
{
    static constexpr std::uint32_t COLLISION_MASK_ALL = -1;

    ecs::entity relation{ecs::INVALID_ENTITY};

    rigidbody_type type{rigidbody_type::DYNAMIC};
    float mass{0.0f};
    float linear_dimmer;
    float angular_dimmer;
    float restitution;
    float friction;

    collision_shape_interface* shape;

    std::uint32_t collision_group{1};
    std::uint32_t collision_mask{COLLISION_MASK_ALL};

    math::float4x4 offset{math::matrix_plain::identity()};
    math::float4x4 offset_inverse{math::matrix_plain::identity()};

    bool in_world{false};

    std::unique_ptr<rigidbody_interface> interface;
};
} // namespace ash::physics