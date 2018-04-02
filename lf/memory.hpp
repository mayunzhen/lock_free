#ifndef LF_MEMORY_HPP
#define LF_MEMORY_HPP

#include "utility.hpp"

#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace lf {

namespace impl {

template <typename T, typename... Us>
auto init(dispatch_1_tag, T*& p, Us&&... us)
-> decltype((void)new(p) T(std::forward<Us>(us)...)) {
  p = new(p) T(std::forward<Us>(us)...);
}

template <typename T, typename... Us>
void init(dispatch_0_tag, T*& p, Us&&... us) {
  p = new(p) T{std::forward<Us>(us)...};
}

} // namespace impl

template <typename T>
T* allocate() {
  return (T*)operator new(sizeof(T));
}

template <typename T>
T* try_allocate() noexcept {
  return (T*)operator new(sizeof(T), std::nothrow);
}

const struct deallocate_t {
  void operator()(void* p) const noexcept {
    operator delete(p);
  }
} deallocate;

// template <typename T, typename... Us>
// void init(T*& p, Us&&... us) {
//   if constexpr (std::is_aggregate_v<T>) {
//     p = new(p) T{std::forward<Us>(us)...};
//   }
//   else {
//     p = new(p) T(std::forward<Us>(us)...);
//   }
// }

template <typename T, typename... Us>
void init(T*& p, Us&&... us) {
  impl::init(dispatch_1_tag{}, p, std::forward<Us>(us)...);
}

template <typename T, typename... Us>
T* make(Us&&... us) {
  auto p = allocate<T>();
  try {
    init(p, std::forward<Us>(us)...);
  }
  catch (...) {
    deallocate(p);
    throw;
  }
  return p;
}

const struct dismiss_t {
  template <typename T>
  void operator()(T* p) const noexcept {
    p->~T();
    deallocate(p);
  }
} dismiss;

template <typename T>
using unique_ptr = std::unique_ptr<T, dismiss_t>;

template <typename T, typename... Us>
auto make_unique(Us&&... us) {
  return unique_ptr<T>(make<T>(std::forward<Us>(us)...));
}

} // namespace lf

#endif // LF_MEMORY_HPP