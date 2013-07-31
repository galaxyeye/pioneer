/*
 * type_traits.h
 *
 *  Created on: Apr 1, 2013
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

#ifndef ATLAS_TYPE_TRAITS_H_
#define ATLAS_TYPE_TRAITS_H_

#include <type_traits>

namespace atlas {

  namespace {

    template<typename ... Elements>
    struct __is_single_parameter_pack_helper {
      typedef typename std::conditional<1 == sizeof...(Elements), std::true_type, std::false_type>::type type;
    };

    template<size_t idx, typename ... Elements>
    struct __is_last_parameter_helper {
      typedef typename std::conditional<idx + 1 == sizeof...(Elements) - 1, std::true_type, std::false_type>::type type;
    };

  }

  template<typename ... Elements>
  struct is_single_parameter_pack: public std::integral_constant<bool,
      __is_single_parameter_pack_helper<Elements...>::type::value> {
  };

  template<size_t idx, typename ... Elements>
  struct is_last_parameter: public std::integral_constant<bool, __is_last_parameter_helper<Elements...>::type::value> {
  };

  template<typename F, typename ...Args>
  struct is_void_call : public std::is_void<std::result_of<F(Args...)>::type>::type {
  };

} // atlas

#endif /* TYPE_TRAITS_H_ */
