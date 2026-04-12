#include <X11/Xlib.h>
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_X11

#include <stdlib.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <webgpu/webgpu.h>

#include "../app/app.h"
#include "gfx.h"
#include "../utils/logger.h"
#include "../utils/error.h"

struct GfxContext {
    WGPUInstance instance;
    WGPUSurface surface;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
};

typedef struct AdapterRequestState {
    GfxContext *context;
    bool completed;
} AdapterRequestState;

typedef struct DeviceRequestState {
    GfxContext *context;
    bool completed;
} DeviceRequestState;

GfxContextConfiguration gfx_default_configuration() {
    return (GfxContextConfiguration) {
        .instance_descriptor = (WGPUInstanceDescriptor){0},
    };
}

static void on_adapter_request(
    WGPURequestAdapterStatus status,
    WGPUAdapter adapter,
    WGPUStringView message,
    void *userdata1,
    void *userdata2
) { 
    AdapterRequestState *request_state = (AdapterRequestState*)userdata1;

    (void)message;
    (void)userdata2;
    if (status != WGPURequestAdapterStatus_Success) ERROR("%s", "Failed to get adapter.");

    wgpuAdapterAddRef(adapter);
    request_state->context->adapter = adapter;
    request_state->completed = true;
}

static void on_device_request(
    WGPURequestDeviceStatus status,
    WGPUDevice device,
    WGPUStringView message,
    void *userdata1,
    void *userdata2
) {
    DeviceRequestState *request_state = (DeviceRequestState*)userdata1;

    (void)message;
    (void)userdata2;
    if (status != WGPURequestDeviceStatus_Success) ERROR("%s", "Failed to get device.");

    request_state->context->device = device;
    request_state->completed = true;
}

GfxContext *gfx_initialize(App *app, GfxContextConfiguration context_configuration) {
    GfxContext *context = (GfxContext*)calloc(1, sizeof(GfxContext));

    if (!context) ERROR("%s", "Could not allocate memory for GFX Context!");

    // We begin by creating a WGPU instance.
    context->instance = wgpuCreateInstance(&context_configuration.instance_descriptor);

    LOG_DEBUG("%s", "Made WGPU Instance Successfully!");

    // Making a WGPU surface and attaching it to our GLFW window.

    WGPUSurfaceDescriptor surface_descriptor;

    switch (glfwGetPlatform()) {
        case GLFW_PLATFORM_X11: {
            WGPUSurfaceSourceXlibWindow x11_source = {
                .chain = {
                    .next = NULL,
                    .sType = WGPUSType_SurfaceSourceXlibWindow,
                },
                .display = glfwGetX11Display(),
                .window = glfwGetX11Window(app_get_window(app))
            };

            surface_descriptor = (WGPUSurfaceDescriptor){
                .nextInChain = (const WGPUChainedStruct *)&x11_source,
                .label = {0},
            };

            break;
        }
        case GLFW_PLATFORM_WAYLAND: {
            WGPUSurfaceSourceWaylandSurface wayland_source = {
                .chain = {
                    .next = NULL,
                    .sType = WGPUSType_SurfaceSourceWaylandSurface,
                },
                .display = glfwGetWaylandDisplay(),
                .surface = glfwGetWaylandWindow(app_get_window(app)),
            };

            surface_descriptor = (WGPUSurfaceDescriptor){
                .nextInChain = (const WGPUChainedStruct *)&wayland_source,
                .label = {0},
            };
            break;
        }
        default: { 
            ERROR("%s", "Platform not currently supported!");
            break;
        }
    }

    context->surface = wgpuInstanceCreateSurface(context->instance, &surface_descriptor);

     // Getting an adapter
     WGPURequestAdapterOptions adapter_request_options = {
        .nextInChain = NULL,
        .featureLevel = WGPUFeatureLevel_Core,
        .powerPreference = WGPUPowerPreference_Undefined,
        .forceFallbackAdapter = false,
        .backendType = WGPUBackendType_Undefined,
        .compatibleSurface = context->surface
     };

     AdapterRequestState adapter_request_state = {
        .context = context,
        .completed = false
     };

     wgpuInstanceRequestAdapter(
        context->instance, 
        &adapter_request_options,
        (WGPURequestAdapterCallbackInfo){
            .nextInChain=NULL,
            .mode=WGPUCallbackMode_AllowProcessEvents,
            .callback=on_adapter_request,
            .userdata1 = &adapter_request_state,
            .userdata2 = NULL
        } 
    );

    while (!adapter_request_state.completed) {
        wgpuInstanceProcessEvents(context->instance);
    }

    // Requesting a device
    WGPUDeviceDescriptor device_descriptor = {
        .nextInChain = NULL,
        .label = (WGPUStringView){0},
        .requiredFeatureCount = 0,
        .requiredFeatures = NULL,
        .requiredLimits = NULL,
        .defaultQueue = (WGPUQueueDescriptor){0},
        .deviceLostCallbackInfo = (WGPUDeviceLostCallbackInfo){0},
        .uncapturedErrorCallbackInfo = (WGPUUncapturedErrorCallbackInfo){0},
    };

    DeviceRequestState device_request_state = {
        .context = context,
        .completed = false
    };

    wgpuAdapterRequestDevice(
        context->adapter,
        &device_descriptor,
        (WGPURequestDeviceCallbackInfo) {
            .nextInChain = NULL,
            .mode = WGPUCallbackMode_AllowProcessEvents,
            .callback = on_device_request,
            .userdata1 = &device_request_state,
            .userdata2 = NULL
        }
    );

    while (!device_request_state.completed) {
        wgpuInstanceProcessEvents(context->instance);
    }

    // Getting the Queue
    context->queue = wgpuDeviceGetQueue(context->device);
    
    //Configure WGPU Surface
    int width, height;
    glfwGetFramebufferSize(app_get_window(app), &width, &height);

    WGPUSurfaceCapabilities capabilities = {0};
    wgpuSurfaceGetCapabilities(context->surface, context->adapter, &capabilities);

    WGPUSurfaceConfiguration surface_config = {
        .nextInChain = NULL,
        .device = context->device,
        .format = capabilities.formats[0],
        .usage = WGPUTextureUsage_RenderAttachment,
        .width = (uint32_t)width,
        .height = (uint32_t)height,
        .viewFormatCount = 0,
        .viewFormats = NULL,
        .alphaMode = capabilities.alphaModes[0],
        .presentMode = WGPUPresentMode_Fifo,
    };

    wgpuSurfaceConfigure(context->surface,&surface_config);

    // Cleaning Up
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);


    return context;
}

void gfx_draw_frame(GfxContext *context) {

    // Get the surface texture for this frame.
    WGPUSurfaceTexture surface_texture = {0};
    wgpuSurfaceGetCurrentTexture(context->surface, &surface_texture);

    WGPUTextureView surface_texture_view = wgpuTextureCreateView(surface_texture.texture, NULL);

    WGPUCommandEncoder command_encoder = wgpuDeviceCreateCommandEncoder(context->device, NULL);
    
    WGPURenderPassColorAttachment color_attachment = {
        .nextInChain = NULL,
        .view = surface_texture_view,
        .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        .resolveTarget = NULL,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = (WGPUColor) {
            .r = 0.1,
            .g = 0.3,
            .b = 0.7,
            .a = 1.0
        }
    };

    WGPURenderPassDescriptor render_pass_descriptor = {
        .nextInChain = NULL,
        .label = (WGPUStringView){0},
        .colorAttachmentCount = 1,
        .colorAttachments = &color_attachment,
        .depthStencilAttachment = NULL,
        .occlusionQuerySet = NULL,
        .timestampWrites = NULL,
    };

    WGPURenderPassEncoder render_pass = wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_descriptor);
    wgpuRenderPassEncoderEnd(render_pass);

    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(command_encoder, NULL);

    wgpuQueueSubmit(context->queue, 1, &command_buffer);

    wgpuSurfacePresent(context->surface);
}
