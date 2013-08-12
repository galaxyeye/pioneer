/*
 * command.h
 *
 *  Created on: Jan 28, 2013
 *      Author: vincent
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

#ifndef PIONEER_RPC_COMMAND_H_
#define PIONEER_RPC_COMMAND_H_

#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <functional>

#include <boost/program_options.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <atlas/serialization/uuid.h>
#include <atlas/rpc.h>

#include <pioneer/net/rpc_clients.h>

#include "service/rfc_func.h"
#include "service/rfc_func.client.ipp"

namespace po = boost::program_options;

namespace pioneer {
  namespace rpc {

    using atlas::rpc::nilctx;

    void print_result(const std::string& result, int err_code, atlas::rpc::async_task& task) {
      std::cout << result << std::endl;
    }

    template<typename MessageSender>
    class commander : public atlas::rpc::remote_caller {
    public:

      commander(MessageSender& sender) : atlas::rpc::remote_caller(rpc::outward_client), _sender(sender) {
        _descs["help"].add_options()
            ("cannounce_inner_node", "all servers connect to the announced data node")
            ("cstart_udp_test", "all servers start udp test")
            ("accumulate", "ask the server to accumulate a list of numbers separated by commas")
            ("quit", "quit client")
            ;

        _descs["cannounce_inner_node"].add_options()
            ("help", "usage : cannounce_inner_node --ips ip, ip2, ip3 ...")
            ("ips", po::value<std::string>(), "the expected server's ips to connect to, if there are several servers,"
                " the ip list should be separated by a comma")
            ;

        _descs["cstart_udp_test"].add_options()
            ("help", "usage : start_udp_test [--rounds] [--test_count] [--interval] [--rest_time]")
            ("rounds", po::value<int>()->default_value(1), "test rounds")
            ("test_count", po::value<int>()->default_value(1), "test times")
            ("interval", po::value<int>()->default_value(10), "time interval between two test in millisecond")
            ("rest_time", po::value<int>()->default_value(1), "rest time in seconds")
            ;

        _descs["accumulate"].add_options()
            ("help", "usage : accumulate --numbers num, num2, num3 ...")
            ("numbers", po::value<std::string>(), "the numbers to be accumulated together,"
                " the numbers should be separated by a comma")
            ;
      }

      virtual ~commander() {}

    public:

      void order(const std::string& command) {
        auto args = tokenize<std::string>(command, " ");

        std::string cmd = args.front();
        args.erase(args.begin());
        parse(cmd, args);
      }

    protected:

      void parse(const std::string& command, const std::vector<std::string>& args) {
        try {
          if (command == "help") {
            std::cout << _descs["help"];
            return;
          }

          auto it = _descs.find(command);
          if (it == _descs.end()) {
            std::cout << "invalid command, type help for more information";
            return;
          }

          const po::options_description& desc = *it->second;
          po::variables_map vm;

          po::command_line_parser parser(args);
          parser.options(desc);
          po::store(parser.run(), vm);
          po::notify(vm);

          if (vm.count("help")) {
            std::cout << desc << "\n";
            return;
          }

          dispatch(command, desc, vm);
        }
        catch (const std::exception& e) {
          std::cerr << "error: " << e.what() << "\n";
        }
        catch (...) {
          std::cerr << "Exception of unknown type!\n";
        }
      }

      bool check_require(const po::variables_map& vm, const std::string& option, const po::options_description& desc) {
        if (!vm.count(option)) {
          std::cout << desc << "\n";

          return false;
        }

        return true;
      }

      void dispatch(const std::string& command, const po::options_description& desc, const po::variables_map& vm) {
        if (command == "cstart_udp_test") {
          call(rpc_func::cstart_udp_test, fn_ids::cstart_udp_test,
              vm["rounds"].as<int>(),
              vm["test_count"].as<int>(),
              vm["interval"].as<int>(),
              vm["rest_time"].as<int>(),
              nilctx);
        }
        else if (command == "cannounce_inner_node") {
          if (!check_require(vm, "ips", desc)) return;

          call(rpc_func::cannounce_inner_node, fn_ids::cannounce_inner_node, vm["ips"].as<std::string>(), nilctx);
        }
        else if (command == "accumulate") {
          if (!check_require(vm, "numbers", desc)) return;

          // once return, call print_result to show the result
          atlas::rpc::rpc_callback_type cb(print_result);
          call(rpc_func::accumulate, fn_ids::accumulate, cb, tokenize<int>(vm["numbers"].as<std::string>()), nilctx);
        }
      }

      template<typename T, typename Container = std::vector<T> >
      Container tokenize(const std::string& args, const char* sep = ",") {
        boost::tokenizer<boost::char_separator<char>> token(args, boost::char_separator<char>(sep));

        Container separated_args;
        std::transform(token.begin(), token.end(), std::back_inserter(separated_args),
            std::bind(boost::lexical_cast<T, std::string>, std::placeholders::_1));
        return separated_args;
      }

      virtual void send(const char* message, size_t size) {
        _sender.send(message, size);
      }

    private:

      boost::ptr_map<std::string, po::options_description> _descs;

      MessageSender& _sender;
    };

  } // client
} // pioneer

#endif /* PIONEER_RPC_COMMAND_H_ */
