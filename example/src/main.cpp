#include <RCNET/RCNET_engine.h>
#include <RCNET/RCNET_logger.h>
#include "server.h"

int main(int argc, char* argv[])
{
    RCNET_log(RCNET_LOG_INFO, "Server Started\n");

#ifdef NDEBUG // If we are in Release mode
    // Keep only error and critical logs
    rcnet_logger_set_priority(RCNET_LOG_ERROR);
#else // If we are in Debug mode
    // Show all log levels
    rcnet_logger_set_priority(RCNET_LOG_DEBUG);
#endif

    RCNET_Callbacks myServerCallbacks;
    myServerCallbacks.rcnet_unload = rcnet_unload;
    myServerCallbacks.rcnet_load = rcnet_load;
    myServerCallbacks.rcnet_update = rcnet_update;

    bool success = rcnet_engine_run(&myServerCallbacks, 60);
    if (!success) 
    {
        RCNET_log(RCNET_LOG_ERROR, "Failed to start the engine\n");
        return 1;
    }

    return 0;
}