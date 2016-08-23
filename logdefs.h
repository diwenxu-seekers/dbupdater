#ifndef LOGDEFS_H
#define LOGDEFS_H

#include "spdlog/logger.h"


// Logger async queue size (1 MB)
#define LOG_QUEUE_SIZE 1048576

// Logger file size to rotate (10 MB)
#define LOG_FILE_SIZE 1048576 * 10

#define LOG_FILE_COUNT 10

// Logger default pattern
#define LOG_PATTERN "%b %d %T %l: %v"

// Logger path
#define LOG_PATH "logs"

// Logger file extension
#define LOG_FILE_EXT "log"

typedef std::shared_ptr<spdlog::logger> Logger;

#endif // LOGDEFS_H

