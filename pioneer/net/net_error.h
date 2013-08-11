/*
 * net_error.h
 *
 *  Created on: Sep 12, 2011
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

#ifndef PIONEER_NET_ERROR_H_
#define PIONEER_NET_ERROR_H_

#include <string>
#include <system_error>

namespace pioneer {
  namespace net {

    enum class errc {
      success,
      bad_connection,
      bad_query,
      bad_request,
      bad_session,
      duplicated_session,
      connection_time_out,
      unknown
    };

    class net_error_category_impl : public std::error_category {
    public:

      virtual const char* name() const noexcept { return "net"; }

      virtual std::string message(int code) const {
        errc ec = static_cast<errc>(code);

        switch (ec) {
        case errc::success:
          return "The network layer is ok";
        case errc::bad_session:
          return "The session has been crashed";
        case errc::duplicated_session:
          return "The session already exists";
        case errc::bad_request:
          return "Can not validate the request";
        case errc::bad_connection:
          return "The connection has lost";
        case errc::connection_time_out:
          return "The connection is timed out";
        default:
          return "Unknown storage error.";
        }
      }
    };

    // TODO : there might bugs
    // 1. can we write a free function in a .h file?
    // 2. can we use static native variable to make the category is unique?
    //    seams have problems in multi-thread environment
    const std::error_category& net_error_category() throw () {
      static net_error_category_impl instance;
      return instance;
    }

    inline std::error_code make_error_code(errc e) {
      return std::error_code(static_cast<int>(e), net_error_category());
    }

    inline std::error_condition make_error_condition(errc e) {
      return std::error_condition(static_cast<int>(e), net_error_category());
    }

    class net_error: public std::runtime_error {
    public:

      net_error(std::error_code ec = std::error_code()) :
          std::runtime_error(ec.message()), _code(ec) {
      }

      net_error(std::error_code ec, const std::string& what) :
          std::runtime_error(what), _code(ec) {
      }

      virtual ~net_error() noexcept {
      }

      const std::error_code& code() const noexcept {
        return _code;
      }

    private:

      std::error_code _code;
    };
  } // net
} // pioneer

namespace std {
  template<>
  struct is_error_code_enum<pioneer::net::errc> : public true_type {
  };
}

#endif /* NET_ERROR_H_ */
