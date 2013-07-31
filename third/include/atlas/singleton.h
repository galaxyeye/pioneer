/*
 * singleton.h
 *
 *  Created on: Apr 8, 2011
 *      Author: Vincent Zhang, ivincent.zhang@gmail.com
 */

/*    Copyright 2011 ~ 2013 vincent.
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

#ifndef ATLAS_SINGLETON_H_
#define ATLAS_SINGLETON_H_

#include <memory>
#include <mutex>

namespace atlas {

  // A thread safe singleton in C++11,
  // the managed object can be constructed with arguments.
  // The managed object can be used by reference or a shared_ptr.

  template<typename T>
  class singleton {
  public:

    ~singleton() = default;

    static T& ref() { return *ptr(); }

    static auto ptr() -> std::shared_ptr<T> {
      std::call_once(_only_one, __init);
      return _value.lock();
    }

  private:

    static void __init() {
      _value = std::make_shared<T>();
    }

  private:

    static std::weak_ptr<T> _value;
    static std::once_flag _only_one;
  };

  template<typename T> std::weak_ptr<T> singleton<T>::_value;
  template<typename T> std::once_flag singleton<T>::_only_one;

} // atlas

#endif /* SINGLETON_H_ */
