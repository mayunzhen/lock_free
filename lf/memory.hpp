#ifndef LF_MEMORY_HPP
#define LF_MEMORY_HPP

#include "utility.hpp"

#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#define LF_MAKE(p, T, ...) \
  auto p = ::lf::allocate<T>(); \
  ::lf::init(p __VA_ARGS__)

#define LF_MAKE_UNIQUE(p, T, ...) \
  auto LF_UNI_NAME(p) = ::lf::allocate<T>(); \
  auto p = ::lf::init_unique(LF_UNI_NAME(p) __VA_ARGS__)

namespace lf {

template <typename T>
T* allocate() {
  return (T*)operator new(sizeof(T));
}

template <typename T>
T* try_allocate() noexcept {
  return (T*)operator new(sizeof(T), std::nothrow);
}

inline constexpr
struct deallocate_t {
  void operator()(void* p) const noexcept {
    operator delete(p);
  }
}
deallocate;

inline constexpr
struct init_no_catch_t {
  template <typename T, typename... Us>
  void operator()(T*& p, Us&&... us) const {
    if constexpr (std::is_aggregate_v<T>) {
      p = new(p) T{std::forward<Us>(us)...};
    }
    else {
      p = new(p) T(std::forward<Us>(us)...);
    }
  }
  template <typename T, typename... Us>
  void operator()(T* const & p, Us&&... us) const {
    if constexpr (std::is_aggregate_v<T>) {
      new(p) T{std::forward<Us>(us)...};
    }
    else {
      new(p) T(std::forward<Us>(us)...);
    }
  }
}
init_no_catch;

inline constexpr
struct init_t {
  template <typename P, typename... Us>
  void operator()(P&& p, Us&&... us) const {
    try {
      init_no_catch(std::forward<P>(p), std::forward<Us>(us)...);
    }
    catch (...) {
      deallocate(p);
      throw;
    }
  }
}
init;

template <typename T, typename... Us>
T emplace(Us&&... us) {
  if constexpr (std::is_aggregate_v<T>) {
    return T{std::forward<Us>(us)...};
  }
  else {
    return T(std::forward<Us>(us)...);
  }
}

inline constexpr
struct dismiss_t {
  template <typename T>
  void operator()(T* p) const noexcept {
    p->~T();
    deallocate(p);
  }
}
dismiss;

template <typename T>
using unique_ptr = std::unique_ptr<T, dismiss_t>;

template <typename T, typename... Us>
auto init_unique(T* p, Us&&... us) {
  init(p, std::forward<Us>(us)...);
  return unique_ptr<T>(p);
}

} // namespace lf

#endif // LF_MEMORY_HPP
