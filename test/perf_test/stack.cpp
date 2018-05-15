#include "cli.hpp"
#include "libtag.hpp"
#include "simulator.hpp"

#include <lf/stack.hpp>
#include <boost/lockfree/stack.hpp>

std::vector<measure_fn> get_lf_fn(
 unsigned thread_cnt, unsigned reps, unsigned margin) {
  auto len = reps + margin * 2;
  static lf::stack<int> stk(thread_cnt * len * 2);
  for (auto i = 0u; i < thread_cnt * len; ++i) {
    stk.try_push(i);
  }
  return {
    []() noexcept {
      auto di = tick::now();
      (void)stk.try_push(0);
      auto da = tick::now();
      return tp_pr{di, da};
    },
    []() noexcept {
      auto di = tick::now();
      (void)stk.try_pop();
      auto da = tick::now();
      return tp_pr{di, da};
    }
  };
}

std::vector<measure_fn> get_boost_fn(
 unsigned thread_cnt, unsigned reps, unsigned margin) {
  auto len = reps + margin * 2;
  static boost::lockfree::stack<int> stk(thread_cnt * len * 2);
  for (auto i = 0u; i < thread_cnt * len; ++i) {
    stk.push(i);
  }
  return {
    [] {
      auto di = tick::now();
      (void)stk.push(0);
      auto da = tick::now();
      return tp_pr{di, da};
    },
    [] {
      auto di = tick::now();
      int ret;
      (void)stk.pop(ret);
      auto da = tick::now();
      return tp_pr{di, da};
    }
  };
}

void confirm_footprint(unsigned thread_cnt, unsigned reps, unsigned margin) {
  auto sim = simulator::estimate_size(thread_cnt, 2, reps, margin);
  auto len = reps + margin * 2;
  auto stk = std::size_t(16) * thread_cnt * len * 2;
  CONFIRM_MEMORY_FOOTPRINT(sim, stk);
}

MAIN(
 lib tag,
 unsigned thread_cnt,
 optional<unsigned, 1_M> reps,
 optional<unsigned, 1_M> margin,
 optional<signed char, ' '> delimiter) {
  confirm_footprint(thread_cnt, reps, margin);
  auto get_fn = tag == lib::lf ? &get_lf_fn : &get_boost_fn;
  auto file_name = mkstr("stack_", tag, '_', thread_cnt);
  simulator::configure(thread_cnt, reps, margin, get_fn(thread_cnt, reps, margin));
  simulator::kickoff();
  simulator::write_result(file_name, delimiter);
}
