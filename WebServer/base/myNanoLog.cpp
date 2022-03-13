//
// Created by lh on 2021/6/2.
//
#include "myNanoLog.h"
namespace {
std::string getTime() {
  time_t now = time(0);
  tm* ltm = localtime(&now);
  char time[32];
  sprintf(time, "%4d-%02d-%02d %02d:%02d:%02d", 1900 + ltm->tm_year,
          1 + ltm->tm_mon, ltm->tm_mday, ltm->tm_hour, ltm->tm_min,
          ltm->tm_sec);
  return time;
}
uint64_t timestamp_now() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}
std::thread::id this_thread_id() {
  static thread_local const std::thread::id id = std::this_thread::get_id();
  return id;
}

template <typename T, typename Tuple>
struct TupleIndex;

template <typename T, typename... Types>
struct TupleIndex<T, std::tuple<T, Types...>> {
  static constexpr const std::size_t value = 0;
};

template <typename T, typename U, typename... Types>
struct TupleIndex<T, std::tuple<U, Types...>> {
  static constexpr const std::size_t value =
      1 + TupleIndex<T, std::tuple<Types...>>::value;
};

}  // namespace
namespace nanolog {
typedef std::tuple<int, uint32_t, uint64_t, int32_t, int64_t, double, char*,
                   char, NanoLogLine::string_literal_t>
    Supported;
struct SpinLock {
  SpinLock(std::atomic_flag& flag) : m_flag{flag} {
    while (m_flag.test_and_set(std::memory_order_acquire))
      ;
  }
  ~SpinLock() { m_flag.clear(std::memory_order_release); }

 private:
  std::atomic_flag& m_flag;
};

struct BufferBase {
  virtual ~BufferBase() = default;
  virtual void push(NanoLogLine&) = 0;
  virtual bool try_pop(NanoLogLine&) = 0;
};

struct RingBuffer : public BufferBase {
 public:
  struct alignas(64) Item {
    Item()
        : flag{ATOMIC_FLAG_INIT},  //初始化为0
          writen{0},
          logline(LogLevel::INFO, NULL, NULL, 0) {
      ;
    }
    std::atomic_flag
        flag;  //用于同步，写进程占用这个flag资源，0资源未被占有，1资源被占有
    char writen;  //标志位，1，写了；0，没写；
    char padding[256 - sizeof(std::atomic_flag) - sizeof(char) -
                 sizeof(NanoLogLine)];
    NanoLogLine logline;
  };

  RingBuffer(const size_t size_)
      : m_size{size_},
        m_ring{new Item[m_size]},
        m_write_index{0},
        m_read_index{0} {
    static_assert(sizeof(Item) == 256, "Unexpected：size!=256");
  }
  ~RingBuffer() { delete[] m_ring; }
  virtual void push(NanoLogLine& logLine) override {
    unsigned int write_index =
        m_write_index.fetch_add(1, std::memory_order_acquire) % m_size;
    Item& item = m_ring[write_index];
    SpinLock spinLock{item.flag};
    item.logline = std::move(logLine);
    item.writen = 1;
  }
  virtual bool try_pop(NanoLogLine& logline)
      override {  // ture 弹出成功，false 弹出失败（没有日志）
    Item& item = m_ring[m_read_index % m_size];
    SpinLock spinLock{item.flag};
    if (item.writen == 1) {
      logline = std::move(item.logline);
      item.writen = 0;
      ++m_read_index;
      return true;
    }
    return false;
  }

 private:
  size_t m_size;  // slot的个数，每个slot的大小是256字节
  Item* m_ring;
  std::atomic<unsigned int> m_write_index;  //多线程写，需要是原子操作
  unsigned int m_read_index;
};
struct FileWriter {
  FileWriter(std::string& log_dirctory, std::string file_name, size_t roll_size)
      : m_log_file_roll_size_bytes{roll_size * 1024 * 1024},
        m_name{log_dirctory + file_name} {
    roll_file();
  }
  void write(NanoLogLine& logLine) {
    auto pos = m_os->tellp();
    logLine.stringfy(*m_os);
    m_bytes_written += m_os->tellp() - pos;
    if (m_bytes_written >= m_log_file_roll_size_bytes) {
      roll_file();
    }
  }

 private:
  void roll_file() {
    if (m_os) {
      m_os->flush();
      m_os->close();
    }
    m_bytes_written = 0;
    std::string log_file_name = m_name;
    log_file_name.append(".");
    log_file_name.append(std::to_string(++n_number));
    log_file_name.append(".txt");
    m_os.reset(
        new std::fstream(log_file_name, std::ios::out | std::ios::trunc));
  }

 private:
  uint32_t n_number = 0;
  std::streamoff m_bytes_written = 0;
  uint32_t const m_log_file_roll_size_bytes;
  std::string const m_name;
  std::unique_ptr<std::fstream> m_os;
};
struct NanoLogger {
  NanoLogger(NonGuaranteedLogger ngl, std::string& log_dirctory,
             std::string& file_name, size_t roll_size)
      : m_state{State::INIT},
        m_buffer_base{new RingBuffer(std::max(1u, ngl.ring_buffer_size_mb))},
        m_file_writer{log_dirctory, file_name, roll_size},
        m_thread{&NanoLogger::pop, this} {
    m_state.store(State::READY);
  }
  void add(NanoLogLine& logline) {  //生产者线程
    m_buffer_base->push(logline);
  }
  void pop() {  //消费者线程
    //等待初始化完毕
    while (m_state.load(std::memory_order_acquire) == State::INIT)
      std::this_thread::sleep_for(std::chrono::microseconds(50));
    NanoLogLine logline(LogLevel::INFO, NULL, NULL, 0);

    //开始工作
    while (m_state.load(std::memory_order_acquire) == State::READY) {
      if (m_buffer_base->try_pop(logline)) m_file_writer.write(logline);
    }

    //将剩下的所有logline全部pop了
    while (m_state.load(std::memory_order_acquire) == State::SHUTDOWN &&
           m_buffer_base->try_pop(logline)) {
      m_file_writer.write(logline);
    }
  }
  ~NanoLogger() {
    m_state.store(State::SHUTDOWN);
    m_thread.join();
  }

 private:
  enum class State : uint8_t { INIT, READY, SHUTDOWN };
  std::atomic<State> m_state;
  std::unique_ptr<BufferBase> m_buffer_base;
  FileWriter m_file_writer;
  std::thread m_thread;  //消费者线程
};

void NanoLogLine::resize_buffer_if_needed(size_t additional) {
  int required = m_bytes_used + additional;
  if (required <= m_bytes_size) return;

  if (!m_heap_buffer) {
    m_bytes_size = std::max(required, 512);
    m_heap_buffer.reset(new char[required]);
    memcpy(m_heap_buffer.get(), m_stack_buffer, m_bytes_used);
    return;
  } else {
    std::unique_ptr<char[]> new_heap_buffer;
    new_heap_buffer.reset(new char[required]);
    memcpy(new_heap_buffer.get(), m_heap_buffer.get(), m_bytes_used);
    m_heap_buffer.swap(new_heap_buffer);
    return;
  }
}
char* NanoLogLine::buffer() {
  return !m_heap_buffer ? &m_stack_buffer[m_bytes_used]
                        : &m_heap_buffer[m_bytes_used];
}
template <class Arg>
void NanoLogLine::encode(Arg arg) {
  *reinterpret_cast<Arg*>(buffer()) = arg;
  m_bytes_used += sizeof(arg);
}
template <class Arg>
void NanoLogLine::encode(Arg arg, uint8_t type_id) {
  resize_buffer_if_needed(sizeof(arg) + sizeof(uint8_t));
  encode<uint8_t>(type_id);
  encode<Arg>(arg);
}
void NanoLogLine::encode(char const* arg) {
  if (arg != nullptr) encode_c_string(arg, strlen(arg));
}

void NanoLogLine::encode(char* arg) {
  if (arg != nullptr) encode_c_string(arg, strlen(arg));
}
void NanoLogLine::encode(string_literal_t arg) {
  encode<string_literal_t>(arg, TupleIndex<string_literal_t, Supported>::value);
}
NanoLogLine::NanoLogLine(LogLevel level, const char* file, const char* fun,
                         int line)
    : m_bytes_used{0}, m_bytes_size{sizeof(m_stack_buffer)} {
  memset((void*)m_stack_buffer, '\0', sizeof(m_stack_buffer));
  encode<uint64_t>(timestamp_now());
  encode<std::thread::id>(this_thread_id());
  encode<string_literal_t>(string_literal_t(file));
  encode<string_literal_t>(string_literal_t(fun));
  encode<uint32_t>(line);
  encode<LogLevel>(level);
}

NanoLogLine& NanoLogLine::operator<<(char arg) {
  encode<char>(arg, TupleIndex<char, Supported>::value);
  return *this;
}
NanoLogLine& NanoLogLine::operator<<(int32_t arg) {
  encode<int32_t>(arg, TupleIndex<int32_t, Supported>::value);
  return *this;
}
NanoLogLine& NanoLogLine::operator<<(uint32_t arg) {
  encode<uint32_t>(arg, TupleIndex<uint32_t, Supported>::value);
  return *this;
}
NanoLogLine& NanoLogLine::operator<<(int64_t arg) {
  encode<int64_t>(arg, TupleIndex<int64_t, Supported>::value);
  return *this;
}
NanoLogLine& NanoLogLine::operator<<(uint64_t arg) {
  encode<uint64_t>(arg, TupleIndex<uint64_t, Supported>::value);
  return *this;
}
NanoLogLine& NanoLogLine::operator<<(double arg) {
  encode<double>(arg, TupleIndex<double, Supported>::value);
  return *this;
}
void NanoLogLine::encode_c_string(const char* s, uint32_t length) {
  if (length == 0) return;
  resize_buffer_if_needed(1 + length + 1);
  char* b = buffer();
  uint8_t type_id = TupleIndex<char*, Supported>::value;
  *reinterpret_cast<uint8_t*>(b++) = type_id;
  memcpy(b, s, length + 1);
  m_bytes_used += 1 + length + 1;
}
NanoLogLine& NanoLogLine::operator<<(std::string const& arg) {  //转成char*储存
  encode_c_string(arg.c_str(), arg.size());
  return *this;
}

//输出到文件
void NanoLogLine::format_timestamp(std::fstream& os, uint64_t timestamp) {
  os << "[" << getTime() << "]";
}
std::string NanoLogLine::tostring(LogLevel& level) {
  switch (level) {
    case LogLevel::INFO:
      return "INFO";
      break;
    case LogLevel::WARN:
      return "INFO";
      break;
    case LogLevel::CRIT:
      return "INFO";
      break;
  }
  return "XXXX";
}
void NanoLogLine::stringfy(std::fstream& os) {
  char* b = !m_heap_buffer ? m_stack_buffer : m_heap_buffer.get();
  char* end = b + m_bytes_used;
  uint64_t timestamp = *reinterpret_cast<uint64_t*>(b);
  b += sizeof(uint64_t);
  std::thread::id thread_id = *reinterpret_cast<std::thread::id*>(b);
  b += sizeof(std::thread::id);
  string_literal_t file = *reinterpret_cast<string_literal_t*>(b);
  b += sizeof(string_literal_t);
  string_literal_t function = *reinterpret_cast<string_literal_t*>(b);
  b += sizeof(string_literal_t);
  uint32_t line = *reinterpret_cast<uint32_t*>(b);
  b += sizeof(uint32_t);
  LogLevel loglevel = *reinterpret_cast<LogLevel*>(b);
  b += sizeof(LogLevel);
  format_timestamp(os, timestamp);
  os << "[" << tostring(loglevel) << "]"
     << "[" << file.m_s << "]"
     << "[" << function.m_s << "]"
     << "[" << line << "]";
  stringfy(os, b, end);
  os << std::endl;
  if (loglevel >= LogLevel::CRIT) {
    os.flush();
  }
}
template <class Arg>
char* NanoLogLine::decode(std::fstream& os, char* b, Arg* dummy) {
  Arg arg = *reinterpret_cast<Arg*>(b);
  os << arg;
  return b + sizeof(Arg);
}

char* NanoLogLine::decode(std::fstream& os, char* b,
                          NanoLogLine::string_literal_t* dummy) {
  NanoLogLine::string_literal_t s =
      *reinterpret_cast<NanoLogLine::string_literal_t*>(b);
  os << s.m_s;
  return b + sizeof(NanoLogLine::string_literal_t);
}

char* NanoLogLine::decode(std::fstream& os, char* b, char** dummy) {
  while (*b != '\0') {
    os << *b;
    b++;
  }
  return ++b;
}

void NanoLogLine::stringfy(std::fstream& os, char* start, char* end) {
  if (start == end) return;
  uint8_t type_id = static_cast<uint8_t>(*start);
  start++;
  switch (type_id) {
    case 0:
      stringfy(
          os,
          decode(os, start,
                 static_cast<std::tuple_element<0, Supported>::type*>(nullptr)),
          end);
      return;
    case 1:
      stringfy(
          os,
          decode(os, start,
                 static_cast<std::tuple_element<1, Supported>::type*>(nullptr)),
          end);
      return;
    case 2:
      stringfy(
          os,
          decode(os, start,
                 static_cast<std::tuple_element<2, Supported>::type*>(nullptr)),
          end);
      return;
    case 3:
      stringfy(
          os,
          decode(os, start,
                 static_cast<std::tuple_element<3, Supported>::type*>(nullptr)),
          end);
      return;
    case 4:
      stringfy(
          os,
          decode(os, start,
                 static_cast<std::tuple_element<4, Supported>::type*>(nullptr)),
          end);
      return;
    case 5:
      stringfy(
          os,
          decode(os, start,
                 static_cast<std::tuple_element<5, Supported>::type*>(nullptr)),
          end);
      return;
    case 6:
      stringfy(
          os,
          decode(os, start,
                 static_cast<std::tuple_element<6, Supported>::type*>(nullptr)),
          end);
      return;
    case 7:
      stringfy(
          os,
          decode(os, start,
                 static_cast<std::tuple_element<7, Supported>::type*>(nullptr)),
          end);
      return;
    case 8:
      stringfy(
          os,
          decode(os, start,
                 static_cast<std::tuple_element<8, Supported>::type*>(nullptr)),
          end);
      return;
  }
}
//全局变量
std::atomic<LogLevel> loglevel{LogLevel::INFO};
bool is_logged(LogLevel&& level) { return level >= loglevel; }
void set_log_level(LogLevel level) {
  loglevel.store(level, std::memory_order_release);
}

std::unique_ptr<NanoLogger> nanologger;
std::atomic<NanoLogger*> atomic_nanologger;
void initialize(NonGuaranteedLogger ngl, std::string directory,
                std::string file_name, size_t roll_size) {
  nanologger.reset(new NanoLogger(ngl, directory, file_name, roll_size));
  atomic_nanologger.store(nanologger.get(), std::memory_order_seq_cst);
}
bool NanoLog::operator==(NanoLogLine& line) {
  atomic_nanologger.load(std::memory_order_acquire)->add(line);
  return true;
}

}  // namespace nanolog