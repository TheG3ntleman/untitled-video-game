#ifndef APP_H
#define APP_H

#include <GLFW/glfw3.h>

// GLFW Related Stuff for Windowing
#include "utils/types.h"

// Making a window settings object
typedef struct WindowConfig {
    uint width;
    uint height;
    const char *title;
    bool resizable;    
} WindowConfig;


// Making an app object
typedef struct App App;

// App Configure struct
typedef struct AppConfig {
    WindowConfig window_config;
} AppConfig;

AppConfig get_default_app_config(void);
App *create_app(AppConfig app_config);
void destroy_app(App *app);
void run_app(App *app);

GLFWwindow *app_get_window(App *app);

#endif
