/*
 * server.cpp
 *
 *  Created on: Oct 21, 2011
 *      Author: Vincent Zhang, ivincent.zhang@gmail.com
 * */

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

#include <signal.h>

#include <cstdlib>
#include <memory>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <muduo/net/EventLoop.h>

#include <pioneer/net/net.h>
#include <pioneer/net/net_pools.h>
#include <pioneer/net/net_handlers.h>
#include <pioneer/net/multicast.h>
#include <pioneer/rfc/clients.h>

#include "service/rfc_func.h"
#include "service/rfc_func.ipp"

// must be included at last of the compile unit
#include <pioneer/post_include.h>

namespace bf = boost::filesystem;

using muduo::net::EventLoop;
using muduo::net::InetAddress;

using namespace pioneer;

std::shared_ptr<EventLoop> g_report_server_base_loop;
std::shared_ptr<EventLoop> g_inward_server_base_loop;
std::shared_ptr<EventLoop> g_outward_server_base_loop;
std::shared_ptr<net::mcast_server> g_mcast_server;

void at_signal() {
  if (system::context::system_quitting) {
    LOG(INFO) << "the system is already quitting";

    return;
  }

  system::context::system_quitting = true;

  // TODO : buggy
  rfc::mcast_client client;
  client.call(rfc::rfc_func::inner_node_quit, rfc::fn_ids::inner_node_quit, rfc::nilctx);
  sleep(2);

  net::inward_client_pool::ref().stop();
  rfc::sync_task_manager::ref().clear();

  net::mcast_client::ref().stop();
  if (g_mcast_server) g_mcast_server->stop();
  if (g_report_server_base_loop) g_report_server_base_loop->quit();
  if (g_inward_server_base_loop) g_inward_server_base_loop->quit();
  if (g_outward_server_base_loop) g_outward_server_base_loop->quit();
}

void signal_handler(int signal_no) {
  switch (signal_no) {
  case SIGHUP:
  case SIGTERM:
  case SIGQUIT:
  case SIGINT:
    at_signal();
    break;
  default:
    break;
  }
}

class pioneer_server {
public:

  pioneer_server(int outward_port, int inward_port, int report_port,
      int outward_server_threads, int inward_server_threads, int icp_threads, bool logtostderr) :
    _outward_server_address(outward_port), _inward_server_address(inward_port), _report_server_address(report_port),
    _outward_server_threads(outward_server_threads), _inward_server_threads(inward_server_threads), _icp_threads(icp_threads),
    _logtostderr(logtostderr)
  {
  }

  ~pioneer_server() {
    try {
      at_exit();
    }
    catch (...) {
      LOG(ERROR) << "exception at exit";
    }
  }

  pioneer_server(const pioneer_server&) = delete;
  const pioneer_server& operator=(const pioneer_server&) = delete;

public:

  void start() {
    // ****************************** local system ****************************
    init_glog();

    install_signal_handlers();

    // ****************************** report server ********************************
    start_report_server();

    // ****************************** main UDP service *****************************
    // start the mcast server so that we can receive UDP messages from the cluster
    start_mcast_server();
    // init mcast client so that we can send UDP message into the cluster
    init_mcast_client();

    // ****************************** main TCP server ******************************
    // start a TCP server in a standalone thread for TCP requests from outward the cluster
    start_outward_server();
    // start a TCP server in a standalone thread for TCP requests from inward the cluster
    start_inward_server();

    // ****************************** main TCP client service ***********************
    // init inner client pool so that we can establish connections to other data nodes
    init_inward_client_pool();

    sleep(5); // TODO : avoid hard coding

    LOG(INFO) << "\n\n====================let's go====================\n\n";

    for (auto t : _main_threads) {
      t.second->join();
    }

    LOG(INFO) << "\n\n=========================exit====================\n\n";

    _main_threads.clear();

    if (g_outward_server_base_loop) g_outward_server_base_loop.reset();
    if (g_inward_server_base_loop) g_inward_server_base_loop.reset();
    if (g_report_server_base_loop) g_report_server_base_loop.reset();
  }

protected:

  void init_glog() {
    if (!_logtostderr) google::InitGoogleLogging("pioneer");

    LOG(INFO) << "glog has been initialized";
  }

  void install_signal_handlers() {
    ::signal(SIGHUP, &signal_handler); // terminal quit
    ::signal(SIGTERM, &signal_handler); // kill
    ::signal(SIGQUIT, &signal_handler); /* ctrl-\ */
    ::signal(SIGINT, &signal_handler); // ctrl-c
  }

  void start_report_server() {
    auto f = [this]() {
      if (g_report_server_base_loop) return;

      LOG(INFO) << "staring report server, listening at " << _report_server_address.toIpPort().c_str() << "...";

      g_report_server_base_loop.reset(new EventLoop);
      net::report_server server(g_report_server_base_loop.get(), _report_server_address, "report server");
      server.start();
      g_report_server_base_loop->loop();

      LOG(INFO) << "quit report server";
    };

    // run in a new thread
    _main_threads["report_server"] = std::make_shared<std::thread>(f);
  }

  void start_mcast_server() {
    auto f = [this]() {
      if (g_mcast_server) return;

      LOG(INFO) << "starting mcast server...";

      g_mcast_server.reset(new net::mcast_server(PIONEER_MULTIGROUP));

      g_mcast_server->set_message_callback(net::message_handler::on_mcast_message);
      g_mcast_server->start();

      LOG(INFO) << "quit mcast server";
    };

    // run in a new thread
    _main_threads["mcast_server"] = std::make_shared<std::thread>(f);
  }

  void init_mcast_client() {
    LOG(INFO) << "initializing mcast client...";

    net::mcast_client::ref().init();
  }

  void start_outward_server() {
    auto f = [this]() {
      if (g_outward_server_base_loop) return;

      LOG(INFO) << "starting outward server, listening at " << _outward_server_address.toIpPort().c_str() << "...";

      g_outward_server_base_loop.reset(new EventLoop);
      net::outward_server server(g_outward_server_base_loop.get(), _outward_server_address, "outward server");
      server.setThreadNum(_outward_server_threads);
      server.start();

      g_outward_server_base_loop->loop();

      LOG(INFO) << "quit outward server";
    };

    // run in a new thread
    _main_threads["outward_server"] = std::make_shared<std::thread>(f);
  }

  void start_inward_server() {
    auto f = [this]() {
      if (g_inward_server_base_loop) return;

      LOG(INFO) << "starting inner server, listening at " << _inward_server_address.toIpPort().c_str() << "...";

      g_inward_server_base_loop.reset(new EventLoop);
      net::inward_server server(g_inward_server_base_loop.get(), _inward_server_address, "inward server");
      server.setThreadNum(_inward_server_threads);
      server.start();
      g_inward_server_base_loop->loop();

      LOG(INFO) << "quit inner server";
    };

    // run in a new thread
    _main_threads["inward_server"] = std::make_shared<std::thread>(f);
  }

  void init_inward_client_pool() {
    auto f = [this]() {
      LOG(INFO) << "starting data node client pool service...";

      auto& tcp_client_pool = net::inward_client_pool::ref();

      tcp_client_pool.set_server_port(pioneer::INNER_SERVER_PORT);
      tcp_client_pool.set_thread_num(_icp_threads);

      tcp_client_pool.set_connection_callback(boost::bind(net::connection_handler::on_inward_client_connection, _1));
      tcp_client_pool.set_message_callback(boost::bind(net::message_handler::on_inward_client_message, _1, _2, _3));
      tcp_client_pool.set_write_complete_callback(boost::bind(net::connection_handler::on_write_complete<net::inward_tag>, _1));

      tcp_client_pool.init();
      tcp_client_pool.start();

      LOG(INFO)<<"quit inner client pool service";
    };

    // run in a new thread
    _main_threads["inward_client_pool"] = std::make_shared<std::thread>(f);
  }

  void at_exit() {
    LOG(INFO) << "all services are stopped, do the cleaning";

    // ensure no pending worker threads
    system::worker_pool::ref().clear();

    // ensure no pending sessions
    net::session_manager::ref().clear();
  }

private:

  InetAddress _outward_server_address; // outward server address
  InetAddress _inward_server_address; // inner server address
  InetAddress _report_server_address; // report server address

  int _outward_server_threads; // outward server thread number
  int _inward_server_threads; // inner server thread number
  int _icp_threads;  // inner client pool thread number

  bool _logtostderr;

  std::map<std::string, std::shared_ptr<std::thread>> _main_threads;
};

int main(int argc, char* argv[]) {
  namespace po = boost::program_options;

  std::string help = std::string("usage : ") + argv[0] + " [options]...";

  po::options_description desc("allowed options:");
  desc.add_options()
      ("help", "Usage : [options]...")
      ("outward_port", po::value<int>()->default_value(pioneer::FOREIGN_SERVER_PORT), "outward server port")
      ("inward_port", po::value<int>()->default_value(pioneer::INNER_SERVER_PORT), "inner server port")
      ("report_port", po::value<int>()->default_value(pioneer::REPORT_SERVER_PORT), "report server port")
      ("outward_server_threads", po::value<int>()->default_value(pioneer::FOREIGN_SERVER_THREADS), "outward server thread number")
      ("inward_server_threads", po::value<int>()->default_value(pioneer::INNER_SERVER_THREADS), "inner server thread number")
      ("icp_threads", po::value<int>()->default_value(pioneer::inward_client_POOL_THREADS), "inner client pool thread number")
      ("logtostderr", po::value<bool>()->default_value(true), "all logs are written to stderr instead of file")
      ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 0;
  }

  // make it a local variable to watch the destruction
  {
    pioneer_server server(
        vm["outward_port"].as<int>(),
        vm["inward_port"].as<int>(),
        vm["report_port"].as<int>(),
        vm["outward_server_threads"].as<int>(),
        vm["inward_server_threads"].as<int>(),
        vm["icp_threads"].as<int>(),
        vm["logtostderr"].as<bool>());

    server.start();
  }

  LOG(INFO) << "bye!";
}
