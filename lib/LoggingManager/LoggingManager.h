#pragma once

#include <LittleFS.h>

#include <ClockService.h>

#define DEFAULT_LOG_FILE "/system.log"
#define DEFAULT_LOG_LEVEL LOG_LEVEL_DEBUG
#define MAX_NR_OF_LINES 512
#define REDUCE_LOG_FILE_TO 256

enum log_level_t
{
  LOG_LEVEL_DEBUG = -1,
  LOG_LEVEL_INFO = 0,
  LOG_LEVEL_INFO_FILE = 1,
  LOG_LEVEL_WARNING = 2,
  LOG_LEVEL_ERROR = 3
};

class LoggingManager
{
private:
  const char *filename;
  log_level_t log_level = LOG_LEVEL_INFO;
  log_level_t file_log_level = LOG_LEVEL_INFO_FILE;

  int file_line_counter = 0;

  ClockService &clockService;

  bool log_sequence = false;
  log_level_t log_sequence_level;

  bool initialized = false;

  String log_string(log_level_t level);
  void append_to_file(log_level_t level, String message);

  void cut_log_file();
  bool delete_first_n_lines(int n);

public:
  LoggingManager(ClockService &clockService);
  ~LoggingManager();

  void begin();
  bool is_initialized();
  bool reset_logs();

  void log(log_level_t level, const char *message);
  void log(log_level_t level, String message);
  void log(log_level_t level, bool message);
  void log(log_level_t level, int message);
  void log(log_level_t level, unsigned int message);
  void log(log_level_t level, long message);
  void log(log_level_t level, unsigned long message);
  void log(log_level_t level, float message);
  void log(log_level_t level, double message);
  void log(log_level_t level, DateTime message);
  void log(log_level_t level, TimeSpan message);

  void start_seq(log_level_t level, const char *message);
  void start_seq(log_level_t level, String message);
  void start_seq(log_level_t level, bool message);
  void start_seq(log_level_t level, int message);
  void start_seq(log_level_t level, unsigned int message);
  void start_seq(log_level_t level, long message);
  void start_seq(log_level_t level, unsigned long message);
  void start_seq(log_level_t level, float message);
  void start_seq(log_level_t level, double message);
  void start_seq(log_level_t level, DateTime message);
  void start_seq(log_level_t level, TimeSpan message);

  void append_seq(const char *message);
  void append_seq(String message);
  void append_seq(bool message);
  void append_seq(int message);
  void append_seq(unsigned int message);
  void append_seq(long message);
  void append_seq(unsigned long message);
  void append_seq(float message);
  void append_seq(double message);
  void append_seq(DateTime message);
  void append_seq(TimeSpan message);

  void end_seq(const char *message);
  void end_seq(String message);
  void end_seq(bool message);
  void end_seq(int message);
  void end_seq(unsigned int message);
  void end_seq(long message);
  void end_seq(unsigned long message);
  void end_seq(float message);
  void end_seq(double message);
  void end_seq(DateTime message);
  void end_seq(TimeSpan message);

  void end_seq();

  int count_log_lines();
  int get_file_line_counter();

  void set_filename(const char *filename);
  const char *get_filename();
  String log_level_to_string(log_level_t level);

  void set_log_level(log_level_t level);
  log_level_t get_log_level();

  void set_file_log_level(log_level_t level);
  log_level_t get_file_log_level();

  String get_logs();
};