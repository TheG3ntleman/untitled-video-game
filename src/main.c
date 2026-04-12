#include <stdbool.h>

#include "src/utils/logger.h"
#include "app/app.h"

int main(void) {
    
    logger_initialize((LoggerConfig){
        .maintain_log_file=true,
        .log_file_path="./log.txt"
    });

    AppConfig application_configuration = get_default_app_config();
    App *app = create_app(application_configuration);
    run_app(app);
    destroy_app(app);

    logger_destroy();

    return 0;
}
