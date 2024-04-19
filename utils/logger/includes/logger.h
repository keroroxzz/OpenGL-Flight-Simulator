#include <stdio.h>

#ifndef LOGGER_H
#define LOGGER_H

#define MAX_LOG_SIZE 4096

#define DEBUG

#define LOG(head, name, fmt, ...)                                                     \
    {                                                                                 \
        char buf[MAX_LOG_SIZE] = {0};                                                 \
        snprintf(buf, sizeof(buf) - 1, "%s%s: " fmt "\n", head, name, ##__VA_ARGS__); \
        _my_logs_ += buf;                                                             \
        std::cout << buf;                                                             \
    }

#define LOGI(fmt, ...)                                \
    {                                                 \
        LOG("\x1b[0m[INFO]", "", fmt, ##__VA_ARGS__); \
    }

#ifdef DEBUG
#define LOGD(fmt, ...)                                               \
    {                                                                \
        LOG("\x1b[1;33m[DEBUG] ", __FUNCTION__, fmt, ##__VA_ARGS__); \
    }
#else
#define LOGD(fmt, ...) \
    {                  \
    }
#endif

#define LOGE(fmt, ...)                                                                              \
    {                                                                                               \
        LOG("\x1b[1;31m[ERROR] ", __FUNCTION__, fmt " {%s:%d}", ##__VA_ARGS__, __FILE__, __LINE__); \
    }

#define LOGF(fmt, ...)                                                                              \
    {                                                                                               \
        LOG("\x1b[1;31m[FATAL] ", __FUNCTION__, fmt " {%s:%d}", ##__VA_ARGS__, __FILE__, __LINE__); \
        exit(1);                                                                                    \
    }

#define USE_LOGS() \
    std::string _my_logs_

#endif // LOGGER_H