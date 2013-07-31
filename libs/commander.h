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

#ifndef PIONEER_RFC_COMMAND_H_
#define PIONEER_RFC_COMMAND_H_

#include <string>
#include <vector>
#include <deque>
#include <algorithm>

#include <boost/program_options.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include <atlas/serialization/uuid.h>

#include <pioneer/rfc/rfc.h>
#include <pioneer/rfc/service/rfc_func.h>

namespace po = boost::program_options;

namespace pioneer {
  namespace rfc {

    void print_count(size_t count, const std::string& result, int err_code, async_task& task) {
      if (err_code) {
        std::cout << result << std::endl;
        return;
      }

      std::cout << "There are " << count << " records" << std::endl;
    }

    template<typename MessageSender>
    class commander : public remote_caller {
    public:

      commander(MessageSender& sender) : remote_caller(rfc::outward_client), _sender(sender) {
        _descs["help"].add_options()
            ("start_udp_test", "start udp test")
            ("stop_udp_test", "stop udp test")
            ;

        _descs["cstart_udp_test"].add_options()
            ("help", "usage : start_udp_test [rounds] [test_count] [interval] [rest_time]")
            ("rounds", po::value<int>()->default_value(1), "test rounds")
            ("test_count", po::value<int>()->default_value(1), "test times")
            ("interval", po::value<int>()->default_value(10), "time interval between two test in millisecond")
            ("rest_time", po::value<int>()->default_value(1), "rest time in seconds")
            ;

        _descs["start_udp_test"].add_options()
            ("help", "usage : start_udp_test [rounds] [test_count] [interval] [rest_time]")
            ("rounds", po::value<int>()->default_value(1), "test rounds")
            ("test_count", po::value<int>()->default_value(1), "test times")
            ("interval", po::value<int>()->default_value(10), "time interval between two test in millisecond")
            ("rest_time", po::value<int>()->default_value(1), "rest time in seconds")
            ;

        _descs["cstop_udp_test"].add_options()
            ("help", "usage : stopt_udp_test")
            ;

        _descs["cannounce_data_node"].add_options()
            ("help", "usage : cannounce_data_node ip_list")
            ("ip_list", po::value<std::string>(), "ip list separated by a comma")
            ;
      }

    public:

      void order(const std::string& c) {
        boost::char_separator<char> sep(" ");
        boost::tokenizer<boost::char_separator<char>> token(c, sep);

        std::string command;
        std::vector<std::string> args;
        int i = 0;
        for (auto it = token.begin(); it != token.end(); ++it) {
          if (i++ == 0) command = *it;
          else args.push_back(*it);
        }

        parse(command, args);
      }

    protected:

      virtual void send(const std::string& message) {
        _sender.send(message);
      }

      virtual void send(const char* message, size_t size) {
        _sender.send(message, size);
      }

    protected:

      bool check_require(const po::variables_map& vm, const std::string& option, const po::options_description& desc) {
        if (!vm.count(option)) {
          std::cout << desc << "\n";

          return false;
        }

        return true;
      }

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

          if (command == "cstart_udp_test") {
            call(rfc_func::cstart_udp_test, fn_ids::cstart_udp_test,
                vm["rounds"].as<int>(),
                vm["test_count"].as<int>(),
                vm["interval"].as<int>(),
                vm["rest_time"].as<int>(),
                nilctx);
          }
          else if (command == "start_udp_test") {
            call(rfc_func::start_udp_test, fn_ids::start_udp_test,
                vm["rounds"].as<int>(),
                vm["test_count"].as<int>(),
                vm["interval"].as<int>(),
                vm["rest_time"].as<int>(),
                nilctx);
          }
          else if (command == "stop_udp_test") {
            call(rfc_func::stop_udp_test, fn_ids::stop_udp_test, nilctx);
          }
          else if (command == "cannounce_data_node") {
            if (!check_require(vm, "ip_list", desc)) return;

            call(rfc_func::cannounce_inward_node, fn_ids::cannounce_inward_node,
                vm["ip_list"].as<std::string>(), nilctx);
          }
        }
        catch (const std::exception& e) {
          std::cerr << "error: " << e.what() << "\n";
        }
        catch (...) {
          std::cerr << "Exception of unknown type!\n";
        }
      }

    private:

      boost::ptr_map<std::string, po::options_description> _descs;

      MessageSender& _sender;
    };

  } // client
} // pioneer

#endif /* RFC_COMMAND_H_ */
