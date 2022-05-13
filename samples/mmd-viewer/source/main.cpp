#include "application.hpp"
#include "geometry.hpp"
#include "graphics.hpp"
#include "log.hpp"
#include "mmd_animation.hpp"
#include "mmd_viewer.hpp"
#include "physics.hpp"
#include "pmx_loader.hpp"
#include "relation.hpp"
#include "scene.hpp"
#include "window.hpp"
#include "window_event.hpp"

namespace ash::sample::mmd
{
class sample_module : public core::system_base
{
public:
    sample_module(core::application* app) : core::system_base("sample_module"), m_app(app) {}

    virtual bool initialize(const dictionary& config) override
    {
        initialize_resource();
        initialize_plane();
        initialize_camera();
        initialize_task();

        system<scene::scene>().sync_local();

        system<core::event>().subscribe<window::event_window_resize>(
            [this](std::uint32_t width, std::uint32_t height) { resize_camera(width, height); });

        return true;
    }

private:
    void initialize_resource()
    {
        auto& world = system<ecs::world>();
        auto& scene = system<scene::scene>();
        auto& relation = system<core::relation>();

        m_actor = system<mmd_viewer>().load_mmd(
            "sora",
            "resource/model/sora/Sora.pmx",
            "resource/model/sora/test.vmd");

        relation.link(m_actor, scene.root());
    }

    void initialize_plane()
    {
        physics::collision_shape_desc desc;
        desc.type = physics::collision_shape_type::BOX;
        desc.box.length = 1000.0f;
        desc.box.height = 0.5f;
        desc.box.width = 1000.0f;
        m_plane_shape = system<physics::physics>().make_shape(desc);

        ecs::world& world = system<ecs::world>();
        m_plane = world.create();

        world.add<core::link, physics::rigidbody, scene::transform>(m_plane);

        auto& t = world.component<scene::transform>(m_plane);
        t.position = {0.0f, -3.0f, 0.0f};

        auto& r = world.component<physics::rigidbody>(m_plane);
        r.shape = m_plane_shape.get();
        r.mass = 0.0f;
        r.relation = m_plane;

        system<core::relation>().link(m_plane, system<scene::scene>().root());
    }

    void initialize_camera()
    {
        auto& world = system<ecs::world>();
        auto& scene = system<scene::scene>();
        auto& graphics = system<graphics::graphics>();

        m_camera = world.create();
        world.add<core::link, graphics::camera, scene::transform>(m_camera);
        auto& camera = world.component<graphics::camera>(m_camera);
        camera.parameter = graphics.make_pipeline_parameter("ash_pass");

        auto& transform = world.component<scene::transform>(m_camera);
        transform.position = {0.0f, 11.0f, -30.0f};
        transform.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        transform.scaling = {1.0f, 1.0f, 1.0f};
        system<core::relation>().link(m_camera, scene.root());

        auto window_rect = system<window::window>().rect();
        resize_camera(window_rect.width, window_rect.height);
    }

    void initialize_task()
    {
        auto& task = system<task::task_manager>();

        auto update_task = task.schedule("test update", [this]() { update(); });
        auto window_task = task.schedule(
            "window tick",
            [this]() { system<window::window>().tick(); },
            task::task_type::MAIN_THREAD);
        auto render_task = task.schedule("render", [this]() {
            auto& graphics = system<graphics::graphics>();
            graphics.begin_frame();
            graphics.render(m_camera);
            graphics.end_frame();
        });

        window_task->add_dependency(*task.find("root"));
        update_task->add_dependency(*window_task);
        render_task->add_dependency(*update_task);
    }

    void resize_camera(std::uint32_t width, std::uint32_t height)
    {
        auto& world = system<ecs::world>();
        auto& graphics = system<graphics::graphics>();

        auto& camera = world.component<graphics::camera>(m_camera);
        camera.set(
            math::to_radians(45.0f),
            static_cast<float>(width) / static_cast<float>(height),
            0.3f,
            1000.0f,
            false);

        graphics::render_target_info render_target_info = {};
        render_target_info.width = width;
        render_target_info.height = height;
        render_target_info.format = graphics.back_buffer_format();
        render_target_info.samples = 4;
        camera.render_target = graphics.make_render_target(render_target_info);

        graphics::depth_stencil_buffer_info depth_stencil_buffer_info = {};
        depth_stencil_buffer_info.width = width;
        depth_stencil_buffer_info.height = height;
        depth_stencil_buffer_info.format = graphics::resource_format::D24_UNORM_S8_UINT;
        depth_stencil_buffer_info.samples = 4;
        camera.depth_stencil_buffer = graphics.make_depth_stencil_buffer(depth_stencil_buffer_info);
    }

    void update_camera(float delta)
    {
        auto& world = system<ecs::world>();
        auto& keyboard = system<window::window>().keyboard();
        auto& mouse = system<window::window>().mouse();

        if (keyboard.key(window::keyboard_key::KEY_1).release())
        {
            if (mouse.mode() == window::mouse_mode::CURSOR_RELATIVE)
                mouse.mode(window::mouse_mode::CURSOR_ABSOLUTE);
            else
                mouse.mode(window::mouse_mode::CURSOR_RELATIVE);
        }

        if (keyboard.key(window::keyboard_key::KEY_3).release())
        {
            static std::size_t index = 0;
            static std::vector<math::float4> colors = {
                math::float4{1.0f, 0.0f, 0.0f, 1.0f},
                math::float4{0.0f, 1.0f, 0.0f, 1.0f},
                math::float4{0.0f, 0.0f, 1.0f, 1.0f}
            };

            auto& v = world.component<graphics::visual>(m_actor);
            v.submesh[0].parameters[1]->set(0, colors[index]);

            index = (index + 1) % colors.size();
        }

        auto& camera_transform = world.component<scene::transform>(m_camera);
        camera_transform.dirty = true;
        if (mouse.mode() == window::mouse_mode::CURSOR_RELATIVE)
        {
            m_heading += mouse.x() * m_rotate_speed * delta;
            m_pitch += mouse.y() * m_rotate_speed * delta;
            m_pitch = std::clamp(m_pitch, -math::PI_PIDIV2, math::PI_PIDIV2);
            camera_transform.rotation =
                math::quaternion_plain::rotation_euler(m_heading, m_pitch, 0.0f);
        }

        float x = 0, z = 0;
        if (keyboard.key(window::keyboard_key::KEY_W).down())
            z += 1.0f;
        if (keyboard.key(window::keyboard_key::KEY_S).down())
            z -= 1.0f;
        if (keyboard.key(window::keyboard_key::KEY_D).down())
            x += 1.0f;
        if (keyboard.key(window::keyboard_key::KEY_A).down())
            x -= 1.0f;

        math::float4_simd s = math::simd::load(camera_transform.scaling);
        math::float4_simd r = math::simd::load(camera_transform.rotation);
        math::float4_simd t = math::simd::load(camera_transform.position);

        math::float4x4_simd affine = math::matrix_simd::affine_transform(s, r, t);
        math::float4_simd forward =
            math::simd::set(x * m_move_speed * delta, 0.0f, z * m_move_speed * delta, 0.0f);
        forward = math::matrix_simd::mul(forward, affine);
        math::simd::store(math::vector_simd::add(forward, t), camera_transform.position);
    }

    void update_actor(float delta)
    {
        auto& world = system<ecs::world>();
        auto& keyboard = system<window::window>().keyboard();

        float move = 0.0f;
        if (keyboard.key(window::keyboard_key::KEY_E).down())
            move += 1.0f;
        if (keyboard.key(window::keyboard_key::KEY_Q).down())
            move -= 1.0f;

        if (move != 0.0f)
        {
            auto& actor_transform = world.component<scene::transform>(m_actor);
            actor_transform.position[0] += move * m_move_speed * delta;
            actor_transform.dirty = true;
        }
    }

    void update()
    {
        if (system<window::window>().keyboard().key(window::keyboard_key::KEY_ESCAPE).down())
            m_app->exit();

        auto& scene = system<scene::scene>();
        scene.reset_sync_counter();

        float delta = system<core::timer>().frame_delta();
        update_camera(delta);
        update_actor(delta);

        system<scene::scene>().sync_local();
        system<mmd_viewer>().update();
    }

    std::string m_title;
    core::application* m_app;

    ecs::entity m_camera;
    ecs::entity m_actor;
    ecs::entity m_plane;

    std::unique_ptr<physics::collision_shape_interface> m_plane_shape;

    float m_heading = 0.0f, m_pitch = 0.0f;

    float m_rotate_speed = 0.2f;
    float m_move_speed = 7.0f;
};

class mmd_viewer_app
{
public:
    mmd_viewer_app() : m_app("resource/config")
    {
        m_app.install<window::window>();
        m_app.install<core::relation>();
        m_app.install<scene::scene>();
        m_app.install<graphics::graphics>();
        m_app.install<physics::physics>();
        m_app.install<mmd_animation>();
        m_app.install<mmd_viewer>();
        m_app.install<sample_module>(&m_app);
    }

    void run() { m_app.run(); }

private:
    core::application m_app;
};
} // namespace ash::sample::mmd

int main()
{
    ash::sample::mmd::mmd_viewer_app app;
    app.run();

    return 0;
}