/*
 * result.h
 *
 *  Created on: Aug 8, 2013
 *      Author: vincent
 */

#ifndef RFC_RESULT_H_
#define RFC_RESULT_H_

#include <string>
#include <memory>

namespace pioneer {
  namespace rfc {

    struct __rfc_result {
      __rfc_result(const std::string& data = "", int ec = 0) : data(data), ec(ec) {}

      __rfc_result(std::string&& data, int ec) : data(data), ec(ec) { }

      __rfc_result(const __rfc_result& other) : data(other.data), ec(other.ec) {}

      std::string data;
      int ec;
    };

    // the result of the function call to the remote side
    // every result brings the result data and an error code, 0 means no error
    // null result means we no response to the remote caller
    class rfc_result {
    public:

      // null result must be the final result
      rfc_result(std::nullptr_t) : _is_final(true) {}

      rfc_result(bool is_final = true, const std::string& data = "", int ec = 0) :
        _is_final(is_final), _impl(new __rfc_result(data, ec)) {
      }

      rfc_result(bool is_final, std::string&& data, int ec = 0) :
        _is_final(is_final), _impl(new __rfc_result(data, ec)) {
      }

      rfc_result(const rfc_result& other) : _is_final(other._is_final) {
        if (other._impl) _impl.reset(new __rfc_result(*other._impl));
      }

      rfc_result(rfc_result&& r) : _is_final(r._is_final), _impl(std::move(r._impl)) { }

      rfc_result& operator=(const rfc_result& other) {
        if (std::addressof(other) == this) return *this;

        _is_final = other._is_final;
        if (other._impl) _impl.reset(new __rfc_result(*other._impl));

        return *this;
      }

      rfc_result& operator=(rfc_result&& other) {
        if (std::addressof(other) == this) return *this;

        _is_final = other._is_final;
        _impl = other._impl;

        return *this;
      }

    public:

      void reset(const std::string& data = "", int ec = 0) {
        _impl.reset(new __rfc_result(data, ec));
      }

      void is_final(bool final) { _is_final = final; }

      bool is_final() { return _is_final; }

      operator bool() const { return _impl.operator bool(); }

    public:

      const std::string& data() const { return _impl->data; }
      int err() const { return _impl->ec; }

    private:

      bool _is_final;
      std::shared_ptr<__rfc_result> _impl;
    };

  } // rfc
} // pioneer

#endif /* RFC_RESULT_H_ */
