//
// Created by lh on 2021/6/2.
//

#ifndef MY_NANOLOG_MYNANOLOG_H
#define MY_NANOLOG_MYNANOLOG_H
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <tuple>

namespace nanolog {
enum class LogLevel : uint8_t { INFO, WARN, CRIT };

struct NonGuaranteedLogger {
  NonGuaranteedLogger(uint32_t ring_buffer_size_mb_)
      : ring_buffer_size_mb{ring_buffer_size_mb_} {}  //缓冲区的大小
  const uint32_t ring_buffer_size_mb;  //每条日志文件的大小mb
};

struct NanoLogLine {
  struct string_literal_t {
    explicit string_literal_t(char const* s) : m_s{s} {}
    char const* m_s;
  };
  NanoLogLine(LogLevel level, const char* file, const char* fun, int line);

  void encode_c_string(const char*, uint32_t len);

  NanoLogLine& operator<<(char arg);
  NanoLogLine& operator<<(int32_t arg);
  NanoLogLine& operator<<(uint32_t arg);
  NanoLogLine& operator<<(int64_t arg);
  NanoLogLine& operator<<(uint64_t arg);
  NanoLogLine& operator<<(double arg);
  NanoLogLine& operator<<(std::string const& arg);

  std::string tostring(LogLevel& level);
  void format_timestamp(std::fstream& os, uint64_t timestamp);
  template <class Arg>
  char* decode(std::fstream& os, char* b, Arg* dummy);

  char* decode(std::fstream& os, char* b, string_literal_t* dummy);

  char* decode(std::fstream& os, char* b, char** dummy);
  void stringfy(std::fstream& os);
  void stringfy(std::fstream& os, char* start, char* end);

 private:
  void resize_buffer_if_needed(size_t additional);
  char* buffer();

  template <class Arg>
  void encode(Arg arg);

  template <class Arg>
  void encode(Arg arg, uint8_t type_id);

  void encode(char* arg);
  void encode(char const* arg);
  void encode(string_literal_t arg);

 private:
  size_t m_bytes_used;
  size_t m_bytes_size;
  std::unique_ptr<char[]> m_heap_buffer;  //智能指针维护的是一个堆上的数组
  char m_stack_buffer[256 - 2 * sizeof(size_t) - sizeof(std::unique_ptr<char>) -
                      8];
};
struct NanoLog {
  bool operator==(NanoLogLine& line);
};

bool is_logged(LogLevel&& level);
void set_log_level(LogLevel);
void initialize(NonGuaranteedLogger, std::string, std::string, size_t);
}  // namespace nanolog
#define NANO_LOG(LEVEL) \
  nanolog::NanoLog() == \
      nanolog::NanoLogLine(LEVEL, __FILE__, __func__, __LINE__)
#define LOG_INFO                                 \
  nanolog::is_logged(nanolog::LogLevel::INFO) && \
      NANO_LOG(nanolog::LogLevel::INFO)
#define LOG_WARN                                 \
  nanolog::is_logged(nanolog::LogLevel::WARN) && \
      NANO_LOG(nanolog::LogLevel::WARN)
#define LOG_CRIT                                 \
  nanolog::is_logged(nanolog::LogLevel::CRIT) && \
      NANO_LOG(nanolog::LogLevel::CRIT)
#endif  // MY_NANOLOG_MYNANOLOG_H
