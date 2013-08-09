/*
 * func_wrapper.h
 *
 *  Created on: Jan 29, 2013
 *      Author: vincent ivincent.zhang@gmail.com
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

#ifndef ATLAS_FUNC_WRAPPER_H_
#define ATLAS_FUNC_WRAPPER_H_

#include <cstddef> // std::nullptr
#include <memory>
#include <tuple>
#include <functional> // std::function
#include <utility> // std::move
#include <type_traits> // std::decay, and others

#include <atlas/apply_tuple.h>

namespace atlas {

  namespace {
    struct useless {
    };
  }

  template<typename Signature> class func_wrapper;

  template<typename Res, typename ... Args>
  class func_wrapper<Res(Args...)> {
  public:

    typedef Res result_type;

    func_wrapper() = default;

    func_wrapper(std::nullptr_t) {}

    func_wrapper(const func_wrapper& other) : _args(other._args), _f(other._f) { }

    func_wrapper(func_wrapper&& other) : _args(std::move(other._args)), _f(std::move(other._f)) {}

    template<typename OArchive, typename Functor, typename... UArgs>
    func_wrapper(OArchive oa, Functor f, UArgs... args,
        typename std::enable_if<!std::is_integral<Functor>::value, useless>::type = useless())
    : _args(args), _f(f)
    {
      oa << _args;
    }

    template<typename IArchive, typename Functor>
    func_wrapper(IArchive ia, Functor f,
        typename std::enable_if<!std::is_integral<Functor>::value, useless>::type = useless())
    : _f(f)
    {
      ia >> _args;
    }

    /*
     * The last parameter can be replaced by a local variable
     * */
    template<typename Functor, typename IArchiver, typename NativeArg>
    func_wrapper(Functor f, IArchiver& ia, const NativeArg& native_arg,
        typename std::enable_if<!std::is_integral<Functor>::value, useless>::type = useless()) :
        _f(f)
    {
      ia >> _args;

      std::get<sizeof...(Args) - 1>(_args) = native_arg;
    }

    /*
     * The last parameter can be replaced by a local variable
     * */
    template<typename Functor, typename IArchiver, typename NativeArg>
    func_wrapper(Functor f, IArchiver& ia, NativeArg&& native_arg,
        typename std::enable_if<!std::is_integral<Functor>::value, useless>::type = useless()) :
        _f(f)
    {
      ia >> _args;

      std::get<sizeof...(Args) - 1>(_args) = native_arg;
    }

    /**
     *  @brief %func_wrapper assignment operator.
     *  @param other A %func_wrapper with identical call signature.
     *  @post @c (bool)*this == (bool)x
     *  @returns @c *this
     *
     *  The target of @a other is copied to @c *this. If @a other has no
     *  target, then @c *this will be empty.
     *
     *  If @a other targets a func_wrapper pointer or a reference to a func_wrapper
     *  object, then this operation will not throw an %exception.
     */
    func_wrapper& operator=(const func_wrapper& other) {
      func_wrapper(other).swap(*this);
      return *this;
    }

    /**
     *  @brief %func_wrapper move-assignment operator.
     *  @param other A %func_wrapper rvalue with identical call signature.
     *  @returns @c *this
     *
     *  The target of @a other is moved to @c *this. If @a other has no
     *  target, then @c *this will be empty.
     *
     *  If @a other targets a func_wrapper pointer or a reference to a func_wrapper
     *  object, then this operation will not throw an %exception.
     */
    func_wrapper& operator=(func_wrapper&& other) {
      func_wrapper(std::move(other)).swap(*this);
      return *this;
    }

    /**
     *  @brief %func_wrapper assignment to zero.
     *  @returns @c *this
     */
    func_wrapper& operator=(std::nullptr_t) {
      func_wrapper().swap(*this);

      return *this;
    }

    /**
     *  @brief %func_wrapper assignment to a new target.
     *  @param f A %func_wrapper object that is callable with parameters of
     *  type @c T1, @c T2, ..., @c TN and returns a value convertible
     *  to @c Res.
     *  @return @c *this
     *
     *  This  %func_wrapper object wrapper will target a copy of @a
     *  f. If @a f is @c std::reference_wrapper<F>, then this func_wrapper
     *  object will contain a reference to the func_wrapper object @c
     *  f.get(). If @a f is a NULL func_wrapper pointer or NULL
     *  pointer-to-member, @c this object will be empty.
     *
     *  If @a f is a non-NULL func_wrapper pointer or an object of type @c
     *  std::reference_wrapper<F>, this func_wrapper will not throw.
     */
    template<typename Functor>
    typename std::enable_if<!std::is_integral<Functor>::value, func_wrapper&>::type
    operator=(Functor&& f) {
      func_wrapper(std::forward<Functor>(f)).swap(*this);
      return *this;
    }

    template<typename Functor>
    typename std::enable_if<!std::is_integral<Functor>::value, func_wrapper&>::type
    operator=(std::reference_wrapper<Functor> f) {
      func_wrapper(f).swap(*this);
      return *this;
    }

    /**
     *  @brief Swap the targets of two %func_wrapper objects.
     *  @param other A %func_wrapper with identical call signature.
     *
     *  Swap the targets of @c this func_wrapper object and @a f. This
     *  func_wrapper will not throw an %exception.
     */
    void swap(func_wrapper& other) {
      std::swap(_args, other._args);
      std::swap(_f, other._f);
    }

    /**
     *  @brief Determine if the %func_wrapper wrapper has a target.
     *
     *  @return @c true when this %func_wrapper object contains a target,
     *  or @c false when it is empty.
     *
     *  This func_wrapper will not throw an %exception.
     */
    explicit operator bool() const
    { return _f.operator bool();}

    // [3.7.2.4] func_wrapper invocation

    /**
     *  @brief Invokes the func_wrapper targeted by @c *this.
     *  @returns the result of the target.
     *  @throws bad_func_wrapper_call when @c !(bool)*this
     *
     *  The func_wrapper call operator invokes the target func_wrapper object
     *  stored by @c this.
     */
    Res operator()(Args... args) const { return apply_tuple(_f, _args); }

  public:

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
      ar & _args;
    }

  public:

    std::tuple<typename std::decay<Args>::type...> _args;
    std::function<Res(Args...)> _f;
  };

} // atlas

#endif /* ATLAS_FUNC_WRAPPER_H_ */
