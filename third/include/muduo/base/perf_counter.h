/*
 * per_counter.h
 *
 *  Created on: Sep 26, 2012
 *      Author: vincent
 */

#ifndef MUDUO_PERF_COUNTER_H_
#define MUDUO_PERF_COUNTER_H_

#include <atomic>

namespace muduo {

  struct perf_counter {
    static std::atomic<unsigned long long> last_io_events;
    static std::atomic<unsigned long long> last_loop_time;
  };
} // muduo

#endif /* MUDUO_PERF_COUNTER_H_ */
