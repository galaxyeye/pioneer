/*
 * rpc_func.ipp
 *
 *  Created on: Oct 10, 2011
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

#include <iterator>

#include <boost/tokenizer.hpp>

#include <atlas/rpc.h>
#include <pioneer/net/net.h>
#include <pioneer/system/context.h>

namespace pioneer {
  namespace rpc {

    using atlas::rpc::rpc_iarchive;
    using atlas::rpc::rpc_context;
    using atlas::rpc::rf_wrapper;
    using atlas::rpc::async_task;
    using atlas::rpc::rpc_callback_type;
    using atlas::rpc::builtin_rfc;
    using atlas::rpc::nilctx;

    class customer_dispatcher {
    public:

      static boost::optional<rpc_result> dispatch(int fn_id, const std::string& message, const rpc_context& context) {
        std::istringstream iss(message);
        rpc_iarchive ia(iss);

        // LOG(INFO) << "dispatch " << method << " for " << context.session_id() << " from " << context.source_ip();

        switch (fn_id) {
        case fn_ids::announce_inner_node: {
          rf_wrapper<decltype(rpc_func::announce_inner_node)> announce_inner_node(rpc_func::announce_inner_node, ia, context);
          return announce_inner_node();
        }
        break;
        case fn_ids::cannounce_inner_node: {
          rf_wrapper<decltype(rpc_func::cannounce_inner_node)> cannounce_inner_node(rpc_func::cannounce_inner_node, ia, context);
          return cannounce_inner_node();
        }
        break;
        case fn_ids::udp_test_received: {
          rf_wrapper<decltype(rpc_func::udp_test_received)> udp_test_received(rpc_func::udp_test_received, ia, context);
          return udp_test_received();
        }
        break;
        case fn_ids::start_udp_test: {
          rf_wrapper<decltype(rpc_func::start_udp_test)> start_udp_test(rpc_func::start_udp_test, ia, context);
          return start_udp_test();
        }
        break;
        case fn_ids::cstart_udp_test: {
          rf_wrapper<decltype(rpc_func::cstart_udp_test)> cstart_udp_test(rpc_func::cstart_udp_test, ia, context);
          return cstart_udp_test();
        }
        break;
        default:
          // not be responsible to this rpc
          return boost::none;
          // DLOG(INFO) << "no method " << method;
        break;
        } // switch

        return boost::none;
      }
    };

    struct ack_callback {
      ack_callback(unsigned long long r) : round(r) {}

      void operator()(const string& data, int e, async_task& task) {
        DLOG(INFO) << "got " << task.response_count() << " while " << task.expected_response_count();

        if (task.ready()) {
          ++system::status::good_ack[round];
        }
      }

      unsigned long long round;
    };

    rpc_result rpc_func::announce_inner_node(const string& ip, rpc_context c) noexcept {
      DLOG(INFO) << "received announcing data node " << ip;

      // catalog and every data node connected to the target data node, including himself
      net::inward_client_pool::ref().connect(ip);

      return nullptr;
    }

    rpc_result rpc_func::cannounce_inner_node(const string& ip_list, rpc_context c) noexcept {
      DLOG(INFO) << "announcing data nodes " << ip_list;

      boost::char_separator<char> sep(",");
      boost::tokenizer<boost::char_separator<char>> token(ip_list, sep);

      for (auto it = token.begin(); it != token.end(); ++it) {
        mcast_client client;
        client.call(announce_inner_node, fn_ids::announce_inner_node, *it, nilctx);
      }

      return nullptr;
    }

    rpc_result rpc_func::outer_node_quit(rpc_context c) noexcept {
      DLOG(INFO) << "out side node quit";

      std::string ip = ip::get_ip_part(c.source_ip_port());

      // critical area
      {
        std::lock_guard<std::mutex> guard(system::context::mutex);
        system::context::outside_ip_list.erase(ip);
      }

      DLOG(INFO) << "remove app engine " << ip;

      return nullptr;
    }

    rpc_result rpc_func::inner_node_quit(rpc_context c) noexcept {
      DLOG(INFO) << "inner node quit";

      std::string ip = ip::get_ip_part(c.source_ip_port());

      // critical area
      {
        std::lock_guard<std::mutex> guard(system::context::mutex);
        system::context::inside_ip_list.erase(ip);
      }

      DLOG(INFO) << "remove data node " << ip;

      return nullptr;
    }

    rpc_result rpc_func::udp_test_received(int round, rpc_context c) noexcept {
      ++system::status::udp_test_received[round];

      return nullptr;
    }

    rpc_result rpc_func::cstart_udp_test(int rounds, int test_count, int interval, int rest_time, rpc_context c) noexcept {
      mcast_client client;
      client.call(start_udp_test, fn_ids::start_udp_test, rounds, test_count, interval, rest_time, nilctx);

      return nullptr;
    }

    rpc_result rpc_func::start_udp_test(int rounds, int test_count, int interval, int rest_time, rpc_context c) noexcept {
      ::sleep(rest_time);

      system::status::test_rounds = rounds;

      // wait for 5 seconds and then we begin to test
      for (int i = 0; i < rounds; ++i) {
        int count = test_count;
        system::status::udp_test_interval[i] = interval * (rounds - i);

        while (0 < count--) {
          ::usleep(system::status::udp_test_interval[i]);

          ack_callback ack_cb(i);
          rpc_callback_type cb(ack_cb);
          mcast_client client(inward_client, system::context::inner_node_count);
          client.call(udp_test_received, fn_ids::udp_test_received, cb, i, nilctx);

          ++system::status::udp_test_sent[i];
        }

        sleep(rest_time);
      }

      return nullptr;
    }

  } // rpc
} // mevo
