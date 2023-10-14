#include "LoggingManager.h"


LoggingManager::LoggingManager(ClockService& clockService) : clockService(clockService) {
  this->filename = DEFAULT_LOG_FILE;
  this->log_level = LOG_LEVEL_DEBUG;
}

LoggingManager::~LoggingManager() {
  // nothing to do here
}

void LoggingManager::begin() {
  if (this->initialized) {
    return;
  }

  // check if file exists
  if (!LittleFS.exists(this->filename)) {
    // create file
    File file = LittleFS.open(this->filename, "w");
    file.close();
  }

  this->file_line_counter = this->count_log_lines();

  this->initialized = true;
}

bool LoggingManager::is_initialized() {
  return this->initialized;
}

std::string LoggingManager::log_string(log_level_t level) {
  std::string log_message = "(";

  log_message += this->clockService.datetime_as_string();

  log_message += ")[";

  log_message += this->log_level_to_string(level);

  log_message += "] \"";

  return log_message;
}

std::string LoggingManager::log_level_to_string(log_level_t level) {
  std::string log_level_string;

  if (level == LOG_LEVEL_DEBUG) {
  log_level_string = "DEBUG";
  } else if (level == LOG_LEVEL_INFO) {
  log_level_string = "INFO";
  } else if (level == LOG_LEVEL_INFO_FILE) {
  log_level_string = "INFO";
  } else if (level == LOG_LEVEL_WARNING) {
  log_level_string = "WARNING";
  } else if (level == LOG_LEVEL_ERROR) {
  log_level_string = "ERROR";
  }

  return log_level_string;
}

void LoggingManager::log(log_level_t level, const char* message) {
  if (level >= this->log_level || !this->log_sequence) {
  std::string log_message = this->log_string(level);

  log_message += message;
  log_message += "\"\n";

  // write to file
  this->append_to_file(level, log_message.c_str());

  // Serial output
  Serial.print(log_message.c_str());
  }
}

void LoggingManager::log(log_level_t level, String message) {
  this->log(level, message.c_str());
}

void LoggingManager::log(log_level_t level, bool message) {
  this->log(level, message ? "true" : "false");
}

void LoggingManager::log(log_level_t level, int message) {
  this->log(level, String(message).c_str());
}

void LoggingManager::log(log_level_t level, unsigned int message) {
  this->log(level, String(message).c_str());
}

void LoggingManager::log(log_level_t level, long message) {
  this->log(level, String(message).c_str());
}

void LoggingManager::log(log_level_t level, unsigned long message) {
  this->log(level, String(message).c_str());
}

void LoggingManager::log(log_level_t level, float message) {
  this->log(level, String(message).c_str());
}

void LoggingManager::log(log_level_t level, double message) {
  this->log(level, String(message).c_str());
}

void LoggingManager::start_seq(log_level_t level, const char* message) {
  if (level >= this->log_level && !this->log_sequence) {
  this->log_sequence = true;
  this->log_sequence_level = level;

  std::string log_message = this->log_string(level);
  log_message += message;

  // write to file
  this->append_to_file(this->log_sequence_level, log_message.c_str());

  // Serial output
  Serial.print(log_message.c_str());
  }
}

void LoggingManager::start_seq(log_level_t level, String message) {
  this->start_seq(level, message.c_str());
}

void LoggingManager::start_seq(log_level_t level, bool message) {
  this->start_seq(level, message ? "true" : "false");
}

void LoggingManager::start_seq(log_level_t level, int message) {
  this->start_seq(level, String(message).c_str());
}

void LoggingManager::start_seq(log_level_t level, unsigned int message) {
  this->start_seq(level, String(message).c_str());
}

void LoggingManager::start_seq(log_level_t level, long message) {
  this->start_seq(level, String(message).c_str());
}

void LoggingManager::start_seq(log_level_t level, unsigned long message) {
  this->start_seq(level, String(message).c_str());
}

void LoggingManager::start_seq(log_level_t level, float message) {
  this->start_seq(level, String(message).c_str());
}

void LoggingManager::start_seq(log_level_t level, double message) {
  this->start_seq(level, String(message).c_str());
}

void LoggingManager::append_seq(const char* message) {
  if (this->log_sequence) {
  std::string log_message = message;

  // write to file
  this->append_to_file(this->log_sequence_level, log_message.c_str());

  // Serial output
  Serial.print(log_message.c_str());
  }
}

void LoggingManager::append_seq(String message) {
  this->append_seq(message.c_str());
}

void LoggingManager::append_seq(bool message) {
  this->append_seq(message ? "true" : "false");
}

void LoggingManager::append_seq(int message) {
  this->append_seq(String(message).c_str());
}

void LoggingManager::append_seq(unsigned int message) {
  this->append_seq(String(message).c_str());
}

void LoggingManager::append_seq(long message) {
  this->append_seq(String(message).c_str());
}

void LoggingManager::append_seq(unsigned long message) {
  this->append_seq(String(message).c_str());
}

void LoggingManager::append_seq(float message) {
  this->append_seq(String(message).c_str());
}

void LoggingManager::append_seq(double message) {
  this->append_seq(String(message).c_str());
}

void LoggingManager::end_seq(const char* message) {
  if (this->log_sequence) {

  std::string log_message = message;
  log_message += "\"\n";

  // write to file
  this->append_to_file(this->log_sequence_level, log_message.c_str());

  // Serial output
  Serial.print(log_message.c_str());
  
  this->log_sequence = false;
  }
}

void LoggingManager::end_seq(String message) {
  this->end_seq(message.c_str());
}

void LoggingManager::end_seq(bool message) {
  this->end_seq(message ? "true" : "false");
}

void LoggingManager::end_seq(int message) {
  this->end_seq(String(message).c_str());
}

void LoggingManager::end_seq(unsigned int message) {
  this->end_seq(String(message).c_str());
}

void LoggingManager::end_seq(long message) {
  this->end_seq(String(message).c_str());
}

void LoggingManager::end_seq(unsigned long message) {
  this->end_seq(String(message).c_str());
}

void LoggingManager::end_seq(float message) {
  this->end_seq(String(message).c_str());
}

void LoggingManager::end_seq(double message) {
  this->end_seq(String(message).c_str());
}

void LoggingManager::end_seq() {
  if (this->log_sequence) {
  // write to file
  this->append_to_file(this->log_sequence_level, "\"\n");

  // Serial output
  Serial.print("\"\n");
  
  this->log_sequence = false;
  }
}

void LoggingManager::set_filename(const char* filename) {
  this->filename = filename;
}

const char* LoggingManager::get_filename() {
  return this->filename;
}

int LoggingManager::get_file_line_counter() {
  return this->file_line_counter;
}

void LoggingManager::append_to_file(log_level_t level, std::string message) {
  if (level < this->file_log_level) {
    return;
  }

  // check if file exists
  if (!LittleFS.exists(this->filename)) {
    return;
  }

  // open file
  File file = LittleFS.open(this->filename, "a");

  // write to file
  file.print(message.c_str());

  // check if \n is in the message to count the lines
  if (message.find("\n") != std::string::npos) {
    this->file_line_counter++;
  }

  // check if file is too big
  if (this->file_line_counter > MAX_NR_OF_LINES) {
    this->cut_log_file();
  }

  // close file
  file.close();
}

void LoggingManager::cut_log_file() {
  // check if file exists
  if (!LittleFS.exists(this->filename)) {
    return;
  }

  // delete first n lines
  if (this->delete_first_n_lines(this->file_line_counter - REDUCE_LOG_FILE_TO)) {
    // reset line counter
    this->file_line_counter = REDUCE_LOG_FILE_TO;

    this->log(LOG_LEVEL_INFO, "Log file was cut");
  } else {
    this->log(LOG_LEVEL_ERROR, "Log file could not be cut");
  }
}

bool LoggingManager::delete_first_n_lines(int n) {
  // check if file exists
  if (!LittleFS.exists(this->filename)) {
    return false;
  }

  return true;

  // TODO: the following code does not work, because the file can not be deleted
  
  // open file
  File file = LittleFS.open(this->filename, "r");

  // count \n in file
  int line_counter = 0;
  while (file.available()) {
    if (file.read() == '\n') {
      line_counter++;
    }
  }

  // close file
  file.close();

  // check if n is bigger than the number of lines
  if (n > line_counter) {
    return false;
  }

  // open file
  file = LittleFS.open(this->filename, "r");

  // open temp file
  File tmp_file = LittleFS.open("/tmp.log", "w");

  // delete first n lines
  int line_counter_tmp = 0;
  while (file.available()) {
    char c = file.read();

    if (c == '\n') {
      line_counter_tmp++;
    }

    if (line_counter_tmp >= n) {
      tmp_file.write(c);
    }
  }

  // close file
  file.close();

  // close temp file
  tmp_file.close();

  // delete file
  LittleFS.remove(this->filename);

  // rename temp file
  LittleFS.rename("/tmp.log", this->filename);

  return true;
}

String LoggingManager::get_logs() {
  // check if file exists
  if (!LittleFS.exists(this->filename)) {
    return "";
  }

  // open file
  File file = LittleFS.open(this->filename, "r");

  // read file
  String logs = "";
  while (file.available()) {
    logs += (char)file.read();
  }

  // close file
  file.close();

  return logs;
}

int LoggingManager::count_log_lines() {
  // check if file exists
  if (!LittleFS.exists(this->filename)) {
    return 0;
  }

  // open file
  File file = LittleFS.open(this->filename, "r");

  // count \n in file
  int line_counter = 0;
  while (file.available()) {
    if (file.read() == '\n') {
      line_counter++;
    }
  }

  // close file
  file.close();

  return line_counter;
}

void LoggingManager::set_log_level(log_level_t level) {
  this->log_level = level;
}

log_level_t LoggingManager::get_log_level() {
  return this->log_level;
}

void LoggingManager::set_file_log_level(log_level_t level) {
  this->file_log_level = level;
}

log_level_t LoggingManager::get_file_log_level() {
  return this->file_log_level;
}