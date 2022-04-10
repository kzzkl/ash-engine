#include "mmd_viewer.hpp"
#include "animation.hpp"
#include "scene.hpp"
#include "transform.hpp"

namespace ash::sample::mmd
{
bool mmd_viewer::initialize(const dictionary& config)
{
    static const std::vector<std::string> internal_toon_path = {
        "toon01.dds",
        "toon02.dds",
        "toon03.dds",
        "toon04.dds",
        "toon05.dds",
        "toon06.dds",
        "toon07.dds",
        "toon08.dds",
        "toon09.dds",
        "toon10.dds",
    };

    auto& graphics = module<graphics::graphics>();

    for (auto& path : internal_toon_path)
        m_internal_toon.push_back(graphics.make_texture("resource/mmd/" + path));

    m_pipeline = graphics.make_render_pipeline<graphics::render_pipeline>("mmd");

    return true;
}

ash::ecs::entity mmd_viewer::load_mmd(std::string_view name, std::string_view path)
{
    pmx_loader loader;
    if (!loader.load(path))
        return ecs::INVALID_ENTITY;

    auto& world = module<ash::ecs::world>();
    auto& graphics = module<ash::graphics::graphics>();

    auto& resource = m_resources[name.data()];
    resource.root = world.create();
    world.add<scene::transform, graphics::visual, skeleton>(resource.root);

    auto visual = world.component<graphics::visual>(resource.root);
    resource.object_parameter = graphics.make_render_parameter("ash_object");
    visual->object = resource.object_parameter.get();

    load_hierarchy(resource, loader);
    load_mesh(resource, loader);
    load_texture(resource, loader);
    load_material(resource, loader);
    load_physics(resource, loader);

    return resource.root;
}

void mmd_viewer::load_mesh(mmd_resource& resource, const pmx_loader& loader)
{
    struct vertex
    {
        math::float3 position;
        math::float3 normal;
        math::float2 uv;

        math::uint4 bone;
        math::float3 bone_weight;
    };

    auto& graphics = module<ash::graphics::graphics>();

    // Make vertex buffer.
    std::vector<vertex> vertices;
    vertices.reserve(loader.vertices().size());
    for (const pmx_vertex& v : loader.vertices())
        vertices.push_back(vertex{v.position, v.normal, v.uv, v.bone, v.weight});

    resource.vertex_buffer = graphics.make_vertex_buffer(vertices.data(), vertices.size());

    // Make index buffer.
    std::vector<std::int32_t> indices;
    indices.reserve(loader.indices().size());
    for (std::int32_t i : loader.indices())
        indices.push_back(i);

    resource.index_buffer = graphics.make_index_buffer(indices.data(), indices.size());
}

void mmd_viewer::load_texture(mmd_resource& resource, const pmx_loader& loader)
{
    auto& graphics = module<ash::graphics::graphics>();

    for (auto& texture_path : loader.textures())
    {
        std::string dds_path = texture_path.substr(0, texture_path.find_last_of('.')) + ".dds";
        resource.textures.push_back(graphics.make_texture(dds_path));
    }
}

void mmd_viewer::load_material(mmd_resource& resource, const pmx_loader& loader)
{
    auto& graphics = module<ash::graphics::graphics>();
    auto& world = module<ash::ecs::world>();

    for (auto& mmd_material : loader.materials())
    {
        auto parameter = graphics.make_render_parameter("mmd_material");
        parameter->set(0, mmd_material.diffuse);
        parameter->set(1, mmd_material.specular);
        parameter->set(2, mmd_material.specular_strength);
        parameter->set(3, mmd_material.toon_index == -1 ? std::uint32_t(0) : std::uint32_t(1));
        parameter->set(4, static_cast<std::uint32_t>(mmd_material.sphere_mode));
        parameter->set(5, resource.textures[mmd_material.texture_index].get());

        if (mmd_material.toon_index != -1)
        {
            if (mmd_material.toon_mode == toon_mode::TEXTURE)
                parameter->set(6, resource.textures[mmd_material.toon_index].get());
            else if (mmd_material.toon_mode == toon_mode::INTERNAL)
                parameter->set(6, m_internal_toon[mmd_material.toon_index].get());
        }
        if (mmd_material.sphere_mode != sphere_mode::DISABLED)
            parameter->set(7, resource.textures[mmd_material.sphere_index].get());

        resource.materials.push_back(std::move(parameter));
    }

    resource.submesh = loader.submesh();

    auto visual = world.component<graphics::visual>(resource.root);
    for (std::size_t i = 0; i < resource.submesh.size(); ++i)
    {
        graphics::render_unit s = {};
        s.index_start = resource.submesh[i].first;
        s.index_end = resource.submesh[i].second;
        s.vertex_buffer = resource.vertex_buffer.get();
        s.index_buffer = resource.index_buffer.get();
        s.pipeline = m_pipeline.get();
        s.parameters = {
            visual->object,
            resource.materials[i].get(),
            world.component<skeleton>(resource.root)->parameter.get()};

        visual->submesh.push_back(s);
    }
}

void mmd_viewer::load_hierarchy(mmd_resource& resource, const pmx_loader& loader)
{
    auto& graphics = module<ash::graphics::graphics>();
    auto& world = module<ash::ecs::world>();
    auto& scene = module<ash::scene::scene>();

    auto actor_skeleton = world.component<skeleton>(resource.root);
    actor_skeleton->offset.resize(loader.bones().size());
    actor_skeleton->parameter = graphics.make_render_parameter("mmd_skeleton");

    resource.hierarchy.reserve(loader.bones().size());
    for (std::size_t i = 0; i < loader.bones().size(); ++i)
    {
        ecs::entity node = world.create();
        world.add<scene::transform>(node);
        resource.hierarchy.push_back(node);
    }

    for (std::size_t i = 0; i < loader.bones().size(); ++i)
    {
        const auto& mmd_bone = loader.bones()[i];

        auto node = world.component<scene::transform>(resource.hierarchy[i]);
        if (mmd_bone.parent_index != -1)
        {
            auto parent_node =
                world.component<scene::transform>(resource.hierarchy[mmd_bone.parent_index]);
            scene.link(*node, *parent_node);

            math::float3 local_position = math::vector_plain::sub(
                mmd_bone.position,
                loader.bones()[mmd_bone.parent_index].position);
            // node->position(local_position);
        }
        else
        {
            // node->position(mmd_bone.position);
            scene.link(*node, *world.component<scene::transform>(resource.root));
        }

        actor_skeleton->nodes.push_back(node->node.get());

        // TODO
    }
}

void mmd_viewer::load_physics(mmd_resource& resource, const pmx_loader& loader)
{
    auto& world = module<ecs::world>();

    std::vector<ecs::write<physics::rigidbody>> rigidbodies;

    resource.collision_shapes.reserve(loader.rigidbodies().size());
    for (auto& mmd_rigidbody : loader.rigidbodies())
    {
        // Make collision shape.
        physics::collision_shape_desc desc = {};
        switch (mmd_rigidbody.shape)
        {
        case pmx_rigidbody_shape_type::SPHERE:
            desc.type = physics::collision_shape_type::SPHERE;
            desc.sphere.radius = mmd_rigidbody.size[0];
            break;
        case pmx_rigidbody_shape_type::BOX:
            desc.type = physics::collision_shape_type::BOX;
            desc.box.length = mmd_rigidbody.size[0] * 2.0f;
            desc.box.height = mmd_rigidbody.size[1] * 2.0f;
            desc.box.width = mmd_rigidbody.size[2] * 2.0f;
            break;
        case pmx_rigidbody_shape_type::CAPSULE:
            desc.type = physics::collision_shape_type::CAPSULE;
            desc.capsule.radius = mmd_rigidbody.size[0];
            desc.capsule.height = mmd_rigidbody.size[1];
            break;
        default:
            break;
        }
        resource.collision_shapes.push_back(module<ash::physics::physics>().make_shape(desc));

        // Make rigidbody.
        ecs::entity node;
        if (mmd_rigidbody.bone_index != -1)
            node = resource.hierarchy[mmd_rigidbody.bone_index];
        else
            node = resource.root;

        world.add<physics::rigidbody>(node);
        auto rigidbody = world.component<physics::rigidbody, ecs::write>(node);
        rigidbodies.push_back(rigidbody);

        rigidbody->shape(resource.collision_shapes.back().get());
        rigidbody->mass(
            mmd_rigidbody.mode == pmx_rigidbody_mode::STATIC ? 0.0f : mmd_rigidbody.mass);
        rigidbody->linear_dimmer(mmd_rigidbody.translate_dimmer);
        rigidbody->angular_dimmer(mmd_rigidbody.rotate_dimmer);
        rigidbody->restitution(mmd_rigidbody.repulsion);
        rigidbody->friction(mmd_rigidbody.friction);

        switch (mmd_rigidbody.mode)
        {
        case pmx_rigidbody_mode::STATIC:
            rigidbody->type(physics::rigidbody_type::KINEMATIC);
            break;
        case pmx_rigidbody_mode::DYNAMIC:
            rigidbody->type(physics::rigidbody_type::DYNAMIC);
            break;
        case pmx_rigidbody_mode::MERGE:
            rigidbody->type(physics::rigidbody_type::KINEMATIC);
            break;
        default:
            break;
        }

        math::float4_simd position_offset = math::simd::load(mmd_rigidbody.translate);
        math::float4_simd rotation_offset = math::simd::load(mmd_rigidbody.rotate);
        math::float4x4 offset;
        math::simd::store(
            math::matrix_simd::affine_transform(
                math::simd::set(1.0f, 1.0f, 1.0f, 0.0f),
                rotation_offset,
                position_offset),
            offset);
        rigidbody->offset(offset);
        rigidbody->collision_group(1 << mmd_rigidbody.group);
        rigidbody->collision_mask(mmd_rigidbody.collision_group);
    }

    for (auto& mmd_joint : loader.joints())
    {
        auto rigidbody_a = rigidbodies[mmd_joint.rigidbody_a_index];
        auto rigidbody_b = rigidbodies[mmd_joint.rigidbody_b_index];

        if (!world.has_component<physics::joint>(rigidbody_a.entity()))
            world.add<physics::joint>(rigidbody_a.entity());

        auto joint = world.component<physics::joint>(rigidbody_a.entity());

        std::size_t index = joint->add_unit();
        joint->rigidbody(index, rigidbody_b);
        joint->location(index, mmd_joint.translate);
        joint->rotation(
            index,
            math::quaternion_plain::rotation_euler(
                mmd_joint.rotate[1],
                mmd_joint.rotate[0],
                mmd_joint.rotate[2]));
        joint->min_linear(index, mmd_joint.translate_min);
        joint->max_linear(index, mmd_joint.translate_max);
        joint->min_angular(index, mmd_joint.rotate_min);
        joint->max_angular(index, mmd_joint.rotate_max);
        joint->spring_translate_factor(index, mmd_joint.spring_translate_factor);
        joint->spring_rotate_factor(index, mmd_joint.spring_rotate_factor);
    }
}
} // namespace ash::sample::mmd