/*
 * rfc.h
 *
 *  Created on: Sep 3, 2011
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

#ifndef PIONEER_RFC_H_
#define PIONEER_RFC_H_

#include <string>

#include <boost/lexical_cast.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>

#ifdef DEBUG_RPC

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#else

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#endif

#include <atlas/func_wrapper.h>

#include <pioneer/rfc/message.h>
#include <pioneer/rfc/task.h>

namespace pioneer {
  namespace rfc {

    using std::string;
    using boost::uuids::uuid;
    using boost::uuids::nil_uuid;
    using boost::uuids::random_generator;

#ifdef DEBUG_RPC

    typedef boost::archive::text_iarchive rfc_iarchive;
    typedef boost::archive::text_oarchive rfc_oarchive;

#else

    typedef boost::archive::binary_iarchive rfc_iarchive;
    typedef boost::archive::binary_oarchive rfc_oarchive;

#endif

    enum client_type {
      outward_client = 0x001, inward_client = 0x002, any_client = outward_client | inward_client
    };

    enum return_type {
      rfc_sync, rfc_async_callback, rfc_async_no_callback
    };

#define REGISTER_REMOTE_FUNC(func_name, func_id) namespace fn_ids { \
    static const int func_name = func_id; \
};

    // predefined rfc
    REGISTER_REMOTE_FUNC(resume_thread, -1);
    REGISTER_REMOTE_FUNC(resume_task, -2);

    struct __rfc_context {

      __rfc_context(rfc::client_type ct, const uuid& session_id, const std::string& source_ip_port) :
          client_type(ct), session_id(session_id), source_ip_port(source_ip_port) {
      }

      __rfc_context(const __rfc_context& other)
        : client_type(other.client_type), session_id(other.session_id), source_ip_port(other.source_ip_port) {}

      rfc::client_type client_type;
      uuid session_id;
      std::string source_ip_port;
    };

    class rfc_context {
    public:

      rfc_context(std::nullptr_t) {}

      rfc_context() : _impl(new __rfc_context) {}

      rfc_context(rfc::client_type ct, const uuid& session_id, const std::string& source_ip_port) :
          _impl(new __rfc_context(ct, session_id, source_ip_port)) {
      }

      rfc_context(const rfc_context& other) : _impl(other._impl ? new __rfc_context(*other._impl)  : nullptr) {
      }

      rfc_context(rfc_context&& other) : _impl(std::move(other._impl)) {}

      rfc_context operator=(const rfc_context& other) {
        if (std::addressof(other) == this) return *this;

        if (other._impl) _impl.reset(new __rfc_context(*other._impl));

        return *this;
      }

      void reset(rfc::client_type ct, const uuid& session_id, const std::string& source_ip_port) {
        _impl->client_type = ct;
        _impl->session_id = session_id;
        _impl->source_ip_port = source_ip_port;
      }

      rfc::client_type client_type() const { return _impl->client_type; }

      const uuid& session_id() const { return _impl->session_id; }

      const std::string& source_ip() const { return ip::get_ip_part(_impl->source_ip_port); }

      std::string source_ip_port() const { return _impl->source_ip_port; }

    private:

      std::shared_ptr<__rfc_context> _impl;
    };

    // constant null value
    const rfc_context nilctx(nullptr);

    struct __rfc_result {
      __rfc_result(size_t count = 1, const std::string& data = "", int ec = 0) :
          count(count), data(data), ec(ec) {
      }

      __rfc_result(const __rfc_result& other) : count(other.count), data(other.data), ec(other.ec) {}

      size_t count;
      std::string data; // record set or other result
      int ec;
    };

    class rfc_result {
    public:

      rfc_result(std::nullptr_t) {}

      rfc_result(size_t count = 1, const std::string& data = "", int ec = 0) :
          _impl(new __rfc_result(count, data, ec)) {
      }

      rfc_result(const rfc_result& other) {
        if (other._impl) _impl.reset(new __rfc_result(*other._impl));
      }

      rfc_result(rfc_result&& r) : _impl(std::move(r._impl)) { }

      void reset(size_t count = 1, const std::string& data = "", int ec = 0) {
        _impl.reset(new __rfc_result(count, data, ec));
      }

      operator bool() const { return _impl.operator bool(); }

    public:

      size_t count() const { return _impl->count; }
      const std::string& data() const { return _impl->data; }
      int err() const { return _impl->ec; }

    private:

      std::shared_ptr<__rfc_result> _impl;
    };

    class builtin_rfc {
    public:

      static rfc_result resume_thread(const uuid& sid, const rfc_result& result, rfc_context c) {
        rfc::sync_task_manager::ref().resume(sid, result.count(), result.data(), result.err());

        return nullptr;
      }

      static rfc_result resume_task(const uuid& sid, const rfc_result& result, rfc_context c) {
        rfc::async_task_manager::ref().resume(sid, result.count(), result.data(), result.err());

        return nullptr;
      }
    };

  } // rfc
} // pioneer

namespace boost {
  namespace serialization {

    template<typename Archive>
    void serialize(Archive& ar, pioneer::rfc::rfc_context& c, const unsigned int version) {
      // NOTE : Nothing to do, this is just a placeholder in the serialization system
    }

    template<class Archive>
    void serialize(Archive & ar, pioneer::rfc::rfc_result& r, const unsigned int file_version) {
      split_free(ar, r, file_version);
    }

    template<class Archive>
    void load(Archive& ar, pioneer::rfc::rfc_result& r, const unsigned int) {
      size_t count = 0;
      std::string data;
      int err = 0;

      ar >> count;
      ar >> data;
      ar >> err;

      r.reset(count, data, err);
    }

    template<class Archive>
    void save(Archive& ar, const pioneer::rfc::rfc_result& r, const unsigned int) {
      size_t count = r.count();
      std::string data = r.data();
      int err = r.err();

      ar << count;
      ar << data;
      ar << err;
    }

  } // serialization
} // boost

namespace pioneer {
  namespace rfc {

    class message_builder {
    public:

      message_builder(client_type client) :
          _client_type(client), _return_type(rfc_async_no_callback) {
      }

    public:

      const uuid& session_id() const { return _session_id; }

      void set_return_type(return_type rt) { _return_type = rt; }

      template<typename Functor, typename ... Args>
      std::string build(Functor f, int fn_id, Args&&... args) {
        typedef typename std::result_of<Functor(Args&&...)>::type result_type;

        _session_id = random_generator()();
        rfc::request_header header = rfc::message::make_header(fn_id, _session_id);
        header.client_type = _client_type;
        header.return_type = _return_type;

        std::ostringstream oss;
        std::string header_str(reinterpret_cast<char*>(&header), sizeof(header));

        rfc_oarchive oa(oss);
        atlas::func_wrapper<result_type(Args...)> rfc(f, std::forward<Args>(args)..., oa);

        std::string body = oss.str();
        std::string message(header_str);
        message += body;

        auto h = reinterpret_cast<rfc::request_header*>(const_cast<char*>(message.data()));
        h->length = message.size();

        return std::move(message);
      }

    private:

      client_type _client_type;
      return_type _return_type;
      uuid _session_id;
    };

    // forward declearation
    class rpc_callback_type;

    class remote_caller {
    public:

      remote_caller(client_type client = rfc::inward_client, int resp_expect = 1) :
          _message_builder(client), _response_expected(resp_expect) {
      }

      virtual ~remote_caller() { }

    public:

      /*
       * 1) Serialize a remote function call with it's arguments,
       * 2) send the message to the target using the derived class's implementation
       * */
      template<typename Functor, typename ... Args>
      void call(Functor f, int fn_id, Args ... args) {
        _message_builder.set_return_type(rfc_async_no_callback);

        send(_message_builder.build(f, fn_id, std::forward<Args>(args)...));
      }

      /*
       * 1) serialize a remote function call with arguments,
       * 2) save the context to wait for the result
       * 3) register the callback for the returning
       * 4) send the message to the target using the derived class's implementation
       * 5) the server should issue a resume_task function call and then the client
       *    calls the callback
       * */
      template<typename Functor, typename ... Args>
      void call(Functor f, int fn_id, rpc_callback_type cb, Args ... args) {
        _message_builder.set_return_type(rfc_async_callback);

        std::string message = _message_builder.build(f, fn_id, std::forward<Args>(args)...);
        async_task_manager::ref().suspend(_message_builder.session_id(), cb, _response_expected);

        send(message);
      }

      /*
       * The remote_caller thread will be blocked to wait for the result
       * */
      template<typename Functor, typename ... Args>
      rfc_result sync_call(Functor f, int fn_id, Args ... args) {
        typedef typename std::result_of<Functor(Args...)>::type result_type;
        _message_builder.set_return_type(rfc_sync);

        std::string message = _message_builder.build(f, fn_id, std::forward<Args>(args)...);
        rfc_result rr = sync_task_manager::ref().suspend(_message_builder.session_id());

        send(message);

        return std::move(rr);
      }

    protected:

      virtual void send(const std::string& message) {
        send(message.data(), message.size());
      }

      virtual void send(const char* message, size_t size) = 0;

    private:

      rfc::message_builder _message_builder;
      int _response_expected;
    };

  } // rfc
} // pioneer

#endif /* RFC_H_ */
