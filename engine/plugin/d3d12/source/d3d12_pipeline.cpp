#include "d3d12_pipeline.hpp"
#include "d3d12_context.hpp"
#include "d3d12_utility.hpp"
#include <d3dcompiler.h>
#include <fstream>
#include <string>

namespace ash::graphics::d3d12
{
namespace
{
D3D12_BLEND_DESC to_d3d12_blend_desc(const blend_desc& desc)
{
    auto convert_factor = [](blend_factor factor) {
        switch (factor)
        {
        case BLEND_FACTOR_ZERO:
            return D3D12_BLEND_ZERO;
        case BLEND_FACTOR_ONE:
            return D3D12_BLEND_ONE;
        case BLEND_FACTOR_SOURCE_COLOR:
            return D3D12_BLEND_SRC_COLOR;
        case BLEND_FACTOR_SOURCE_ALPHA:
            return D3D12_BLEND_SRC_ALPHA;
        case BLEND_FACTOR_SOURCE_INV_ALPHA:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case BLEND_FACTOR_TARGET_COLOR:
            return D3D12_BLEND_DEST_COLOR;
        case BLEND_FACTOR_TARGET_ALPHA:
            return D3D12_BLEND_DEST_ALPHA;
        case BLEND_FACTOR_TARGET_INV_ALPHA:
            return D3D12_BLEND_INV_DEST_ALPHA;
        default:
            throw d3d12_exception("Invalid blend factor.");
        };
    };

    auto convert_op = [](blend_op op) {
        switch (op)
        {
        case BLEND_OP_ADD:
            return D3D12_BLEND_OP_ADD;
        case BLEND_OP_SUBTRACT:
            return D3D12_BLEND_OP_SUBTRACT;
        case BLEND_OP_MIN:
            return D3D12_BLEND_OP_MIN;
        case BLEND_OP_MAX:
            return D3D12_BLEND_OP_MAX;
        default:
            throw d3d12_exception("Invalid blend op.");
        }
    };

    D3D12_BLEND_DESC result = {};
    if (desc.enable)
    {
        result.AlphaToCoverageEnable = true;

        D3D12_RENDER_TARGET_BLEND_DESC render_target_blend = {};
        render_target_blend.BlendEnable = true;
        render_target_blend.SrcBlend = convert_factor(desc.source_factor);
        render_target_blend.DestBlend = convert_factor(desc.target_factor);
        render_target_blend.BlendOp = convert_op(desc.op);
        render_target_blend.SrcBlendAlpha = convert_factor(desc.source_alpha_factor);
        render_target_blend.DestBlendAlpha = convert_factor(desc.target_alpha_factor);
        render_target_blend.BlendOpAlpha = convert_op(desc.alpha_op);
        render_target_blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
            result.RenderTarget[i] = render_target_blend;
    }
    else
    {
        result = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    }

    return result;
}

D3D12_DEPTH_STENCIL_DESC to_d3d12_depth_stencil_desc(const depth_stencil_desc& desc)
{
    D3D12_DEPTH_STENCIL_DESC result = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    switch (desc.depth_functor)
    {
    case DEPTH_FUNCTOR_NEVER:
        result.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
        break;
    case DEPTH_FUNCTOR_LESS:
        result.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        break;
    case DEPTH_FUNCTOR_EQUAL:
        result.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
        break;
    case DEPTH_FUNCTOR_LESS_EQUAL:
        result.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        break;
    case DEPTH_FUNCTOR_GREATER:
        result.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
        break;
    case DEPTH_FUNCTOR_NOT_EQUAL:
        result.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
        break;
    case DEPTH_FUNCTOR_GREATER_EQUAL:
        result.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        break;
    case DEPTH_FUNCTOR_ALWAYS:
        result.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        break;
    default:
        throw d3d12_exception("Invalid depth functor.");
    };

    return result;
}

D3D12_RASTERIZER_DESC to_d3d12_rasterizer_desc(const rasterizer_desc& desc)
{
    D3D12_RASTERIZER_DESC result = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    switch (desc.cull_mode)
    {
    case CULL_MODE_NONE:
        result.CullMode = D3D12_CULL_MODE_NONE;
        break;
    case CULL_MODE_FRONT:
        result.CullMode = D3D12_CULL_MODE_FRONT;
        break;
    case CULL_MODE_BACK:
        result.CullMode = D3D12_CULL_MODE_BACK;
        break;
    default:
        throw d3d12_exception("Invalid depth functor.");
    };

    return result;
}

d3d12_ptr<D3DBlob> load_shader(std::string_view file)
{
    d3d12_ptr<D3DBlob> result;
    d3d12_ptr<D3DBlob> error;

    std::ifstream fin(std::string(file) + ".cso", std::ios::in | std::ios::binary);
    if (!fin)
        throw d3d12_exception("Unable to open shader file.");

    fin.seekg(0, std::ios::end);
    throw_if_failed(D3DCreateBlob(fin.tellg(), &result));
    fin.seekg(0, std::ios::beg);
    fin.read(static_cast<char*>(result->GetBufferPointer()), result->GetBufferSize());
    fin.close();

    return result;
}
} // namespace

static const std::vector<CD3DX12_STATIC_SAMPLER_DESC> static_samplers = {
    CD3DX12_STATIC_SAMPLER_DESC(
        0,                                // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP), // addressW

    CD3DX12_STATIC_SAMPLER_DESC(
        1,                                 // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP), // addressW

    CD3DX12_STATIC_SAMPLER_DESC(
        2,                               // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressW
        0.0f,                            // mipLODBias
        1,                               // maxAnisotropy
        D3D12_COMPARISON_FUNC_ALWAYS),   // comparisonFunc

    CD3DX12_STATIC_SAMPLER_DESC(
        3,                                 // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP), // addressW

    CD3DX12_STATIC_SAMPLER_DESC(
        4,                               // shaderRegister
        D3D12_FILTER_ANISOTROPIC,        // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressW
        0.0f,                            // mipLODBias
        8),                              // maxAnisotropy

    CD3DX12_STATIC_SAMPLER_DESC(
        5,                                // shaderRegister
        D3D12_FILTER_ANISOTROPIC,         // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressW
        0.0f,                             // mipLODBias
        8),                               // maxAnisotropy

    CD3DX12_STATIC_SAMPLER_DESC(
        6,                                                // shaderRegister
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressW
        0.0f,                                             // mipLODBias
        16,                                               // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK),

    CD3DX12_STATIC_SAMPLER_DESC(
        7,                                // shaderRegister
        D3D12_FILTER_ANISOTROPIC,         // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressW
        0.0f,                             // mipLODBias
        16,                               // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        0.0f,
        0.0f,
        D3D12_SHADER_VISIBILITY_PIXEL)};

d3d12_pipeline_parameter_layout::d3d12_pipeline_parameter_layout(
    const pipeline_parameter_layout_desc& desc)
    : m_cbv_count(0),
      m_srv_count(0),
      m_uav_count(0),
      m_constant_buffer_size(0)
{
    auto cal_align = [](std::size_t begin, std::size_t align) {
        return (begin + align - 1) & ~(align - 1);
    };

    for (std::size_t i = 0; i < desc.parameter_count; ++i)
    {
        switch (desc.parameters[i].type)
        {
        case PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE:
            ++m_srv_count;
            break;
        case PIPELINE_PARAMETER_TYPE_UNORDERED_ACCESS:
            ++m_uav_count;
            break;
        default:
            m_cbv_count = 1;
            break;
        }
    }

    std::size_t constant_offset = 0;
    std::size_t descriptor_offset = m_cbv_count;
    m_parameters.reserve(desc.parameter_count);
    for (std::size_t i = 0; i < desc.parameter_count; ++i)
    {
        std::size_t offset = 0;
        std::size_t size = 0;
        switch (desc.parameters[i].type)
        {
        case PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER:
            offset = cal_align(constant_offset, 16);
            size = desc.parameters[i].size;
            constant_offset = offset + size;
            break;
        case PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE:
        case PIPELINE_PARAMETER_TYPE_UNORDERED_ACCESS:
            offset = descriptor_offset;
            ++descriptor_offset;
            break;
        default:
            break;
        }

        m_parameters.push_back(parameter_info{offset, size, desc.parameters[i].type});
    }

    m_constant_buffer_size = cal_align(constant_offset, 256);
}

d3d12_pipeline_parameter::d3d12_pipeline_parameter(pipeline_parameter_layout_interface* layout)
    : m_tier_info(d3d12_frame_counter::frame_resource_count()),
      m_layout(static_cast<d3d12_pipeline_parameter_layout*>(layout)),
      m_current_index(0),
      m_last_sync_frame(0)
{
    std::size_t cbv_count = m_layout->cbv_count();
    std::size_t srv_count = m_layout->srv_count();
    std::size_t uav_count = m_layout->uav_count();

    std::size_t buffer_size = m_layout->constant_buffer_size();

    if (cbv_count != 0)
    {
        m_gpu_buffer = std::make_unique<d3d12_upload_buffer>(
            buffer_size * d3d12_frame_counter::frame_resource_count());
    }

    if (cbv_count != 0 && srv_count == 0 && uav_count == 0)
    {
        m_tier = d3d12_parameter_tier_type::TIER1;

        std::size_t gpu_buffer_offset = 0;
        for (auto& iter_info : m_tier_info)
        {
            iter_info.tier1.type = D3D12_ROOT_PARAMETER_TYPE_CBV;
            iter_info.tier1.address =
                m_gpu_buffer->handle()->GetGPUVirtualAddress() + gpu_buffer_offset;
            gpu_buffer_offset += buffer_size;
        }
    }
    else if (srv_count != 0 || uav_count != 0)
    {
        m_views.resize(cbv_count + srv_count + uav_count);

        m_tier = d3d12_parameter_tier_type::TIER2;

        std::size_t view_count = m_layout->view_count();
        auto heap = d3d12_context::resource()->visible_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        std::size_t handle_offset =
            heap->allocate(view_count * d3d12_frame_counter::frame_resource_count());
        std::size_t gpu_buffer_offset = 0;
        for (auto& iter_info : m_tier_info)
        {
            iter_info.tier2.base_cpu_handle = heap->cpu_handle(handle_offset);
            iter_info.tier2.base_gpu_handle = heap->gpu_handle(handle_offset);
            iter_info.tier2.size = view_count;

            if (cbv_count != 0)
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
                cbv_desc.BufferLocation =
                    m_gpu_buffer->handle()->GetGPUVirtualAddress() + gpu_buffer_offset;
                cbv_desc.SizeInBytes = static_cast<UINT>(buffer_size);
                d3d12_context::device()->CreateConstantBufferView(
                    &cbv_desc,
                    heap->cpu_handle(handle_offset));
                gpu_buffer_offset += buffer_size;
            }

            handle_offset += view_count;
        }
    }
}

d3d12_pipeline_parameter::~d3d12_pipeline_parameter()
{
}

void d3d12_pipeline_parameter::set(std::size_t index, const void* data, size_t size)
{
    copy_buffer(data, size, m_layout->parameter_offset(index));
}

void d3d12_pipeline_parameter::set(std::size_t index, resource_interface* texture)
{
    std::size_t offset = m_layout->parameter_offset(index);
    if (m_layout->parameter_type(index) == PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE)
        copy_descriptor(static_cast<d3d12_resource*>(texture)->srv(), offset);
    else if (m_layout->parameter_type(index) == PIPELINE_PARAMETER_TYPE_UNORDERED_ACCESS)
        copy_descriptor(static_cast<d3d12_resource*>(texture)->uav(), offset);
}

void* d3d12_pipeline_parameter::constant_buffer_pointer(std::size_t index)
{
    sync_frame_resource();

    std::uint8_t* mapped_pointer = reinterpret_cast<std::uint8_t*>(m_gpu_buffer->mapped_pointer());
    return mapped_pointer + m_current_index * m_layout->constant_buffer_size() +
           m_layout->parameter_offset(index);
}

void d3d12_pipeline_parameter::sync_frame_resource()
{
    std::size_t current_frame = d3d12_frame_counter::frame_counter();
    if (m_last_sync_frame == current_frame)
        return;

    std::size_t frame_resource_count = d3d12_frame_counter::frame_resource_count();
    std::size_t next_index = (m_current_index + 1) % frame_resource_count;

    // Copy constant buffer.
    std::size_t buffer_size = m_layout->constant_buffer_size();
    m_gpu_buffer->copy(m_current_index * buffer_size, buffer_size, next_index * buffer_size);

    // Copy descriptor.
    for (std::size_t i = 0; i < m_views.size(); ++i)
    {
        auto& [descriptor, dirty_counter] = m_views[i];
        if (dirty_counter == 0)
            continue;
        else
            --dirty_counter;

        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        CD3DX12_CPU_DESCRIPTOR_HANDLE target_handle(
            m_tier_info[next_index].tier2.base_cpu_handle,
            static_cast<INT>(i),
            heap->increment_size());

        d3d12_context::device()->CopyDescriptorsSimple(
            1,
            target_handle,
            descriptor,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    m_current_index = next_index;
    m_last_sync_frame = current_frame;
}

void d3d12_pipeline_parameter::copy_buffer(const void* data, std::size_t size, std::size_t offset)
{
    sync_frame_resource();

    m_gpu_buffer->upload(data, size, offset + m_current_index * m_layout->constant_buffer_size());
}

void d3d12_pipeline_parameter::copy_descriptor(
    D3D12_CPU_DESCRIPTOR_HANDLE descriptor,
    std::size_t offset)
{
    sync_frame_resource();

    m_views[offset] = {descriptor, d3d12_frame_counter::frame_resource_count() - 1};
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE target_handle(
        m_tier_info[m_current_index].tier2.base_cpu_handle,
        static_cast<INT>(offset),
        heap->increment_size());

    d3d12_context::device()->CopyDescriptorsSimple(
        1,
        target_handle,
        descriptor,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

d3d12_root_signature::d3d12_root_signature(
    pipeline_parameter_layout_interface* const* parameters,
    std::size_t parameter_count)
{
    std::vector<CD3DX12_ROOT_PARAMETER> root_parameter;
    root_parameter.resize(parameter_count);

    std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>> range;
    range.resize(parameter_count);

    for (std::size_t i = 0; i < parameter_count; ++i)
    {
        auto layout = static_cast<const d3d12_pipeline_parameter_layout*>(parameters[i]);

        UINT cbv_count = static_cast<UINT>(layout->cbv_count());
        UINT srv_count = static_cast<UINT>(layout->srv_count());
        UINT uav_count = static_cast<UINT>(layout->uav_count());

        UINT register_space = static_cast<UINT>(i);
        if (cbv_count == 1 && srv_count == 0 && uav_count == 0)
        {
            root_parameter[i].InitAsConstantBufferView(0, register_space);
        }
        else
        {
            if (cbv_count != 0)
            {
                CD3DX12_DESCRIPTOR_RANGE cbv_range;
                cbv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, cbv_count, 0, register_space);
                range[i].push_back(cbv_range);
            }

            if (srv_count != 0)
            {
                CD3DX12_DESCRIPTOR_RANGE srv_range;
                srv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srv_count, 0, register_space);
                range[i].push_back(srv_range);
            }

            if (uav_count != 0)
            {
                CD3DX12_DESCRIPTOR_RANGE uav_range;
                uav_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, uav_count, 0, register_space);
                range[i].push_back(uav_range);
            }

            root_parameter[i].InitAsDescriptorTable(
                static_cast<UINT>(range[i].size()),
                range[i].data());
        }
    }

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(
        static_cast<UINT>(root_parameter.size()),
        root_parameter.data(),
        static_cast<UINT>(static_samplers.size()),
        static_samplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    d3d12_ptr<D3DBlob> root_signature;
    d3d12_ptr<D3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(
        &root_signature_desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &root_signature,
        &error);

    if (error != nullptr)
    {
        OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
    }
    else
    {
        throw_if_failed(d3d12_context::device()->CreateRootSignature(
            0,
            root_signature->GetBufferPointer(),
            root_signature->GetBufferSize(),
            IID_PPV_ARGS(&m_root_signature)));
    }
}

d3d12_frame_buffer_layout::d3d12_frame_buffer_layout(
    const attachment_desc* attachments,
    std::size_t count)
{
    for (std::size_t i = 0; i < count; ++i)
        m_attachments.push_back(attachments[i]);
}

d3d12_frame_buffer::d3d12_frame_buffer(
    d3d12_render_pipeline* pipeline,
    const d3d12_camera_info& camera_info)
{
    static const D3D12_RESOURCE_STATES resource_state_map[] = {
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_STATE_PRESENT};

    auto [width, heigth] = camera_info.render_target->extent();
    for (auto& attachment : pipeline->frame_buffer_layout())
    {
        switch (attachment.type)
        {
        case ATTACHMENT_TYPE_RENDER_TARGET: {
            auto render_target = std::make_unique<d3d12_render_target>(
                width,
                heigth,
                attachment.samples,
                attachment.format);
            m_render_targets.push_back(render_target->rtv());
            m_attachments.push_back(attachment_info{
                render_target.get(),
                resource_state_map[static_cast<std::size_t>(attachment.initial_state)],
                resource_state_map[static_cast<std::size_t>(attachment.final_state)]});
            m_attachment_container.push_back(std::move(render_target));
            break;
        }
        case ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE: {
            m_attachments.push_back(attachment_info{
                camera_info.render_target_resolve,
                resource_state_map[static_cast<std::size_t>(attachment.initial_state)],
                resource_state_map[static_cast<std::size_t>(attachment.final_state)]});
            break;
        }
        case ATTACHMENT_TYPE_CAMERA_RENDER_TARGET: {
            m_render_targets.push_back(camera_info.render_target->rtv());
            m_attachments.push_back(attachment_info{
                camera_info.render_target,
                resource_state_map[static_cast<std::size_t>(attachment.initial_state)],
                resource_state_map[static_cast<std::size_t>(attachment.final_state)]});
            break;
        }
        case ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL: {
            m_depth_stencil = camera_info.depth_stencil_buffer->dsv();
            m_attachments.push_back(attachment_info{
                camera_info.depth_stencil_buffer,
                resource_state_map[static_cast<std::size_t>(attachment.initial_state)],
                resource_state_map[static_cast<std::size_t>(attachment.final_state)]});
            break;
        }
        default:
            throw d3d12_exception("Invalid attachment type.");
        }
    }
}

void d3d12_frame_buffer::begin_render(D3D12GraphicsCommandList* command_list)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.reserve(m_attachments.size());
    for (auto& attachment : m_attachments)
    {
        if (attachment.resource->resource_state() != attachment.initial_state)
        {
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                attachment.resource->handle(),
                attachment.resource->resource_state(),
                attachment.initial_state));
            attachment.resource->resource_state(attachment.initial_state);
        }
    }

    if (!barriers.empty())
        command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
}

void d3d12_frame_buffer::end_render(D3D12GraphicsCommandList* command_list)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    barriers.reserve(m_attachments.size());
    for (auto& attachment : m_attachments)
    {
        if (attachment.resource->resource_state() != attachment.final_state)
        {
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                attachment.resource->handle(),
                attachment.resource->resource_state(),
                attachment.final_state));
            attachment.resource->resource_state(attachment.final_state);
        }
    }

    if (!barriers.empty())
        command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
}

d3d12_render_pass::d3d12_render_pass(const render_pass_desc& desc)
    : m_current_frame_buffer(nullptr),
      m_depth_index(-1)
{
    m_root_signature =
        std::make_unique<d3d12_root_signature>(desc.parameters, desc.parameter_count);

    initialize_vertex_layout(desc);
    initialize_pipeline_state(desc);

    for (std::size_t i = 0; i < desc.reference_count; ++i)
    {
        switch (desc.references[i].type)
        {
        case ATTACHMENT_REFERENCE_TYPE_INPUT:
        case ATTACHMENT_REFERENCE_TYPE_COLOR:
            m_color_indices.push_back(i);
            break;
        case ATTACHMENT_REFERENCE_TYPE_DEPTH:
            m_depth_index = i;
            break;
        case ATTACHMENT_REFERENCE_TYPE_RESOLVE:
            m_resolve_indices.push_back({i, desc.references[i].resolve_relation});
            break;
        default:
            break;
        }
    }
}

void d3d12_render_pass::begin(
    D3D12GraphicsCommandList* command_list,
    d3d12_frame_buffer* frame_buffer)
{
    command_list->SetPipelineState(m_pipeline_state.Get());
    command_list->SetGraphicsRootSignature(m_root_signature->handle());

    auto& attachments = frame_buffer->attachments();

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> render_targets;
    for (std::size_t i : m_color_indices)
        render_targets.push_back(attachments[i].resource->rtv());

    D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil = attachments[m_depth_index].resource->dsv();

    command_list->OMSetRenderTargets(
        static_cast<UINT>(render_targets.size()),
        render_targets.data(),
        false,
        &depth_stencil);

    m_current_frame_buffer = frame_buffer;
}

void d3d12_render_pass::end(D3D12GraphicsCommandList* command_list, bool final)
{
    if (m_resolve_indices.empty())
        return;

    auto& attachments = m_current_frame_buffer->attachments();

    std::vector<D3D12_RESOURCE_BARRIER> barriers;
    for (auto [target_index, source_index] : m_resolve_indices)
    {
        auto target = attachments[target_index].resource;
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
            target->handle(),
            target->resource_state(),
            D3D12_RESOURCE_STATE_RESOLVE_DEST));
        target->resource_state(D3D12_RESOURCE_STATE_RESOLVE_DEST);

        auto source = attachments[source_index].resource;
        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
            source->handle(),
            source->resource_state(),
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE));
        source->resource_state(D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
    }

    if (!barriers.empty())
        command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

    for (auto [target_index, source_index] : m_resolve_indices)
    {
        auto target = attachments[target_index].resource;
        auto source = attachments[source_index].resource;
        command_list->ResolveSubresource(
            target->handle(),
            0,
            source->handle(),
            0,
            source->handle()->GetDesc().Format);
    }

    if (!final)
    {
        barriers.clear();

        for (auto [target_index, source_index] : m_resolve_indices)
        {
            auto target = attachments[target_index].resource;
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                target->handle(),
                target->resource_state(),
                attachments[target_index].initial_state));
            target->resource_state(attachments[target_index].initial_state);

            auto source = attachments[source_index].resource;
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                source->handle(),
                source->resource_state(),
                attachments[source_index].initial_state));
            source->resource_state(attachments[source_index].initial_state);
        }

        if (!barriers.empty())
            command_list->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    }
}

void d3d12_render_pass::initialize_vertex_layout(const render_pass_desc& desc)
{
    auto get_type = [](vertex_attribute_type type) -> DXGI_FORMAT {
        switch (type)
        {
        case VERTEX_ATTRIBUTE_TYPE_INT:
            return DXGI_FORMAT_R32_SINT;
        case VERTEX_ATTRIBUTE_TYPE_INT2:
            return DXGI_FORMAT_R32G32_SINT;
        case VERTEX_ATTRIBUTE_TYPE_INT3:
            return DXGI_FORMAT_R32G32B32_SINT;
        case VERTEX_ATTRIBUTE_TYPE_INT4:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case VERTEX_ATTRIBUTE_TYPE_UINT:
            return DXGI_FORMAT_R32_UINT;
        case VERTEX_ATTRIBUTE_TYPE_UINT2:
            return DXGI_FORMAT_R32G32_UINT;
        case VERTEX_ATTRIBUTE_TYPE_UINT3:
            return DXGI_FORMAT_R32G32B32_UINT;
        case VERTEX_ATTRIBUTE_TYPE_UINT4:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case VERTEX_ATTRIBUTE_TYPE_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case VERTEX_ATTRIBUTE_TYPE_FLOAT2:
            return DXGI_FORMAT_R32G32_FLOAT;
        case VERTEX_ATTRIBUTE_TYPE_FLOAT3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case VERTEX_ATTRIBUTE_TYPE_FLOAT4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case VERTEX_ATTRIBUTE_TYPE_COLOR:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        default:
            return DXGI_FORMAT_UNKNOWN;
        };
    };

    for (std::size_t i = 0; i < desc.vertex_attribute_count; ++i)
    {
        auto& attribute = desc.vertex_attributes[i];
        D3D12_INPUT_ELEMENT_DESC desc = {
            attribute.name,
            0,
            get_type(attribute.type),
            static_cast<UINT>(i),
            0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0};
        m_vertex_layout.push_back(desc);
    }
}

void d3d12_render_pass::initialize_pipeline_state(const render_pass_desc& desc)
{
    d3d12_ptr<D3DBlob> vs_blob = load_shader(desc.vertex_shader);
    d3d12_ptr<D3DBlob> ps_blob = load_shader(desc.pixel_shader);

    // Query render target sample level.
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS sample_level = {};
    sample_level.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    sample_level.Format = RENDER_TARGET_FORMAT;
    sample_level.NumQualityLevels = 0;
    sample_level.SampleCount = static_cast<UINT>(desc.samples);
    throw_if_failed(d3d12_context::device()->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &sample_level,
        sizeof(sample_level)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.InputLayout = {m_vertex_layout.data(), static_cast<UINT>(m_vertex_layout.size())};
    pso_desc.pRootSignature = m_root_signature->handle();
    pso_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
    pso_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    pso_desc.DepthStencilState = to_d3d12_depth_stencil_desc(desc.depth_stencil);
    pso_desc.BlendState = to_d3d12_blend_desc(desc.blend);
    pso_desc.RasterizerState = to_d3d12_rasterizer_desc(desc.rasterizer);
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = RENDER_TARGET_FORMAT;
    pso_desc.SampleDesc.Count = sample_level.SampleCount;
    pso_desc.SampleDesc.Quality = sample_level.NumQualityLevels - 1;
    pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    if (desc.primitive_topology == PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    else if (desc.primitive_topology == PRIMITIVE_TOPOLOGY_TYPE_LINE)
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

    throw_if_failed(d3d12_context::device()->CreateGraphicsPipelineState(
        &pso_desc,
        IID_PPV_ARGS(&m_pipeline_state)));
}

d3d12_render_pipeline::d3d12_render_pipeline(const render_pipeline_desc& desc)
    : m_current_index(0),
      m_current_frame_buffer(0),
      m_frame_buffer_layout(desc.attachments, desc.attachment_count)
{
    for (std::size_t i = 0; i < desc.pass_count; ++i)
        m_passes.push_back(std::make_unique<d3d12_render_pass>(desc.passes[i]));
}

void d3d12_render_pipeline::begin(
    D3D12GraphicsCommandList* command_list,
    const d3d12_camera_info& camera_info)
{
    m_current_index = 0;
    m_current_frame_buffer =
        d3d12_context::frame_buffer().get_or_create_frame_buffer(this, camera_info);
    m_current_frame_buffer->begin_render(command_list);

    auto [width, height] = camera_info.render_target->extent();

    D3D12_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    command_list->RSSetViewports(1, &viewport);

    m_passes[m_current_index]->begin(command_list, m_current_frame_buffer);
}

void d3d12_render_pipeline::end(D3D12GraphicsCommandList* command_list)
{
    m_passes[m_current_index]->end(command_list, true);
    m_current_frame_buffer->end_render(command_list);
}

void d3d12_render_pipeline::next(D3D12GraphicsCommandList* command_list)
{
    m_passes[m_current_index]->end(command_list);
    ++m_current_index;
    m_passes[m_current_index]->begin(command_list, m_current_frame_buffer);
}

d3d12_frame_buffer* d3d12_frame_buffer_manager::get_or_create_frame_buffer(
    d3d12_render_pipeline* pipeline,
    const d3d12_camera_info& camera_info)
{
    auto& result = m_frame_buffers[camera_info];
    if (result == nullptr)
        result = std::make_unique<d3d12_frame_buffer>(pipeline, camera_info);

    return result.get();
}

void d3d12_frame_buffer_manager::notify_destroy(d3d12_resource* resource)
{
    for (auto iter = m_frame_buffers.begin(); iter != m_frame_buffers.end();)
    {
        if (iter->first.render_target == resource ||
            iter->first.render_target_resolve == resource ||
            iter->first.depth_stencil_buffer == resource)
            iter = m_frame_buffers.erase(iter);
        else
            ++iter;
    }
}

d3d12_compute_pipeline::d3d12_compute_pipeline(const compute_pipeline_desc& desc)
{
    m_root_signature =
        std::make_unique<d3d12_root_signature>(desc.parameters, desc.parameter_count);

    d3d12_ptr<D3DBlob> cs_blob = load_shader(desc.compute_shader);

    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = m_root_signature->handle();
    pso_desc.CS = CD3DX12_SHADER_BYTECODE(cs_blob.Get());
    pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    throw_if_failed(d3d12_context::device()->CreateComputePipelineState(
        &pso_desc,
        IID_PPV_ARGS(&m_pipeline_state)));
}

void d3d12_compute_pipeline::begin(D3D12GraphicsCommandList* command_list)
{
    command_list->SetPipelineState(m_pipeline_state.Get());
    command_list->SetComputeRootSignature(m_root_signature->handle());
}

void d3d12_compute_pipeline::end(D3D12GraphicsCommandList* command_list)
{
}
} // namespace ash::graphics::d3d12