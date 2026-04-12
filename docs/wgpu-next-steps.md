# WGPU Next Steps

This guide starts from your current code shape:

- you can create a `WGPUInstance`
- you can create a `WGPUSurface` from the GLFW window
- you can request an adapter and device
- you do **not** yet configure the surface or render a frame

Goal:
- stabilize the async init path
- configure the surface
- clear to a background color
- add a shader module
- create a render pipeline
- draw a centered red triangle

## Two Pitfalls That Already Bit You

These are worth writing down first, because they caused the two crashes before:

1. Do **not** use:

```c
wgpuInstanceWaitAny(instance, 1, &wait_info, UINT64_MAX);
```

On this runtime, that counted as a timed wait and led to the Rust side panicking.

Use:
- `WGPUCallbackMode_AllowProcessEvents`
- `wgpuInstanceProcessEvents(instance)` in a loop

2. Do **not** interpret `WGPUFuture.id` as a success/failure code.

`WGPUFuture.id` is an opaque id, not an error code.

## What To Fix In Your Current Init Path

Your current `gfx_initialize(...)` should keep the adapter/device callbacks, but use callback completion flags instead of `wgpuInstanceWaitAny(...)`.

Pattern:

```c
typedef struct AdapterRequestState {
    GfxContext *context;
    bool completed;
} AdapterRequestState;
```

Inside the callback:

```c
request_state->context->adapter = adapter;
request_state->completed = true;
```

Then after `wgpuInstanceRequestAdapter(...)`:

```c
while (!adapter_request_state.completed) {
    wgpuInstanceProcessEvents(context->instance);
}
```

Do the same for the device request.

One correctness note:
- add `wgpuAdapterAddRef(adapter);` in the adapter callback before storing it

## Step 1: Store The Surface Configuration

Right now your `GfxContext` only stores:
- instance
- surface
- adapter
- device
- queue

You will need to add:

```c
WGPUSurfaceConfiguration surface_configuration;
bool surface_configured;
```

Why:
- the surface must be configured before you can render
- you will need the chosen surface format later when creating the render pipeline

## Step 2: Configure The Surface

You need these APIs:

```c
WGPUStatus wgpuSurfaceGetCapabilities(
    WGPUSurface surface,
    WGPUAdapter adapter,
    WGPUSurfaceCapabilities *capabilities
);

void wgpuSurfaceConfigure(
    WGPUSurface surface,
    WGPUSurfaceConfiguration const *config
);

void wgpuSurfaceCapabilitiesFreeMembers(
    WGPUSurfaceCapabilities surfaceCapabilities
);
```

Relevant structs:

```c
typedef struct WGPUSurfaceCapabilities {
    WGPUChainedStructOut *nextInChain;
    WGPUTextureUsage usages;
    size_t formatCount;
    WGPUTextureFormat const *formats;
    size_t presentModeCount;
    WGPUPresentMode const *presentModes;
    size_t alphaModeCount;
    WGPUCompositeAlphaMode const *alphaModes;
} WGPUSurfaceCapabilities;

typedef struct WGPUSurfaceConfiguration {
    WGPUChainedStruct const *nextInChain;
    WGPUDevice device;
    WGPUTextureFormat format;
    WGPUTextureUsage usage;
    uint32_t width;
    uint32_t height;
    size_t viewFormatCount;
    WGPUTextureFormat const *viewFormats;
    WGPUCompositeAlphaMode alphaMode;
    WGPUPresentMode presentMode;
} WGPUSurfaceConfiguration;
```

What you need to do:

1. Get framebuffer size from GLFW:

```c
int width = 0;
int height = 0;
glfwGetFramebufferSize(app_get_window(app), &width, &height);
```

2. Query capabilities:

```c
WGPUSurfaceCapabilities capabilities = {0};
wgpuSurfaceGetCapabilities(context->surface, context->adapter, &capabilities);
```

3. Fill config:

```c
context->surface_configuration = (WGPUSurfaceConfiguration){
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
```

4. Configure:

```c
wgpuSurfaceConfigure(context->surface, &context->surface_configuration);
```

5. Free capability arrays:

```c
wgpuSurfaceCapabilitiesFreeMembers(capabilities);
```

After this, the surface is ready to render into.

## Step 3: Render A Clear Color First

Before the triangle, get a background clear working.

You need these APIs:

```c
void wgpuSurfaceGetCurrentTexture(WGPUSurface surface, WGPUSurfaceTexture *surfaceTexture);
WGPUTextureView wgpuTextureCreateView(WGPUTexture texture, const WGPUTextureViewDescriptor *descriptor);
WGPUCommandEncoder wgpuDeviceCreateCommandEncoder(WGPUDevice device, const WGPUCommandEncoderDescriptor *descriptor);
WGPURenderPassEncoder wgpuCommandEncoderBeginRenderPass(WGPUCommandEncoder encoder, const WGPURenderPassDescriptor *descriptor);
void wgpuRenderPassEncoderEnd(WGPURenderPassEncoder renderPassEncoder);
WGPUCommandBuffer wgpuCommandEncoderFinish(WGPUCommandEncoder encoder, const WGPUCommandBufferDescriptor *descriptor);
void wgpuQueueSubmit(WGPUQueue queue, size_t commandCount, const WGPUCommandBuffer *commands);
WGPUStatus wgpuSurfacePresent(WGPUSurface surface);
```

Relevant frame types:

```c
typedef struct WGPUSurfaceTexture {
    WGPUChainedStructOut *nextInChain;
    WGPUTexture texture;
    WGPUSurfaceGetCurrentTextureStatus status;
} WGPUSurfaceTexture;

typedef struct WGPURenderPassColorAttachment {
    WGPUChainedStruct const *nextInChain;
    WGPUTextureView view;
    uint32_t depthSlice;
    WGPUTextureView resolveTarget;
    WGPULoadOp loadOp;
    WGPUStoreOp storeOp;
    WGPUColor clearValue;
} WGPURenderPassColorAttachment;

typedef struct WGPURenderPassDescriptor {
    WGPUChainedStruct const *nextInChain;
    WGPUStringView label;
    size_t colorAttachmentCount;
    WGPURenderPassColorAttachment const *colorAttachments;
    const WGPURenderPassDepthStencilAttachment *depthStencilAttachment;
    WGPUQuerySet occlusionQuerySet;
    const WGPURenderPassTimestampWrites *timestampWrites;
} WGPURenderPassDescriptor;
```

Minimal flow:

1. get current surface texture
2. create view from that texture
3. create command encoder
4. begin render pass with `loadOp = WGPULoadOp_Clear`
5. end pass
6. finish command buffer
7. submit
8. present

Clear attachment example:

```c
WGPURenderPassColorAttachment color_attachment = {
    .nextInChain = NULL,
    .view = surface_view,
    .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
    .resolveTarget = NULL,
    .loadOp = WGPULoadOp_Clear,
    .storeOp = WGPUStoreOp_Store,
    .clearValue = (WGPUColor){
        .r = 100.0 / 255.0,
        .g = 149.0 / 255.0,
        .b = 237.0 / 255.0,
        .a = 1.0,
    },
};
```

At this point, you have the cornflower-blue background.

## Step 4: Add A Shader Module

For the very first triangle, the simplest approach is:
- no vertex buffer
- generate positions in WGSL using `@builtin(vertex_index)`

You need:

```c
WGPUShaderModule wgpuDeviceCreateShaderModule(
    WGPUDevice device,
    WGPUShaderModuleDescriptor const *descriptor
);
```

Relevant types:

```c
typedef struct WGPUShaderModuleDescriptor {
    WGPUChainedStruct const *nextInChain;
    WGPUStringView label;
} WGPUShaderModuleDescriptor;

typedef struct WGPUShaderSourceWGSL {
    WGPUChainedStruct chain;
    WGPUStringView code;
} WGPUShaderSourceWGSL;
```

Pattern:

```c
static const char *triangle_shader_source =
    "@vertex\n"
    "fn vs_main(@builtin(vertex_index) vertex_index : u32) -> @builtin(position) vec4f {\n"
    "    var positions = array<vec2f, 3>(\n"
    "        vec2f(0.0, 0.55),\n"
    "        vec2f(-0.55, -0.45),\n"
    "        vec2f(0.55, -0.45)\n"
    "    );\n"
    "    let position = positions[vertex_index];\n"
    "    return vec4f(position, 0.0, 1.0);\n"
    "}\n"
    "\n"
    "@fragment\n"
    "fn fs_main() -> @location(0) vec4f {\n"
    "    return vec4f(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";
```

Then:

```c
WGPUShaderSourceWGSL shader_source = {
    .chain = {
        .next = NULL,
        .sType = WGPUSType_ShaderSourceWGSL,
    },
    .code = (WGPUStringView){ .data = triangle_shader_source, .length = WGPU_STRLEN },
};

WGPUShaderModuleDescriptor shader_module_descriptor = {
    .nextInChain = (const WGPUChainedStruct *)&shader_source,
    .label = (WGPUStringView){0},
};
```

Then create the module.

## Step 5: Create The Render Pipeline

You need:

```c
WGPURenderPipeline wgpuDeviceCreateRenderPipeline(
    WGPUDevice device,
    WGPURenderPipelineDescriptor const *descriptor
);
```

Relevant types:

```c
typedef struct WGPUColorTargetState {
    WGPUChainedStruct const *nextInChain;
    WGPUTextureFormat format;
    WGPUBlendState const *blend;
    WGPUColorWriteMask writeMask;
} WGPUColorTargetState;

typedef struct WGPUVertexState {
    WGPUChainedStruct const *nextInChain;
    WGPUShaderModule module;
    WGPUStringView entryPoint;
    size_t constantCount;
    WGPUConstantEntry const *constants;
    size_t bufferCount;
    WGPUVertexBufferLayout const *buffers;
} WGPUVertexState;

typedef struct WGPUFragmentState {
    WGPUChainedStruct const *nextInChain;
    WGPUShaderModule module;
    WGPUStringView entryPoint;
    size_t constantCount;
    WGPUConstantEntry const *constants;
    size_t targetCount;
    WGPUColorTargetState const *targets;
} WGPUFragmentState;

typedef struct WGPUPrimitiveState {
    WGPUChainedStruct const *nextInChain;
    WGPUPrimitiveTopology topology;
    WGPUIndexFormat stripIndexFormat;
    WGPUFrontFace frontFace;
    WGPUCullMode cullMode;
    WGPUBool unclippedDepth;
} WGPUPrimitiveState;

typedef struct WGPUMultisampleState {
    WGPUChainedStruct const *nextInChain;
    uint32_t count;
    uint32_t mask;
    WGPUBool alphaToCoverageEnabled;
} WGPUMultisampleState;

typedef struct WGPURenderPipelineDescriptor {
    WGPUChainedStruct const *nextInChain;
    WGPUStringView label;
    WGPUPipelineLayout layout;
    WGPUVertexState vertex;
    WGPUPrimitiveState primitive;
    const WGPUDepthStencilState *depthStencil;
    WGPUMultisampleState multisample;
    const WGPUFragmentState *fragment;
} WGPURenderPipelineDescriptor;
```

The important values for your first triangle:

- `format = context->surface_configuration.format`
- `writeMask = WGPUColorWriteMask_All`
- `topology = WGPUPrimitiveTopology_TriangleList`
- `frontFace = WGPUFrontFace_CCW`
- `cullMode = WGPUCullMode_None`
- `count = 1`
- `bufferCount = 0`
- `buffers = NULL`
- `layout = NULL`

Why `bufferCount = 0`:
- because this version generates positions in the shader instead of reading a vertex buffer

## Step 6: Draw The Triangle In The Render Pass

Once the pipeline exists, the render-pass body becomes:

```c
render_pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_descriptor);
wgpuRenderPassEncoderSetPipeline(render_pass, triangle_pipeline);
wgpuRenderPassEncoderDraw(render_pass, 3, 1, 0, 0);
wgpuRenderPassEncoderEnd(render_pass);
```

That one draw call means:
- `3` vertices
- `1` instance
- start at vertex `0`
- start at instance `0`

Because the vertex shader uses `vertex_index`, those three vertices become your triangle.

## Suggested Order Of Work

If you want the shortest path from where you are now to the triangle:

1. Fix adapter/device request flow with `ProcessEvents`
2. Add `surface_configuration` to `GfxContext`
3. Query surface capabilities and configure the surface
4. Make the clear-color frame work
5. Add a WGSL shader module
6. Add a render pipeline
7. In the render pass, set the pipeline and call `wgpuRenderPassEncoderDraw(...)`

## Useful Commands To Inspect The API Yourself

Search relevant pieces:

```bash
rg -n "wgpuInstanceRequestAdapter|wgpuAdapterRequestDevice|wgpuInstanceProcessEvents|wgpuSurfaceGetCapabilities|wgpuSurfaceConfigure|wgpuSurfaceGetCurrentTexture|wgpuDeviceCreateShaderModule|wgpuDeviceCreateRenderPipeline|wgpuRenderPassEncoderDraw" external/wgpu-native/include/webgpu/webgpu.h
```

Open the surrounding type definitions:

```bash
sed -n '360,420p' external/wgpu-native/include/webgpu/webgpu.h
sed -n '1660,1835p' external/wgpu-native/include/webgpu/webgpu.h
sed -n '1930,2215p' external/wgpu-native/include/webgpu/webgpu.h
sed -n '3468,3858p' external/wgpu-native/include/webgpu/webgpu.h
```

## The One-Sentence Mental Model

To get the triangle on screen from your current point:

request adapter -> request device -> get queue -> configure surface -> acquire frame -> begin render pass -> clear -> set pipeline -> draw 3 vertices -> submit -> present

## Appendix: Failure Notes

These are the two specific mistakes that caused the earlier crashes and false failures.

### 1. `wgpuInstanceWaitAny(..., UINT64_MAX)` caused a runtime panic

This looked reasonable at first:

```c
wgpuInstanceWaitAny(instance, 1, &wait_info, UINT64_MAX);
```

But on this runtime, that counted as a timed wait. The vendored `webgpu.h` documents that timed waits depend on instance capabilities, and in your case this path ended up panicking inside the Rust implementation instead of giving a friendly failure.

What to do instead:
- use `WGPUCallbackMode_AllowProcessEvents`
- then loop on `wgpuInstanceProcessEvents(instance)` until your callback marks the request complete

### 2. `WGPUFuture.id == 0` is not a failure check

This was another bad assumption:

```c
if (future.id == 0) {
    ERROR("request failed to start");
}
```

`WGPUFuture.id` is documented as an opaque id, not a success/failure code. That means you should not interpret zero specially unless the API explicitly says to do that.

What to do instead:
- wait for your callback completion flag
- then inspect the actual result you stored, like `context->adapter` or `context->device`

### Safe Pattern To Remember

For adapter/device requests, the safe pattern you now want to remember is:

1. create a small request-state struct with `completed = false`
2. pass it through `userdata1`
3. in the callback:
   - validate the status
   - store the adapter/device
   - set `completed = true`
4. loop on `wgpuInstanceProcessEvents(...)` until `completed`

That pattern is much safer than trying to infer success from the opaque future handle.
