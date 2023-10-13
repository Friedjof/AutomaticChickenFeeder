#ifndef __LOGGING_MANAGER_H__
#define __LOGGING_MANAGER_H__

#ifndef __LITTLEFS_H__
#define __LITTLEFS_H__
#include <LittleFS.h>
#endif

#ifndef __CLOCKSERVICE_H__
#define __CLOCKSERVICE_H__
#include <ClockService.h>
#endif

#define DEFAULT_LOG_FILE "/system.log"
#define DEFAULT_LOG_LEVEL LOG_LEVEL_DEBUG


enum log_level_t {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
};


class LoggingManager {
    private:
        const char* filename;
        log_level_t log_level;

        ClockService clockService;

        bool log_sequence = false;

        std::string log_string(log_level_t level);

    public:
        LoggingManager(const char* filename, ClockService clockService);
        LoggingManager(const char* filename);
        LoggingManager(ClockService clockService);
        LoggingManager();
        ~LoggingManager();

        void log(log_level_t level, const char *message);

        void start_sequence(log_level_t level, const char* message);
        void append_sequence(const char *message);
        void end_sequence();

        void set_filename(const char* filename);
        const char* get_filename();
        std::string log_level_to_string(log_level_t level);

        void set_log_level(log_level_t level);
        log_level_t get_log_level();
};

#endif