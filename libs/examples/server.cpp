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
#include <pioneer/net/multicast_server.h>
#include <pioneer/net/multicast_client.h>
#include <pioneer/system/context.h>

// must be included at last of the compile unit
#include <pioneer/post_include.h>

namespace bf = boost::filesystem;

using muduo::net::EventLoop;
using muduo::net::InetAddress;

using namespace pioneer::net;
using namespace pioneer::rfc;
using namespace pioneer::system;

std::shared_ptr<EventLoop> g_report_server_base_loop;
std::shared_ptr<EventLoop> g_inner_server_base_loop;
std::shared_ptr<EventLoop> g_foreign_server_base_loop;
std::shared_ptr<EventLoop> g_catalog_server_base_loop;
std::shared_ptr<multicast_server> g_multicast_server;

void at_signal() {
  if (context::system_quitting) {
    LOG(INFO) << "the system is already quitting";
    return;
  }

  context::system_quitting = true;

  // for data node
  if (!context::is_catalog_server) {
    mcast_client client;
    client.call(rfc_func::inward_node_quit, fn_ids::inward_node_quit, nilctx);
    sleep(2);

    inward_client_pool::ref().stop();
    catalog_client_pool::ref().stop();
    sync_task_manager::ref().clear();
  }

  multicast_client::ref().stop();
  if (g_multicast_server) g_multicast_server->stop();
  if (g_report_server_base_loop) g_report_server_base_loop->quit();
  if (g_catalog_server_base_loop) g_catalog_server_base_loop->quit();
  if (g_inner_server_base_loop) g_inner_server_base_loop->quit();
  if (g_foreign_server_base_loop) g_foreign_server_base_loop->quit();
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

  pioneer_server(int fsport, int isport, int rsport, int clport, int fsthreads, int isthreads, int icpthreads,
      bool single_mode, bool is_catalog, bool logtostderr) :
    _fsaddress(fsport), _isaddress(isport), _rsaddress(rsport), _claddress(clport),
    _fsthreads(fsthreads), _isthreads(isthreads), _icpthreads(icpthreads),
    _single_mode(single_mode),
    _is_catalog(is_catalog),
    _logtostderr(logtostderr)
  {
    context::is_catalog_server = _is_catalog;
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

    init_glob();

    pioneer::system::context::is_catalog_server = _is_catalog;
    if (_single_mode) LOG(INFO) << "single mode enabled";
    if (_is_catalog) LOG(INFO) << "this is the catalog service";

    install_signal_handlers();

    // ****************************** report server ********************************
    start_report_server();

    if (!_single_mode) {
      // ****************************** main UDP service *****************************
      // start the multicast server so that we can receive UDP messages from the cluster
      start_multicast_server();
      // init multicast client so that we can send UDP message into the cluster
      init_multicast_client();
    }

    if (!_is_catalog) {
      // ****************************** main TCP server ******************************
      // start a TCP server in a standalone thread for TCP requests from outward the cluster
      start_foreign_server();
      // start a TCP server in a standalone thread for TCP requests from inward the cluster
      start_inner_server();

      // ****************************** main TCP client service ***********************
      // init inner client pool so that we can establish connections to other data nodes
      init_data_node_client_pool();

      if (!_single_mode) {
        // ****************************** catalog client service ***********************
        // init catalog client pool so that we can establish connections to the catalog
        init_catalog_client_pool();
      }
    }
    else {
      // ****************************** catalog server ******************************
      start_catalog_server();
    }

    sleep(5); // TODO : avoid hard coding

    LOG(INFO) << "\n\n====================let's go====================\n\n";

    // ****************************** registering ***********************************
    if (!_single_mode && !_is_catalog) detect_catalog_server();

    for (auto t : _main_threads) {
      t.second->join();
    }

    LOG(INFO) << "\n\n=========================exit====================\n\n";

    _main_threads.clear();

    if (g_foreign_server_base_loop) g_foreign_server_base_loop.reset();
    if (g_inner_server_base_loop) g_inner_server_base_loop.reset();
    if (g_catalog_server_base_loop) g_catalog_server_base_loop.reset();
    if (g_report_server_base_loop) g_report_server_base_loop.reset();
  }

protected:

  void init_glog() {
    if (!_logtostderr) google::InitGoogleLogging("morpheus");

    LOG(INFO) << "glog has been initialized";
  }

  void install_signal_handlers() {
    ::signal(SIGHUP, &signal_handler); // terminal quit
    ::signal(SIGTERM, &signal_handler); // kill
    ::signal(SIGQUIT, &signal_handler); /* ctrl-\ */
    ::signal(SIGINT, &signal_handler); // ctrl-c
  }

  void init_glob() {
    for (size_t i = 0; i < status::udp_test_interval.size(); ++i) {
      status::udp_test_interval[i] = {0};
    }

    for (size_t i = 0; i < status::udp_test_sent.size(); ++i) {
      status::udp_test_sent[i] = {0};
    }

    for (size_t i = 0; i < status::udp_test_received.size(); ++i) {
      status::udp_test_received[i] = {0};
    }

    for (size_t i = 0; i < status::good_ack.size(); ++i) {
      status::good_ack[i] = {0};
    }
  }

  void start_report_server() {
    auto f = [this]() {
      if (g_report_server_base_loop) return;

      LOG(INFO) << "staring report server, listening at " << _rsaddress.toIpPort().c_str() << "...";

      g_report_server_base_loop.reset(new EventLoop);
      report_server server(g_report_server_base_loop.get(), _rsaddress);
      server.start();
      g_report_server_base_loop->loop();

      LOG(INFO) << "quit report server";
    };

    // run in a new thread
    _main_threads["report_server"] = std::make_shared<std::thread>(f);
  }

  void start_multicast_server() {
    auto f = [this]() {
      if (g_multicast_server) return;

      LOG(INFO) << "starting multicast server...";

      g_multicast_server.reset(new multicast_server(pioneer::EG_MULTIGROUP));

      g_multicast_server->set_message_callback(message_handler::on_mcast_message);
      g_multicast_server->start();

      LOG(INFO) << "quit multicast server";
    };

    // run in a new thread
    _main_threads["multicast_server"] = std::make_shared<std::thread>(f);
  }

  void init_multicast_client() {
    LOG(INFO) << "initializing multicast client...";

    multicast_client::ref().init();
  }

  void start_catalog_server() {
    auto f = [this]() {
      if (g_catalog_server_base_loop) return;

      LOG(INFO) << "starting catalog server, listening at " << _claddress.toIpPort().c_str() << "...";

      g_catalog_server_base_loop.reset(new EventLoop);
      catalog_server server(g_catalog_server_base_loop.get(), _claddress);
      server.start();
      g_catalog_server_base_loop->loop();
      server.stop();

      LOG(INFO) << "quit catalog server";
    };

    // run in a new thread
    _main_threads["foreign_server"] = std::make_shared<std::thread>(f);
  }

  void start_foreign_server() {
    auto f = [this]() {
      if (g_foreign_server_base_loop) return;

      LOG(INFO) << "starting foreign server, listening at " << _fsaddress.toIpPort().c_str() << "...";

      g_foreign_server_base_loop.reset(new EventLoop);
      foreign_server server(g_foreign_server_base_loop.get(), _fsaddress);
      server.set_thread_num(_fsthreads);
      server.start();
      g_foreign_server_base_loop->loop();
      server.stop();

      LOG(INFO) << "quit foreign server";
    };

    // run in a new thread
    _main_threads["foreign_server"] = std::make_shared<std::thread>(f);
  }

  void start_inner_server() {
    auto f = [this]() {
      if (g_inner_server_base_loop) return;

      LOG(INFO) << "starting inner server, listening at " << _isaddress.toIpPort().c_str() << "...";

      g_inner_server_base_loop.reset(new EventLoop);
      inner_server server(g_inner_server_base_loop.get(), _isaddress);
      server.set_thread_num(_isthreads);
      server.start();
      g_inner_server_base_loop->loop();
      server.stop();

      LOG(INFO) << "quit inner server";
    };

    // run in a new thread
    _main_threads["inner_server"] = std::make_shared<std::thread>(f);
  }

  void init_catalog_client_pool() {
    auto f = [this]() {
      LOG(INFO) << "starting catalog client pool service...";

      auto& tcp_client_pool = catalog_client_pool::ref();

      tcp_client_pool.set_thread_num(pioneer::CATALOG_CLIENT_POOL_THREADS);
      tcp_client_pool.set_server_port(pioneer::CATALOG_SERVER_PORT);

      tcp_client_pool.set_connection_callback(boost::bind(connection_handler::on_catalog_client_connection, _1));
      tcp_client_pool.set_message_callback(boost::bind(message_handler::on_catalog_client_message, _1, _2, _3));
      tcp_client_pool.set_write_complete_callback(boost::bind(connection_handler::on_write_complete<catalog_tag>, _1));

      tcp_client_pool.init();
      tcp_client_pool.start();

      LOG(INFO)<<"quit catalog client pool service";
    };

    // run in a new thread
    _main_threads["catalog_client_pool"] = std::make_shared<std::thread>(f);
  }

  void init_data_node_client_pool() {
    auto f = [this]() {
      LOG(INFO) << "starting data node client pool service...";

      auto& tcp_client_pool = data_node_client_pool::ref();

      tcp_client_pool.set_server_port(pioneer::INNER_SERVER_PORT);
      tcp_client_pool.set_thread_num(_icpthreads);

      tcp_client_pool.set_connection_callback(boost::bind(connection_handler::on_inward_client_connection, _1));
      tcp_client_pool.set_message_callback(boost::bind(message_handler::on_inward_client_message, _1, _2, _3));
      tcp_client_pool.set_write_complete_callback(boost::bind(connection_handler::on_write_complete<inward_tag>, _1));

      tcp_client_pool.init();
      tcp_client_pool.start();

      LOG(INFO)<<"quit inner client pool service";
    };

    // run in a new thread
    _main_threads["data_node_client_pool"] = std::make_shared<std::thread>(f);
  }

  void detect_catalog_server() {
    LOG(INFO) << "try detect catalog";

    // unreliable multicast
    multicast_client client;
    client.call(rfc_func::detect_catalog, fn_ids::detect_catalog, std::string("data_node"), nilctx);
  }

  void at_exit() {
    LOG(INFO) << "all services are stopped, do the cleaning";

    // ensure no pending worker threads
    pioneer::system::the_worker_pool::ref().clear();

    // ensure no pending sessions
    session_manager::ref().clear();
  }

private:

  InetAddress _fsaddress; // foreign server address
  InetAddress _isaddress; // inner server address
  InetAddress _rsaddress; // report server address
  InetAddress _claddress; // catalog server address

  int _fsthreads; // foreign server thread number
  int _isthreads; // inner server thread number
  int _icpthreads;  // inner client pool thread number
  bool _single_mode;
  bool _is_catalog;
  bool _logtostderr;

  std::map<std::string, std::shared_ptr<std::thread>> _main_threads;
};

int main(int argc, char* argv[]) {
  namespace po = boost::program_options;

  std::string help = std::string("usage : ") + argv[0] + " [options]...";

  po::options_description desc("allowed options:");
  desc.add_options()
      ("help", "Usage : [options]...")
      ("fsport", po::value<int>()->default_value(pioneer::FOREIGN_SERVER_PORT), "foreign server port")
      ("isport", po::value<int>()->default_value(pioneer::INNER_SERVER_PORT), "inner server port")
      ("rsport", po::value<int>()->default_value(pioneer::REPORT_SERVER_PORT), "report server port")
      ("clport", po::value<int>()->default_value(pioneer::CATALOG_SERVER_PORT), "catalog server port")
      ("fsthreads", po::value<int>()->default_value(pioneer::FOREIGN_SERVER_THREADS), "foreign server thread number")
      ("isthreads", po::value<int>()->default_value(pioneer::INNER_SERVER_THREADS), "inner server thread number")
      ("icpthreads", po::value<int>()->default_value(pioneer::inward_client_POOL_THREADS), "inner client pool thread number")
      ("single_mode", po::value<bool>()->default_value(false), "config entity grid as single mode")
      ("catalog", po::value<bool>()->default_value(false), "config this host to be a catalog")
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
        vm["fsport"].as<int>(),
        vm["isport"].as<int>(),
        vm["rsport"].as<int>(),
        vm["clport"].as<int>(),
        vm["fsthreads"].as<int>(),
        vm["isthreads"].as<int>(),
        vm["icpthreads"].as<int>(),
        vm["single_mode"].as<bool>(),
        vm["catalog"].as<bool>(),
        vm["logtostderr"].as<bool>());

    server.start();
  }

  LOG(INFO) << "bye!";
}
