/*
 * rfc_func.ipp
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

#include <pioneer/rfc/service/rfc_func.h>
#include <pioneer/rfc/rfc.h>
#include <pioneer/net/net.h>
#include <pioneer/system/context.h>

namespace pioneer {
  namespace rfc {

    struct p2n_callback {
      p2n_callback(const rfc_context& c) : context(c) {}

      void operator()(size_t count, const string& data, int e, async_task& task) {
        if (!data.empty()) { task.put_data(data); }

        DLOG(INFO) << "got " << task.resp_count() << " while " << task.resp_expect() << " expected, count " << count;

        if (task.ready()) {
          rfc_result result(task.record_count(), task.merge_data());
          net::p2p_client client(context.client_type(), context.source_ip_port());
          client.call(builtin_rfc::resume_task, fn_ids::resume_task, context.session_id(), result, nilctx);
        }
      }

      rfc_context context;
    };

    struct ack_callback {
      ack_callback(unsigned long long r) : round(r) {}

      void operator()(size_t count, const string& data, int e, async_task& task) {
        DLOG(INFO) << "got " << task.resp_count() << " while " << task.resp_expect() << " expected, count " << count;

        if (task.ready()) {
          ++status::good_ack[round];
        }
      }

      unsigned long long round;
    };

    rfc_result rfc_func::announce_inward_node(const string& ip, rfc_context c) {
      DLOG(INFO) << "received announcing data node " << ip;

      if (system::context::is_catalog_server) {
        DLOG(INFO) << "i am not a data node, nothing to do";
        return nullptr;
      }

      // catalog and every data node connected to the target data node, including himself
      net::inward_client_pool::ref().connect(ip);

      return nullptr;
    }

    rfc_result rfc_func::cannounce_inward_node(const string& ip_list, rfc_context c) {
      DLOG(INFO) << "announcing data nodes " << ip_list;

      if (system::context::is_catalog_server) {
        DLOG(INFO) << "i am not a data node, nothing to do";
        return nullptr;
      }

      boost::char_separator<char> sep(",");
      boost::tokenizer<boost::char_separator<char>> token(ip_list, sep);

      for (auto it = token.begin(); it != token.end(); ++it) {
        net::mcast_client client;
        client.call(announce_inward_node, fn_ids::announce_inward_node, *it, nilctx);
      }

      return nullptr;
    }

    rfc_result rfc_func::outside_node_quit(rfc_context c) {
      DLOG(INFO) << "quit app engine";

      if (!system::context::is_catalog_server) {
        DLOG(INFO) << "i am not a catalog, nothing to do";
        return nullptr;
      }

      std::string ip = ip::get_ip_part(c.source_ip_port());

      // critical area
      {
        std::lock_guard<std::mutex> guard(system::context::mutex);
        system::context::outside_ip_list.erase(ip);
      }

      DLOG(INFO) << "remove app engine " << ip;

      return nullptr;
    }

    rfc_result rfc_func::inside_node_quit(rfc_context c) {
      DLOG(INFO) << "quit data node";

      if (!system::context::is_catalog_server) {
        DLOG(INFO) << "i am not a catalog, nothing to do";
        return nullptr;
      }

      std::string ip = ip::get_ip_part(c.source_ip_port());

      // critical area
      {
        std::lock_guard<std::mutex> guard(system::context::mutex);
        system::context::inside_ip_list.erase(ip);
      }

      DLOG(INFO) << "remove data node " << ip;

      return nullptr;
    }

    rfc_result rfc_func::udp_test_received(int round, rfc_context c) {
      ++status::udp_test_received[round];

      rfc_result r(1);
      return r;
    }

    rfc_result rfc_func::cstart_udp_test(int rounds, int test_count, int interval, int rest_time, rfc_context c) {
      mcast_client client;
      client.call(start_udp_test, query_method::start_udp_test, rounds, test_count, interval, rest_time, nilctx);

      return nullptr;
    }

    rfc_result rfc_func::start_udp_test(int rounds, int test_count, int interval, int rest_time, rfc_context c) {
      sleep(rest_time);

      status::test_rounds = rounds;

      // wait for 5 seconds and then we begin to test
      for (int i = 0; i < rounds; ++i) {
        int count = test_count;
        status::udp_test_interval[i] = interval * (rounds - i);

        while (system::context::udp_test_enabled && 0 < count--) {
          ::usleep(status::udp_test_interval[i]);

          ack_callback ack_cb(i);
          rfc_callback_type cb(ack_cb);
          mcast_client client(inward_client, system::context::inward_node_count);
          client.call(udp_test_received, query_method::udp_test_received, cb, i, nilctx);

          ++status::udp_test_sent[i];
        }

        sleep(rest_time);
      }

      return nullptr;
    }

    rfc_result rfc_func::stop_udp_test(rfc_context c) {
      system::context::udp_test_enabled = false;

      return nullptr;
    }

  } // rfc
} // mevo
