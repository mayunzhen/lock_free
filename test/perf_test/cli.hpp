#include <cstdio>
#include <chrono>
#include <iostream>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>

#define MAIN(...) \
  void main_(__VA_ARGS__); \
  int main(int argc, char* argv[]) { \
    impl::guarded_run(main_, argc, (const char**)argv, #__VA_ARGS__); \
  } \
  void main_(__VA_ARGS__)

using tick = std::chrono::high_resolution_clock;

template <typename T, T def>
struct optional {
  operator T&() noexcept {
    return value;
  }
  operator const T&() const noexcept {
    return value;
  }
  T value{def};
};

template <typename... Us>
std::string mkstr(const Us&... vs) {
  std::ostringstream os;
  (void)(os << ... << vs);
  return os.str();
}

namespace impl {

template <typename T>
void set_arg(T& arg, const char* str) {
  std::istringstream is(str);
  if (!(is >> arg) || is.peek() != std::char_traits<char>::eof()) {
    throw std::invalid_argument(
      mkstr("Invalid argument: ", str));
  }
}

inline void set_arg(std::string& arg, const char* str) {
  arg = str;
}

inline void set_args(const char** p, const char** end) {
  if (p != end) {
    throw std::invalid_argument("Too many arguments.");
  }
}

template <typename T, typename... Us>
void set_args(const char** p, const char** end, T& arg, Us&... args) {
  if (p == end) throw std::invalid_argument("Too few arguments.");
  set_arg(arg, *p++);
  set_args(p, end, args...);
}

template <typename T, T def, typename... Us>
void set_args(const char** p, const char** end, optional<T, def>& arg, Us&... args) {
  if (p == end) return;
  set_arg(arg, *p++);
  set_args(p, end, args...);
}

template <typename Rep, typename Period>
std::string format(std::chrono::duration<Rep, Period> du) {
  using namespace std::chrono;
  using days = duration<int, std::ratio<86400>>;
  auto d = duration_cast<days>(du);
  auto h = duration_cast<hours>(du -= d);
  auto m = duration_cast<minutes>(du -= h);
  auto s = duration_cast<seconds>(du -= m);
  char hms[] = "HH:mm:ss";
  std::sprintf(hms, "%02d:%02d:%02d",
    (int)h.count(), (int)m.count(), (int)s.count());
  return mkstr(d.count(), "d ", hms);
}

template <typename... Us>
void guarded_run(
 void(*f)(Us...), int argc, const char** argv, const char* params) noexcept {
  auto epoch = tick::now();
  try {
    auto p = argv + 1, end = argv + argc;
    std::tuple<Us...> args;
    std::apply([p, end](auto&... args) {
      set_args(p, end, args...);
    }, args);
    std::apply(f, std::move(args));
  }
  catch (const std::invalid_argument& e) {
    std::cout << e.what() << '\n'
              << "Usage: (" << params << ')' << std::endl;
    return;
  }
  catch (const std::exception& e) {
    std::cout << "std::exception caught with message:\n"
              << e.what() << std::endl;
  }
  catch (...) {
    std::cout << "Unknown exception caught." << std::endl;
  }
  std::cout << "-------------------\n"
            << "Ran for " << format(tick::now() - epoch) << std::endl;
}

} // namespace impl
