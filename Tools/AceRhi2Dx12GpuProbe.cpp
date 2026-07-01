#include "ArhqenCognitionEngine/Renderer/RHI/AceDx12Rhi.h"

#include <iostream>
#include <string>

namespace
{
    int failures = 0;

    void pass(const std::string& name)
    {
        std::cout << "PASS|" << name << "\n";
    }

    void fail(const std::string& name, const std::string& detail = {})
    {
        ++failures;
        std::cout << "FAIL|" << name;
        if (!detail.empty())
        {
            std::cout << "|" << detail;
        }
        std::cout << "\n";
    }
}

int main()
{
    using namespace am::renderer::rhi;

#if !defined(_WIN32)
    pass("dx12_probe_skipped_non_windows");
    return 0;
#else
    if (!Dx12RuntimeAvailable())
    {
        fail("dx12_runtime_available", "No hardware D3D12 adapter found.");
        return 1;
    }
    pass("dx12_runtime_available");

    std::string error;
    auto device = CreateDx12DeviceConcrete();

    DeviceDesc desc{};
    desc.backend = Backend::Dx12;
    desc.name = Name{"AceRhi2Dx12GpuProbe"};
    desc.framesInFlight = 2;

    if (device->initialize(desc, &error))
    {
        pass("dx12_device_initializes");
    }
    else
    {
        fail("dx12_device_initializes", error);
        return 1;
    }

    BufferDesc vb{};
    vb.name = Name{"probe_gpu_vertex_default"};
    vb.size = 256;
    vb.stride = 16;
    vb.usage = Usage::Vertex | Usage::CopyDst;
    vb.memory = Memory::GpuOnly;

    auto buffer = device->resources().create(vb, &error);
    if (buffer.valid() && device->ensureNative(buffer, &error))
    {
        pass("dx12_native_default_buffer_created");
    }
    else
    {
        fail("dx12_native_default_buffer_created", error);
    }

    const float vertices[12] = {
        0.0f, 0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 1.0f
    };

    if (device->upload(buffer, vertices, sizeof(vertices), &error))
    {
        pass("dx12_upload_buffer_executes_gpu_copy");
    }
    else
    {
        fail("dx12_upload_buffer_executes_gpu_copy", error);
    }

    TextureDesc color{};
    color.name = Name{"probe_gpu_color"};
    color.extent = {512, 512, 1};
    color.format = Format::BGRA8;
    color.usage = Usage::RenderTarget | Usage::ShaderResource;
    color.memory = Memory::GpuOnly;
    color.clear.color = {0.02f, 0.03f, 0.06f, 1.0f};

    auto target = device->resources().create(color, &error);
    if (target.valid() && device->ensureNative(target, &error))
    {
        pass("dx12_native_render_target_created");
    }
    else
    {
        fail("dx12_native_render_target_created", error);
    }

    if (device->clear(target, {0.10f, 0.20f, 0.35f, 1.0f}, &error))
    {
        pass("dx12_render_target_clear_executes_on_gpu");
    }
    else
    {
        fail("dx12_render_target_clear_executes_on_gpu", error);
    }

    if (device->gpuSmokeTest(&error))
    {
        pass("dx12_gpu_smoke_test_upload_and_clear");
    }
    else
    {
        fail("dx12_gpu_smoke_test_upload_and_clear", error);
    }

    const auto stats = device->gpuStats();
    if (stats.nativeBuffers >= 2 && stats.nativeTextures >= 2 && stats.uploadBytesAllocated > 0 && stats.submittedGpuCommandLists >= 3)
    {
        pass("dx12_gpu_stats_track_native_work");
    }
    else
    {
        fail("dx12_gpu_stats_track_native_work", "unexpected GPU stats");
    }

    device->waitIdle();
    device->shutdown();

    return failures == 0 ? 0 : 1;
#endif
}
