/*
 * tuple.h
 *
 *  Created on: Apr 8, 2011
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

#ifndef ATLAS_TUPLE_H_
#define ATLAS_TUPLE_H_

#include <tuple>
#include <atlas/type_traits.h>

namespace atlas {
  namespace {

    typedef std::true_type single_parameter_pack_tag;
    typedef std::false_type multiple_parameter_pack_tag;
    typedef std::true_type last_parameter_tag;
    typedef std::false_type not_last_parameter_tag;

    template<typename Archive, typename ... Elements>
    void aux_serialize(Archive& ar, const std::tuple<Elements...>& t, single_parameter_pack_tag) {
      ar & std::get<0>(t);
    }

    template<typename Archive, typename ... Elements>
    void aux_serialize(Archive& ar, const std::tuple<Elements...>& t, multiple_parameter_pack_tag) {
      aux_serialize<0>(ar, t, std::false_type());
    }

    template<size_t idx, typename Archive, typename ... Elements>
    void aux_serialize_2(Archive& ar, const std::tuple<Elements...>& t, not_last_parameter_tag) {
      static_assert(idx < sizeof...(Elements), "Out of range");

      ar & std::get<idx>(t);

      aux_serialize_2<idx + 1>(ar, t, atlas::is_last_parameter<Elements...>::value);
    }

    template<size_t idx, typename Archive, typename ... Elements>
    void aux_serialize_2(Archive& ar, const std::tuple<Elements...>& t, last_parameter_tag) {
      static_assert(idx < sizeof...(Elements), "Out of range");

      ar & std::get<idx>(t);
    }
  }
} //clown

namespace boost {
  namespace serialization {

    template<typename Archive, typename ... Elements>
    Archive& serialize(Archive& ar, const std::tuple<Elements...>& t) {
      atlas::aux_serialize(ar, t, atlas::is_single_parameter_pack<Elements...>::value);
    }

  } // serialization
} // boost

#endif /* TUPLE_H_ */
