#include "light_viewer.hpp"
#include "core/relation.hpp"
#include "core/timer.hpp"
#include "graphics/camera.hpp"
#include "graphics/graphics.hpp"
#include "graphics/rhi.hpp"
#include "scene/scene.hpp"
#include "task/task_manager.hpp"
#include "window/window.hpp"

namespace ash::sample
{
light_viewer::light_viewer() : core::system_base("light_viewer")
{
}

bool light_viewer::initialize(const dictionary& config)
{
    initialize_graphics_resource();
    initialize_task();
    initialize_scene();

    return true;
}

void light_viewer::initialize_graphics_resource()
{
    // Initialize cube mesh.
    m_cube_mesh_data = graphics::geometry::box(1.0f, 1.0f, 1.0f);
    m_cube_positon_buffer = graphics::rhi::make_vertex_buffer(
        m_cube_mesh_data.position.data(),
        m_cube_mesh_data.position.size());
    m_cube_normal_buffer = graphics::rhi::make_vertex_buffer(
        m_cube_mesh_data.normal.data(),
        m_cube_mesh_data.normal.size());
    m_cube_index_buffer = graphics::rhi::make_index_buffer(
        m_cube_mesh_data.indices.data(),
        m_cube_mesh_data.indices.size());

    // Initialize sphere mesh.
    m_sphere_mesh_data = graphics::geometry::shpere(1.0f, 40, 20);
    m_sphere_positon_buffer = graphics::rhi::make_vertex_buffer(
        m_sphere_mesh_data.position.data(),
        m_sphere_mesh_data.position.size());
    m_sphere_normal_buffer = graphics::rhi::make_vertex_buffer(
        m_sphere_mesh_data.normal.data(),
        m_sphere_mesh_data.normal.size());
    m_sphere_index_buffer = graphics::rhi::make_index_buffer(
        m_sphere_mesh_data.indices.data(),
        m_sphere_mesh_data.indices.size());

    // Initialize blinn phong pipeline.
    m_material = std::make_unique<graphics::blinn_phong_material_pipeline_parameter>();
    m_material->diffuse(math::float3{1.0f, 1.0f, 1.0f});
    m_material->fresnel(math::float3{0.01f, 0.01f, 0.01f});
    m_material->roughness(0.2f);
    m_pipeline = std::make_unique<graphics::blinn_phong_pipeline>();

    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    // Initialize main camera.
    m_camera = world.create("main camera");
    world.add<core::link, graphics::camera, scene::transform>(m_camera);
    auto& camera = world.component<graphics::camera>(m_camera);
    camera.field_of_view(math::to_radians(30.0f));
    camera.clipping_planes(0.01f, 1000.0f);

    auto& transform = world.component<scene::transform>(m_camera);
    transform.position(math::float3{0.0f, 0.0f, -38.0f});
    relation.link(m_camera, scene.root());

    auto& graphics = system<graphics::graphics>();
    auto extent = graphics.render_extent();
    resize_camera(extent.width, extent.height);

    graphics.game_camera(m_camera);
}

void light_viewer::initialize_task()
{
    auto& task = system<task::task_manager>();
    auto tick_task = task.schedule("bvh tick", [this]() {
        update_camera();
        system<scene::scene>().sync_local();
    });

    tick_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_START));
    task.find(task::TASK_GAME_LOGIC_END)->add_dependency(*tick_task);
}

void light_viewer::initialize_scene()
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    // Light.
    m_light = world.create("light");
    world.add<scene::transform, core::link, graphics::directional_light>(m_light);
    auto& directional_light = world.component<graphics::directional_light>(m_light);
    directional_light.color(math::float3{0.5f, 0.5f, 0.5f});
    relation.link(m_light, scene.root());

    {
        // Cube.
        m_cube = world.create("cube");
        world.add<scene::transform, scene::bounding_box, graphics::mesh_render, core::link>(m_cube);

        auto& mesh = world.component<graphics::mesh_render>(m_cube);
        mesh.vertex_buffers = {m_cube_positon_buffer.get(), m_cube_normal_buffer.get()};
        mesh.index_buffer = m_cube_index_buffer.get();
        mesh.object_parameter = std::make_unique<graphics::object_pipeline_parameter>();

        graphics::material material = {};
        material.pipeline = m_pipeline.get();
        material.parameters = {mesh.object_parameter->interface(), m_material->interface()};
        mesh.materials.push_back(material);
        mesh.submeshes.push_back(graphics::submesh{0, m_cube_mesh_data.indices.size(), 0});

        auto& transform = world.component<scene::transform>(m_cube);
        auto& bounding_box = world.component<scene::bounding_box>(m_cube);
        bounding_box.aabb(m_cube_mesh_data.position, transform.to_world(), true);

        relation.link(m_cube, scene.root());
    }

    {
        // Sphere.
        m_sphere = world.create("sphere");
        world.add<scene::transform, scene::bounding_box, graphics::mesh_render, core::link>(
            m_sphere);

        auto& mesh = world.component<graphics::mesh_render>(m_sphere);
        mesh.vertex_buffers = {m_sphere_positon_buffer.get(), m_sphere_normal_buffer.get()};
        mesh.index_buffer = m_sphere_index_buffer.get();
        mesh.object_parameter = std::make_unique<graphics::object_pipeline_parameter>();

        graphics::material material = {};
        material.pipeline = m_pipeline.get();
        material.parameters = {mesh.object_parameter->interface(), m_material->interface()};
        mesh.materials.push_back(material);
        mesh.submeshes.push_back(graphics::submesh{0, m_sphere_mesh_data.indices.size(), 0});

        auto& transform = world.component<scene::transform>(m_sphere);
        transform.position(math::float3{0.0f, 3.0f, 0.0f});

        auto& bounding_box = world.component<scene::bounding_box>(m_sphere);
        bounding_box.aabb(m_sphere_mesh_data.position, transform.to_world(), true);

        relation.link(m_sphere, scene.root());
    }

    scene.sync_local();
}

void light_viewer::update_camera()
{
    auto& world = system<ecs::world>();
    auto& keyboard = system<window::window>().keyboard();
    auto& mouse = system<window::window>().mouse();

    float delta = system<core::timer>().frame_delta();

    if (keyboard.key(window::KEYBOARD_KEY_1).release())
    {
        if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
            mouse.mode(window::MOUSE_MODE_ABSOLUTE);
        else
            mouse.mode(window::MOUSE_MODE_RELATIVE);
    }

    auto& transform = world.component<scene::transform>(m_camera);
    if (mouse.mode() == window::MOUSE_MODE_RELATIVE)
    {
        m_heading += mouse.x() * m_rotate_speed * delta;
        m_pitch += mouse.y() * m_rotate_speed * delta;
        m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
        transform.rotation_euler({m_heading, m_pitch, 0.0f});
    }

    float x = 0, z = 0;
    if (keyboard.key(window::KEYBOARD_KEY_W).down())
        z += 1.0f;
    if (keyboard.key(window::KEYBOARD_KEY_S).down())
        z -= 1.0f;
    if (keyboard.key(window::KEYBOARD_KEY_D).down())
        x += 1.0f;
    if (keyboard.key(window::KEYBOARD_KEY_A).down())
        x -= 1.0f;

    math::float4_simd s = math::simd::load(transform.scale());
    math::float4_simd r = math::simd::load(transform.rotation());
    math::float4_simd t = math::simd::load(transform.position());

    math::float4x4_simd affine = math::matrix_simd::affine_transform(s, r, t);
    math::float4_simd forward =
        math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
    forward = math::matrix_simd::mul(forward, affine);

    transform.position(math::vector_simd::add(forward, t));
}

void light_viewer::resize_camera(std::uint32_t width, std::uint32_t height)
{
    auto& world = system<ecs::world>();
    auto& camera = world.component<graphics::camera>(m_camera);
    camera.render_groups |= graphics::RENDER_GROUP_DEBUG;

    graphics::render_target_info render_target_info = {};
    render_target_info.width = width;
    render_target_info.height = height;
    render_target_info.format = graphics::rhi::back_buffer_format();
    render_target_info.samples = 4;
    m_render_target = graphics::rhi::make_render_target(render_target_info);
    camera.render_target(m_render_target.get());

    graphics::depth_stencil_buffer_info depth_stencil_buffer_info = {};
    depth_stencil_buffer_info.width = width;
    depth_stencil_buffer_info.height = height;
    depth_stencil_buffer_info.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_buffer_info.samples = 4;
    m_depth_stencil_buffer = graphics::rhi::make_depth_stencil_buffer(depth_stencil_buffer_info);
    camera.depth_stencil_buffer(m_depth_stencil_buffer.get());
}
} // namespace ash::sample