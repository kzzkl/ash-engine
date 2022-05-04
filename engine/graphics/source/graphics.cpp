#include "graphics.hpp"
#include "context.hpp"
#include "graphics_config.hpp"
#include "log.hpp"
#include "math.hpp"
#include "relation.hpp"
#include "scene.hpp"
#include "window.hpp"
#include "window_event.hpp"
#include <fstream>

using namespace ash::math;

namespace ash::graphics
{
graphics::graphics() noexcept : system_base("graphics")
{
}

bool graphics::initialize(const dictionary& config)
{
    m_config.load(config);

    auto& window = system<ash::window::window>();

    renderer_desc desc = {};
    desc.window_handle = window.handle();
    window::window_rect rect = window.rect();
    desc.width = rect.width;
    desc.height = rect.height;
    desc.render_concurrency = m_config.render_concurrency();
    desc.multiple_sampling = m_config.multiple_sampling();
    desc.frame_resource = m_config.frame_resource();

    if (!m_plugin.load(m_config.plugin()))
        return false;

    m_renderer.reset(m_plugin.factory().make_renderer(desc));

    pass_parameter_layout_info ash_object;
    ash_object.parameters = {
        {pass_parameter_type::FLOAT4x4, 1}, // transform_m
        {pass_parameter_type::FLOAT4x4, 1}, // transform_mv
        {pass_parameter_type::FLOAT4x4, 1}  // transform_mvp
    };
    make_render_parameter_layout("ash_object", ash_object);

    pass_parameter_layout_info ash_pass;
    ash_pass.parameters = {
        {pass_parameter_type::FLOAT4,   1}, // camera_position
        {pass_parameter_type::FLOAT4,   1}, // camera_direction
        {pass_parameter_type::FLOAT4x4, 1}, // transform_v
        {pass_parameter_type::FLOAT4x4, 1}, // transform_p
        {pass_parameter_type::FLOAT4x4, 1}  // transform_vp
    };
    make_render_parameter_layout("ash_pass", ash_pass);

    /* adapter_info info[4] = {};
     std::size_t num_adapter = m_renderer->adapter(info, 4);
     for (std::size_t i = 0; i < num_adapter; ++i)
     {
         log::debug("graphics adapter: {}", info[i].description);
     }*/

    auto& world = system<ecs::world>();
    auto& event = system<core::event>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    world.register_component<visual>();
    world.register_component<main_camera>();
    world.register_component<camera>();
    m_visual_view = world.make_view<visual>();
    m_object_view = world.make_view<visual, scene::transform>();
    m_camera_view = world.make_view<main_camera, camera, scene::transform>();
    // m_tv = world.make_view<scene::transform>();

    event.subscribe<window::event_window_resize>(
        [this](std::uint32_t width, std::uint32_t height) { m_renderer->resize(width, height); });

    m_debug = std::make_unique<graphics_debug>(m_config.frame_resource(), *this, world);
    m_debug->initialize();
    // relation.link(m_debug->entity(), scene.root());

    return true;
}

void graphics::render(ecs::entity camera_entity)
{
    auto& world = system<ecs::world>();

    auto& c = world.component<camera>(camera_entity);
    auto& t = world.component<scene::transform>(camera_entity);

    if (c.mask & visual::mask_type::DEBUG)
        m_debug->sync();

    // Update camera data.
    math::float4x4_simd transform_v;
    math::float4x4_simd transform_p;
    math::float4x4_simd transform_vp;

    if (t.sync_count != 0)
    {
        math::float4x4_simd world_simd = math::simd::load(t.world_matrix);
        transform_v = math::matrix_simd::inverse(world_simd);
        math::simd::store(transform_v, c.view);
    }
    else
    {
        transform_v = math::simd::load(c.view);
    }

    transform_p = math::simd::load(c.projection);
    transform_vp = math::matrix_simd::mul(transform_v, transform_p);

    if (t.sync_count != 0)
    {
        math::float4x4 view, projection, view_projection;
        math::simd::store(transform_v, view);
        math::simd::store(transform_p, projection);
        math::simd::store(transform_vp, view_projection);

        c.parameter->set(0, float4{1.0f, 2.0f, 3.0f, 4.0f});
        c.parameter->set(1, float4{5.0f, 6.0f, 7.0f, 8.0f});
        c.parameter->set(2, view);
        c.parameter->set(3, projection);
        c.parameter->set(4, view_projection);
    }

    // Update object data.
    m_object_view->each([&, this](visual& visual, scene::transform& transform) {
        if ((visual.mask & c.mask) == 0)
            return;

        math::float4x4_simd transform_m = math::simd::load(transform.world_matrix);
        math::float4x4_simd transform_mv = math::matrix_simd::mul(transform_m, transform_v);
        math::float4x4_simd transform_mvp = math::matrix_simd::mul(transform_mv, transform_p);

        math::float4x4 model, model_view, model_view_projection;
        math::simd::store(transform_m, model);
        math::simd::store(transform_mv, model_view);
        math::simd::store(transform_mvp, model_view_projection);

        visual.object->set(0, model);
        visual.object->set(1, model_view);
        visual.object->set(2, model_view_projection);
    });

    m_visual_view->each([&, this](visual& visual) {
        if ((visual.mask & c.mask) == 0)
            return;

        for (std::size_t i = 0; i < visual.submesh.size(); ++i)
        {
            m_techniques.insert(visual.submesh[i].technique);
            visual.submesh[i].technique->add(&visual.submesh[i]);
        }
    });

    // Render.
    auto command = m_renderer->allocate_command();

    if (c.render_target == nullptr)
    {
        for (auto technique : m_techniques)
            technique->render(c, command);
    }
    else
    {
        /*command->begin_render(c.render_target);
        command->clear_render_target(c.render_target);
        command->clear_depth_stencil(c.depth_stencil);
        for (auto pipeline : m_render_pipelines)
            pipeline->render(c.render_target, c.depth_stencil, command, c.parameter.get());
        command->end_render(c.render_target);*/
    }

    for (auto technique : m_techniques)
        technique->clear();
    m_techniques.clear();

    m_renderer->execute(command);
}

void graphics::begin_frame()
{
    m_renderer->begin_frame();
    m_debug->begin_frame();
}

void graphics::end_frame()
{
    m_debug->end_frame();
    m_renderer->end_frame();
}

void graphics::make_render_parameter_layout(
    std::string_view name,
    pass_parameter_layout_info& info)
{
    auto& factory = m_plugin.factory();
    m_parameter_layouts[name.data()].reset(factory.make_pass_parameter_layout(info.convert()));
}

std::unique_ptr<render_parameter> graphics::make_render_parameter(std::string_view name)
{
    auto layout = m_parameter_layouts[name.data()].get();
    ASH_ASSERT(layout);
    auto& factory = m_plugin.factory();
    return std::make_unique<render_parameter>(factory.make_pass_parameter(layout));
}

std::unique_ptr<render_target_set_interface> graphics::make_render_target_set(
    render_target_set_info& info)
{
    auto& factory = m_plugin.factory();
    return std::unique_ptr<render_target_set_interface>(
        factory.make_render_target_set(info.convert()));
}

std::unique_ptr<resource> graphics::make_texture(std::string_view file)
{
    /*std::ifstream fin(file.data(), std::ios::in | std::ios::binary);
    if (!fin)
    {
        log::error("Can not open texture: {}.", file);
        return nullptr;
    }

    std::vector<uint8_t> dds_data(fin.seekg(0, std::ios::end).tellg());
    fin.seekg(0, std::ios::beg).read((char*)dds_data.data(), dds_data.size());
    fin.close();*/

    auto& factory = m_plugin.factory();
    return std::unique_ptr<resource>(factory.make_texture(file.data()));
}

std::vector<resource*> graphics::back_buffers() const
{
    std::vector<resource*> result;
    for (std::size_t i = 0; i < m_renderer->back_buffer_count(); ++i)
        result.push_back(m_renderer->back_buffer(i));
    return result;
}

technique_interface* graphics::make_technique_interface(technique_info& info)
{
    auto& factory = m_plugin.factory();

    // make layout
    for (auto& subpass : info.subpasses)
    {
        auto pass_layout_desc = subpass.pass_layout_info.convert();

        std::vector<pass_parameter_layout_interface*> parameter_layouts;
        for (auto& parameter_layout : subpass.pass_layout_info.parameters)
            parameter_layouts.push_back(m_parameter_layouts[parameter_layout].get());
        pass_layout_desc.parameters = parameter_layouts.data();

        subpass.pass_layout = factory.make_pass_layout(pass_layout_desc);
    }

    return factory.make_technique(info.convert());
}
} // namespace ash::graphics