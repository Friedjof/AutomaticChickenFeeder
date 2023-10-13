#include "LoggingManager.h"


LoggingManager::LoggingManager(const char* filename, ClockService clockService) {
    this->filename = filename;
    this->clockService = clockService;

    this->log_level = LOG_LEVEL_DEBUG;
}

LoggingManager::LoggingManager(const char* filename) {
    this->filename = filename;
    this->log_level = LOG_LEVEL_DEBUG;
}

LoggingManager::LoggingManager(ClockService clockService) {
    this->filename = DEFAULT_LOG_FILE;
    this->clockService = clockService;

    this->log_level = LOG_LEVEL_DEBUG;
}

LoggingManager::LoggingManager() {
    this->filename = DEFAULT_LOG_FILE;
    this->log_level = LOG_LEVEL_DEBUG;
}

LoggingManager::~LoggingManager() {
    // nothing to do here
}

std::string LoggingManager::log_string(log_level_t level) {
    std::string log_message = "[";

    log_message += this->clockService.datetime_as_string();

    log_message += "] - [";

    log_message += this->log_level_to_string(level);

    log_message += "] - [";

    return log_message;
}

std::string LoggingManager::log_level_to_string(log_level_t level) {
    std::string log_level_string;

    if (level == LOG_LEVEL_DEBUG) {
        log_level_string = "DEBUG";
    } else if (level == LOG_LEVEL_INFO) {
        log_level_string = "INFO";
    } else if (level == LOG_LEVEL_WARNING) {
        log_level_string = "WARNING";
    } else if (level == LOG_LEVEL_ERROR) {
        log_level_string = "ERROR";
    }

    return log_level_string;
}

/*
  [YYYY-MM-DD HH:MM:SS] - [LOG_LEVEL] - [message]
*/
void LoggingManager::log(log_level_t level, const char* message) {
    if (level >= this->log_level || !this->log_sequence) {
        // open file
        File file = LittleFS.open(this->filename, "a");
        if (!file) {
            Serial.println("LoggingManager::log: failed to open file");
            return;
        }

        std::string log_message = this->log_string(level);

        log_message += message;
        log_message += "]\n";

        // write to file
        //file.print(log_message.c_str());

        // Serial output
        Serial.print(log_message.c_str());

        // close file
        file.close();
    }
}

void LoggingManager::start_sequence(log_level_t level, const char* message) {
    if (level >= this->log_level && !this->log_sequence) {
        this->log_sequence = true;
    }
}

void LoggingManager::append_sequence(const char* message) {
    if (this->log_sequence) {
        
    }
}

void LoggingManager::end_sequence() {
    this->log_sequence = false;
}

void LoggingManager::set_filename(const char* filename) {
    this->filename = filename;
}

const char* LoggingManager::get_filename() {
    return this->filename;
}

void LoggingManager::set_log_level(log_level_t level) {
    this->log_level = level;
}

log_level_t LoggingManager::get_log_level() {
    return this->log_level;
}