#ifndef LOGGER_H
#define LOGGER_H

#define DEBUG

#define LOG(head, name, fmt, ...)                                      \
    {                                                                  \
        char buf[1024];                                                \
        sprintf(buf, "%s%s: " fmt "\n", head, name, ##__VA_ARGS__); \
        _my_logs_ += buf;                                              \
        printf(buf);                                                   \
    }

#define LOGI(fmt, ...)                       \
    {                                        \
        LOG("\x1b[0m[INFO]", "", fmt, ##__VA_ARGS__); \
    }

#ifdef DEBUG
#define LOGD(fmt, ...)                                  \
    {                                                   \
        LOG("\x1b[1;33m[DEBUG] ", __FUNCTION__, fmt, ##__VA_ARGS__); \
    }
#else
#define LOGD(fmt, ...) \
    {                  \
    }
#endif

#define LOGE(fmt, ...)                                                   \
    {                                                                    \
        LOG("\x1b[1;31m[ERROR] ", __FUNCTION__, fmt " {%s:%d}", ##__VA_ARGS__, __FILE__,__LINE__); \
    }

#define USE_LOGS()             \
        std::string _my_logs_

#endif // LOGGER_H