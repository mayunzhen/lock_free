#ifndef LF_UTILITY_HPP
#define LF_UTILITY_HPP

#include <cassert>
#include <cstdint>
#include <atomic>
#include <iterator>
#include <memory>
#include <stdexcept>

namespace lf {

constexpr auto rlx = std::memory_order_relaxed;
constexpr auto rel = std::memory_order_release;
constexpr auto acq = std::memory_order_acquire;
constexpr auto eat = std::memory_order_consume;
constexpr auto cst = std::memory_order_seq_cst;
constexpr auto acq_rel = std::memory_order_acq_rel;

constexpr auto ext_cnt = (std::uint64_t)1 << 32;

template <typename T>
struct counted_ptr {
  T* ptr{};
  std::uint64_t cnt{};
};

static_assert(sizeof(void*) == 8, "Requires 64-bit pointer.");

template <typename T>
using atomic_counted_ptr = std::atomic<counted_ptr<T>>;

template <typename T>
void hold_ptr(
 atomic_counted_ptr<T>& stub,
 counted_ptr<T>& ori,
 std::memory_order mem_ord) noexcept {
 counted_ptr<T> neo;
  do {
    neo = ori;
    neo.cnt += ext_cnt;
  }
  while (!stub.compare_exchange_weak(ori, neo, mem_ord, rlx));
  ori.cnt = neo.cnt;
}

template <typename T>
bool hold_ptr_if_not_null(
 atomic_counted_ptr<T>& stub,
 counted_ptr<T>& ori,
 std::memory_order mem_ord) noexcept {
  counted_ptr<T> neo;
  do {
    if (!ori.ptr) return false;
    neo = ori;
    neo.cnt += ext_cnt;
  }
  while (!stub.compare_exchange_weak(ori, neo, mem_ord, rlx));
  ori.cnt = neo.cnt;
  return true;
}

template <typename T, typename Del = std::default_delete<T>>
void unhold_ptr_acq(T* p, Del&& del = Del{}) noexcept {
  if (p->cnt.fetch_sub(ext_cnt, rlx) == ext_cnt) {
    p->cnt.load(acq);
    del(p);
  }
}

template <typename T, typename Del = std::default_delete<T>>
void unhold_ptr_acq(
 counted_ptr<T> cp,
 std::uint64_t int_cnt,
 Del&& del = Del{}) noexcept {
  auto delta = cp.cnt - ext_cnt - int_cnt;
  if (cp.ptr->cnt.fetch_add(delta, rlx) == -delta) {
    cp.ptr->cnt.load(acq);
    del(cp.ptr);
  }
}

template <typename T, typename Del = std::default_delete<T>>
void unhold_ptr_rel(
 counted_ptr<T> cp,
 std::uint64_t int_cnt,
 Del&& del = Del{}) noexcept {
  auto delta = cp.cnt - ext_cnt - int_cnt;
  if (cp.ptr->cnt.fetch_add(delta, rel) == -delta) {
    del(cp.ptr);
  }
}

template <typename T>
T* allocate() {
  return (T*)operator new(sizeof(T));
}

template <typename T>
T* try_allocate() noexcept {
  return (T*)operator new(sizeof(T), std::nothrow);
}

inline
void deallocate(void* p) noexcept {
  operator delete(p);
}

template <typename FwdIt>
std::size_t range_extent(FwdIt first, FwdIt last) {
  auto n = std::distance(first, last);
  assert(n >= 0);
  return n >= 0 ? n :
         throw std::invalid_argument("Invalid range.");
}

} // namespace lf

#endif // LF_UTILITY_HPP
