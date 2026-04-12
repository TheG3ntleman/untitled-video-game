#include <GLFW/glfw3.h>
#include <stdlib.h>


#include "app.h"
#include "src/utils/error.h"
#include "src/utils/logger.h"
#include "src/gfx/gfx.h"

struct App {
    AppConfig config;
    GLFWwindow *window;
    GfxContext *gfx_context;
};

AppConfig get_default_app_config(void) {
    LOG_DEBUG("%s", "Created Default App Config!");
    return (AppConfig) {
        .window_config = {
            .width = 800,
            .height = 600,
            .title = "Comprehensible Horrors",
            .resizable = true,
        }
    };
}

App *create_app(AppConfig app_config) {
    App *app = calloc(1, sizeof *app);

    if (!app) ERROR("%s", "Could not allocate memory for app object!");
    
    app->config = app_config;

    // Initialize GLFW
    if (!glfwInit()) {
        ERROR("Failed to initialize GLFW.");
    }

    // Going to make sure the window doesn't also spawn an OpenGL context.
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Now we are going to make a window.

    app->window = glfwCreateWindow(
        (int)app->config.window_config.width, 
        (int)app->config.window_config.height,
        app->config.window_config.title,
        NULL,
        NULL
    );

    if (!app->window) {
        destroy_app(app);
        ERROR("Failed to create GLFW Window!");
    }

    LOG_DEBUG("%s", "Initialized App and GLFW!");

    // Making GFX Context
    app->gfx_context = gfx_initialize(app, gfx_default_configuration());

    return app;
}

void destroy_app(App *app) {
    if (!app) ERROR("%s", "NULL App pointer passed to destory app");
    if (app->window)    glfwDestroyWindow(app->window);
    glfwTerminate();
    free(app);
    LOG_DEBUG("%s", "Freed App Resources (hopefully)!");
}

void run_app(App *app) {
    while (!glfwWindowShouldClose(app->window)) {
        glfwPollEvents();
        gfx_draw_frame(app->gfx_context);
    }   
}

GLFWwindow *app_get_window(App *app) {
       return app->window;
}
