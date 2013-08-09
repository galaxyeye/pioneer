/*
 * per_counter.ipp
 *
 *  Created on: Apr 23, 2011 ~ 2013
 *      Author: Vincent Zhang, ivincent.zhang@gmail.com
 */

/*    Copyright 2011 ~ 2013 Vincent Zhang, ivincent.zhang@gmail.com
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef PERF_COUNTER_IPP_
#define PERF_COUNTER_IPP_

#include <atomic>
#include <ctime>

namespace pioneer {

  std::atomic<long> status::last_check_time = ATOMIC_VAR_INIT(::time(0));

  // mcast
  std::atomic<unsigned long long> status::mcast_sent = ATOMIC_VAR_INIT(0);
  std::atomic<unsigned long long> status::mcast_received = ATOMIC_VAR_INIT(0);

  // connections
  std::atomic<unsigned long long> status::active_outer_connections = ATOMIC_VAR_INIT(0);
  std::atomic<unsigned long long> status::failed_outer_connections = ATOMIC_VAR_INIT(0);

  std::atomic<unsigned long long> status::active_inner_connections = ATOMIC_VAR_INIT(0);
  std::atomic<unsigned long long> status::failed_inner_connections = ATOMIC_VAR_INIT(0);

  // udp_test
  std::atomic<unsigned long long> status::test_rounds = ATOMIC_VAR_INIT(0);
  std::array<std::atomic<unsigned long long>, max_udp_test_rounds> status::udp_test_interval;
  std::array<std::atomic<unsigned long long>, max_udp_test_rounds> status::udp_test_sent;
  std::array<std::atomic<unsigned long long>, max_udp_test_rounds> status::udp_test_received;
  std::array<std::atomic<unsigned long long>, max_udp_test_rounds> status::good_ack;
} // mevo

#endif /* PERF_COUNTER_H_ */
