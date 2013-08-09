/*
 * net_pools.h
 *
 *  Created on: Jul 22, 2013
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

#ifndef NET_POOLS_H_
#define NET_POOLS_H_

#include <string>
#include <atomic>
#include <map>
#include <mutex>
#include <condition_variable>

#include <boost/optional.hpp>
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <glog/logging.h>
#include <atlas/singleton.h>
#include <atlas/concurrent_box.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>

#include <pioneer/net/net_error.h>

namespace pioneer {
  namespace net {

    namespace mn = muduo::net;

    template<typename pool_tag>
    class connection_pool : public atlas::singleton<connection_pool<pool_tag>> {
    public:

      // static const uint64_t default_wait_time = 60 * 1000; // 1 minute

      typedef std::map<std::string, mn::TcpConnectionPtr>::const_iterator iterator;

    private:

      friend class atlas::singleton<connection_pool<pool_tag>>;
      connection_pool(connection_pool&)= delete;
      connection_pool& operator=(const connection_pool&)= delete;

      // connection_pool() : _wait_time(default_wait_time) {}

    public:

      // take a connection and remove it from the pool, so it's safe in multi-thread environment
      mn::TcpConnectionPtr take(const std::string& ip_port) {
        return _connections.take(ip_port);
      }

      mn::TcpConnectionPtr random_take() { return _connections.random_take(); }

      void put(const mn::TcpConnectionPtr& conn) {
        auto ip_port = conn->peerAddress().toIpPort();
        _connections.put(ip_port, conn);

        DLOG(INFO) << "put " << ip_port << ", pool size : " << _connections.size();
      }

      void erase(const std::string& ip_port) {
        _connections.erase(ip_port);

        LOG(INFO) << "pool size : " << _connections.size();
      }

      void clear() { _connections.clear(); }

      bool empty() const { return _connections.empty(); }

      size_t size() const { return _connections.size(); }

    private:

      atlas::concurrent_box<std::string, mn::TcpConnectionPtr> _connections;
    };

    // we may need several different TCP client pool singletons, so we make it a template
    // for example, if we need a catalog server in the cluster
    template<typename pool_tag>
    class tcp_client_pool : public atlas::singleton<tcp_client_pool<pool_tag>> {
    private:

      friend class atlas::singleton<tcp_client_pool<pool_tag>>;
      tcp_client_pool(tcp_client_pool&)= delete;
      tcp_client_pool& operator=(const tcp_client_pool&)= delete;

      typedef boost::ptr_multimap<std::string, mn::TcpClient> tcp_client_container;

      tcp_client_pool() : _stopping(false), _stopped(false), _thread_num(1), _server_port(0), _base_loop(nullptr) {}

      /// init/deinit section
    public:

      ~tcp_client_pool() = default;

      void set_server_port(unsigned short server_port) { _server_port = server_port; }

      void set_thread_num(int num) { _thread_num = num; }

      void set_connection_callback(const mn::ConnectionCallback& cb) { _on_connection = cb; }

      void set_message_callback(const mn::MessageCallback& cb) { _on_message = cb; }

      void set_write_complete_callback(const mn::WriteCompleteCallback& cb) { _on_write_complete = cb; }

      void init() {
        _base_loop = new mn::EventLoop;

        // all the loops in the thread pool will stop if the base loop quits
        _io_thread_pool.reset(new mn::EventLoopThreadPool(_base_loop));
        if (_thread_num) _io_thread_pool->setThreadNum(_thread_num);
      }

      void start() {
        _io_thread_pool->start();
        _base_loop->loop();
      }

      void stop() {
        if (_stopping || _stopped) return;

        _stopping = true;

        _base_loop->runInLoop(boost::bind(&tcp_client_pool::do_stop, this));
      }

      /// data structure access section
    public:

      void erase(const std::string& peer_ip_port) {
        std::lock_guard<std::mutex> guard(_tcp_client_pool_mutex);
        _tcp_client_pool.erase(peer_ip_port);
      }

      size_t size() const {
        std::lock_guard<std::mutex> guard(_tcp_client_pool_mutex);
        return _tcp_client_pool.size();
      }

      bool empty() const {
        std::lock_guard<std::mutex> guard(_tcp_client_pool_mutex);
        return _tcp_client_pool.empty();
      }

      /// network functionality section
    public:

      /*
       * Thread safe
       * */
      void connect(const std::string& target_ip) noexcept {
        _base_loop->runInLoop(boost::bind(&tcp_client_pool::do_connect, this, target_ip));
      }

      /*
       * Thread safe
       * */
      void disconnect(const std::string& target_ip) noexcept {
        _base_loop->runInLoop(boost::bind(&tcp_client_pool::do_disconnect, this, target_ip));
      }

      /*
       * Thread safe
       * */
      void refresh(const std::string& target_ip) noexcept {
        _base_loop->runInLoop(boost::bind(&tcp_client_pool::do_refresh, this, target_ip));
      }

      /*
       * Thread safe
       * */
      void disconnect_all() noexcept {
        _base_loop->runInLoop(boost::bind(&tcp_client_pool::do_disconnect_all, this));
      }

      /*
       * Thread safe
       * */
      void refresh_all() noexcept {
        _base_loop->runInLoop(boost::bind(&tcp_client_pool::do_refresh_all, this));
      }

      void destroy() { _destroyed_cv.notify_one(); }

    protected:

      void do_stop() {
        LOG(INFO) << "stopping client pool, please wait...";

        // client side half-close, which means the write channel is closed,
        // but the socket file descriptor is not closed by system call close(2) yet.
        // it's still possible to receive data from the socket after disconnect is called,
        // until the server side closes the socket file descriptor by close(2)
        do_disconnect_all();

        // wait until all connections raised by TcpClient are disconnected, and then
        // we can safely destroy all the connections
        std::unique_lock<std::mutex> lock(_destroyed_mutex);
        // TODO : avoid hard coding
        _destroyed_cv.wait_for(lock, std::chrono::seconds(30), [this]() {
          return _tcp_client_pool.empty();
        });

        if (!_tcp_client_pool.empty()) {
          LOG(INFO) << "force disconnect " << _tcp_client_pool.size() << " connections";

          _tcp_client_pool.clear();
        }

        // quit all sub loops and threads
        _io_thread_pool.reset();

        // quit the loop and exit client pool thread
        _base_loop->quit();

        LOG(INFO) << "inner client pool stopped";

        _stopped = true;
      }

      void do_connect(const std::string& target_ip) {
        if (_stopping) {
          LOG(INFO) << "sorry, have a rest";
          return;
        }

        // DLOG(INFO) << "try establish a connection " << system::context::local_ip << " -> " << target_ip;

        mn::InetAddress server_address(target_ip, _server_port);
        std::string name = std::string("tcp_client_") + std::to_string(_tcp_client_pool.size());

        mn::TcpClient* client = new mn::TcpClient(_io_thread_pool->getNextLoop(), server_address, name);
        client->setConnectionCallback(_on_connection);
        client->setMessageCallback(_on_message);
        client->setWriteCompleteCallback(_on_write_complete);
        client->connect();

        std::string peer_ip_port = server_address.toIpPort();
        // DLOG(INFO) << "save the TcpClient for server : " << peer_ip_port;
        std::lock_guard<std::mutex> guard(_tcp_client_pool_mutex);
        _tcp_client_pool.insert(peer_ip_port, client);
      }

      void do_disconnect(const std::string& target_ip) {
        std::lock_guard<std::mutex> guard(_tcp_client_pool_mutex);

        auto it = _tcp_client_pool.find(target_ip);

        if (it != _tcp_client_pool.end()) {
          it->second->disconnect();
        }
      }

      void do_refresh(const std::string& target_ip) {
        std::lock_guard<std::mutex> guard(_tcp_client_pool_mutex);

        auto it = _tcp_client_pool.find(target_ip);

        if (it != _tcp_client_pool.end()) {
          it->second->disconnect();
          it->second->connect();
        }
      }

      void do_disconnect_all() {
        std::lock_guard<std::mutex> guard(_tcp_client_pool_mutex);

        typedef typename tcp_client_container::reference reference;
        std::for_each(_tcp_client_pool.begin(), _tcp_client_pool.end(), [this](reference v) {
          v.second->disconnect();
        });
      }

      void do_refresh_all() {
        std::lock_guard<std::mutex> guard(_tcp_client_pool_mutex);

        typedef typename tcp_client_container::reference reference;
        std::for_each(_tcp_client_pool.begin(), _tcp_client_pool.end(), [this](reference v) {
          v.second->disconnect();
          v.second->connect();
        });
      }

    private:

      std::atomic<bool> _stopping;
      std::atomic<bool> _stopped;
      int _thread_num;
      unsigned short _server_port;

      mn::EventLoop* _base_loop;
      std::shared_ptr<mn::EventLoopThreadPool> _io_thread_pool;

      mn::ConnectionCallback _on_connection;
      mn::MessageCallback _on_message;
      mn::WriteCompleteCallback _on_write_complete;

      mutable std::mutex _tcp_client_pool_mutex;
      tcp_client_container _tcp_client_pool;

      mutable std::mutex _destroyed_mutex;
      std::condition_variable _destroyed_cv;
    };

  } // net
} // pioneer



#endif /* NET_POOLS_H_ */
