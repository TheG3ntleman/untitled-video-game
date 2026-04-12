#ifndef GFX_H
#define GFX_H

#include <webgpu/webgpu.h>

#include "../app/app.h"

typedef struct GfxContextConfiguration {

    // WGPU Descriptors
    WGPUInstanceDescriptor instance_descriptor;
} GfxContextConfiguration;

typedef struct GfxContext GfxContext;

GfxContextConfiguration gfx_default_configuration();
GfxContext *gfx_initialize(App *app, GfxContextConfiguration);
void gfx_draw_frame(GfxContext *context);

#endif
