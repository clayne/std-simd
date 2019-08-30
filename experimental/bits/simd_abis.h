// Simd Abi specific implementations -*- C++ -*-

// Copyright © 2015-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
//                       Matthias Kretz <m.kretz@gsi.de>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the names of contributing organizations nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _GLIBCXX_EXPERIMENTAL_SIMD_ABIS_H_
#define _GLIBCXX_EXPERIMENTAL_SIMD_ABIS_H_

#if __cplusplus >= 201703L

#include "simd.h"
#include <array>
#include <cmath>
#include <cstdlib>

_GLIBCXX_SIMD_BEGIN_NAMESPACE
// _S_allbits{{{
template <typename _V>
static inline constexpr _V _S_allbits = reinterpret_cast<_V>(
  ~std::conditional_t<
    (sizeof(_V) < sizeof(__intrinsic_type_t<_LLong, 2>)),
    __vector_type_t<int, sizeof(_V) / sizeof(int)>,
    __intrinsic_type_t<_LLong, sizeof(_V) / sizeof(_LLong)>>());

// }}}
// _S_signmask, _S_absmask{{{
template <typename _V, typename = _VectorTraits<_V>>
static inline constexpr _V _S_signmask = __xor(_V() + 1, _V() - 1);
template <typename _V, typename = _VectorTraits<_V>>
static inline constexpr _V _S_absmask = __andnot(_S_signmask<_V>, _S_allbits<_V>);

//}}}
// __simd_tuple_element {{{1
template <size_t _I, typename _Tp> struct __simd_tuple_element;
template <typename _Tp, typename _A0, typename... _As>
struct __simd_tuple_element<0, _SimdTuple<_Tp, _A0, _As...>> {
    using type = std::experimental::simd<_Tp, _A0>;
};
template <size_t _I, typename _Tp, typename _A0, typename... _As>
struct __simd_tuple_element<_I, _SimdTuple<_Tp, _A0, _As...>> {
    using type = typename __simd_tuple_element<_I - 1, _SimdTuple<_Tp, _As...>>::type;
};
template <size_t _I, typename _Tp>
using __simd_tuple_element_t = typename __simd_tuple_element<_I, _Tp>::type;

// __simd_tuple_concat {{{1
template <typename _Tp, typename... _A0s, typename... _A1s>
_GLIBCXX_SIMD_INTRINSIC constexpr _SimdTuple<_Tp, _A0s..., _A1s...>
  __simd_tuple_concat(const _SimdTuple<_Tp, _A0s...>& __left,
		      const _SimdTuple<_Tp, _A1s...>& __right)
{
  if constexpr (sizeof...(_A0s) == 0)
    return __right;
  else if constexpr (sizeof...(_A1s) == 0)
    return __left;
  else
    return {__left.first, __simd_tuple_concat(__left.second, __right)};
}

template <typename _Tp, typename _A10, typename... _A1s>
_GLIBCXX_SIMD_INTRINSIC constexpr _SimdTuple<_Tp,
					       simd_abi::scalar,
					       _A10,
					       _A1s...>
  __simd_tuple_concat(const _Tp&                              __left,
		      const _SimdTuple<_Tp, _A10, _A1s...>& __right)
{
  return {__left, __right};
}

// __simd_tuple_pop_front {{{1
template <size_t _N, typename _Tp>
_GLIBCXX_SIMD_INTRINSIC constexpr decltype(auto)
  __simd_tuple_pop_front(_Tp&& __x)
{
  if constexpr (_N == 0)
    return std::forward<_Tp>(__x);
  else
    return __simd_tuple_pop_front<_N - 1>(__x.second);
}

// __get_simd_at<_N> {{{1
struct __as_simd {};
struct __as_simd_tuple {};
template <typename _Tp, typename _A0, typename... _Abis>
_GLIBCXX_SIMD_INTRINSIC constexpr simd<_Tp, _A0> __simd_tuple_get_impl(
    __as_simd, const _SimdTuple<_Tp, _A0, _Abis...> &__t, _SizeConstant<0>)
{
    return {__private_init, __t.first};
}
template <typename _Tp, typename _A0, typename... _Abis>
_GLIBCXX_SIMD_INTRINSIC constexpr const auto &__simd_tuple_get_impl(
    __as_simd_tuple, const _SimdTuple<_Tp, _A0, _Abis...> &__t, _SizeConstant<0>)
{
    return __t.first;
}
template <typename _Tp, typename _A0, typename... _Abis>
_GLIBCXX_SIMD_INTRINSIC constexpr auto &__simd_tuple_get_impl(
    __as_simd_tuple, _SimdTuple<_Tp, _A0, _Abis...> &__t, _SizeConstant<0>)
{
    return __t.first;
}

template <typename _R, size_t _N, typename _Tp, typename... _Abis>
_GLIBCXX_SIMD_INTRINSIC constexpr auto __simd_tuple_get_impl(
    _R, const _SimdTuple<_Tp, _Abis...> &__t, _SizeConstant<_N>)
{
    return __simd_tuple_get_impl(_R(), __t.second, _SizeConstant<_N - 1>());
}
template <size_t _N, typename _Tp, typename... _Abis>
_GLIBCXX_SIMD_INTRINSIC constexpr auto &__simd_tuple_get_impl(
    __as_simd_tuple, _SimdTuple<_Tp, _Abis...> &__t, _SizeConstant<_N>)
{
    return __simd_tuple_get_impl(__as_simd_tuple(), __t.second, _SizeConstant<_N - 1>());
}

template <size_t _N, typename _Tp, typename... _Abis>
_GLIBCXX_SIMD_INTRINSIC constexpr auto __get_simd_at(const _SimdTuple<_Tp, _Abis...> &__t)
{
    return __simd_tuple_get_impl(__as_simd(), __t, _SizeConstant<_N>());
}

// }}}
// __get_tuple_at<_N> {{{
template <size_t _N, typename _Tp, typename... _Abis>
_GLIBCXX_SIMD_INTRINSIC constexpr auto __get_tuple_at(const _SimdTuple<_Tp, _Abis...> &__t)
{
    return __simd_tuple_get_impl(__as_simd_tuple(), __t, _SizeConstant<_N>());
}

template <size_t _N, typename _Tp, typename... _Abis>
_GLIBCXX_SIMD_INTRINSIC constexpr auto &__get_tuple_at(_SimdTuple<_Tp, _Abis...> &__t)
{
    return __simd_tuple_get_impl(__as_simd_tuple(), __t, _SizeConstant<_N>());
}

// __tuple_element_meta {{{1
template <typename _Tp, typename _Abi, size_t _Offset>
struct __tuple_element_meta : public _Abi::_SimdImpl {
  static_assert(is_same_v<typename _Abi::_SimdImpl::abi_type,
			  _Abi>); // this fails e.g. when _SimdImpl is an alias
				  // for _SimdImplBuiltin<_DifferentAbi>
  using value_type                    = _Tp;
  using abi_type                      = _Abi;
  using _Traits                       = _SimdTraits<_Tp, _Abi>;
  using _MaskImpl                     = typename _Traits::_MaskImpl;
  using _MaskMember                   = typename _Traits::_MaskMember;
  using simd_type                     = std::experimental::simd<_Tp, _Abi>;
  static constexpr size_t    _S_offset = _Offset;
  static constexpr size_t    size() { return simd_size<_Tp, _Abi>::value; }
  static constexpr _MaskImpl _S_mask_impl = {};

  template <size_t _N>
  _GLIBCXX_SIMD_INTRINSIC static _MaskMember __make_mask(std::bitset<_N> __bits)
  {
    constexpr _Tp* __type_tag = nullptr;
    return _MaskImpl::__from_bitset(
      std::bitset<size()>((__bits >> _Offset).to_ullong()), __type_tag);
  }

  _GLIBCXX_SIMD_INTRINSIC static _ULLong __mask_to_shifted_ullong(_MaskMember __k)
  {
    return __vector_to_bitset(__k).to_ullong() << _Offset;
  }
};

template <size_t _Offset, typename _Tp, typename _Abi, typename... _As>
__tuple_element_meta<_Tp, _Abi, _Offset> __make_meta(const _SimdTuple<_Tp, _Abi, _As...> &)
{
    return {};
}

// }}}1
// _WithOffset wrapper class {{{
template <size_t _Offset, typename _Base>
struct _WithOffset : public _Base
{
  static inline constexpr size_t _S_offset = _Offset;

  _GLIBCXX_SIMD_INTRINSIC char* __as_charptr()
  {
    return reinterpret_cast<char*>(this) +
	   _S_offset * sizeof(typename _Base::value_type);
  }
  _GLIBCXX_SIMD_INTRINSIC const char* __as_charptr() const
  {
    return reinterpret_cast<const char*>(this) +
	   _S_offset * sizeof(typename _Base::value_type);
  }
};

// make _WithOffset<_WithOffset> ill-formed to use:
template <size_t _O0, size_t _O1, typename _Base>
struct _WithOffset<_O0, _WithOffset<_O1, _Base>> {};

template <size_t _Offset, typename _Tp>
decltype(auto) __add_offset(_Tp& __base)
{
  return static_cast<_WithOffset<_Offset, __remove_cvref_t<_Tp>>&>(__base);
}
template <size_t _Offset, typename _Tp>
decltype(auto) __add_offset(const _Tp& __base)
{
  return static_cast<const _WithOffset<_Offset, __remove_cvref_t<_Tp>>&>(
    __base);
}
template <size_t _Offset, size_t _ExistingOffset, typename _Tp>
decltype(auto) __add_offset(_WithOffset<_ExistingOffset, _Tp>& __base)
{
  return static_cast<_WithOffset<_Offset + _ExistingOffset, _Tp>&>(
    static_cast<_Tp&>(__base));
}
template <size_t _Offset, size_t _ExistingOffset, typename _Tp>
decltype(auto) __add_offset(const _WithOffset<_ExistingOffset, _Tp>& __base)
{
  return static_cast<const _WithOffset<_Offset + _ExistingOffset, _Tp>&>(
    static_cast<const _Tp&>(__base));
}

template <typename _Tp>
constexpr inline size_t __offset = 0;
template <size_t _Offset, typename _Tp>
constexpr inline size_t __offset<_WithOffset<_Offset, _Tp>> =
  _WithOffset<_Offset, _Tp>::_S_offset;
template <typename _Tp>
constexpr inline size_t __offset<const _Tp> = __offset<_Tp>;
template <typename _Tp>
constexpr inline size_t __offset<_Tp&> = __offset<_Tp>;
template <typename _Tp>
constexpr inline size_t __offset<_Tp&&> = __offset<_Tp>;

// }}}
// _SimdTuple specializations {{{1
// empty {{{2
template <typename _Tp> struct _SimdTuple<_Tp> {
    using value_type = _Tp;
    static constexpr size_t _S_tuple_size = 0;
    static constexpr size_t size() { return 0; }
};

// _SimdTupleData {{{2
template <typename _FirstType, typename _SecondType>
struct _SimdTupleData
{
  _FirstType  first;
  _SecondType second;
};

template <typename _FirstType, typename _Tp>
struct _SimdTupleData<_FirstType, _SimdTuple<_Tp>>
{
  _FirstType  first;
  static constexpr _SimdTuple<_Tp> second = {};
};

// 1 or more {{{2
template <class _Tp, class _Abi0, class... _Abis>
struct _SimdTuple<_Tp, _Abi0, _Abis...>
: _SimdTupleData<typename _SimdTraits<_Tp, _Abi0>::_SimdMember,
		 _SimdTuple<_Tp, _Abis...>>
{
  using value_type  = _Tp;
  using _FirstType  = typename _SimdTraits<_Tp, _Abi0>::_SimdMember;
  using _FirstAbi   = _Abi0;
  using _SecondType = _SimdTuple<_Tp, _Abis...>;
  static constexpr size_t _S_tuple_size = sizeof...(_Abis) + 1;
  static constexpr size_t size()
  {
    return simd_size_v<_Tp, _Abi0> + _SecondType::size();
  }
  static constexpr size_t _S_first_size = simd_size_v<_Tp, _Abi0>;

  using _Base = _SimdTupleData<typename _SimdTraits<_Tp, _Abi0>::_SimdMember,
			       _SimdTuple<_Tp, _Abis...>>;
  using _Base::first;
  using _Base::second;

  _SimdTuple() = default;

  _GLIBCXX_SIMD_INTRINSIC char* __as_charptr()
  {
    return reinterpret_cast<char*>(this);
  }
  _GLIBCXX_SIMD_INTRINSIC const char* __as_charptr() const
  {
    return reinterpret_cast<const char*>(this);
  }

  template <size_t _Offset = 0, class _F>
  _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdTuple
    __generate(_F&& __gen, _SizeConstant<_Offset> = {})
  {
    auto &&__first = __gen(__tuple_element_meta<_Tp, _Abi0, _Offset>());
    if constexpr (_S_tuple_size == 1)
      return {__first};
    else
      return {__first, _SecondType::__generate(
			 std::forward<_F>(__gen),
			 _SizeConstant<_Offset + simd_size_v<_Tp, _Abi0>>())};
  }

  template <size_t _Offset = 0, class _F, class... _More>
  _GLIBCXX_SIMD_INTRINSIC _SimdTuple
			  __apply_wrapped(_F&& __fun, const _More&... __more) const
  {
    auto&& __first= __fun(__make_meta<_Offset>(*this), first, __more.first...);
    if constexpr (_S_tuple_size == 1)
      return { __first };
    else
      return {
	__first,
	second.template __apply_wrapped<_Offset + simd_size_v<_Tp, _Abi0>>(
	  std::forward<_F>(__fun), __more.second...)};
  }

  template <size_t _Size,
	    size_t _Offset = 0,
	    typename _R    = __fixed_size_storage_t<_Tp, _Size>>
  _GLIBCXX_SIMD_INTRINSIC constexpr _R __extract_tuple_with_size() const
  {
    if constexpr (_Size == _S_first_size && _Offset == 0)
      return {first};
    else if constexpr (_Size > _S_first_size && _Offset == 0 && _S_tuple_size > 1)
      return {
	first,
	second.template __extract_tuple_with_size<_Size - _S_first_size>()};
    else if constexpr (_Size == 1)
      return {operator[](_SizeConstant<_Offset>())};
    else if constexpr (_R::_S_tuple_size == 1)
      {
	static_assert(_Offset % _Size == 0);
	static_assert(_S_first_size % _Size == 0);
	return {typename _R::_FirstType(
	  __private_init,
	  __extract_part<_Offset / _Size, _S_first_size / _Size>(first))};
      }
    else
      __assert_unreachable<_SizeConstant<_Size>>();
  }

  template <typename _Tup>
  _GLIBCXX_SIMD_INTRINSIC constexpr decltype(auto)
    __extract_argument(_Tup&& __tup) const
  {
    using _TupT = typename __remove_cvref_t<_Tup>::value_type;
    if constexpr (is_same_v<_SimdTuple, __remove_cvref_t<_Tup>>)
      return __tup.first;
    else if (__builtin_is_constant_evaluated())
      return __fixed_size_storage_t<_TupT, _S_first_size>::__generate([&](
	auto __meta) constexpr {
	return __meta.__generator(
	  [&](auto __i) constexpr { return __tup[__i]; },
	  static_cast<_TupT*>(nullptr));
      });
    else
      return [&]() {
	__fixed_size_storage_t<_TupT, _S_first_size> __r;
	__builtin_memcpy(__r.__as_charptr(), __tup.__as_charptr(), sizeof(__r));
	return __r;
      }();
  }

  template <typename _Tup>
  _GLIBCXX_SIMD_INTRINSIC constexpr auto& __skip_argument(_Tup&& __tup) const
  {
    static_assert(_S_tuple_size > 1);
    using _U               = __remove_cvref_t<_Tup>;
    constexpr size_t __off = __offset<_U>;
    if constexpr (_S_first_size == _U::_S_first_size && __off == 0)
      return __tup.second;
    else if constexpr (_S_first_size > _U::_S_first_size &&
		       _S_first_size % _U::_S_first_size == 0 && __off == 0)
      return __simd_tuple_pop_front<_S_first_size / _U::_S_first_size>(__tup);
    else if constexpr (_S_first_size + __off < _U::_S_first_size)
      return __add_offset<_S_first_size>(__tup);
    else if constexpr (_S_first_size + __off == _U::_S_first_size)
      return __tup.second;
    else
      __assert_unreachable<_Tup>();
  }

  template <size_t _Offset, typename... _More>
  _GLIBCXX_SIMD_INTRINSIC constexpr void
    __assign_front(const _SimdTuple<_Tp, _Abi0, _More...>& __x) &
  {
    static_assert(_Offset == 0);
    first = __x.first;
    if constexpr (sizeof...(_More) > 0)
      {
	static_assert(sizeof...(_Abis) >= sizeof...(_More));
	second.template __assign_front<0>(__x.second);
      }
  }

  template <size_t _Offset>
  _GLIBCXX_SIMD_INTRINSIC constexpr void
    __assign_front(const _FirstType& __x) &
  {
    static_assert(_Offset == 0);
    first = __x;
  }

  template <size_t _Offset, typename... _As>
  _GLIBCXX_SIMD_INTRINSIC constexpr void
    __assign_front(const _SimdTuple<_Tp, _As...>& __x) &
  {
    __builtin_memcpy(__as_charptr() + _Offset * sizeof(value_type),
		     __x.__as_charptr(),
		     sizeof(_Tp) * _SimdTuple<_Tp, _As...>::size());
  }

  /*
   * Iterate over the first objects in this _SimdTuple and call __fun for each
   * of them. If additional arguments are passed via __more, chunk them into
   * _SimdTuple or __vector_type_t objects of the same number of values.
   */
  template <class _F, class... _More>
  _GLIBCXX_SIMD_INTRINSIC constexpr _SimdTuple
    __apply_per_chunk(_F&& __fun, _More&&... __more) const
  {
    if constexpr ((... || conjunction_v<
			    is_lvalue_reference<_More>,
			    negation<is_const<remove_reference_t<_More>>>>))
      {
	// need to write back at least one of __more after calling __fun
	auto&& __first = [&](auto... __args) constexpr {
	  auto __r =
	    __fun(__tuple_element_meta<_Tp, _Abi0, 0>(), first, __args...);
	  [[maybe_unused]] auto&& __ignore_me = {(
	    [](auto&& __dst, const auto& __src) {
	      if constexpr (is_assignable_v<decltype(__dst), decltype(__dst)>)
		{
		  __dst.template __assign_front<__offset<decltype(__dst)>>(
		    __src);
		}
	    }(std::forward<_More>(__more), __args),
	    0)...};
	  return __r;
	}(__extract_argument(__more)...);
	if constexpr (_S_tuple_size == 1)
	  return { __first };
	else
	  return {__first,
		  second.__apply_per_chunk(std::forward<_F>(__fun),
					   __skip_argument(__more)...)};
      }
    else
      {
	auto&& __first = __fun(__tuple_element_meta<_Tp, _Abi0, 0>(), first,
			       __extract_argument(__more)...);
	if constexpr (_S_tuple_size == 1)
	  return { __first };
	else
	  return {__first,
		  second.__apply_per_chunk(std::forward<_F>(__fun),
					   __skip_argument(__more)...)};
      }
  }

  template <typename _R = _Tp, typename _F, typename... _More>
  _GLIBCXX_SIMD_INTRINSIC auto
    __apply_r(_F&& __fun, const _More&... __more) const
  {
    auto&& __first =
      __fun(__tuple_element_meta<_Tp, _Abi0, 0>(), first, __more.first...);
    if constexpr (_S_tuple_size == 1)
      return __first;
    else
      return __simd_tuple_concat<_R>(
	__first, second.template __apply_r<_R>(std::forward<_F>(__fun),
					       __more.second...));
  }

  template <typename _F, typename... _More>
  _GLIBCXX_SIMD_INTRINSIC friend std::bitset<size()>
    __test(_F&& __fun, const _SimdTuple& __x, const _More&... __more)
  {
    const auto __first = __vector_to_bitset(
      __fun(__tuple_element_meta<_Tp, _Abi0, 0>(), __x.first, __more.first...));
    if constexpr (_S_tuple_size == 1)
      {
	static_assert(__first.size() >= size());
	return __first.to_ullong();
      }
    else
      return __first.to_ullong() |
	     (__test(__fun, __x.second, __more.second...).to_ullong()
	      << simd_size_v<_Tp, _Abi0>);
  }

  template <typename _U, _U _I>
  _GLIBCXX_SIMD_INTRINSIC constexpr _Tp
    operator[](std::integral_constant<_U, _I>) const noexcept
  {
    if constexpr (_I < simd_size_v<_Tp, _Abi0>)
      return __subscript_read(_I);
    else
      return second[std::integral_constant<_U, _I - simd_size_v<_Tp, _Abi0>>()];
  }

  _Tp operator[](size_t __i) const noexcept
  {
    if constexpr (_S_tuple_size == 1)
      return __subscript_read(__i);
    else
      {
#ifdef _GLIBCXX_SIMD_USE_ALIASING_LOADS
	return reinterpret_cast<const __may_alias<_Tp>*>(this)[__i];
#else
	if constexpr (__is_abi<_Abi0, simd_abi::scalar>())
	  {
	    const _Tp* ptr = &first;
	    return ptr[__i];
	  }
	else
	  return __i < simd_size_v<_Tp, _Abi0>
		   ? __subscript_read(__i)
		   : second[__i - simd_size_v<_Tp, _Abi0>];
#endif
      }
  }

  void __set(size_t __i, _Tp __val) noexcept
  {
    if constexpr (_S_tuple_size == 1)
      return __subscript_write(__i, __val);
    else
      {
#ifdef _GLIBCXX_SIMD_USE_ALIASING_LOADS
	reinterpret_cast<__may_alias<_Tp>*>(this)[__i] = __val;
#else
	if (__i < simd_size_v<_Tp, _Abi0>)
	  __subscript_write(__i, __val);
	else
	  second.__set(__i - simd_size_v<_Tp, _Abi0>, __val);
#endif
      }
  }

private:
  // __subscript_read/_write {{{
  _Tp __subscript_read([[maybe_unused]] size_t __i) const noexcept
  {
    if constexpr (__is_vectorizable_v<_FirstType>)
      return first;
    else
      return first[__i];
  }

  void __subscript_write([[maybe_unused]] size_t __i, _Tp __y) noexcept
  {
    if constexpr (__is_vectorizable_v<_FirstType>)
      first = __y;
    else
      first.__set(__i, __y);
  }

  // }}}
};

// __make_simd_tuple {{{1
template <typename _Tp, typename _A0>
_GLIBCXX_SIMD_INTRINSIC _SimdTuple<_Tp, _A0> __make_simd_tuple(
    std::experimental::simd<_Tp, _A0> __x0)
{
    return {__data(__x0)};
}
template <typename _Tp, typename _A0, typename... _As>
_GLIBCXX_SIMD_INTRINSIC _SimdTuple<_Tp, _A0, _As...> __make_simd_tuple(
    const std::experimental::simd<_Tp, _A0> &__x0,
    const std::experimental::simd<_Tp, _As> &... __xs)
{
    return {__data(__x0), __make_simd_tuple(__xs...)};
}

template <typename _Tp, typename _A0>
_GLIBCXX_SIMD_INTRINSIC _SimdTuple<_Tp, _A0> __make_simd_tuple(
    const typename _SimdTraits<_Tp, _A0>::_SimdMember &__arg0)
{
    return {__arg0};
}

template <typename _Tp, typename _A0, typename _A1, typename... _Abis>
_GLIBCXX_SIMD_INTRINSIC _SimdTuple<_Tp, _A0, _A1, _Abis...> __make_simd_tuple(
    const typename _SimdTraits<_Tp, _A0>::_SimdMember &__arg0,
    const typename _SimdTraits<_Tp, _A1>::_SimdMember &__arg1,
    const typename _SimdTraits<_Tp, _Abis>::_SimdMember &... __args)
{
    return {__arg0, __make_simd_tuple<_Tp, _A1, _Abis...>(__arg1, __args...)};
}

// __to_simd_tuple {{{1
template <typename _Tp,
	  size_t _N,
	  typename _V,
	  size_t _NV,
	  typename... _VX>
_GLIBCXX_SIMD_INTRINSIC constexpr __fixed_size_storage_t<_Tp, _N>
  __to_simd_tuple(const std::array<_V, _NV>& __from, const _VX... __fromX);

template <typename _Tp,
	  size_t _N,
	  size_t _Offset = 0, // skip this many elements in __from0
	  typename _R    = __fixed_size_storage_t<_Tp, _N>,
	  typename _V0,
	  typename _V0VT = _VectorTraits<_V0>,
	  typename... _VX>
_GLIBCXX_SIMD_INTRINSIC
  _R constexpr __to_simd_tuple(const _V0 __from0, const _VX... __fromX)
{
  static_assert(std::is_same_v<typename _V0VT::value_type, _Tp>);
  static_assert(_Offset < _V0VT::_S_width);
  using _R0 = __vector_type_t<_Tp, _R::_S_first_size>;
  if constexpr (_R::_S_tuple_size == 1)
    {
      if constexpr (_N == 1)
	return _R{__from0[_Offset]};
      else if constexpr (_Offset == 0 && _V0VT::_S_width >= _N)
	return _R{__intrin_bitcast<_R0>(__from0)};
      else if constexpr (_Offset * 2 == _V0VT::_S_width &&
			 _V0VT::_S_width / 2 >= _N)
	return _R{__intrin_bitcast<_R0>(__extract_part<1, 2>(__from0))};
      else if constexpr (_Offset * 4 == _V0VT::_S_width &&
			 _V0VT::_S_width / 4 >= _N)
	return _R{__intrin_bitcast<_R0>(__extract_part<1, 4>(__from0))};
      else
	__assert_unreachable<_Tp>();
    }
  else
    {
      if constexpr (1 == _R::_S_first_size)
	{ // extract one scalar and recurse
	  if constexpr (_Offset + 1 < _V0VT::_S_width)
	    return _R{
	      __from0[_Offset],
	      __to_simd_tuple<_Tp, _N - 1, _Offset + 1>(__from0, __fromX...)};
	  else
	    return _R{__from0[_Offset],
		      __to_simd_tuple<_Tp, _N - 1, 0>(__fromX...)};
	}

      // place __from0 into _R::first and recurse for __fromX -> _R::second
      else if constexpr (_V0VT::_S_width == _R::_S_first_size && _Offset == 0)
	return _R{__from0,
		  __to_simd_tuple<_Tp, _N - _R::_S_first_size>(__fromX...)};

      // place lower part of __from0 into _R::first and recurse with _Offset
      else if constexpr (_V0VT::_S_width > _R::_S_first_size && _Offset == 0)
	return _R{
	  __intrin_bitcast<_R0>(__from0),
	  __to_simd_tuple<_Tp, _N - _R::_S_first_size, _R::_S_first_size>(
	    __from0, __fromX...)};

      // place lower part of second quarter of __from0 into _R::first and recurse
      // with _Offset
      else if constexpr (_Offset * 4 == _V0VT::_S_width &&
			 _V0VT::_S_width >= 4 * _R::_S_first_size)
	return _R{
	  __intrin_bitcast<_R0>(__extract_part<2, 4>(__from0)),
	  __to_simd_tuple<_Tp, _N - _R::_S_first_size,
			  _Offset + _R::_S_first_size>(__from0, __fromX...)};

      // place lower half of high half of __from0 into _R::first and recurse
      // with _Offset
      else if constexpr (_Offset * 2 == _V0VT::_S_width &&
			 _V0VT::_S_width >= 4 * _R::_S_first_size)
	return _R{__intrin_bitcast<_R0>(__extract_part<2, 4>(__from0)),
	  __to_simd_tuple<_Tp, _N - _R::_S_first_size,
			  _Offset + _R::_S_first_size>(__from0, __fromX...)};

      // place high half of __from0 into _R::first and recurse with __fromX
      else if constexpr (_Offset * 2 == _V0VT::_S_width &&
			 _V0VT::_S_width / 2 >= _R::_S_first_size)
	return _R{__intrin_bitcast<_R0>(__extract_part<1, 2>(__from0)),
		  __to_simd_tuple<_Tp, _N - _R::_S_first_size, 0>(__fromX...)};

      // ill-formed if some unforseen pattern is needed
      else
	__assert_unreachable<_Tp>();
    }
}

template <typename _Tp,
	  size_t _N,
	  typename _V,
	  size_t _NV,
	  typename... _VX>
_GLIBCXX_SIMD_INTRINSIC constexpr __fixed_size_storage_t<_Tp, _N>
  __to_simd_tuple(const std::array<_V, _NV>& __from, const _VX... __fromX)
{
  if constexpr (std::is_same_v<_Tp, _V>)
    {
      static_assert(
	sizeof...(_VX) == 0,
	"An array of scalars must be the last argument to __to_simd_tuple");
      return __call_with_subscripts(
	__from,
	std::make_index_sequence<_NV>(), [&](const auto... __args) constexpr {
	  return __simd_tuple_concat(
	    _SimdTuple<_Tp, simd_abi::scalar>{__args}..., _SimdTuple<_Tp>());
	});
    }
  else
  return __call_with_subscripts(
    __from,
    std::make_index_sequence<_NV>(), [&](const auto... __args) constexpr {
      return __to_simd_tuple<_Tp, _N>(__args..., __fromX...);
    });
}

template <size_t, typename _Tp> using __to_tuple_helper = _Tp;
template <typename _Tp,
	  typename _A0,
	  size_t _NOut,
	  size_t _N,
	  size_t... _Indexes>
_GLIBCXX_SIMD_INTRINSIC __fixed_size_storage_t<_Tp, _NOut> __to_simd_tuple_impl(
  std::index_sequence<_Indexes...>,
  const std::array<__vector_type_t<_Tp, simd_size_v<_Tp, _A0>>, _N>& __args)
{
  return __make_simd_tuple<_Tp, __to_tuple_helper<_Indexes, _A0>...>(__args[_Indexes]...);
}

template <typename _Tp,
	  typename _A0,
	  size_t _NOut,
	  size_t _N,
	  typename _R = __fixed_size_storage_t<_Tp, _NOut>>
_GLIBCXX_SIMD_INTRINSIC _R __to_simd_tuple_sized(
  const std::array<__vector_type_t<_Tp, simd_size_v<_Tp, _A0>>, _N>& __args)
{
  static_assert(_N * simd_size_v<_Tp, _A0> >= _NOut);
  return __to_simd_tuple_impl<_Tp, _A0, _NOut>(
    std::make_index_sequence<_R::_S_tuple_size>(), __args);
}

template <typename _Tp, typename _A0, size_t _N>
[[deprecated]] _GLIBCXX_SIMD_INTRINSIC auto __to_simd_tuple(
  const std::array<__vector_type_t<_Tp, simd_size_v<_Tp, _A0>>, _N>& __args)
{
  return __to_simd_tuple<_Tp, _N * simd_size_v<_Tp, _A0>>(__args);
}

// __optimize_simd_tuple {{{1
template <class _Tp> _GLIBCXX_SIMD_INTRINSIC _SimdTuple<_Tp> __optimize_simd_tuple(const _SimdTuple<_Tp>)
{
    return {};
}

template <class _Tp, class _A>
_GLIBCXX_SIMD_INTRINSIC const _SimdTuple<_Tp, _A> &__optimize_simd_tuple(const _SimdTuple<_Tp, _A> &__x)
{
    return __x;
}

template <class _Tp, class _A0, class _A1, class... _Abis,
          class _R = __fixed_size_storage_t<_Tp, _SimdTuple<_Tp, _A0, _A1, _Abis...>::size()>>
_GLIBCXX_SIMD_INTRINSIC _R __optimize_simd_tuple(const _SimdTuple<_Tp, _A0, _A1, _Abis...> &__x)
{
    using _Tup = _SimdTuple<_Tp, _A0, _A1, _Abis...>;
    if constexpr (std::is_same_v<_R, _Tup>)
      {
	return __x;
      }
    else if constexpr (_R::_S_first_size == simd_size_v<_Tp, _A0>)
      {
	return __simd_tuple_concat(_SimdTuple<_Tp, typename _R::_FirstAbi>{__x.first},
                            __optimize_simd_tuple(__x.second));
      }
    else if constexpr (_R::_S_first_size ==
		       simd_size_v<_Tp, _A0> + simd_size_v<_Tp, _A1>)
      {
	return __simd_tuple_concat(_SimdTuple<_Tp, typename _R::_FirstAbi>{__data(
                                std::experimental::concat(__get_simd_at<0>(__x), __get_simd_at<1>(__x)))},
                            __optimize_simd_tuple(__x.second.second));
      }
    else if constexpr (_R::_S_first_size ==
		       4 * __simd_tuple_element_t<0, _Tup>::size())
      {
	return __simd_tuple_concat(
	  _SimdTuple<_Tp, typename _R::_FirstAbi>{
	    __data(concat(__get_simd_at<0>(__x), __get_simd_at<1>(__x),
			  __get_simd_at<2>(__x), __get_simd_at<3>(__x)))},
	  __optimize_simd_tuple(__x.second.second.second.second));
      }
    else if constexpr (_R::_S_first_size ==
		       8 * __simd_tuple_element_t<0, _Tup>::size())
      {
	return __simd_tuple_concat(
	  _SimdTuple<_Tp, typename _R::_FirstAbi>{__data(concat(
	    __get_simd_at<0>(__x), __get_simd_at<1>(__x), __get_simd_at<2>(__x),
	    __get_simd_at<3>(__x), __get_simd_at<4>(__x), __get_simd_at<5>(__x),
	    __get_simd_at<6>(__x), __get_simd_at<7>(__x)))},
	  __optimize_simd_tuple(
	    __x.second.second.second.second.second.second.second.second));
      }
    else if constexpr (_R::_S_first_size ==
		       16 * __simd_tuple_element_t<0, _Tup>::size())
      {
	return __simd_tuple_concat(
	  _SimdTuple<_Tp, typename _R::_FirstAbi>{__data(concat(
	    __get_simd_at<0>(__x), __get_simd_at<1>(__x), __get_simd_at<2>(__x),
	    __get_simd_at<3>(__x), __get_simd_at<4>(__x), __get_simd_at<5>(__x),
	    __get_simd_at<6>(__x), __get_simd_at<7>(__x), __get_simd_at<8>(__x),
	    __get_simd_at<9>(__x), __get_simd_at<10>(__x),
	    __get_simd_at<11>(__x), __get_simd_at<12>(__x),
	    __get_simd_at<13>(__x), __get_simd_at<14>(__x),
	    __get_simd_at<15>(__x)))},
	  __optimize_simd_tuple(
	    __x.second.second.second.second.second.second.second.second.second
	      .second.second.second.second.second.second.second));
      }
    else
      {
	return __x;
      }
}

// __for_each(const _SimdTuple &, Fun) {{{1
template <size_t _Offset = 0, class _Tp, class _A0, class _F>
_GLIBCXX_SIMD_INTRINSIC constexpr void
  __for_each(const _SimdTuple<_Tp, _A0>& __t, _F&& __fun)
{
  std::forward<_F>(__fun)(__make_meta<_Offset>(__t), __t.first);
}
template <size_t _Offset = 0,
	  class _Tp,
	  class _A0,
	  class _A1,
	  class... _As,
	  class _F>
_GLIBCXX_SIMD_INTRINSIC constexpr void
  __for_each(const _SimdTuple<_Tp, _A0, _A1, _As...>& __t, _F&& __fun)
{
  __fun(__make_meta<_Offset>(__t), __t.first);
  __for_each<_Offset + simd_size<_Tp, _A0>::value>(__t.second,
						   std::forward<_F>(__fun));
}

// __for_each(_SimdTuple &, Fun) {{{1
template <size_t _Offset = 0, class _Tp, class _A0, class _F>
_GLIBCXX_SIMD_INTRINSIC constexpr void
  __for_each(_SimdTuple<_Tp, _A0>& __t, _F&& __fun)
{
  std::forward<_F>(__fun)(__make_meta<_Offset>(__t), __t.first);
}
template <size_t _Offset = 0,
	  class _Tp,
	  class _A0,
	  class _A1,
	  class... _As,
	  class _F>
_GLIBCXX_SIMD_INTRINSIC constexpr void
  __for_each(_SimdTuple<_Tp, _A0, _A1, _As...>& __t, _F&& __fun)
{
  __fun(__make_meta<_Offset>(__t), __t.first);
  __for_each<_Offset + simd_size<_Tp, _A0>::value>(__t.second,
						   std::forward<_F>(__fun));
}

// __for_each(_SimdTuple &, const _SimdTuple &, Fun) {{{1
template <size_t _Offset = 0, class _Tp, class _A0, class _F>
_GLIBCXX_SIMD_INTRINSIC constexpr void
  __for_each(_SimdTuple<_Tp, _A0>&       __a,
	     const _SimdTuple<_Tp, _A0>& __b,
	     _F&&                          __fun)
{
  std::forward<_F>(__fun)(__make_meta<_Offset>(__a), __a.first, __b.first);
}
template <size_t _Offset = 0,
	  class _Tp,
	  class _A0,
	  class _A1,
	  class... _As,
	  class _F>
_GLIBCXX_SIMD_INTRINSIC constexpr void
  __for_each(_SimdTuple<_Tp, _A0, _A1, _As...>&       __a,
	     const _SimdTuple<_Tp, _A0, _A1, _As...>& __b,
	     _F&&                                      __fun)
{
  __fun(__make_meta<_Offset>(__a), __a.first, __b.first);
  __for_each<_Offset + simd_size<_Tp, _A0>::value>(__a.second, __b.second,
						   std::forward<_F>(__fun));
}

// __for_each(const _SimdTuple &, const _SimdTuple &, Fun) {{{1
template <size_t _Offset = 0, class _Tp, class _A0, class _F>
_GLIBCXX_SIMD_INTRINSIC constexpr void
  __for_each(const _SimdTuple<_Tp, _A0>& __a,
	     const _SimdTuple<_Tp, _A0>& __b,
	     _F&&                          __fun)
{
  std::forward<_F>(__fun)(__make_meta<_Offset>(__a), __a.first, __b.first);
}
template <size_t _Offset = 0,
	  class _Tp,
	  class _A0,
	  class _A1,
	  class... _As,
	  class _F>
_GLIBCXX_SIMD_INTRINSIC constexpr void
  __for_each(const _SimdTuple<_Tp, _A0, _A1, _As...>& __a,
	     const _SimdTuple<_Tp, _A0, _A1, _As...>& __b,
	     _F&&                                      __fun)
{
  __fun(__make_meta<_Offset>(__a), __a.first, __b.first);
  __for_each<_Offset + simd_size<_Tp, _A0>::value>(__a.second, __b.second,
						   std::forward<_F>(__fun));
}

// }}}1
// __cmpord{{{
template <class _Tp, class _TVT = _VectorTraits<_Tp>>
_GLIBCXX_SIMD_INTRINSIC auto __cmpord(_Tp __x, _Tp __y)
{
  static_assert(is_floating_point_v<typename _TVT::value_type>);
#if _GLIBCXX_SIMD_X86INTRIN // {{{
  if constexpr (__have_sse && _TVT::template __is<float, 4>)
    return __intrin_bitcast<_Tp>(_mm_cmpord_ps(__x, __y));
  else if constexpr (__have_sse2 && _TVT::template __is<double, 2>)
    return __intrin_bitcast<_Tp>(_mm_cmpord_pd(__x, __y));
  else if constexpr (__have_avx && _TVT::template __is<float, 8>)
    return __intrin_bitcast<_Tp>(_mm256_cmp_ps(__x, __y, _CMP_ORD_Q));
  else if constexpr (__have_avx && _TVT::template __is<double, 4>)
    return __intrin_bitcast<_Tp>(_mm256_cmp_pd(__x, __y, _CMP_ORD_Q));
  else if constexpr (__have_avx512f && _TVT::template __is<float, 16>)
    return _mm512_cmp_ps_mask(__x, __y, _CMP_ORD_Q);
  else if constexpr (__have_avx512f && _TVT::template __is<double, 8>)
    return _mm512_cmp_pd_mask(__x, __y, _CMP_ORD_Q);
  else
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
    {
      return reinterpret_cast<_Tp>((__x < __y) != (__x >= __y));
    }
}

// }}}
// __cmpunord{{{
template <class _Tp, class _TVT = _VectorTraits<_Tp>>
_GLIBCXX_SIMD_INTRINSIC auto __cmpunord(_Tp __x, _Tp __y)
{
  static_assert(is_floating_point_v<typename _TVT::value_type>);
#if _GLIBCXX_SIMD_X86INTRIN // {{{
  if constexpr (__have_sse && _TVT::template __is<float, 4>)
    return __intrin_bitcast<_Tp>(_mm_cmpunord_ps(__x, __y));
  else if constexpr (__have_sse2 && _TVT::template __is<double, 2>)
    return __intrin_bitcast<_Tp>(_mm_cmpunord_pd(__x, __y));
  else if constexpr (__have_avx && _TVT::template __is<float, 8>)
    return __intrin_bitcast<_Tp>(_mm256_cmp_ps(__x, __y, _CMP_UNORD_Q));
  else if constexpr (__have_avx && _TVT::template __is<double, 4>)
    return __intrin_bitcast<_Tp>(_mm256_cmp_pd(__x, __y, _CMP_UNORD_Q));
  else if constexpr (__have_avx512f && _TVT::template __is<float, 16>)
    return _mm512_cmp_ps_mask(__x, __y, _CMP_UNORD_Q);
  else if constexpr (__have_avx512f && _TVT::template __is<double, 8>)
    return _mm512_cmp_pd_mask(__x, __y, _CMP_UNORD_Q);
  else
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
    {
      return reinterpret_cast<_Tp>((__x < __y) == (__x >= __y));
    }
}

// }}}
// __maskstore (non-converting; with optimizations for SSE2-AVX512BWVL) {{{
template <typename _V,
	  typename _VVT = _VectorTraits<_V>,
	  typename _Tp = typename _VVT::value_type,
	  class _F>
_GLIBCXX_SIMD_INTRINSIC void
  __maskstore(_V __v, _Tp* __mem, _F, _SimdWrapper<bool, _VVT::_S_width> __k)
{
  [[maybe_unused]] const auto __vi = __to_intrin(__v);
#if _GLIBCXX_SIMD_X86INTRIN // {{{
  if constexpr (sizeof(__vi) == 64)
    {
      static_assert(sizeof(__v) == 64 && __have_avx512f);
      if constexpr (__have_avx512bw && sizeof(_Tp) == 1)
	_mm512_mask_storeu_epi8(__mem, __k, __vi);
      else if constexpr (__have_avx512bw && sizeof(_Tp) == 2)
	_mm512_mask_storeu_epi16(__mem, __k, __vi);
      else if constexpr (__have_avx512f && sizeof(_Tp) == 4)
	{
	  if constexpr (__is_aligned_v<_F, 64> && std::is_integral_v<_Tp>)
	    _mm512_mask_store_epi32(__mem, __k, __vi);
	  else if constexpr (__is_aligned_v<_F, 64> &&
			     std::is_floating_point_v<_Tp>)
	    _mm512_mask_store_ps(__mem, __k, __vi);
	  else if constexpr (std::is_integral_v<_Tp>)
	    _mm512_mask_storeu_epi32(__mem, __k, __vi);
	  else
	    _mm512_mask_storeu_ps(__mem, __k, __vi);
	}
      else if constexpr (__have_avx512f && sizeof(_Tp) == 8)
	{
	  if constexpr (__is_aligned_v<_F, 64> && std::is_integral_v<_Tp>)
	    _mm512_mask_store_epi64(__mem, __k, __vi);
	  else if constexpr (__is_aligned_v<_F, 64> &&
			     std::is_floating_point_v<_Tp>)
	    _mm512_mask_store_pd(__mem, __k, __vi);
	  else if constexpr (std::is_integral_v<_Tp>)
	    _mm512_mask_storeu_epi64(__mem, __k, __vi);
	  else
	    _mm512_mask_storeu_pd(__mem, __k, __vi);
	}
      else if constexpr (__have_sse2)
	{
	  constexpr int _N = 16 / sizeof(_Tp);
	  using _M         = __vector_type_t<_Tp, _N>;
	  _mm_maskmoveu_si128(__auto_bitcast(__extract<0, 4>(__v._M_data)),
			      __auto_bitcast(__convert_mask<_M>(__k._M_data)),
			      reinterpret_cast<char*>(__mem));
	  _mm_maskmoveu_si128(
	    __auto_bitcast(__extract<1, 4>(__v._M_data)),
	    __auto_bitcast(__convert_mask<_M>(__k._M_data >> 1 * _N)),
	    reinterpret_cast<char*>(__mem) + 1 * 16);
	  _mm_maskmoveu_si128(
	    __auto_bitcast(__extract<2, 4>(__v._M_data)),
	    __auto_bitcast(__convert_mask<_M>(__k._M_data >> 2 * _N)),
	    reinterpret_cast<char*>(__mem) + 2 * 16);
	  _mm_maskmoveu_si128(
	    __auto_bitcast(__extract<3, 4>(__v._M_data)),
	    __auto_bitcast(__convert_mask<_M>(__k._M_data >> 3 * _N)),
	    reinterpret_cast<char*>(__mem) + 3 * 16);
	}
      else
	__assert_unreachable<_Tp>();
    }
  else if constexpr (sizeof(__vi) == 32)
    {
      if constexpr (__have_avx512bw_vl && sizeof(_Tp) == 1)
	_mm256_mask_storeu_epi8(__mem, __k, __vi);
      else if constexpr (__have_avx512bw_vl && sizeof(_Tp) == 2)
	_mm256_mask_storeu_epi16(__mem, __k, __vi);
      else if constexpr (__have_avx512vl && sizeof(_Tp) == 4)
	{
	  if constexpr (__is_aligned_v<_F, 32> && std::is_integral_v<_Tp>)
	    _mm256_mask_store_epi32(__mem, __k, __vi);
	  else if constexpr (__is_aligned_v<_F, 32> &&
			     std::is_floating_point_v<_Tp>)
	    _mm256_mask_store_ps(__mem, __k, __vi);
	  else if constexpr (std::is_integral_v<_Tp>)
	    _mm256_mask_storeu_epi32(__mem, __k, __vi);
	  else
	    _mm256_mask_storeu_ps(__mem, __k, __vi);
	}
      else if constexpr (__have_avx512vl && sizeof(_Tp) == 8)
	{
	  if constexpr (__is_aligned_v<_F, 32> && std::is_integral_v<_Tp>)
	    _mm256_mask_store_epi64(__mem, __k, __vi);
	  else if constexpr (__is_aligned_v<_F, 32> &&
			     std::is_floating_point_v<_Tp>)
	    _mm256_mask_store_pd(__mem, __k, __vi);
	  else if constexpr (std::is_integral_v<_Tp>)
	    _mm256_mask_storeu_epi64(__mem, __k, __vi);
	  else
	    _mm256_mask_storeu_pd(__mem, __k, __vi);
	}
      else if constexpr (__have_avx512f &&
			 (sizeof(_Tp) >= 4 || __have_avx512bw))
	{
	  // use a 512-bit maskstore, using zero-extension of the bitmask
	  __maskstore(_SimdWrapper64<_Tp>(
			__intrin_bitcast<__vector_type64_t<_Tp>>(__v._M_data)),
		      __mem,
		      // careful, vector_aligned has a stricter meaning in the
		      // 512-bit maskstore:
		      std::conditional_t<std::is_same_v<_F, vector_aligned_tag>,
					 overaligned_tag<32>, _F>(),
		      _SimdWrapper<bool, 64 / sizeof(_Tp)>(__k._M_data));
	}
      else
	{
	  __maskstore(
	    __v, __mem, _F(),
	    _SimdWrapper32<_Tp>(
	      __convert_mask<__vector_type_t<_Tp, 32 / sizeof(_Tp)>>(__k)));
	}
    }
  else
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
    if constexpr (sizeof(__vi) == 16)
    {
#if _GLIBCXX_SIMD_X86INTRIN // {{{
      // the store is aligned if _F is overaligned_tag<16> (or higher) or _F
      // is vector_aligned_tag while __v is actually a 16-Byte vector (could
      // be 2/4/8 as well)
      [[maybe_unused]] constexpr bool __aligned =
	__is_aligned_v<_F, 16> &&
	(sizeof(__v) == 16 || !std::is_same_v<_F, vector_aligned_tag>);
      if constexpr (__have_avx512bw_vl && sizeof(_Tp) == 1)
	_mm_mask_storeu_epi8(__mem, __k, __vi);
      else if constexpr (__have_avx512bw_vl && sizeof(_Tp) == 2)
	_mm_mask_storeu_epi16(__mem, __k, __vi);
      else if constexpr (__have_avx512vl && sizeof(_Tp) == 4)
	{
	  if constexpr (__aligned && std::is_integral_v<_Tp>)
	    _mm_mask_store_epi32(__mem, __k, __vi);
	  else if constexpr (__aligned &&
			     std::is_floating_point_v<_Tp>)
	    _mm_mask_store_ps(__mem, __k, __vi);
	  else if constexpr (std::is_integral_v<_Tp>)
	    _mm_mask_storeu_epi32(__mem, __k, __vi);
	  else
	    _mm_mask_storeu_ps(__mem, __k, __vi);
	}
      else if constexpr (__have_avx512vl && sizeof(_Tp) == 8)
	{
	  if constexpr (__aligned && std::is_integral_v<_Tp>)
	    _mm_mask_store_epi64(__mem, __k, __vi);
	  else if constexpr (__aligned &&
			     std::is_floating_point_v<_Tp>)
	    _mm_mask_store_pd(__mem, __k, __vi);
	  else if constexpr (std::is_integral_v<_Tp>)
	    _mm_mask_storeu_epi64(__mem, __k, __vi);
	  else
	    _mm_mask_storeu_pd(__mem, __k, __vi);
	}
      else if constexpr (__have_avx512f &&
			 (sizeof(_Tp) >= 4 || __have_avx512bw))
	{
	  // use a 512-bit maskstore, using zero-extension of the bitmask
	  __maskstore(
	    _SimdWrapper64<_Tp>(
	      __intrin_bitcast<__intrinsic_type64_t<_Tp>>(__v._M_data)),
	    __mem,
	    // careful, vector_aligned has a stricter meaning in the 512-bit
	    // maskstore:
	    std::conditional_t<std::is_same_v<_F, vector_aligned_tag>,
			       overaligned_tag<sizeof(__v)>, _F>(),
	    _SimdWrapper<bool, 64 / sizeof(_Tp)>(__k._M_data));
	}
      else
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
	{
	  __maskstore(
	    __v, __mem, _F(),
	    _SimdWrapper16<_Tp>(
	      __convert_mask<__vector_type_t<_Tp, 16 / sizeof(_Tp)>>(__k)));
	}
    }
  else
    __assert_unreachable<_V>();
}

template <typename _V,
	  typename _VVT = _VectorTraits<_V>,
	  typename _Tp  = typename _VVT::value_type,
	  class _F>
_GLIBCXX_SIMD_INTRINSIC void
  __maskstore(_V __v, _Tp* __mem, _F, _SimdWrapper<_Tp, _VVT::_S_width> __k)
{
  constexpr size_t _N = _VVT::_S_width;
#if _GLIBCXX_SIMD_X86INTRIN // {{{
  if constexpr (sizeof(_V) <= 16)
    {
      [[maybe_unused]] const auto __vi = __intrin_bitcast<__m128i>(__as_vector(__v));
      [[maybe_unused]] const auto __ki = __intrin_bitcast<__m128i>(__as_vector(__k));
      if constexpr (__have_avx512bw_vl && sizeof(_Tp) == 1)
	_mm_mask_storeu_epi8(__mem, _mm_movepi8_mask(__ki), __vi);
      else if constexpr (__have_avx512bw_vl && sizeof(_Tp) == 2)
	_mm_mask_storeu_epi16(__mem, _mm_movepi16_mask(__ki), __vi);
      else if constexpr (__have_avx2 && sizeof(_Tp) == 4 &&
			 std::is_integral_v<_Tp>)
	_mm_maskstore_epi32(reinterpret_cast<int*>(__mem), __ki, __vi);
      else if constexpr (__have_avx && sizeof(_Tp) == 4)
	_mm_maskstore_ps(reinterpret_cast<float*>(__mem), __ki,
			 __vector_bitcast<float>(__vi));
      else if constexpr (__have_avx2 && sizeof(_Tp) == 8 &&
			 std::is_integral_v<_Tp>)
	_mm_maskstore_epi64(reinterpret_cast<_LLong*>(__mem), __ki, __vi);
      else if constexpr (__have_avx && sizeof(_Tp) == 8)
	_mm_maskstore_pd(reinterpret_cast<double*>(__mem), __ki,
			 __vector_bitcast<double>(__vi));
      else if constexpr (__have_sse2)
	_mm_maskmoveu_si128(__vi, __ki, reinterpret_cast<char*>(__mem));
    }
  else if constexpr (sizeof(_V) == 32)
    {
      [[maybe_unused]] const auto __vi = __intrin_bitcast<__m256i>(__as_vector(__v));
      [[maybe_unused]] const auto __ki = __intrin_bitcast<__m256i>(__as_vector(__k));
      if constexpr (__have_avx512bw_vl && sizeof(_Tp) == 1)
	_mm256_mask_storeu_epi8(__mem, _mm256_movepi8_mask(__ki), __vi);
      else if constexpr (__have_avx512bw_vl && sizeof(_Tp) == 2)
	_mm256_mask_storeu_epi16(__mem, _mm256_movepi16_mask(__ki), __vi);
      else if constexpr (__have_avx2 && sizeof(_Tp) == 4 &&
			 std::is_integral_v<_Tp>)
	_mm256_maskstore_epi32(reinterpret_cast<int*>(__mem), __ki, __vi);
      else if constexpr (sizeof(_Tp) == 4)
	_mm256_maskstore_ps(reinterpret_cast<float*>(__mem), __ki,
			    __vector_bitcast<float>(__v));
      else if constexpr (__have_avx2 && sizeof(_Tp) == 8 &&
			 std::is_integral_v<_Tp>)
	_mm256_maskstore_epi64(reinterpret_cast<_LLong*>(__mem), __ki, __vi);
      else if constexpr (__have_avx && sizeof(_Tp) == 8)
	_mm256_maskstore_pd(reinterpret_cast<double*>(__mem), __ki,
			    __vector_bitcast<double>(__v));
      else if constexpr (__have_sse2)
	{
	  _mm_maskmoveu_si128(__lo128(__vi), __lo128(__ki),
			      reinterpret_cast<char*>(__mem));
	  _mm_maskmoveu_si128(__hi128(__vi), __hi128(__ki),
			      reinterpret_cast<char*>(__mem) + 16);
	}
    }
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
  if constexpr (_N <= sizeof(long) * CHAR_BIT)
    __bit_iteration(
      __vector_to_bitset(__k).to_ulong(), [&](auto __i) constexpr {
	__mem[__i] = __v[__i];
      });
  else if constexpr (_N <= sizeof(long long) * CHAR_BIT)
    __bit_iteration(
      __vector_to_bitset(__k._M_data).to_ullong(), [&](auto __i) constexpr {
	__mem[__i] = __v[__i];
      });
  else
    __assert_unreachable<_Tp>();
}

// }}}
// __xzyw{{{
// shuffles the complete vector, swapping the inner two quarters. Often useful for AVX for
// fixing up a shuffle result.
template <class _Tp, class _TVT = _VectorTraits<_Tp>>
_GLIBCXX_SIMD_INTRINSIC _Tp __xzyw(_Tp __a)
{
    if constexpr (sizeof(_Tp) == 16) {
        static_assert(sizeof(float) == 4 && sizeof(int) == 4);
        const auto __x = __vector_bitcast<
            conditional_t<is_floating_point_v<typename _TVT::value_type>, float, int>>(
            __a);
        return reinterpret_cast<_Tp>(decltype(__x){__x[0], __x[2], __x[1], __x[3]});
    } else if constexpr (sizeof(_Tp) == 32) {
        static_assert(sizeof(double) == 8 && sizeof(_LLong) == 8);
        const auto __x =
            __vector_bitcast<conditional_t<is_floating_point_v<typename _TVT::value_type>,
                                           double, _LLong>>(__a);
        return reinterpret_cast<_Tp>(decltype(__x){__x[0], __x[2], __x[1], __x[3]});
    } else if constexpr (sizeof(_Tp) == 64) {
        static_assert(sizeof(double) == 8 && sizeof(_LLong) == 8);
        const auto __x =
            __vector_bitcast<conditional_t<is_floating_point_v<typename _TVT::value_type>,
                                           double, _LLong>>(__a);
        return reinterpret_cast<_Tp>(decltype(__x){__x[0], __x[1], __x[4], __x[5], __x[2],
                                                  __x[3], __x[6], __x[7]});
    } else {
        __assert_unreachable<_Tp>();
    }
}

// }}}
// __shift_elements_right{{{
// if (__shift % 2ⁿ == 0) => the low n Bytes are correct
template <unsigned __shift, class _Tp, class _TVT = _VectorTraits<_Tp>>
_GLIBCXX_SIMD_INTRINSIC _Tp __shift_elements_right(_Tp __v)
{
  [[maybe_unused]] const auto __iv = __to_intrin(__v);
  static_assert(__shift <= sizeof(_Tp));
  if constexpr (__shift == 0)
    return __v;
  else if constexpr (__shift == sizeof(_Tp))
    return _Tp();
#if _GLIBCXX_SIMD_X86INTRIN // {{{
  else if constexpr (__have_sse && __shift == 8 &&
		     _TVT::template __is<float, 4>)
    return _mm_movehl_ps(__iv, __iv);
  else if constexpr (__have_sse2 && __shift == 8 &&
		     _TVT::template __is<double, 2>)
    return _mm_unpackhi_pd(__iv, __iv);
  else if constexpr (__have_sse2 && sizeof(_Tp) == 16)
    return reinterpret_cast<typename _TVT::type>(
      _mm_srli_si128(reinterpret_cast<__m128i>(__iv), __shift));
  else if constexpr (__shift == 16 && sizeof(_Tp) == 32)
    {
      /*if constexpr (__have_avx && _TVT::template __is<double, 4>)
	return _mm256_permute2f128_pd(__iv, __iv, 0x81);
      else if constexpr (__have_avx && _TVT::template __is<float, 8>)
	return _mm256_permute2f128_ps(__iv, __iv, 0x81);
      else if constexpr (__have_avx)
	return reinterpret_cast<typename _TVT::type>(
	  _mm256_permute2f128_si256(__iv, __iv, 0x81));
      else*/
	return __zero_extend(__hi128(__v));
    }
  else if constexpr (__have_avx2 && sizeof(_Tp) == 32 && __shift < 16)
    {
      const auto __vll = __vector_bitcast<_LLong>(__v);
      return reinterpret_cast<typename _TVT::type>(_mm256_alignr_epi8(
	_mm256_permute2x128_si256(__vll, __vll, 0x81), __vll, __shift));
    }
  else if constexpr (__have_avx && sizeof(_Tp) == 32 && __shift < 16)
    {
      const auto __vll = __vector_bitcast<_LLong>(__v);
      return reinterpret_cast<typename _TVT::type>(
	__concat(_mm_alignr_epi8(__hi128(__vll), __lo128(__vll), __shift),
		 _mm_srli_si128(__hi128(__vll), __shift)));
    }
  else if constexpr (sizeof(_Tp) == 32 && __shift > 16)
    return __zero_extend(__shift_elements_right<__shift - 16>(__hi128(__v)));
  else if constexpr (sizeof(_Tp) == 64 && __shift == 32)
    return __zero_extend(__hi256(__v));
  else if constexpr (__have_avx512f && sizeof(_Tp) == 64)
    {
      if constexpr(__shift >= 48)
	return __zero_extend( __shift_elements_right<__shift - 48>(__extract<3, 4>(__v)));
      else if constexpr(__shift >= 32)
	return __zero_extend(__shift_elements_right<__shift - 32>(__hi256(__v)));
      else if constexpr (__shift % 8 == 0)
	return reinterpret_cast<typename _TVT::type>(_mm512_alignr_epi64(
	  __m512i(), __intrin_bitcast<__m512i>(__v), __shift / 8));
      else if constexpr (__shift % 4 == 0)
	return reinterpret_cast<typename _TVT::type>(_mm512_alignr_epi32(
	  __m512i(), __intrin_bitcast<__m512i>(__v), __shift / 4));
      else if constexpr(__have_avx512bw && __shift < 16)
	{
	  const auto __vll = __vector_bitcast<_LLong>(__v);
	  return reinterpret_cast<typename _TVT::type>(_mm512_alignr_epi8(
	    _mm512_shuffle_i32x4(__vll, __vll, 0xf9), __vll, __shift));
	}
      else if constexpr(__have_avx512bw && __shift < 32)
	{
	  const auto __vll = __vector_bitcast<_LLong>(__v);
	  return reinterpret_cast<typename _TVT::type>(_mm512_alignr_epi8(
	    _mm512_shuffle_i32x4(__vll, __m512i(), 0xee),
	    _mm512_shuffle_i32x4(__vll, __vll, 0xf9), __shift - 16));
	}
      else
	__assert_unreachable<_Tp>();
    }
/*
    } else if constexpr (__shift % 16 == 0 && sizeof(_Tp) == 64)
        return __auto_bitcast(__extract<__shift / 16, 4>(__v));
*/
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
  else
    {
      constexpr int __chunksize =
	__shift % 8 == 0 ? 8 : __shift % 4 == 0 ? 4 : __shift % 2 == 0 ? 2 : 1;
      auto __w = __vector_bitcast<__int_with_sizeof_t<__chunksize>>(__v);
      using _U = decltype(__w);
      return __intrin_bitcast<_Tp>(
	__call_with_n_evaluations<(sizeof(_Tp) - __shift) / __chunksize>(
	  [](auto... __chunks) { return _U{__chunks...}; },
	  [&](auto __i) { return __w[__shift / __chunksize + __i]; }));
    }
}

// }}}
// __extract_part(_Tp) {{{
template <int _Index, int _Total, int _Combine, typename _Tp, size_t _N>
_GLIBCXX_SIMD_INTRINSIC
  _GLIBCXX_SIMD_CONST _SimdWrapper<_Tp, _N / _Total * _Combine>
		      __extract_part(const _SimdWrapper<_Tp, _N> __x)
{
  if constexpr (_Index % 2 == 0 && _Total % 2 == 0 && _Combine % 2 == 0)
    return __extract_part<_Index / 2, _Total / 2, _Combine / 2>(__x);
  else
    {
      constexpr size_t __values_per_part = _N / _Total;
      constexpr size_t __values_to_skip  = _Index * __values_per_part;
      constexpr size_t __return_size     = __values_per_part * _Combine;
      using _R = __vector_type_t<_Tp, __return_size>;
      static_assert((_Index + _Combine) * __values_per_part * sizeof(_Tp) <=
		      sizeof(__x),
		    "out of bounds __extract_part");
      // the following assertion would ensure no "padding" to be read
      //static_assert(_Total >= _Index + _Combine, "_Total must be greater than _Index");

      // static_assert(__return_size * _Total == _N, "_N must be divisible by
      // _Total");
      if constexpr (_Index == 0 && _Total == 1)
	return __x;
      else if constexpr (_Index == 0)
	return __intrin_bitcast<_R>(__as_vector(__x));
#if _GLIBCXX_SIMD_X86INTRIN // {{{
      else if constexpr (sizeof(__x) == 32 && __return_size * sizeof(_Tp) <= 16)
	{
	  constexpr size_t __bytes_to_skip = __values_to_skip * sizeof(_Tp);
	  if constexpr (__bytes_to_skip == 16)
	    return __vector_bitcast<_Tp, __return_size>(
	      __hi128(__as_vector(__x)));
	  else
	    return __vector_bitcast<_Tp, __return_size>(_mm_alignr_epi8(
	      __hi128(__vector_bitcast<_LLong>(__x)),
	      __lo128(__vector_bitcast<_LLong>(__x)), __bytes_to_skip));
	}
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
      else if constexpr (_Index > 0 &&
			 (__values_to_skip % __return_size != 0 ||
			  sizeof(_R) >= 8) &&
			 (__values_to_skip + __return_size) * sizeof(_Tp) <=
			   64 &&
			 sizeof(__x) >= 16)
	return __intrin_bitcast<_R>(
	  __shift_elements_right<__values_to_skip * sizeof(_Tp)>(
	    __as_vector(__x)));
      else
	{
	  _R __r = {};
	  __builtin_memcpy(&__r,
			   reinterpret_cast<const char*>(&__x) +
			     sizeof(_Tp) * __values_to_skip,
			   __return_size * sizeof(_Tp));
	  return __r;
	}
    }
}

// }}}
// __extract_part(_SimdWrapper<bool, _N>) {{{
template <int _Index, int _Total, int _Combine = 1, size_t _N>
_GLIBCXX_SIMD_INTRINSIC constexpr __bool_storage_member_type_t<_N / _Total>
  __extract_part(_SimdWrapper<bool, _N> __x)
{
  static_assert(_Combine == 1, "_Combine != 1 not implemented");
  static_assert(__have_avx512f && _N == _N);
  static_assert(_Total >= 2 && _Index + _Combine <= _Total && _Index >= 0);
  return __x._M_data >> (_Index * _N / _Total);
}

// }}}
// __extract_part(_SimdTuple) {{{
template <int _Index,
	  int _Total,
	  int _Combine,
	  typename _Tp,
	  typename _A0,
	  typename... _As>
_GLIBCXX_SIMD_INTRINSIC auto // __vector_type_t or _SimdTuple
  __extract_part(const _SimdTuple<_Tp, _A0, _As...>& __x)
{
  // worst cases:
  // (a) 4, 4, 4 => 3, 3, 3, 3 (_Total = 4)
  // (b) 2, 2, 2 => 3, 3       (_Total = 2)
  // (c) 4, 2 => 2, 2, 2       (_Total = 3)
  using _Tuple = _SimdTuple<_Tp, _A0, _As...>;
  static_assert(_Index + _Combine <= _Total && _Index >= 0 && _Total >= 1);
  constexpr size_t _N = _Tuple::size();
  static_assert(_N >= _Total && _N % _Total == 0);
  constexpr size_t __values_per_part = _N / _Total;
  [[maybe_unused]] constexpr size_t __values_to_skip = _Index * __values_per_part;
  constexpr size_t __return_size = __values_per_part * _Combine;
  using _RetAbi = simd_abi::deduce_t<_Tp, __return_size>;

  // handle (optimize) the simple cases
  if constexpr (_Index == 0 && _Tuple::_S_first_size == __return_size)
    return __x.first._M_data;
  else if constexpr (_Index == 0 && _Total == _Combine)
    return __x;
  else if constexpr (_Index == 0 && _Tuple::_S_first_size >= __return_size)
    return __intrin_bitcast<__vector_type_t<_Tp, __return_size>>(
      __as_vector(__x.first));

  // recurse to skip unused data members at the beginning of _SimdTuple
  else if constexpr (__values_to_skip >= _Tuple::_S_first_size)
    { // recurse
      if constexpr (_Tuple::_S_first_size % __values_per_part == 0)
	{
	  constexpr int __parts_in_first =
	    _Tuple::_S_first_size / __values_per_part;
	  return __extract_part<_Index - __parts_in_first,
				_Total - __parts_in_first, _Combine>(
	    __x.second);
	}
      else
	return __extract_part<__values_to_skip - _Tuple::_S_first_size,
			      _N - _Tuple::_S_first_size, __return_size>(
	  __x.second);
    }

  // extract from multiple _SimdTuple data members
  else if constexpr (__return_size > _Tuple::_S_first_size - __values_to_skip)
    {
#ifdef _GLIBCXX_SIMD_USE_ALIASING_LOADS
      const __may_alias<_Tp>* const element_ptr =
	reinterpret_cast<const __may_alias<_Tp>*>(&__x) + __values_to_skip;
      return __as_vector(simd<_Tp, _RetAbi>(element_ptr, element_aligned));
#else
      [[maybe_unused]] constexpr size_t __offset = __values_to_skip;
      return __as_vector(simd<_Tp, _RetAbi>([&](auto __i) constexpr {
	constexpr _SizeConstant<__i + __offset> __k;
	return __x[__k];
      }));
#endif
    }

  // all of the return values are in __x.first
  else if constexpr (_Tuple::_S_first_size % __values_per_part == 0)
    return __extract_part<_Index, _Tuple::_S_first_size / __values_per_part,
			  _Combine>(__x.first);
  else
    return __extract_part<__values_to_skip, _Tuple::_S_first_size,
			  _Combine * __values_per_part>(__x.first);
}

// }}}
// _ToWrapper specializations for bitset and __mmask<_N> {{{
#if _GLIBCXX_SIMD_HAVE_AVX512_ABI
template <size_t _N> class _ToWrapper<std::bitset<_N>>
{
    std::bitset<_N> _M_data;

public:
    // can convert to larger storage for _Abi::_S_is_partial == true
    template <class _U, size_t _M> constexpr operator _SimdWrapper<_U, _M>() const
    {
        static_assert(_M >= _N);
        return __convert_mask<_SimdWrapper<_U, _M>>(_M_data);
    }
};

#define _GLIBCXX_SIMD_TO_STORAGE(_Type)                                                  \
    template <> class _ToWrapper<_Type>                                                \
    {                                                                                    \
        _Type _M_data;                                                                         \
                                                                                         \
    public:                                                                              \
        template <class _U, size_t _N> constexpr operator _SimdWrapper<_U, _N>() const      \
        {                                                                                \
            static_assert(_N >= sizeof(_Type) * CHAR_BIT);                               \
            return reinterpret_cast<__vector_type_t<_U, _N>>(                            \
                __convert_mask<_SimdWrapper<_U, _N>>(_M_data));                                   \
        }                                                                                \
                                                                                         \
        template <size_t _N> constexpr operator _SimdWrapper<bool, _N>() const              \
        {                                                                                \
            static_assert(                                                               \
                std::is_same_v<_Type, typename __bool_storage_member_type<_N>::type>);   \
            return _M_data;                                                                    \
        }                                                                                \
    }
_GLIBCXX_SIMD_TO_STORAGE(__mmask8);
_GLIBCXX_SIMD_TO_STORAGE(__mmask16);
_GLIBCXX_SIMD_TO_STORAGE(__mmask32);
_GLIBCXX_SIMD_TO_STORAGE(__mmask64);
#undef _GLIBCXX_SIMD_TO_STORAGE
#endif  // _GLIBCXX_SIMD_HAVE_AVX512_ABI

// }}}

#if _GLIBCXX_SIMD_HAVE_SSE && defined _GLIBCXX_SIMD_WORKAROUND_PR85048
#include "simd_x86_conversions.h"
#endif  // SSE && _GLIBCXX_SIMD_WORKAROUND_PR85048

// __convert function{{{
template <class _To, class _From, class... _More>
_GLIBCXX_SIMD_INTRINSIC auto __convert(_From __v0, _More... __vs)
{
  if constexpr (__is_vectorizable_v<_From>)
    {
      static_assert((true && ... && is_same_v<_From, _More>));
      using _V = typename _VectorTraits<_To>::type;
      using _Tp = typename _VectorTraits<_To>::value_type;
      return _V{static_cast<_Tp>(__v0), static_cast<_Tp>(__vs)...};
    }
  else if constexpr (!__is_vector_type_v<_From>)
    return __convert<_To>(__as_vector(__v0), __as_vector(__vs)...);
  else
    {
      static_assert((true && ... && is_same_v<_From, _More>));
      if constexpr (__is_vectorizable_v<_To>)
	return __convert<__vector_type_t<_To, (_VectorTraits<_From>::_S_width *
					       (1 + sizeof...(_More)))>>(
	  __v0, __vs...);
      else if constexpr (!__is_vector_type_v<_To>)
	return _To(__convert<typename _To::_BuiltinType>(__v0, __vs...));
      else
	{
	  static_assert(
	    sizeof...(_More) == 0 ||
	      _VectorTraits<_To>::_S_width >=
		(1 + sizeof...(_More)) * _VectorTraits<_From>::_S_width,
	    "__convert(...) requires the input to fit into the output");
	  return __vector_convert<_To>(__v0, __vs...);
	}
    }
}

// }}}
// __convert_all{{{
// Converts __v into std::array<_To, N>, where N is _NParts if non-zero or
// otherwise deduced from _To such that N * #elements(_To) <= #elements(__v).
// Note: this function may return less than all converted elements
template <typename _To,
	  size_t _NParts = 0, // allows to convert fewer or more (only last _To,
			      // to be partially filled) than all
	  size_t _Offset = 0, // where to start, # of elements (not Bytes or
			      // Parts)
	  typename _From,
	  typename _FromVT = _VectorTraits<_From>>
_GLIBCXX_SIMD_INTRINSIC auto __convert_all(_From __v)
{
  if constexpr (std::is_arithmetic_v<_To> && _NParts != 1)
    {
      static_assert(_Offset < _FromVT::_S_width);
      constexpr auto _N = _NParts == 0 ? _FromVT::_S_partial_width - _Offset : _NParts;
      return __generate_from_n_evaluations<_N, std::array<_To, _N>>(
	[&](auto __i) { return static_cast<_To>(__v[__i + _Offset]); });
    }
  else
    {
      static_assert(__is_vector_type_v<_To>);
      using _ToVT = _VectorTraits<_To>;
      if constexpr (__is_vector_type_v<_From>)
	return __convert_all<_To, _NParts>(__as_wrapper(__v));
      else if constexpr (_NParts == 1)
	{
	  static_assert(_Offset % _ToVT::_S_width == 0);
	  return std::array<_To, 1>{__vector_convert<_To>(
	    __extract_part<_Offset / _ToVT::_S_width,
			   __div_roundup(_FromVT::_S_partial_width,
					 _ToVT::_S_width)>(__v))};
	}
#if _GLIBCXX_SIMD_X86INTRIN // {{{
      else if constexpr (!__have_sse4_1 && _Offset == 0 &&
			 is_integral_v<typename _FromVT::value_type> &&
			 sizeof(typename _FromVT::value_type) <
			   sizeof(typename _ToVT::value_type) &&
			 !(sizeof(typename _FromVT::value_type) == 4 &&
			   is_same_v<typename _ToVT::value_type, double>))
	{
	  using _ToT   = typename _ToVT::value_type;
	  using _FromT = typename _FromVT::value_type;
	  constexpr size_t _N =
	    _NParts != 0 ? _NParts
			 : (_FromVT::_S_partial_width / _ToVT::_S_width);
	  using _R = std::array<_To, _N>;
	  // __adjust modifies its input to have _N (use _SizeConstant) entries
	  // so that no unnecessary intermediate conversions are requested and,
	  // more importantly, no intermediate conversions are missing
	  [[maybe_unused]] auto __adjust =
	    [](auto __n,
	       auto __vv) -> _SimdWrapper<_FromT, decltype(__n)::value> {
	    return __vector_bitcast<_FromT, decltype(__n)::value>(__vv);
	  };
	  [[maybe_unused]] const auto __vi = __to_intrin(__v);
	  auto&& __make_array         = [](std::initializer_list<auto> __xs) {
	    return __call_with_subscripts(
	      __xs.begin(), std::make_index_sequence<_N>(),
	      [](auto... __ys) { return _R{__vector_bitcast<_ToT>(__ys)...}; });
	  };

	  if constexpr (_N == 0)
	    return _R{};
	  else if constexpr (sizeof(_FromT) == 1 && sizeof(_ToT) == 2)
	    {
	      static_assert(std::is_integral_v<_FromT>);
	      static_assert(std::is_integral_v<_ToT>);
	      if constexpr (is_unsigned_v<_FromT>)
		return __make_array({_mm_unpacklo_epi8(__vi, __m128i()),
				     _mm_unpackhi_epi8(__vi, __m128i())});
	      else
		return __make_array(
		  {_mm_srai_epi16(_mm_unpacklo_epi8(__vi, __vi), 8),
		   _mm_srai_epi16(_mm_unpackhi_epi8(__vi, __vi), 8)});
	    }
	  else if constexpr (sizeof(_FromT) == 2 && sizeof(_ToT) == 4)
	    {
	      static_assert(std::is_integral_v<_FromT>);
	      if constexpr (is_floating_point_v<_ToT>)
		{
		  const auto __ints = __convert_all<__vector_type16_t<int>, _N>(
		    __adjust(_SizeConstant<_N * 4>(), __v));
		  return __generate_from_n_evaluations<_N, _R>([&](auto __i) {
		    return __vector_convert<_To>(__ints[__i]);
		  });
		}
	      else if constexpr (is_unsigned_v<_FromT>)
		return __make_array({_mm_unpacklo_epi16(__vi, __m128i()),
				     _mm_unpackhi_epi16(__vi, __m128i())});
	      else
		return __make_array(
		  {_mm_srai_epi32(_mm_unpacklo_epi16(__vi, __vi), 16),
		   _mm_srai_epi32(_mm_unpackhi_epi16(__vi, __vi), 16)});
	    }
	  else if constexpr (sizeof(_FromT) == 4 && sizeof(_ToT) == 8 &&
			     is_integral_v<_FromT> && is_integral_v<_ToT>)
	    {
	      if constexpr (is_unsigned_v<_FromT>)
		return __make_array({_mm_unpacklo_epi32(__vi, __m128i()),
				     _mm_unpackhi_epi32(__vi, __m128i())});
	      else
		return __make_array(
		  {_mm_unpacklo_epi32(__vi, _mm_srai_epi32(__vi, 31)),
		   _mm_unpackhi_epi32(__vi, _mm_srai_epi32(__vi, 31))});
	    }
	  else if constexpr (sizeof(_FromT) == 4 && sizeof(_ToT) == 8 &&
			     is_integral_v<_FromT> && is_integral_v<_ToT>)
	    {
	      if constexpr (is_unsigned_v<_FromT>)
		return __make_array({_mm_unpacklo_epi32(__vi, __m128i()),
				     _mm_unpackhi_epi32(__vi, __m128i())});
	      else
		return __make_array(
		  {_mm_unpacklo_epi32(__vi, _mm_srai_epi32(__vi, 31)),
		   _mm_unpackhi_epi32(__vi, _mm_srai_epi32(__vi, 31))});
	    }
	  else if constexpr (sizeof(_FromT) == 1 && sizeof(_ToT) >= 4 &&
			     is_signed_v<_FromT>)
	    {
	      const __m128i __vv[2] = {_mm_unpacklo_epi8(__vi, __vi),
				       _mm_unpackhi_epi8(__vi, __vi)};
	      const __vector_type16_t<int> __vvvv[4] = {
		__vector_bitcast<int>(_mm_unpacklo_epi16(__vv[0], __vv[0])),
		__vector_bitcast<int>(_mm_unpackhi_epi16(__vv[0], __vv[0])),
		__vector_bitcast<int>(_mm_unpacklo_epi16(__vv[1], __vv[1])),
		__vector_bitcast<int>(_mm_unpackhi_epi16(__vv[1], __vv[1]))};
	      if constexpr (sizeof(_ToT) == 4)
		return __generate_from_n_evaluations<_N, _R>([&](auto __i) {
		  return __vector_convert<_To>(__vvvv[__i] >> 24);
		});
	      else if constexpr (is_integral_v<_ToT>)
		return __generate_from_n_evaluations<_N, _R>([&](auto __i) {
		  const auto __signbits = __to_intrin(__vvvv[__i / 2] >> 31);
		  const auto __sx32     = __to_intrin(__vvvv[__i / 2] >> 24);
		  return __vector_bitcast<_ToT>(
		    __i % 2 == 0 ? _mm_unpacklo_epi32(__sx32, __signbits)
				 : _mm_unpackhi_epi32(__sx32, __signbits));
		});
	      else
		return __generate_from_n_evaluations<_N, _R>([&](auto __i) {
		  const auto __int4 = __vvvv[__i / 2] >> 24;
		  return __vector_convert<_To>(
		    __i % 2 == 0
		      ? __int4
		      : __vector_bitcast<int>(_mm_unpackhi_epi64(
			  __to_intrin(__int4), __to_intrin(__int4))));
		});
	    }
	  else if constexpr (sizeof(_FromT) == 1 && sizeof(_ToT) == 4)
	    {
	      const auto __shorts = __convert_all<__vector_type16_t<
		conditional_t<is_signed_v<_FromT>, short, unsigned short>>>(
		__adjust(_SizeConstant<(_N + 1) / 2 * 8>(), __v));
	      return __generate_from_n_evaluations<_N, _R>([&](auto __i) {
		return __convert_all<_To>(__shorts[__i / 2])[__i % 2];
	      });
	    }
	  else if constexpr (sizeof(_FromT) == 2 && sizeof(_ToT) == 8 &&
			     is_signed_v<_FromT> && is_integral_v<_ToT>)
	    {
	      const __m128i __vv[2] = {_mm_unpacklo_epi16(__vi, __vi),
				       _mm_unpackhi_epi16(__vi, __vi)};
	      const __vector_type16_t<int> __vvvv[4] = {
		__vector_bitcast<int>(_mm_unpacklo_epi32(
		  _mm_srai_epi32(__vv[0], 16), _mm_srai_epi32(__vv[0], 31))),
		__vector_bitcast<int>(_mm_unpackhi_epi32(
		  _mm_srai_epi32(__vv[0], 16), _mm_srai_epi32(__vv[0], 31))),
		__vector_bitcast<int>(_mm_unpacklo_epi32(
		  _mm_srai_epi32(__vv[1], 16), _mm_srai_epi32(__vv[1], 31))),
		__vector_bitcast<int>(_mm_unpackhi_epi32(
		  _mm_srai_epi32(__vv[1], 16), _mm_srai_epi32(__vv[1], 31)))};
	      return __generate_from_n_evaluations<_N, _R>(
		[&](auto __i) { return __vector_bitcast<_ToT>(__vvvv[__i]); });
	    }
	  else if constexpr (sizeof(_FromT) <= 2 && sizeof(_ToT) == 8)
	    {
	      const auto __ints = __convert_all<__vector_type16_t<
		conditional_t<is_signed_v<_FromT> || is_floating_point_v<_ToT>,
			      int, unsigned int>>>(
		__adjust(_SizeConstant<(_N + 1) / 2 * 4>(), __v));
	      return __generate_from_n_evaluations<_N, _R>([&](auto __i) {
		return __convert_all<_To>(__ints[__i / 2])[__i % 2];
	      });
	    }
	  else
	    __assert_unreachable<_To>();
	}
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
      else if constexpr ((_FromVT::_S_partial_width - _Offset) >
			 _ToVT::_S_width)
	{
	  /*
	  static_assert(
	    (_FromVT::_S_partial_width & (_FromVT::_S_partial_width - 1)) == 0,
	    "__convert_all only supports power-of-2 number of elements.
	  Otherwise " "the return type cannot be std::array<_To, N>.");
	    */
	  constexpr size_t _NTotal =
	    (_FromVT::_S_partial_width - _Offset) / _ToVT::_S_width;
	  constexpr size_t _N = _NParts == 0 ? _NTotal : _NParts;
	  static_assert(
	    _N <= _NTotal ||
	    (_N == _NTotal + 1 &&
	     (_FromVT::_S_partial_width - _Offset) % _ToVT::_S_width > 0));
	  using _R = std::array<_To, _N>;
	  if constexpr (_N == 1)
	    return _R{__vector_convert<_To>(
	      __as_vector(__extract_part<_Offset, _FromVT::_S_partial_width,
					 _ToVT::_S_width>(__v)))};
	  else
	    return __generate_from_n_evaluations<_N, _R>([&](
	      auto __i) constexpr {
	      auto __part =
		__extract_part<__i * _ToVT::_S_width + _Offset,
			       _FromVT::_S_partial_width, _ToVT::_S_width>(__v);
	      return __vector_convert<_To>(__part);
	    });
	}
      else if constexpr (_Offset == 0)
	return std::array<_To, 1>{__vector_convert<_To>(__as_vector(__v))};
      else
	return std::array<_To, 1>{__vector_convert<_To>(__as_vector(
	  __extract_part<_Offset, _FromVT::_S_partial_width,
			 _FromVT::_S_partial_width - _Offset>(__v)))};
    }
}

// }}}
// __converts_via_decomposition{{{
// This lists all cases where a __vector_convert needs to fall back to conversion of
// individual scalars (i.e. decompose the input vector into scalars, convert, compose
// output vector). In those cases, __masked_load & __masked_store prefer to use the
// __bit_iteration implementation.
template <class _From, class _To, size_t _ToSize> struct __converts_via_decomposition {
private:
    static constexpr bool _S_i_to_i = is_integral_v<_From> && is_integral_v<_To>;
    static constexpr bool _S_f_to_i = is_floating_point_v<_From> && is_integral_v<_To>;
    static constexpr bool _S_f_to_f = is_floating_point_v<_From> && is_floating_point_v<_To>;
    static constexpr bool _S_i_to_f = is_integral_v<_From> && is_floating_point_v<_To>;

    template <size_t _A, size_t _B>
    static constexpr bool _S_sizes = sizeof(_From) == _A && sizeof(_To) == _B;

public:
    static constexpr bool value =
        (_S_i_to_i && _S_sizes<8, 2> && !__have_ssse3 && _ToSize == 16) ||
        (_S_i_to_i && _S_sizes<8, 1> && !__have_avx512f && _ToSize == 16) ||
        (_S_f_to_i && _S_sizes<4, 8> && !__have_avx512dq) ||
        (_S_f_to_i && _S_sizes<8, 8> && !__have_avx512dq) ||
        (_S_f_to_i && _S_sizes<8, 4> && !__have_sse4_1 && _ToSize == 16) ||
        (_S_i_to_f && _S_sizes<8, 4> && !__have_avx512dq && _ToSize == 16) ||
        (_S_i_to_f && _S_sizes<8, 8> && !__have_avx512dq && _ToSize < 64);
};

template <class _From, class _To, size_t _ToSize>
inline constexpr bool __converts_via_decomposition_v =
    __converts_via_decomposition<_From, _To, _ToSize>::value;

// }}}
// __is_bitset {{{
template <class _Tp> struct __is_bitset : false_type {};
template <size_t _N> struct __is_bitset<std::bitset<_N>> : true_type {};
template <class _Tp> inline constexpr bool __is_bitset_v = __is_bitset<_Tp>::value;

// }}}
// __is_storage {{{
template <class _Tp> struct __is_storage : false_type {};
template <class _Tp, size_t _N> struct __is_storage<_SimdWrapper<_Tp, _N>> : true_type {};
template <class _Tp> inline constexpr bool __is_storage_v = __is_storage<_Tp>::value;

// }}}
// __convert_mask{{{
template <class _To, class _From>
inline _To __convert_mask(_From __k)
{
  if constexpr (std::is_same_v<_To, _From>)
    { // also covers bool -> bool
      return __k;
    } else if constexpr (std::is_unsigned_v<_From> && std::is_unsigned_v<_To>) {
        // bits -> bits
        return __k;  // zero-extends or truncates
    } else if constexpr (__is_bitset_v<_From>) {
        // from std::bitset {{{
        static_assert(__k.size() <= sizeof(_ULLong) * CHAR_BIT);
        using _Tp = std::conditional_t<
            (__k.size() <= sizeof(_UShort) * CHAR_BIT),
            std::conditional_t<(__k.size() <= CHAR_BIT), _UChar, _UShort>,
            std::conditional_t<(__k.size() <= sizeof(_UInt) * CHAR_BIT), _UInt, _ULLong>>;
        return __convert_mask<_To>(static_cast<_Tp>(__k.to_ullong()));
        // }}}
    } else if constexpr (__is_bitset_v<_To>) {
        // to std::bitset {{{
        static_assert(_To().size() <= sizeof(_ULLong) * CHAR_BIT);
        using _Tp = std::conditional_t<
            (_To().size() <= sizeof(_UShort) * CHAR_BIT),
            std::conditional_t<(_To().size() <= CHAR_BIT), _UChar, _UShort>,
            std::conditional_t<(_To().size() <= sizeof(_UInt) * CHAR_BIT), _UInt, _ULLong>>;
        return __convert_mask<_Tp>(__k);
        // }}}
    } else if constexpr (__is_storage_v<_From>) {
        return __convert_mask<_To>(__k._M_data);
    } else if constexpr (__is_storage_v<_To>) {
        return __convert_mask<typename _To::_BuiltinType>(__k);
    } else if constexpr (std::is_unsigned_v<_From> && __is_vector_type_v<_To>) {
        // bits -> vector {{{
        using _Trait = _VectorTraits<_To>;
        constexpr size_t _N_in = sizeof(_From) * CHAR_BIT;
        using _ToT = typename _Trait::value_type;
        constexpr size_t _N_out = _Trait::_S_width;
        constexpr size_t _N = std::min(_N_in, _N_out);
        constexpr size_t __bytes_per_output_element = sizeof(_ToT);
#if _GLIBCXX_SIMD_X86INTRIN // {{{
        if constexpr (__have_avx512f) {
            if constexpr (__bytes_per_output_element == 1 && sizeof(_To) <= 16) {
                if constexpr (__have_avx512bw_vl) {
                    return __intrin_bitcast<_To>(_mm_movm_epi8(__k));
                } else if constexpr (__have_avx512bw) {
                    return __intrin_bitcast<_To>(__lo128(_mm512_movm_epi8(__k)));
                } else {
                    auto __as32bits = _mm512_maskz_mov_epi32(__k, ~__m512i());
                    auto __as16bits = __xzyw(
                        _mm256_packs_epi32(__lo256(__as32bits), __hi256(__as32bits)));
                    return __intrin_bitcast<_To>(
                        _mm_packs_epi16(__lo128(__as16bits), __hi128(__as16bits)));
                }
            } else if constexpr (__bytes_per_output_element == 1 && sizeof(_To) == 32) {
                if constexpr (__have_avx512bw_vl) {
                    return __vector_bitcast<_ToT>(_mm256_movm_epi8(__k));
                } else if constexpr (__have_avx512bw) {
                    return __vector_bitcast<_ToT>(__lo256(_mm512_movm_epi8(__k)));
                } else {
                    auto __as16bits =  // 0 16 1 17 ... 15 31
                        _mm512_srli_epi32(_mm512_maskz_mov_epi32(__k, ~__m512i()), 16) |
                        _mm512_slli_epi32(_mm512_maskz_mov_epi32(__k >> 16, ~__m512i()),
                                          16);
                    auto __0_16_1_17 = __xzyw(_mm256_packs_epi16(
                        __lo256(__as16bits),
                        __hi256(__as16bits))  // 0 16 1 17 2 18 3 19 8 24 9 25 ...
                    );
                    // deinterleave:
                    return __vector_bitcast<_ToT>(__xzyw(_mm256_shuffle_epi8(
                        __0_16_1_17,  // 0 16 1 17 2 ...
                        _mm256_setr_epi8(0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13,
                                         15, 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11,
                                         13, 15))));  // 0-7 16-23 8-15 24-31 -> xzyw
                                                      // 0-3  8-11 16-19 24-27
                                                      // 4-7 12-15 20-23 28-31
                }
            } else if constexpr (__bytes_per_output_element == 1 && sizeof(_To) == 64) {
                return reinterpret_cast<__vector_type_t<_SChar, 64>>(_mm512_movm_epi8(__k));
            } else if constexpr (__bytes_per_output_element == 2 && sizeof(_To) <= 16) {
                if constexpr (__have_avx512bw_vl) {
                    return __intrin_bitcast<_To>(_mm_movm_epi16(__k));
                } else if constexpr (__have_avx512bw) {
                    return __intrin_bitcast<_To>(__lo128(_mm512_movm_epi16(__k)));
                } else {
                    __m256i __as32bits;
                    if constexpr (__have_avx512vl) {
                        __as32bits = _mm256_maskz_mov_epi32(__k, ~__m256i());
                    } else {
                        __as32bits = __lo256(_mm512_maskz_mov_epi32(__k, ~__m512i()));
                    }
                    return __intrin_bitcast<_To>(
                        _mm_packs_epi32(__lo128(__as32bits), __hi128(__as32bits)));
                }
            } else if constexpr (__bytes_per_output_element == 2 && sizeof(_To) == 32) {
                if constexpr (__have_avx512bw_vl) {
                    return __vector_bitcast<_ToT>(_mm256_movm_epi16(__k));
                } else if constexpr (__have_avx512bw) {
                    return __vector_bitcast<_ToT>(__lo256(_mm512_movm_epi16(__k)));
                } else {
                    auto __as32bits = _mm512_maskz_mov_epi32(__k, ~__m512i());
                    return __vector_bitcast<_ToT>(__xzyw(
                        _mm256_packs_epi32(__lo256(__as32bits), __hi256(__as32bits))));
                }
            } else if constexpr (__bytes_per_output_element == 2 && sizeof(_To) == 64) {
                return __vector_bitcast<_ToT>(_mm512_movm_epi16(__k));
            } else if constexpr (__bytes_per_output_element == 4 && sizeof(_To) <= 16) {
                return __intrin_bitcast<_To>(
                    __have_avx512dq_vl
                        ? _mm_movm_epi32(__k)
                        : __have_avx512dq
                              ? __lo128(_mm512_movm_epi32(__k))
                              : __have_avx512vl
                                    ? _mm_maskz_mov_epi32(__k, ~__m128i())
                                    : __lo128(_mm512_maskz_mov_epi32(__k, ~__m512i())));
            } else if constexpr (__bytes_per_output_element == 4 && sizeof(_To) == 32) {
                return __vector_bitcast<_ToT>(
                    __have_avx512dq_vl
                        ? _mm256_movm_epi32(__k)
                        : __have_avx512dq
                              ? __lo256(_mm512_movm_epi32(__k))
                              : __have_avx512vl
                                    ? _mm256_maskz_mov_epi32(__k, ~__m256i())
                                    : __lo256(_mm512_maskz_mov_epi32(__k, ~__m512i())));
            } else if constexpr (__bytes_per_output_element == 4 && sizeof(_To) == 64) {
                return __vector_bitcast<_ToT>(__have_avx512dq
                                             ? _mm512_movm_epi32(__k)
                                             : _mm512_maskz_mov_epi32(__k, ~__m512i()));
            } else if constexpr (__bytes_per_output_element == 8 && sizeof(_To) == 16) {
                return __vector_bitcast<_ToT>(
                    __have_avx512dq_vl
                        ? _mm_movm_epi64(__k)
                        : __have_avx512dq
                              ? __lo128(_mm512_movm_epi64(__k))
                              : __have_avx512vl
                                    ? _mm_maskz_mov_epi64(__k, ~__m128i())
                                    : __lo128(_mm512_maskz_mov_epi64(__k, ~__m512i())));
            } else if constexpr (__bytes_per_output_element == 8 && sizeof(_To) == 32) {
                return __vector_bitcast<_ToT>(
                    __have_avx512dq_vl
                        ? _mm256_movm_epi64(__k)
                        : __have_avx512dq
                              ? __lo256(_mm512_movm_epi64(__k))
                              : __have_avx512vl
                                    ? _mm256_maskz_mov_epi64(__k, ~__m256i())
                                    : __lo256(_mm512_maskz_mov_epi64(__k, ~__m512i())));
            } else if constexpr (__bytes_per_output_element == 8 && sizeof(_To) == 64) {
                return __vector_bitcast<_ToT>(__have_avx512dq
                                             ? _mm512_movm_epi64(__k)
                                             : _mm512_maskz_mov_epi64(__k, ~__m512i()));
            } else {
                __assert_unreachable<_To>();
            }
        } else if constexpr (__have_sse) {
            using _U = std::make_unsigned_t<__int_for_sizeof_t<_ToT>>;
            using _V = __vector_type_t<_U, _N>;  // simd<_U, _Abi>;
            static_assert(sizeof(_V) <= 32);  // can't be AVX512
            constexpr size_t __bits_per_element = sizeof(_U) * CHAR_BIT;
            if constexpr (!__have_avx2 && __have_avx && sizeof(_V) == 32) {
                if constexpr (_N == 8) {
                    return _mm256_cmp_ps(
                        _mm256_and_ps(
                            _mm256_castsi256_ps(_mm256_set1_epi32(__k)),
                            _mm256_castsi256_ps(_mm256_setr_epi32(
                                0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80))),
                        _mm256_setzero_ps(), _CMP_NEQ_UQ);
                } else if constexpr (_N == 4) {
                    return _mm256_cmp_pd(
                        _mm256_and_pd(
                            _mm256_castsi256_pd(_mm256_set1_epi64x(__k)),
                            _mm256_castsi256_pd(
                                _mm256_setr_epi64x(0x01, 0x02, 0x04, 0x08))),
                        _mm256_setzero_pd(), _CMP_NEQ_UQ);
                } else {
                    __assert_unreachable<_To>();
                }
            } else if constexpr (__bits_per_element >= _N)
	      {
		constexpr auto __bitmask =
		  __generate_vector<__vector_type_t<_U, _N_out>>(
		    [](auto __i) constexpr->_U {
		      return __i < _N ? 1ull << __i : 0;
		    });
		return __vector_bitcast<_ToT>(
		  (__vector_broadcast<_N_out, _U>(__k) & __bitmask) != 0);
	      }
	    else if constexpr (sizeof(_V) == 16 && sizeof(_ToT) == 1 && __have_ssse3) {
                const auto __bitmask = __to_intrin(__make_vector<_UChar>(
                    1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128));
                return __vector_bitcast<_ToT>(
                    __vector_bitcast<_ToT>(
                        _mm_shuffle_epi8(
                            __to_intrin(__vector_type_t<_ULLong, 2>{__k}),
                            _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
                                          1)) &
                        __bitmask) != 0);
            } else if constexpr (sizeof(_V) == 32 && sizeof(_ToT) == 1 && __have_avx2) {
                const auto __bitmask =
                    _mm256_broadcastsi128_si256(__to_intrin(__make_vector<_UChar>(
                        1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128)));
                return __vector_bitcast<_ToT>(
                    __vector_bitcast<_ToT>(_mm256_shuffle_epi8(
                                        _mm256_broadcastsi128_si256(__to_intrin(
                                            __vector_type_t<_ULLong, 2>{__k})),
                                        _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
                                                         1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
                                                         2, 2, 3, 3, 3, 3, 3, 3, 3, 3)) &
                                    __bitmask) != 0);
                /* TODO:
                } else if constexpr (sizeof(_V) == 32 && sizeof(_ToT) == 2 && __have_avx2) {
                    constexpr auto __bitmask = _mm256_broadcastsi128_si256(
                        _mm_setr_epi8(0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000)); return
                __vector_bitcast<_ToT>( _mm256_shuffle_epi8(
                                   _mm256_broadcastsi128_si256(__m128i{__k}),
                                   _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
                1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3)) & __bitmask) != 0;
                */
            } else {
                const _V __tmp = __generate_vector<_V>([&](auto __i) constexpr {
                                  return static_cast<_U>(
                                      __k >> (__bits_per_element * (__i / __bits_per_element)));
                              }) &
                              __generate_vector<_V>([](auto __i) constexpr {
                                  return static_cast<_U>(1ull << (__i % __bits_per_element));
                              });  // mask bit index
                return __vector_bitcast<_ToT>(__tmp != _V());
            }
        } else
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
	  {
	    using _I = __int_for_sizeof_t<_ToT>;
	    return reinterpret_cast<_To>(
	      __generate_vector<__vector_type_t<_I, _N_out>>([&](auto __i) constexpr {
		return ((__k >> __i) & 1) == 0 ? _I() : ~_I();
	      }));
	  }
	// }}}
    } else if constexpr (__is_vector_type_v<_From> && std::is_unsigned_v<_To>) {
        // vector -> bits {{{
#if _GLIBCXX_SIMD_X86INTRIN // {{{
        using _Trait = _VectorTraits<_From>;
        using _Tp = typename _Trait::value_type;
        constexpr size_t _FromN = sizeof(_From) < 16 ? 16 / sizeof(_Tp) : _Trait::_S_width;
        constexpr size_t cvt_id = _FromN * 10 + sizeof(_Tp);
        constexpr bool __have_avx512_int = __have_avx512f && std::is_integral_v<_Tp>;
        [[maybe_unused]]  // PR85827
        const auto __intrin = __to_intrin(__k);

             if constexpr (cvt_id == 16'1 && __have_avx512bw_vl) { return    _mm_movepi8_mask(__intrin); }
        else if constexpr (cvt_id == 16'1 && __have_avx512bw   ) { return _mm512_movepi8_mask(__zero_extend(__intrin)); }
        else if constexpr (cvt_id == 16'1                      ) { return    _mm_movemask_epi8(__intrin); }
        else if constexpr (cvt_id == 32'1 && __have_avx512bw_vl) { return _mm256_movepi8_mask(__intrin); }
        else if constexpr (cvt_id == 32'1 && __have_avx512bw   ) { return _mm512_movepi8_mask(__zero_extend(__intrin)); }
        else if constexpr (cvt_id == 32'1                      ) { return _mm256_movemask_epi8(__intrin); }
        else if constexpr (cvt_id == 64'1 && __have_avx512bw   ) { return _mm512_movepi8_mask(__intrin); }
        else if constexpr (cvt_id ==  8'2 && __have_avx512bw_vl) { return    _mm_movepi16_mask(__intrin); }
        else if constexpr (cvt_id ==  8'2 && __have_avx512bw   ) { return _mm512_movepi16_mask(__zero_extend(__intrin)); }
        else if constexpr (cvt_id ==  8'2                      ) { return movemask_epi16(__intrin); }
        else if constexpr (cvt_id == 16'2 && __have_avx512bw_vl) { return _mm256_movepi16_mask(__intrin); }
        else if constexpr (cvt_id == 16'2 && __have_avx512bw   ) { return _mm512_movepi16_mask(__zero_extend(__intrin)); }
        else if constexpr (cvt_id == 16'2                      ) { return movemask_epi16(__intrin); }
        else if constexpr (cvt_id == 32'2 && __have_avx512bw   ) { return _mm512_movepi16_mask(__intrin); }
        else if constexpr (cvt_id ==  4'4 && __have_avx512dq_vl) { return    _mm_movepi32_mask(__intrin_bitcast<__m128i>(__k)); }
        else if constexpr (cvt_id ==  4'4 && __have_avx512dq   ) { return _mm512_movepi32_mask(__zero_extend(__intrin_bitcast<__m128i>(__k))); }
        else if constexpr (cvt_id ==  4'4 && __have_avx512vl   ) { return    _mm_cmp_epi32_mask(__intrin_bitcast<__m128i>(__k), __m128i(), _MM_CMPINT_LT); }
        else if constexpr (cvt_id ==  4'4 && __have_avx512_int ) { return _mm512_cmp_epi32_mask(__zero_extend(__intrin), __m512i(), _MM_CMPINT_LT); }
        else if constexpr (cvt_id ==  4'4                      ) { return    _mm_movemask_ps(__intrin); }
        else if constexpr (cvt_id ==  8'4 && __have_avx512dq_vl) { return _mm256_movepi32_mask(__vector_bitcast<_LLong>(__k)); }
        else if constexpr (cvt_id ==  8'4 && __have_avx512dq   ) { return _mm512_movepi32_mask(__zero_extend(__vector_bitcast<_LLong>(__k))); }
        else if constexpr (cvt_id ==  8'4 && __have_avx512vl   ) { return _mm256_cmp_epi32_mask(__vector_bitcast<_LLong>(__k), __m256i(), _MM_CMPINT_LT); }
        else if constexpr (cvt_id ==  8'4 && __have_avx512_int ) { return _mm512_cmp_epi32_mask(__zero_extend(__intrin), __m512i(), _MM_CMPINT_LT); }
        else if constexpr (cvt_id ==  8'4                      ) { return _mm256_movemask_ps(__intrin); }
        else if constexpr (cvt_id == 16'4 && __have_avx512dq   ) { return _mm512_movepi32_mask(__vector_bitcast<_LLong>(__k)); }
        else if constexpr (cvt_id == 16'4                      ) { return _mm512_cmp_epi32_mask(__vector_bitcast<_LLong>(__k), __m512i(), _MM_CMPINT_LT); }
        else if constexpr (cvt_id ==  2'8 && __have_avx512dq_vl) { return    _mm_movepi64_mask(__intrin_bitcast<__m128i>(__k)); }
        else if constexpr (cvt_id ==  2'8 && __have_avx512dq   ) { return _mm512_movepi64_mask(__zero_extend(__intrin_bitcast<__m128i>(__k))); }
        else if constexpr (cvt_id ==  2'8 && __have_avx512vl   ) { return    _mm_cmp_epi64_mask(__intrin_bitcast<__m128i>(__k), __m128i(), _MM_CMPINT_LT); }
        else if constexpr (cvt_id ==  2'8 && __have_avx512_int ) { return _mm512_cmp_epi64_mask(__zero_extend(__intrin), __m512i(), _MM_CMPINT_LT); }
        else if constexpr (cvt_id ==  2'8                      ) { return    _mm_movemask_pd(__intrin); }
        else if constexpr (cvt_id ==  4'8 && __have_avx512dq_vl) { return _mm256_movepi64_mask(__vector_bitcast<_LLong>(__k)); }
        else if constexpr (cvt_id ==  4'8 && __have_avx512dq   ) { return _mm512_movepi64_mask(__zero_extend(__vector_bitcast<_LLong>(__k))); }
        else if constexpr (cvt_id ==  4'8 && __have_avx512vl   ) { return _mm256_cmp_epi64_mask(__vector_bitcast<_LLong>(__k), __m256i(), _MM_CMPINT_LT); }
        else if constexpr (cvt_id ==  4'8 && __have_avx512_int ) { return _mm512_cmp_epi64_mask(__zero_extend(__intrin), __m512i(), _MM_CMPINT_LT); }
        else if constexpr (cvt_id ==  4'8                      ) { return _mm256_movemask_pd(__intrin); }
        else if constexpr (cvt_id ==  8'8 && __have_avx512dq   ) { return _mm512_movepi64_mask(__vector_bitcast<_LLong>(__k)); }
        else if constexpr (cvt_id ==  8'8                      ) { return _mm512_cmp_epi64_mask(__vector_bitcast<_LLong>(__k), __m512i(), _MM_CMPINT_LT); }
        else
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
        __assert_unreachable<_To>();
        // }}}
    } else if constexpr (__is_vector_type_v<_From> && __is_vector_type_v<_To>) {
        // vector -> vector {{{
        using _ToTrait = _VectorTraits<_To>;
        using _FromTrait = _VectorTraits<_From>;
        using _ToT = typename _ToTrait::value_type;
        using _Tp = typename _FromTrait::value_type;
        [[maybe_unused]] constexpr size_t _FromN = _FromTrait::_S_width;
        constexpr size_t _ToN = _ToTrait::_S_width;
        constexpr int _FromBytes = sizeof(_Tp);
        constexpr int _ToBytes = sizeof(_ToT);

	if constexpr (_FromBytes == _ToBytes)
	  return __intrin_bitcast<_To>(__k);
#if _GLIBCXX_SIMD_X86INTRIN // {{{
        else if constexpr (sizeof(_To) == 16 && sizeof(__k) == 16)
	{ // SSE -> SSE {{{
            if constexpr (_FromBytes == 4 && _ToBytes == 8) {
                if constexpr(std::is_integral_v<_Tp>) {
                    return __vector_bitcast<_ToT>(__interleave128_lo(__k, __k));
                } else {
                    return __vector_bitcast<_ToT>(__interleave128_lo(__k, __k));
                }
            } else if constexpr (_FromBytes == 2 && _ToBytes == 8) {
                const auto __y = __vector_bitcast<int>(__interleave128_lo(__k, __k));
                return __vector_bitcast<_ToT>(__interleave128_lo(__y, __y));
            } else if constexpr (_FromBytes == 1 && _ToBytes == 8) {
                auto __y = __vector_bitcast<short>(__interleave128_lo(__k, __k));
                auto __z = __vector_bitcast<int>(__interleave128_lo(__y, __y));
                return __vector_bitcast<_ToT>(__interleave128_lo(__z, __z));
            } else if constexpr (_FromBytes == 8 && _ToBytes == 4) {
		if constexpr (__have_sse2)
		  return __vector_bitcast<_ToT>(
		    _mm_packs_epi32(__vector_bitcast<_LLong>(__k), __m128i()));
		else
		  return __vector_shuffle<1, 3, 6, 7>(
		    __vector_bitcast<_ToT>(__k), _To());
	    } else if constexpr (_FromBytes == 2 && _ToBytes == 4) {
                return __vector_bitcast<_ToT>(__interleave128_lo(__k, __k));
            } else if constexpr (_FromBytes == 1 && _ToBytes == 4) {
                const auto __y = __vector_bitcast<short>(__interleave128_lo(__k, __k));
                return __vector_bitcast<_ToT>(__interleave128_lo(__y, __y));
            } else if constexpr (_FromBytes == 8 && _ToBytes == 2) {
		if constexpr (__have_sse2 && !__have_ssse3)
		  return __vector_bitcast<_ToT>(_mm_packs_epi32(
		    _mm_packs_epi32(__vector_bitcast<_LLong>(__k), __m128i()),
		    __m128i()));
		else
		  return __vector_permute<3, 7, -1, -1, -1, -1, -1, -1>(
		    __vector_bitcast<_ToT>(__k));
	    } else if constexpr (_FromBytes == 4 && _ToBytes == 2) {
                return __vector_bitcast<_ToT>(
                    _mm_packs_epi32(__vector_bitcast<_LLong>(__k), __m128i()));
            } else if constexpr (_FromBytes == 1 && _ToBytes == 2) {
                return __vector_bitcast<_ToT>(__interleave128_lo(__k, __k));
            } else if constexpr (_FromBytes == 8 && _ToBytes == 1) {
                if constexpr(__have_ssse3) {
                    return __vector_bitcast<_ToT>(
                        _mm_shuffle_epi8(__vector_bitcast<_LLong>(__k),
                                         _mm_setr_epi8(7, 15, -1, -1, -1, -1, -1, -1, -1,
                                                       -1, -1, -1, -1, -1, -1, -1)));
                } else {
                    auto __y = _mm_packs_epi32(__vector_bitcast<_LLong>(__k), __m128i());
                    __y = _mm_packs_epi32(__y, __m128i());
                    return __vector_bitcast<_ToT>(_mm_packs_epi16(__y, __m128i()));
                }
		return __vector_permute<7, 15, -1, -1, -1, -1, -1, -1, -1, -1,
					-1, -1, -1, -1, -1, -1>(
		  __vector_bitcast<_ToT>(__k));
	    } else if constexpr (_FromBytes == 4 && _ToBytes == 1) {
                if constexpr(__have_ssse3) {
                    return __vector_bitcast<_ToT>(
                        _mm_shuffle_epi8(__vector_bitcast<_LLong>(__k),
                                         _mm_setr_epi8(3, 7, 11, 15, -1, -1, -1, -1, -1,
                                                       -1, -1, -1, -1, -1, -1, -1)));
                } else {
                    const auto __y = _mm_packs_epi32(__vector_bitcast<_LLong>(__k), __m128i());
                    return __vector_bitcast<_ToT>(_mm_packs_epi16(__y, __m128i()));
                }
		return __vector_permute<3, 7, 11, 15, -1, -1, -1, -1, -1, -1,
					-1, -1, -1, -1, -1, -1>(
		  __vector_bitcast<_ToT>(__k));
	    } else if constexpr (_FromBytes == 2 && _ToBytes == 1) {
                return __vector_bitcast<_ToT>(_mm_packs_epi16(__vector_bitcast<_LLong>(__k), __m128i()));
	    } else {
                static_assert(!std::is_same_v<_Tp, _Tp>, "should be unreachable");
            }
	  } // }}}
	else if constexpr (sizeof(_To) == 32 && sizeof(__k) == 32)
	  { // AVX -> AVX {{{
            if constexpr (_FromBytes == _ToBytes) {  // keep low 1/2
                __assert_unreachable<_Tp>();
            } else if constexpr (_FromBytes == _ToBytes * 2) {
                const auto __y = __vector_bitcast<_LLong>(__k);
                return __vector_bitcast<_ToT>(
                    _mm256_castsi128_si256(_mm_packs_epi16(__lo128(__y), __hi128(__y))));
            } else if constexpr (_FromBytes == _ToBytes * 4) {
                const auto __y = __vector_bitcast<_LLong>(__k);
                return __vector_bitcast<_ToT>(_mm256_castsi128_si256(
                    _mm_packs_epi16(_mm_packs_epi16(__lo128(__y), __hi128(__y)), __m128i())));
            } else if constexpr (_FromBytes == _ToBytes * 8) {
                const auto __y = __vector_bitcast<_LLong>(__k);
                return __vector_bitcast<_ToT>(_mm256_castsi128_si256(
                    _mm_shuffle_epi8(_mm_packs_epi16(__lo128(__y), __hi128(__y)),
                                     _mm_setr_epi8(3, 7, 11, 15, -1, -1, -1, -1, -1, -1,
                                                   -1, -1, -1, -1, -1, -1))));
            } else if constexpr (_FromBytes * 2 == _ToBytes) {
                auto __y = __xzyw(__to_intrin(__k));
                if constexpr(std::is_floating_point_v<_Tp>) {
                    return __vector_bitcast<_ToT>(_mm256_unpacklo_ps(__y, __y));
                } else {
                    return __vector_bitcast<_ToT>(_mm256_unpacklo_epi8(__y, __y));
                }
            } else if constexpr (_FromBytes * 4 == _ToBytes) {
                auto __y = _mm_unpacklo_epi8(__lo128(__vector_bitcast<_LLong>(__k)),
                                           __lo128(__vector_bitcast<_LLong>(__k)));  // drops 3/4 of input
                return __vector_bitcast<_ToT>(
                    __concat(_mm_unpacklo_epi16(__y, __y), _mm_unpackhi_epi16(__y, __y)));
            } else if constexpr (_FromBytes == 1 && _ToBytes == 8) {
                auto __y = _mm_unpacklo_epi8(__lo128(__vector_bitcast<_LLong>(__k)),
                                           __lo128(__vector_bitcast<_LLong>(__k)));  // drops 3/4 of input
                __y = _mm_unpacklo_epi16(__y, __y);  // drops another 1/2 => 7/8 total
                return __vector_bitcast<_ToT>(
                    __concat(_mm_unpacklo_epi32(__y, __y), _mm_unpackhi_epi32(__y, __y)));
            } else {
                __assert_unreachable<_Tp>();
            }
	  } // }}}
	else if constexpr (sizeof(_To) == 32 && sizeof(__k) == 16)
	  { // SSE -> AVX {{{
            if constexpr (_FromBytes == _ToBytes) {
                return __vector_bitcast<_ToT>(
                    __intrinsic_type_t<_Tp, 32 / sizeof(_Tp)>(__zero_extend(__to_intrin(__k))));
            } else if constexpr (_FromBytes * 2 == _ToBytes) {  // keep all
                return __vector_bitcast<_ToT>(__concat(_mm_unpacklo_epi8(__vector_bitcast<_LLong>(__k), __vector_bitcast<_LLong>(__k)),
                                         _mm_unpackhi_epi8(__vector_bitcast<_LLong>(__k), __vector_bitcast<_LLong>(__k))));
            } else if constexpr (_FromBytes * 4 == _ToBytes) {
                if constexpr (__have_avx2) {
                    return __vector_bitcast<_ToT>(_mm256_shuffle_epi8(
                        __concat(__vector_bitcast<_LLong>(__k), __vector_bitcast<_LLong>(__k)),
                        _mm256_setr_epi8(0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
                                         4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7,
                                         7)));
                } else {
                    return __vector_bitcast<_ToT>(
                        __concat(_mm_shuffle_epi8(__vector_bitcast<_LLong>(__k),
                                                _mm_setr_epi8(0, 0, 0, 0, 1, 1, 1, 1, 2,
                                                              2, 2, 2, 3, 3, 3, 3)),
                               _mm_shuffle_epi8(__vector_bitcast<_LLong>(__k),
                                                _mm_setr_epi8(4, 4, 4, 4, 5, 5, 5, 5, 6,
                                                              6, 6, 6, 7, 7, 7, 7))));
                }
            } else if constexpr (_FromBytes * 8 == _ToBytes) {
                if constexpr (__have_avx2) {
                    return __vector_bitcast<_ToT>(_mm256_shuffle_epi8(
                        __concat(__vector_bitcast<_LLong>(__k), __vector_bitcast<_LLong>(__k)),
                        _mm256_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                                         2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3,
                                         3)));
                } else {
                    return __vector_bitcast<_ToT>(
                        __concat(_mm_shuffle_epi8(__vector_bitcast<_LLong>(__k),
                                                _mm_setr_epi8(0, 0, 0, 0, 0, 0, 0, 0, 1,
                                                              1, 1, 1, 1, 1, 1, 1)),
                               _mm_shuffle_epi8(__vector_bitcast<_LLong>(__k),
                                                _mm_setr_epi8(2, 2, 2, 2, 2, 2, 2, 2, 3,
                                                              3, 3, 3, 3, 3, 3, 3))));
                }
            } else if constexpr (_FromBytes == _ToBytes * 2) {
                return __vector_bitcast<_ToT>(
                    __m256i(__zero_extend(_mm_packs_epi16(__vector_bitcast<_LLong>(__k), __m128i()))));
            } else if constexpr (_FromBytes == 8 && _ToBytes == 2) {
                return __vector_bitcast<_ToT>(__m256i(__zero_extend(
                    _mm_shuffle_epi8(__vector_bitcast<_LLong>(__k),
                                     _mm_setr_epi8(6, 7, 14, 15, -1, -1, -1, -1, -1, -1,
                                                   -1, -1, -1, -1, -1, -1)))));
            } else if constexpr (_FromBytes == 4 && _ToBytes == 1) {
                return __vector_bitcast<_ToT>(__m256i(__zero_extend(
                    _mm_shuffle_epi8(__vector_bitcast<_LLong>(__k),
                                     _mm_setr_epi8(3, 7, 11, 15, -1, -1, -1, -1, -1, -1,
                                                   -1, -1, -1, -1, -1, -1)))));
            } else if constexpr (_FromBytes == 8 && _ToBytes == 1) {
                return __vector_bitcast<_ToT>(__m256i(__zero_extend(
                    _mm_shuffle_epi8(__vector_bitcast<_LLong>(__k),
                                     _mm_setr_epi8(7, 15, -1, -1, -1, -1, -1, -1, -1, -1,
                                                   -1, -1, -1, -1, -1, -1)))));
            } else {
                static_assert(!std::is_same_v<_Tp, _Tp>, "should be unreachable");
            }
	  } // }}}
	else if constexpr (sizeof(_To) == 16 && sizeof(__k) == 32)
	  { // AVX -> SSE {{{
            if constexpr (_FromBytes == _ToBytes) {  // keep low 1/2
                return __vector_bitcast<_ToT>(__lo128(__k));
            } else if constexpr (_FromBytes == _ToBytes * 2) {  // keep all
                auto __y = __vector_bitcast<_LLong>(__k);
                return __vector_bitcast<_ToT>(_mm_packs_epi16(__lo128(__y), __hi128(__y)));
            } else if constexpr (_FromBytes == _ToBytes * 4) {  // add 1/2 undef
                auto __y = __vector_bitcast<_LLong>(__k);
                return __vector_bitcast<_ToT>(
                    _mm_packs_epi16(_mm_packs_epi16(__lo128(__y), __hi128(__y)), __m128i()));
            } else if constexpr (_FromBytes == 8 && _ToBytes == 1) {  // add 3/4 undef
                auto __y = __vector_bitcast<_LLong>(__k);
                return __vector_bitcast<_ToT>(
                    _mm_shuffle_epi8(_mm_packs_epi16(__lo128(__y), __hi128(__y)),
                                     _mm_setr_epi8(3, 7, 11, 15, -1, -1, -1, -1, -1, -1,
                                                   -1, -1, -1, -1, -1, -1)));
            } else if constexpr (_FromBytes * 2 == _ToBytes) {  // keep low 1/4
                auto __y = __lo128(__vector_bitcast<_LLong>(__k));
                return __vector_bitcast<_ToT>(_mm_unpacklo_epi8(__y, __y));
            } else if constexpr (_FromBytes * 4 == _ToBytes) {  // keep low 1/8
                auto __y = __lo128(__vector_bitcast<_LLong>(__k));
                __y = _mm_unpacklo_epi8(__y, __y);
                return __vector_bitcast<_ToT>(_mm_unpacklo_epi8(__y, __y));
            } else if constexpr (_FromBytes * 8 == _ToBytes) {  // keep low 1/16
                auto __y = __lo128(__vector_bitcast<_LLong>(__k));
                __y = _mm_unpacklo_epi8(__y, __y);
                __y = _mm_unpacklo_epi8(__y, __y);
                return __vector_bitcast<_ToT>(_mm_unpacklo_epi8(__y, __y));
            } else {
                static_assert(!std::is_same_v<_Tp, _Tp>, "should be unreachable");
            }
	  } // }}}
#endif // _GLIBCXX_SIMD_X86INTRIN }}}
	else
	  {
	    using _I = __int_for_sizeof_t<_ToT>;
	    return reinterpret_cast<_To>(
	      __generate_vector<__vector_type_t<_I, _ToN>>([&](
		auto __i) constexpr {
		if constexpr (__i >= _FromN)
		  return _I();
		else
		  return __k[int(__i)] == 0 ? _I() : ~_I();
	      }));
	  }
	/*
        } else if constexpr (_FromBytes > _ToBytes) {
	    const _To     __y      = __vector_bitcast<_ToT>(__k);
	    return [&] <std::size_t... _Is> (std::index_sequence<_Is...>) {
	      constexpr int _Stride = _FromBytes / _ToBytes;
	      return _To{__y[(_Is + 1) * _Stride - 1]...};
	    }(std::make_index_sequence<std::min(_ToN, _FromN)>());
	} else {
	    // {0, 0, 1, 1} (_Dups = 2, _Is<4>)
	    // {0, 0, 0, 0, 1, 1, 1, 1} (_Dups = 4, _Is<8>)
	    // {0, 0, 1, 1, 2, 2, 3, 3} (_Dups = 2, _Is<8>)
	    // ...
	    return [&] <std::size_t... _Is> (std::index_sequence<_Is...>) {
	      constexpr int __dup = _ToBytes / _FromBytes;
	      return __vector_bitcast<_ToT>(_From{__k[_Is / __dup]...});
	    }(std::make_index_sequence<_FromN>());
	}
	*/
        // }}}
    } else {
        __assert_unreachable<_To>();
    }
}

// }}}

template <class _Abi> struct _SimdMathFallback {  //{{{
  using _SuperImpl = typename _Abi::_SimdImpl;

#define _GLIBCXX_SIMD_MATH_FALLBACK(__name)                                    \
  template <typename _Tp, typename... _More>                                   \
  static _Tp __##__name(const _Tp& __x, const _More&... __more)                \
  {                                                                            \
    if constexpr ((__is_vectorizable_v<_Tp> && ... &&                          \
		   __is_vectorizable_v<_More>))                                \
      return std::__name(__x, __more...);                                      \
    else if constexpr (__is_vectorizable_v<_Tp>)                               \
      return std::__name(__x, __more[0]...);                                   \
    else                                                                       \
      return __generate_vector<_Tp>(                                           \
	[&](auto __i) { return std::__name(__x[__i], __more[__i]...); });      \
  }

#define _GLIBCXX_SIMD_MATH_FALLBACK_MASKRET(__name)                            \
  template <typename _Tp, typename... _More>                                   \
  static                                                                       \
    typename _Tp::mask_type __##__name(const _Tp& __x, const _More&... __more) \
  {                                                                            \
    if constexpr ((__is_vectorizable_v<_Tp> && ... &&                          \
		   __is_vectorizable_v<_More>))                                \
      return std::__name(__x, __more...);                                      \
    else if constexpr (__is_vectorizable_v<_Tp>)                               \
      return std::__name(__x, __more[0]...);                                   \
    else                                                                       \
      return __generate_vector<_Tp>(                                           \
	[&](auto __i) { return std::__name(__x[__i], __more[__i]...); });      \
  }

#define _GLIBCXX_SIMD_MATH_FALLBACK_FIXEDRET(_RetTp, __name)                   \
  template <typename _Tp, typename... _More>                                   \
  static auto __##__name(const _Tp& __x, const _More&... __more)               \
  {                                                                            \
    if constexpr (__is_vectorizable_v<_Tp>)                                    \
      return _SimdTuple<_RetTp, simd_abi::scalar>{                             \
	std::__name(__x, __more...)};                                          \
    else                                                                       \
      return __fixed_size_storage_t<_RetTp,                                    \
				    _VectorTraits<_Tp>::_S_partial_width>::    \
	__generate([&](auto __meta) constexpr {                                \
	  return __meta.__generator(                                           \
	    [&](auto __i) {                                                    \
	      return std::__name(__x[__meta._S_offset + __i],                  \
				 __more[__meta._S_offset + __i]...);           \
	    },                                                                 \
	    static_cast<_RetTp*>(nullptr));                                    \
	});                                                                    \
  }

  _GLIBCXX_SIMD_MATH_FALLBACK(acos)
  _GLIBCXX_SIMD_MATH_FALLBACK(asin)
  _GLIBCXX_SIMD_MATH_FALLBACK(atan)
  _GLIBCXX_SIMD_MATH_FALLBACK(atan2)
  _GLIBCXX_SIMD_MATH_FALLBACK(cos)
  _GLIBCXX_SIMD_MATH_FALLBACK(sin)
  _GLIBCXX_SIMD_MATH_FALLBACK(tan)
  _GLIBCXX_SIMD_MATH_FALLBACK(acosh)
  _GLIBCXX_SIMD_MATH_FALLBACK(asinh)
  _GLIBCXX_SIMD_MATH_FALLBACK(atanh)
  _GLIBCXX_SIMD_MATH_FALLBACK(cosh)
  _GLIBCXX_SIMD_MATH_FALLBACK(sinh)
  _GLIBCXX_SIMD_MATH_FALLBACK(tanh)
  _GLIBCXX_SIMD_MATH_FALLBACK(exp)
  _GLIBCXX_SIMD_MATH_FALLBACK(exp2)
  _GLIBCXX_SIMD_MATH_FALLBACK(expm1)
  _GLIBCXX_SIMD_MATH_FALLBACK(ldexp)
  _GLIBCXX_SIMD_MATH_FALLBACK_FIXEDRET(int, ilogb)
  _GLIBCXX_SIMD_MATH_FALLBACK(log)
  _GLIBCXX_SIMD_MATH_FALLBACK(log10)
  _GLIBCXX_SIMD_MATH_FALLBACK(log1p)
  _GLIBCXX_SIMD_MATH_FALLBACK(log2)
  _GLIBCXX_SIMD_MATH_FALLBACK(logb)

  //modf implemented in simd_math.h
  _GLIBCXX_SIMD_MATH_FALLBACK(scalbn)
  _GLIBCXX_SIMD_MATH_FALLBACK(scalbln)
  _GLIBCXX_SIMD_MATH_FALLBACK(cbrt)
  _GLIBCXX_SIMD_MATH_FALLBACK(abs)
  _GLIBCXX_SIMD_MATH_FALLBACK(fabs)
  _GLIBCXX_SIMD_MATH_FALLBACK(pow)
  _GLIBCXX_SIMD_MATH_FALLBACK(sqrt)
  _GLIBCXX_SIMD_MATH_FALLBACK(erf)
  _GLIBCXX_SIMD_MATH_FALLBACK(erfc)
  _GLIBCXX_SIMD_MATH_FALLBACK(lgamma)
  _GLIBCXX_SIMD_MATH_FALLBACK(tgamma)
  _GLIBCXX_SIMD_MATH_FALLBACK(ceil)
  _GLIBCXX_SIMD_MATH_FALLBACK(floor)
  _GLIBCXX_SIMD_MATH_FALLBACK(nearbyint)

  _GLIBCXX_SIMD_MATH_FALLBACK(rint)
  _GLIBCXX_SIMD_MATH_FALLBACK_FIXEDRET(long, lrint)
  _GLIBCXX_SIMD_MATH_FALLBACK_FIXEDRET(long long, llrint)

  _GLIBCXX_SIMD_MATH_FALLBACK(round)
  _GLIBCXX_SIMD_MATH_FALLBACK_FIXEDRET(long, lround)
  _GLIBCXX_SIMD_MATH_FALLBACK_FIXEDRET(long long, llround)

  _GLIBCXX_SIMD_MATH_FALLBACK(trunc)
  _GLIBCXX_SIMD_MATH_FALLBACK(fmod)
  _GLIBCXX_SIMD_MATH_FALLBACK(remainder)

  template <typename _Tp, typename = std::enable_if_t<__is_vectorizable_v<_Tp>>>
  static _Tp __remquo(const _Tp                          __x,
		      const _Tp                          __y,
		      _SimdTuple<int, simd_abi::scalar>* __z)
  {
    return std::remquo(__x, __y, &__z->first);
  }

  template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
  static _Tp __remquo(const _Tp                                   __x,
		      const _Tp                                   __y,
		      __fixed_size_storage_t<int, _TVT::_S_partial_width>* __z)
  {
    return __generate_vector<_Tp>([&](auto __i) {
      int  __tmp;
      auto __r    = std::remquo(__x[__i], __y[__i], &__tmp);
      __z->__set(__i, __tmp);
      return __r;
    });
  }

  // copysign in simd_math.h
  _GLIBCXX_SIMD_MATH_FALLBACK(nextafter)
  _GLIBCXX_SIMD_MATH_FALLBACK(fdim)
  _GLIBCXX_SIMD_MATH_FALLBACK(fmax)
  _GLIBCXX_SIMD_MATH_FALLBACK(fmin)
  _GLIBCXX_SIMD_MATH_FALLBACK(fma)
  _GLIBCXX_SIMD_MATH_FALLBACK_FIXEDRET(int, fpclassify)

  template <class _Tp>
  _GLIBCXX_SIMD_INTRINSIC simd_mask<_Tp, _Abi>
			  __isfinite(const simd<_Tp, _Abi>& __x)
  {
    return simd<_Tp, _Abi>([&](auto __i) { return std::isfinite(__x[__i]); });
  }

  template <class _Tp>
  _GLIBCXX_SIMD_INTRINSIC simd_mask<_Tp, _Abi>
			  __isinf(const simd<_Tp, _Abi>& __x)
  {
    return simd<_Tp, _Abi>([&](auto __i) { return std::isinf(__x[__i]); });
  }

  template <class _Tp>
  _GLIBCXX_SIMD_INTRINSIC simd_mask<_Tp, _Abi>
			  __isnan(const simd<_Tp, _Abi>& __x)
  {
    return simd<_Tp, _Abi>([&](auto __i) { return std::isnan(__x[__i]); });
  }

  template <class _Tp>
  _GLIBCXX_SIMD_INTRINSIC simd_mask<_Tp, _Abi>
			  __isnormal(const simd<_Tp, _Abi>& __x)
  {
    return simd<_Tp, _Abi>([&](auto __i) { return std::isnormal(__x[__i]); });
  }

  template <class _Tp>
  _GLIBCXX_SIMD_INTRINSIC simd_mask<_Tp, _Abi>
			  __signbit(const simd<_Tp, _Abi>& __x)
  {
    return simd<_Tp, _Abi>([&](auto __i) { return std::signbit(__x[__i]); });
  }

  template <typename _Tp>
  _GLIBCXX_SIMD_INTRINSIC static constexpr auto
    __isgreater(const _Tp& __x, const _Tp& __y)
  {
    return _SuperImpl::__less(__y, __x);
  }
  template <typename _Tp>
  _GLIBCXX_SIMD_INTRINSIC static constexpr auto
    __isgreaterequal(const _Tp& __x, const _Tp& __y)
  {
    return _SuperImpl::__less_equal(__y, __x);
  }
  template <typename _Tp>
  _GLIBCXX_SIMD_INTRINSIC static constexpr auto
    __isless(const _Tp& __x, const _Tp& __y)
  {
    return _SuperImpl::__less(__x, __y);
  }
  template <typename _Tp>
  _GLIBCXX_SIMD_INTRINSIC static constexpr auto
    __islessequal(const _Tp& __x, const _Tp& __y)
  {
    return _SuperImpl::__less_equal(__x, __y);
  }
  template <typename _Tp>
  _GLIBCXX_SIMD_INTRINSIC static constexpr auto
    __islessgreater(const _Tp& __x, const _Tp& __y)
  {
    return __or(_SuperImpl::__less(__y, __x), _SuperImpl::__less(__x, __y));
  }

  template <typename _Tp>
  _GLIBCXX_SIMD_INTRINSIC static constexpr auto
    __isunordered(const _Tp& __x, const _Tp& __y)
  {
    return __cmpunord(__x, __y);
  }

#undef _GLIBCXX_SIMD_MATH_FALLBACK
#undef _GLIBCXX_SIMD_MATH_FALLBACK_FIXEDRET
};  // }}}
// _SimdImplScalar {{{
struct _SimdImplScalar : _SimdMathFallback<simd_abi::scalar> {
    // member types {{{2
    using abi_type = std::experimental::simd_abi::scalar;
    using _MaskMember = bool;
    template <class _Tp> using _SimdMember = _Tp;
    template <class _Tp> using _Simd = std::experimental::simd<_Tp, abi_type>;
    template <class _Tp> using _TypeTag = _Tp *;

    // broadcast {{{2
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp __broadcast(_Tp __x) noexcept
    {
        return __x;
    }

    // __generator {{{2
    template <class _F, class _Tp>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _Tp
      __generator(_F&& __gen, _TypeTag<_Tp>)
    {
        return __gen(_SizeConstant<0>());
    }

    // __load {{{2
    template <class _Tp, class _U, class _F>
    static inline _Tp __load(const _U *__mem, _F, _TypeTag<_Tp>) noexcept
    {
        return static_cast<_Tp>(__mem[0]);
    }

    // __masked_load {{{2
    template <class _Tp, class _U, class _F>
    static inline _Tp __masked_load(_Tp __merge, bool __k, const _U *__mem, _F) noexcept
    {
        if (__k) {
            __merge = static_cast<_Tp>(__mem[0]);
        }
        return __merge;
    }

    // __store {{{2
    template <class _Tp, class _U, class _F>
    static inline void __store(_Tp __v, _U *__mem, _F, _TypeTag<_Tp>) noexcept
    {
        __mem[0] = static_cast<_Tp>(__v);
    }

    // __masked_store {{{2
    template <class _Tp, class _U, class _F>
    static inline void __masked_store(const _Tp __v, _U *__mem, _F, const bool __k) noexcept
    {
        if (__k) {
            __mem[0] = __v;
        }
    }

    // __negate {{{2
    template <class _Tp> static inline bool __negate(_Tp __x) noexcept { return !__x; }

    // __reduce {{{2
    template <class _Tp, class _BinaryOperation>
    static inline _Tp __reduce(const _Simd<_Tp> &__x, _BinaryOperation &)
    {
        return __x._M_data;
    }

    // __min, __max {{{2
    template <class _Tp> static inline _Tp __min(const _Tp __a, const _Tp __b)
    {
        return std::min(__a, __b);
    }

    template <class _Tp> static inline _Tp __max(const _Tp __a, const _Tp __b)
    {
        return std::max(__a, __b);
    }

    // __complement {{{2
    template <class _Tp> static inline _Tp __complement(_Tp __x) noexcept
    {
        return static_cast<_Tp>(~__x);
    }

    // __unary_minus {{{2
    template <class _Tp> static inline _Tp __unary_minus(_Tp __x) noexcept
    {
        return static_cast<_Tp>(-__x);
    }

    // arithmetic operators {{{2
    template <class _Tp> static inline _Tp __plus(_Tp __x, _Tp __y)
    {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x) +
                              __promote_preserving_unsigned(__y));
    }

    template <class _Tp> static inline _Tp __minus(_Tp __x, _Tp __y)
    {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x) -
                              __promote_preserving_unsigned(__y));
    }

    template <class _Tp> static inline constexpr _Tp __multiplies(_Tp __x, _Tp __y)
    {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x) *
                              __promote_preserving_unsigned(__y));
    }

    template <class _Tp> static inline _Tp __divides(_Tp __x, _Tp __y)
    {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x) /
                              __promote_preserving_unsigned(__y));
    }

    template <class _Tp> static inline _Tp __modulus(_Tp __x, _Tp __y)
    {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x) %
                              __promote_preserving_unsigned(__y));
    }

    template <class _Tp>
    static inline _Tp __bit_and(_Tp __x, _Tp __y)
    {
      if constexpr (is_floating_point_v<_Tp>)
	{
	  using _I     = __int_for_sizeof_t<_Tp>;
	  const _I __r = reinterpret_cast<const __may_alias<_I>&>(__x) &
			 reinterpret_cast<const __may_alias<_I>&>(__y);
	  return reinterpret_cast<const __may_alias<_Tp>&>(__r);
	}
      else
	{
	  return static_cast<_Tp>(__promote_preserving_unsigned(__x) &
				 __promote_preserving_unsigned(__y));
	}
    }

    template <class _Tp>
    static inline _Tp __bit_or(_Tp __x, _Tp __y)
    {
      if constexpr (is_floating_point_v<_Tp>)
	{
	  using _I     = __int_for_sizeof_t<_Tp>;
	  const _I __r = reinterpret_cast<const __may_alias<_I>&>(__x) |
			 reinterpret_cast<const __may_alias<_I>&>(__y);
	  return reinterpret_cast<const __may_alias<_Tp>&>(__r);
	}
      else
	{
	  return static_cast<_Tp>(__promote_preserving_unsigned(__x) |
				 __promote_preserving_unsigned(__y));
	}
    }

    template <class _Tp>
    static inline _Tp __bit_xor(_Tp __x, _Tp __y)
    {
      if constexpr (is_floating_point_v<_Tp>)
	{
	  using _I     = __int_for_sizeof_t<_Tp>;
	  const _I __r = reinterpret_cast<const __may_alias<_I>&>(__x) ^
			 reinterpret_cast<const __may_alias<_I>&>(__y);
	  return reinterpret_cast<const __may_alias<_Tp>&>(__r);
	}
      else
	{
	  return static_cast<_Tp>(__promote_preserving_unsigned(__x) ^
				 __promote_preserving_unsigned(__y));
	}
    }

    template <class _Tp> static inline _Tp __bit_shift_left(_Tp __x, int __y)
    {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x) << __y);
    }

    template <class _Tp> static inline _Tp __bit_shift_right(_Tp __x, int __y)
    {
        return static_cast<_Tp>(__promote_preserving_unsigned(__x) >> __y);
    }

    // math {{{2
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static _Tp __abs(_Tp __x) { return _Tp(std::abs(__x)); }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static _Tp __sqrt(_Tp __x) { return std::sqrt(__x); }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static _Tp __trunc(_Tp __x) { return std::trunc(__x); }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static _Tp __floor(_Tp __x) { return std::floor(__x); }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static _Tp __ceil(_Tp __x) { return std::ceil(__x); }

    template <typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC static _Tp
      __remquo(_Tp __x, _Tp __y, _SimdTuple<int, simd_abi::_ScalarAbi>* __z)
    {
      return std::remquo(__x, __y, &__z->first);
    }
    template <typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC static _Tp __remquo(_Tp __x, _Tp __y, int* __z)
    {
      return std::remquo(__x, __y, __z);
    }

    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static _SimdTuple<int, abi_type> __fpclassify(_Tp __x)
    {
        return {std::fpclassify(__x)};
    }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static bool __isfinite(_Tp __x) { return std::isfinite(__x); }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static bool __isinf(_Tp __x) { return std::isinf(__x); }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static bool __isnan(_Tp __x) { return std::isnan(__x); }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static bool __isnormal(_Tp __x) { return std::isnormal(__x); }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static bool __signbit(_Tp __x) { return std::signbit(__x); }
    template <class _Tp> _GLIBCXX_SIMD_INTRINSIC static bool __isunordered(_Tp __x, _Tp __y) { return std::isunordered(__x, __y); }

    // __increment & __decrement{{{2
    template <class _Tp> static inline void __increment(_Tp &__x) { ++__x; }
    template <class _Tp> static inline void __decrement(_Tp &__x) { --__x; }

    // compares {{{2
    template <class _Tp> static bool __equal_to(_Tp __x, _Tp __y) { return __x == __y; }
    template <class _Tp> static bool __not_equal_to(_Tp __x, _Tp __y) { return __x != __y; }
    template <class _Tp> static bool __less(_Tp __x, _Tp __y) { return __x < __y; }
    template <class _Tp> static bool __less_equal(_Tp __x, _Tp __y) { return __x <= __y; }

    // smart_reference access {{{2
    template <class _Tp, class _U>
    static void __set(_Tp& __v, [[maybe_unused]] int __i, _U&& __x) noexcept
    {
      _GLIBCXX_DEBUG_ASSERT(__i == 0);
      __v = std::forward<_U>(__x);
    }

    // __masked_assign {{{2
    template <typename _Tp> _GLIBCXX_SIMD_INTRINSIC static void __masked_assign(bool __k, _Tp &__lhs, _Tp __rhs)
    {
        if (__k) {
            __lhs = __rhs;
        }
    }

    // __masked_cassign {{{2
    template <typename _Op, typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC static void
      __masked_cassign(const bool __k, _Tp& __lhs, const _Tp __rhs, _Op __op)
    {
      if (__k)
	__lhs = __op(_SimdImplScalar{}, __lhs, __rhs);
    }

    // __masked_unary {{{2
    template <template <typename> class _Op, typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC static _Tp __masked_unary(const bool __k, const _Tp __v)
    {
        return static_cast<_Tp>(__k ? _Op<_Tp>{}(__v) : __v);
    }

    // }}}2
};

// }}}
// _MaskImplScalar {{{
struct _MaskImplScalar {
    // member types {{{2
    template <class _Tp> using _TypeTag = _Tp *;

    // __from_bitset {{{2
    template <class _Tp>
    _GLIBCXX_SIMD_INTRINSIC static bool __from_bitset(std::bitset<1> __bs, _TypeTag<_Tp>) noexcept
    {
        return __bs[0];
    }

    // __masked_load {{{2
    template <class _F>
    _GLIBCXX_SIMD_INTRINSIC static bool __masked_load(bool __merge, bool __mask, const bool *__mem,
                                         _F) noexcept
    {
        if (__mask) {
            __merge = __mem[0];
        }
        return __merge;
    }

    // __store {{{2
    template <class _F> _GLIBCXX_SIMD_INTRINSIC static void __store(bool __v, bool *__mem, _F) noexcept
    {
        __mem[0] = __v;
    }

    // __masked_store {{{2
    template <class _F>
    _GLIBCXX_SIMD_INTRINSIC static void __masked_store(const bool __v, bool *__mem, _F,
                                          const bool __k) noexcept
    {
        if (__k) {
            __mem[0] = __v;
        }
    }

    // logical and bitwise operators {{{2
    static constexpr bool __logical_and(bool __x, bool __y) { return __x && __y; }
    static constexpr bool __logical_or(bool __x, bool __y) { return __x || __y; }
    static constexpr bool __bit_and(bool __x, bool __y) { return __x && __y; }
    static constexpr bool __bit_or(bool __x, bool __y) { return __x || __y; }
    static constexpr bool __bit_xor(bool __x, bool __y) { return __x != __y; }

    // smart_reference access {{{2
    static void __set(bool& __k, [[maybe_unused]] int __i, bool __x) noexcept
    {
      _GLIBCXX_DEBUG_ASSERT(__i == 0);
      __k = __x;
    }

    // __masked_assign {{{2
    _GLIBCXX_SIMD_INTRINSIC static void __masked_assign(bool __k, bool &__lhs, bool __rhs)
    {
        if (__k) {
            __lhs = __rhs;
        }
    }

    // }}}2
};

// }}}

// ISA & type detection {{{1
template <class _Tp, size_t _N>
constexpr bool __is_sse_ps()
{
  return __have_sse && std::is_same_v<_Tp, float> &&
	 sizeof(__intrinsic_type_t<_Tp, _N>) == 16;
}
template <class _Tp, size_t _N>
constexpr bool __is_sse_pd()
{
  return __have_sse2 && std::is_same_v<_Tp, double> &&
	 sizeof(__intrinsic_type_t<_Tp, _N>) == 16;
}
template <class _Tp, size_t _N>
constexpr bool __is_avx_ps()
{
  return __have_avx && std::is_same_v<_Tp, float> &&
	 sizeof(__intrinsic_type_t<_Tp, _N>) == 32;
}
template <class _Tp, size_t _N>
constexpr bool __is_avx_pd()
{
  return __have_avx && std::is_same_v<_Tp, double> &&
	 sizeof(__intrinsic_type_t<_Tp, _N>) == 32;
}
template <class _Tp, size_t _N>
constexpr bool __is_avx512_ps()
{
  return __have_avx512f && std::is_same_v<_Tp, float> &&
	 sizeof(__intrinsic_type_t<_Tp, _N>) == 64;
}
template <class _Tp, size_t _N>
constexpr bool __is_avx512_pd()
{
  return __have_avx512f && std::is_same_v<_Tp, double> &&
	 sizeof(__intrinsic_type_t<_Tp, _N>) == 64;
}

template <class _Tp, size_t _N> constexpr bool __is_neon_ps()
{
    constexpr auto _Bytes = sizeof(__vector_type_t<_Tp, _N>);
    return __have_neon && std::is_same_v<_Tp, float> && (_Bytes == 16 || _Bytes == 8);
}
template <class _Tp, size_t _N> constexpr bool __is_neon_pd()
{
    return __have_neon && std::is_same_v<_Tp, double> && sizeof(__vector_type_t<_Tp, _N>) == 16;
}

// _SimdImplBuiltin {{{1
template <class _Abi> struct _SimdImplBuiltin : _SimdMathFallback<_Abi> {
    // member types {{{2
    using abi_type = _Abi;
    template <class _Tp> using _TypeTag = _Tp *;
    template <class _Tp>
    using _SimdMember = typename _Abi::template __traits<_Tp>::_SimdMember;
    template <class _Tp>
    using _MaskMember = typename _Abi::template __traits<_Tp>::_MaskMember;
    template <class _Tp> static constexpr size_t _S_size = _Abi::template size<_Tp>;
    template <class _Tp> static constexpr size_t _S_full_size = _Abi::template _S_full_size<_Tp>;
    using _SuperImpl = typename _Abi::_SimdImpl;

    // __make_simd(_SimdWrapper/__intrinsic_type_t) {{{2
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static simd<_Tp, _Abi> __make_simd(_SimdWrapper<_Tp, _N> __x)
    {
        return {__private_init, __x};
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static simd<_Tp, _Abi> __make_simd(__intrinsic_type_t<_Tp, _N> __x)
    {
        return {__private_init, __vector_bitcast<_Tp>(__x)};
    }

    // __broadcast {{{2
    template <class _Tp>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdMember<_Tp> __broadcast(_Tp __x) noexcept
    {
        return __vector_broadcast<_S_full_size<_Tp>>(__x);
    }

    // __generator {{{2
    template <class _F, class _Tp>
    inline static constexpr _SimdMember<_Tp>
      __generator(_F&& __gen, _TypeTag<_Tp>)
    {
      return __generate_vector<_Tp, _S_full_size<_Tp>>([&](
	auto __i) constexpr {
	if constexpr (__i < _S_size<_Tp>)
	  return __gen(__i);
	else
	  return 0;
      });
    }

    // __load {{{2
    template <class _Tp, class _U, class _F>
    _GLIBCXX_SIMD_INTRINSIC static _SimdMember<_Tp>
      __load(const _U* __mem,
	     _F,
	     _TypeTag<_Tp>) _GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
    {
      constexpr size_t _N = _S_size<_Tp>;
      constexpr size_t __max_load_size =
	(sizeof(_U) >= 4 && __have_avx512f) || __have_avx512bw
	  ? 64
	  : (std::is_floating_point_v<_U> && __have_avx) || __have_avx2 ? 32
									: 16;
      constexpr size_t __bytes_to_load = sizeof(_U) * _N;
      if constexpr (sizeof(_U) > 8)
	return __generate_vector<_Tp, _SimdMember<_Tp>::_S_width>([&](
	  auto __i) constexpr {
	  return static_cast<_Tp>(__i < _N ? __mem[__i] : 0);
	});
      else if constexpr (std::is_same_v<_U, _Tp>)
	return __vector_load<_Tp, _S_full_size<_Tp>, _N * sizeof(_Tp)>(__mem,
								       _F());
      else if constexpr (__bytes_to_load <= __max_load_size)
	return __convert<_SimdMember<_Tp>>(__vector_load<_U, _N>(__mem, _F()));
      else if constexpr(__bytes_to_load % __max_load_size == 0)
	{
	  constexpr size_t __n_loads = __bytes_to_load / __max_load_size;
	  constexpr size_t __elements_per_load = _N / __n_loads;
	  return __call_with_n_evaluations<__n_loads>(
	    [](auto... __uncvted) {
	      return __convert<_SimdMember<_Tp>>(__uncvted...);
	    },
	    [&](auto __i) {
	      return __vector_load<_U, __elements_per_load>(
		__mem + __i * __elements_per_load, _F());
	    });
	}
      else if constexpr (__bytes_to_load % (__max_load_size / 2) == 0 &&
			 __max_load_size > 16)
	{ // e.g. int[] -> <char, 12> with AVX2
	  constexpr size_t __n_loads = __bytes_to_load / (__max_load_size / 2);
	  constexpr size_t __elements_per_load = _N / __n_loads;
	  return __call_with_n_evaluations<__n_loads>(
	    [](auto... __uncvted) {
	      return __convert<_SimdMember<_Tp>>(__uncvted...);
	    },
	    [&](auto __i) {
	      return __vector_load<_U, __elements_per_load>(
		__mem + __i * __elements_per_load, _F());
	    });
	}
      else // e.g. int[] -> <char, 9>
	return __call_with_subscripts(
	  __mem, make_index_sequence<_N>(), [](auto... __args) {
	    return __vector_type_t<_Tp, _S_full_size<_Tp>>{
	      static_cast<_Tp>(__args)...};
	  });
    }

    // __masked_load {{{2
    template <class _Tp, size_t _N, class _U, class _F>
    static inline _SimdWrapper<_Tp, _N> __masked_load(_SimdWrapper<_Tp, _N> __merge,
                                                _MaskMember<_Tp> __k,
                                                const _U *__mem,
                                                _F) _GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
    {
      __bit_iteration(__vector_to_bitset(__k._M_data).to_ullong(), [&](auto __i) {
		      __merge.__set(__i, static_cast<_Tp>(__mem[__i]));
		      });
      return __merge;
    }

    // __store {{{2
    template <class _Tp, class _U, class _F>
    _GLIBCXX_SIMD_INTRINSIC static void
      __store(_SimdMember<_Tp> __v, _U* __mem, _F, _TypeTag<_Tp>)
	_GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
    {
      // TODO: converting int -> "smaller int" can be optimized with AVX512
      constexpr size_t _N = _S_size<_Tp>;
      constexpr size_t __max_store_size =
	(sizeof(_U) >= 4 && __have_avx512f) || __have_avx512bw
	  ? 64
	  : (std::is_floating_point_v<_U> && __have_avx) || __have_avx2 ? 32
									: 16;
      if constexpr (sizeof(_U) > 8)
	__execute_n_times<_N>([&](auto __i) constexpr {
	  __mem[__i] = __v[__i];
	});
      else if constexpr (std::is_same_v<_U, _Tp>)
	__vector_store<sizeof(_Tp) * _N, _N>(__v._M_data, __mem, _F());
      else if constexpr (sizeof(_U) * _N < 16)
	__vector_store<sizeof(_U) * _N>(__convert<_U>(__v),
					__mem, _F());
      else if constexpr (sizeof(_U) * _N <= __max_store_size)
	__vector_store<0, _N>(__convert<__vector_type_t<_U, _N>>(__v), __mem, _F());
      else
	{
	  constexpr size_t __vsize = __max_store_size / sizeof(_U);
	  // round up to convert the last partial vector as well:
	  constexpr size_t __stores = __div_roundup(_N, __vsize);
	  constexpr size_t __full_stores = _N / __vsize;
	  using _V = __vector_type_t<_U, __vsize>;
	  const std::array<_V, __stores> __converted =
	    __convert_all<_V, __stores>(__v);
	  __execute_n_times<__full_stores>([&](auto __i) constexpr {
	    __vector_store(__converted[__i], __mem + __i * __vsize, _F());
	  });
	  if constexpr (__full_stores < __stores)
	    __vector_store<(_N - __full_stores * __vsize) * sizeof(_U)>(
	      __converted[__full_stores], __mem + __full_stores * __vsize,
	      _F());
	}
    }

    // __masked_store {{{2
    template <typename _V,
	      typename _VVT = _VectorTraits<_V>,
	      typename _Tp  = typename _VVT::value_type,
	      class _U,
	      class _F>
    static inline void
      __masked_store(const _V __v, _U* __mem, _F, const _MaskMember<_Tp> __k)
	_GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
    {
      constexpr size_t            _V_size = _S_size<_Tp>;
      [[maybe_unused]] const auto __vi    = __to_intrin(__v);
      constexpr size_t            __max_store_size =
	(sizeof(_U) >= 4 && __have_avx512f) || __have_avx512bw
	  ? 64
	  : (std::is_floating_point_v<_U> && __have_avx) || __have_avx2 ? 32
									: 16;
      if constexpr (std::is_same_v<_Tp, _U> ||
		    (std::is_integral_v<_Tp> && std::is_integral_v<_U> &&
		     sizeof(_Tp) == sizeof(_U)))
	{
	  // bitwise or no conversion, reinterpret:
	  const auto __kk = [&]() {
	    if constexpr (__is_bitmask_v<decltype(__k)>)
	      return _MaskMember<_U>(__k._M_data);
	    else
	      return __wrapper_bitcast<_U>(__k);
	  }();
	  __maskstore(__wrapper_bitcast<_U>(__v), __mem, _F(), __kk);
	}
      else if constexpr (sizeof(_U) <= 8 && // no long double
			 !__converts_via_decomposition_v<
			   _Tp, _U,
			   __max_store_size> // conversion via decomposition
					     // is better handled via the
					     // bit_iteration fallback below
      )
	{
	  using _UV = __vector_type_t<_U, std::min(_V_size, __max_store_size /
							      sizeof(_U))>;
	  constexpr size_t _UV_size = _VectorTraits<_UV>::_S_width;
	  constexpr bool __prefer_bitmask =
	    (__have_avx512f && sizeof(_U) >= 4) || __have_avx512bw;
	  using _M =
	    _SimdWrapper<std::conditional_t<__prefer_bitmask, bool, _U>,
			 _UV_size>;

	  if constexpr (_UV_size >= _V_size)
	    {
	      _SimdWrapper<_U, _UV_size> __converted(__convert<_UV>(__v));
	      // if _UV has more elements than the input (_V_size),
	      // vector_aligned is incorrect:
	      std::conditional_t<(_UV_size > _V_size),
				 overaligned_tag<sizeof(_U) * _V_size>, _F>
		__flag;
	      __maskstore(__converted, __mem, __flag, __convert_mask<_M>(__k));
	    }
	  else
	    {
	      constexpr size_t _NFullStores = _V_size / _UV_size;
	      constexpr size_t _NAllStores  = __div_roundup(_V_size, _UV_size);
	      constexpr size_t _NParts      = _S_full_size<_Tp> / _UV_size;
	      const std::array<_UV, _NAllStores> __converted =
		__convert_all<_UV, _NAllStores>(__v);
	      __execute_n_times<_NFullStores>(
		[&](auto __i) {
		  __maskstore(
		    __converted[__i], __mem + __i * _UV_size, _F(),
		    __convert_mask<_M>(__extract_part<__i, _NParts>(__k.__as_full_vector())));
		});
	      if constexpr (_NAllStores >
			    _NFullStores) // one partial at the end
		__maskstore(
		  __converted[_NFullStores], __mem + _NFullStores * _UV_size,
		  _F(),
		  __convert_mask<_M>(__extract_part<_NFullStores, _NParts>(
		    __k.__as_full_vector())));
	    }
	}
      else
	{
	  __bit_iteration(
	    __vector_to_bitset(__k._M_data).to_ullong(), [&](
							   auto __i) constexpr {
	      __mem[__i] = static_cast<_U>(__v[__i]);
	    });
	}
    }

    // __complement {{{2
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __complement(_SimdWrapper<_Tp, _N> __x) noexcept
    {
        return ~__x._M_data;
    }

    // __unary_minus {{{2
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __unary_minus(_SimdWrapper<_Tp, _N> __x) noexcept
    {
        // GCC doesn't use the psign instructions, but pxor & psub seem to be just as good
        // a choice as pcmpeqd & psign. So meh.
        return -__x._M_data;
    }

    // arithmetic operators {{{2
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __plus(_SimdWrapper<_Tp, _N> __x,
                                                                    _SimdWrapper<_Tp, _N> __y)
    {
      return __x._M_data + __y._M_data;
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __minus(_SimdWrapper<_Tp, _N> __x,
                                                                     _SimdWrapper<_Tp, _N> __y)
    {
      return __x._M_data - __y._M_data;
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __multiplies(
        _SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
      return __x._M_data * __y._M_data;
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __divides(
        _SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
      // Note that division by 0 is always UB, so we must ensure we avoid the
      // case for partial registers
      if constexpr (!_Abi::_S_is_partial)
        return __x._M_data / __y._M_data;
      else
	{
	  constexpr auto __one =
	    __andnot(_Abi::template __implicit_mask<_Tp>(),
		  __vector_broadcast<_S_full_size<_Tp>>(_Tp(1)));
	  return __as_vector(__x) / __or(__as_vector(__y), __one);
	}
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __modulus(_SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
      static_assert(std::is_integral<_Tp>::value,
		    "modulus is only supported for integral types");
      if constexpr (!_Abi::_S_is_partial)
	return __x._M_data % __y._M_data;
      else
	return __as_vector(__x) %
	       (__as_vector(__y) | ~_Abi::template __implicit_mask<_Tp>());
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __bit_and(_SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
        return __and(__x._M_data, __y._M_data);
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __bit_or(_SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
        return __or(__x._M_data, __y._M_data);
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __bit_xor(_SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
        return __xor(__x._M_data, __y._M_data);
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N> __bit_shift_left(_SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
        return __x._M_data << __y._M_data;
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N> __bit_shift_right(_SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
        return __x._M_data >> __y._M_data;
    }

    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __bit_shift_left(_SimdWrapper<_Tp, _N> __x, int __y)
    {
        return __x._M_data << __y;
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __bit_shift_right(_SimdWrapper<_Tp, _N> __x,
                                                                         int __y)
    {
        return __x._M_data >> __y;
    }

    // compares {{{2
    // __equal_to {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp> __equal_to(
        _SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
      return _ToWrapper(__x._M_data == __y._M_data);
    }

    // __not_equal_to {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp> __not_equal_to(
        _SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
      return _ToWrapper(__x._M_data != __y._M_data);
    }

    // __less {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp> __less(_SimdWrapper<_Tp, _N> __x,
                                                           _SimdWrapper<_Tp, _N> __y)
    {
      return _ToWrapper(__x._M_data < __y._M_data);
    }

    // __less_equal {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp> __less_equal(_SimdWrapper<_Tp, _N> __x,
                                                                 _SimdWrapper<_Tp, _N> __y)
    {
      return _ToWrapper(__x._M_data <= __y._M_data);
    }

    // negation {{{2
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp> __negate(_SimdWrapper<_Tp, _N> __x) noexcept
    {
      return _ToWrapper(!__x._M_data);
    }

    // __min, __max, __minmax {{{2
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_NORMAL_MATH _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __min(_SimdWrapper<_Tp, _N> __a,
                                                                   _SimdWrapper<_Tp, _N> __b)
    {
        return __a._M_data < __b._M_data ? __a._M_data : __b._M_data;
    }
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_NORMAL_MATH _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N> __max(_SimdWrapper<_Tp, _N> __a,
                                                                   _SimdWrapper<_Tp, _N> __b)
    {
        return __a._M_data > __b._M_data ? __a._M_data : __b._M_data;
    }

    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_NORMAL_MATH _GLIBCXX_SIMD_INTRINSIC static constexpr std::pair<_SimdWrapper<_Tp, _N>, _SimdWrapper<_Tp, _N>>
    __minmax(_SimdWrapper<_Tp, _N> __a, _SimdWrapper<_Tp, _N> __b)
    {
        return {__a._M_data < __b._M_data ? __a._M_data : __b._M_data, __a._M_data < __b._M_data ? __b._M_data : __a._M_data};
    }

    // reductions {{{2
    template <size_t _N,
	      size_t... _Is,
	      size_t... _Zeros,
	      class _Tp,
	      class _BinaryOperation>
    _GLIBCXX_SIMD_INTRINSIC static _Tp
      __reduce_partial(std::index_sequence<_Is...>,
		       std::index_sequence<_Zeros...>,
		       simd<_Tp, _Abi>    __x,
		       _BinaryOperation&& __binary_op)
    {
      using _V = __vector_type_t<_Tp, _N / 2>;
      static_assert(sizeof(_V) <= sizeof(__x));
      // _S_width is the size of the smallest native SIMD register that can
      // store _N/2 elements:
      using _FullSimd =
	__deduced_simd<_Tp, _VectorTraits<_V>::_S_width>;
      using _HalfSimd = __deduced_simd<_Tp, _N / 2>;
      const auto __xx = __as_vector(__x);
      return _HalfSimd::abi_type::_SimdImpl::__reduce(
	static_cast<_HalfSimd>(__as_vector(__binary_op(
	  static_cast<_FullSimd>(__intrin_bitcast<_V>(__xx)),
	  static_cast<_FullSimd>(__intrin_bitcast<_V>(
	    __vector_permute<(_N / 2 + _Is)..., (int(_Zeros * 0) - 1)...>(
	      __xx)))))),
	__binary_op);
    }

    template <class _Tp, class _BinaryOperation>
    _GLIBCXX_SIMD_INTRINSIC static _Tp
      __reduce(simd<_Tp, _Abi> __x, _BinaryOperation&& __binary_op)
    {
      constexpr size_t _N = simd_size_v<_Tp, _Abi>;
      if constexpr (_Abi::_S_is_partial) //{{{
	{
	  [[maybe_unused]] constexpr auto __full_size =
	    _Abi::template _S_full_size<_Tp>;
	  if constexpr (_N == 1)
	    return __x[0];
	  else if constexpr (_N == 2)
	    return __binary_op(simd<_Tp, simd_abi::scalar>(__x[0]),
			       simd<_Tp, simd_abi::scalar>(__x[1]))[0];
	  else if constexpr (_N == 3)
	    return __binary_op(__binary_op(simd<_Tp, simd_abi::scalar>(__x[0]),
					   simd<_Tp, simd_abi::scalar>(__x[1])),
			       simd<_Tp, simd_abi::scalar>(__x[2]))[0];
	  else if constexpr (std::is_same_v<__remove_cvref_t<_BinaryOperation>,
					    std::plus<>>)
	    {
	      using _A = simd_abi::deduce_t<_Tp, __full_size>;
	      return _A::_SimdImpl::__reduce(
		simd<_Tp, _A>(__private_init,
			      __and(__data(__x)._M_data,
				    _Abi::template __implicit_mask<_Tp>())),
		__binary_op);
	    }
	  else if constexpr (std::is_same_v<__remove_cvref_t<_BinaryOperation>,
					    std::multiplies<>>)
	    {
	      using _A = simd_abi::deduce_t<_Tp, __full_size>;
	      return _A::_SimdImpl::__reduce(
		simd<_Tp, _A>(__private_init,
			      __blend(_Abi::template __implicit_mask<_Tp>(),
				      __vector_broadcast<__full_size>(_Tp(1)),
				      __as_vector(__x))),
		__binary_op);
	    }
	  else if constexpr (_N & 1)
	    {
	      using _A = simd_abi::deduce_t<_Tp, _N-1>;
	      return __binary_op(
		simd<_Tp, simd_abi::scalar>(_A::_SimdImpl::__reduce(
		  simd<_Tp, _A>(__intrin_bitcast<__vector_type_t<_Tp, _N - 1>>(
		    __as_vector(__x))),
		  __binary_op)),
		simd<_Tp, simd_abi::scalar>(__x[_N - 1]))[0];
	    }
	  else
	    return __reduce_partial<_N>(
	      std::make_index_sequence<_N / 2>(),
	      std::make_index_sequence<__full_size - _N / 2>(), __x,
	      __binary_op);
	} //}}}
#if _GLIBCXX_SIMD_HAVE_NEON // {{{
      else if constexpr (sizeof(__x) == 8 || sizeof(__x) == 16)
	{
	  static_assert(_N <= 8); // either 64-bit vectors or 128-bit double
	  if constexpr (_N == 8)
	    {
	      __x = __binary_op(
		__x, __make_simd<_Tp, _N>(
		       __vector_permute<1, 0, 3, 2, 5, 4, 7, 6>(__x._M_data)));
	      __x = __binary_op(
		__x, __make_simd<_Tp, _N>(
		       __vector_permute<3, 2, 1, 0, 7, 6, 5, 4>(__x._M_data)));
	      __x = __binary_op(
		__x, __make_simd<_Tp, _N>(
		       __vector_permute<7, 6, 5, 4, 3, 2, 1, 0>(__x._M_data)));
              return __x[0];
	    }
	  else if constexpr (_N == 4)
	    {
	      __x = __binary_op(
		__x, __make_simd<_Tp, _N>(
		       __vector_permute<1, 0, 3, 2>(__x._M_data)));
	      __x = __binary_op(
		__x, __make_simd<_Tp, _N>(
		       __vector_permute<3, 2, 1, 0>(__x._M_data)));
              return __x[0];
	    }
	  else
	    {
	      static_assert(_N == 2);
	      __x = __binary_op(
		__x, __make_simd<_Tp, _N>(__vector_permute<1, 0>(__x._M_data)));
	      return __x[0];
	    }
	}
#endif // _GLIBCXX_SIMD_HAVE_NEON }}}
      else if constexpr (sizeof(__x) == 16) //{{{
	{
	  if constexpr (_N == 16)
	    {
	      const auto __y = __x._M_data;
	      __x            = __binary_op(
                __make_simd<_Tp, _N>(__vector_permute<0, 0, 1, 1, 2, 2, 3, 3, 4,
                                                    4, 5, 5, 6, 6, 7, 7>(__y)),
                __make_simd<_Tp, _N>(
                  __vector_permute<8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
                                   14, 14, 15, 15>(__y)));
	    }
	  if constexpr (_N >= 8)
	    {
	      const auto __y = __vector_bitcast<short>(__x._M_data);
	      __x =
		__binary_op(__make_simd<_Tp, _N>(__vector_bitcast<_Tp>(
			      __vector_permute<0, 0, 1, 1, 2, 2, 3, 3>(__y))),
			    __make_simd<_Tp, _N>(__vector_bitcast<_Tp>(
			      __vector_permute<4, 4, 5, 5, 6, 6, 7, 7>(__y))));
	    }
	  if constexpr (_N >= 4)
	    {
	      using _U =
		std::conditional_t<std::is_floating_point_v<_Tp>, float, int>;
	      const auto __y = __vector_bitcast<_U>(__x._M_data);
	      __x = __binary_op(__x, __make_simd<_Tp, _N>(__vector_bitcast<_Tp>(
				       __vector_permute<3, 2, 1, 0>(__y))));
	    }
	  using _U =
	    std::conditional_t<std::is_floating_point_v<_Tp>, double, _LLong>;
	  const auto __y = __vector_bitcast<_U>(__x._M_data);
	  __x = __binary_op(__x, __make_simd<_Tp, _N>(__vector_bitcast<_Tp>(
				   __vector_permute<1, 1>(__y))));
	  return __x[0];
	} //}}}
      else if constexpr(_N == 2)
	return __binary_op(__x, simd<_Tp, _Abi>(__x[1]))[0];
      else
	{
	  static_assert(sizeof(__x) > __min_vector_size<_Tp>);
	  static_assert((_N & (_N - 1)) == 0); // _N must be a power of 2
	  using _A = simd_abi::deduce_t<_Tp, _N / 2>;
	  using _V = std::experimental::simd<_Tp, _A>;
	  return _A::_SimdImpl::__reduce(
	    __binary_op(
	      _V(__private_init, __extract<0, 2>(__data(__x)._M_data)),
	      _V(__private_init, __extract<1, 2>(__data(__x)._M_data))),
	    std::forward<_BinaryOperation>(__binary_op));
	}
    }

    // math {{{2
    // __abs {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N>
      __abs(_SimdWrapper<_Tp, _N> __x) noexcept
    {
      // if (__builtin_is_constant_evaluated())
      //  {
      //    return __x._M_data < 0 ? -__x._M_data : __x._M_data;
      //  }
      if constexpr (std::is_floating_point_v<_Tp>)
	// `v < 0 ? -v : v` cannot compile to the efficient implementation of
	// masking the signbit off because it must consider v == -0

	// ~(-0.) & v would be easy, but breaks with fno-signed-zeros
	return __and(_S_absmask<__vector_type_t<_Tp, _N>>, __x._M_data);
      else
#ifdef _GLIBCXX_SIMD_WORKAROUND_PR91533
	if constexpr (sizeof(__x) < 16 && std::is_signed_v<_Tp>)
	{
	  if constexpr (sizeof(_Tp) == 4)
	    return __auto_bitcast(_mm_abs_epi32(__to_intrin(__x)));
	  else if constexpr (sizeof(_Tp) == 2)
	    return __auto_bitcast(_mm_abs_epi16(__to_intrin(__x)));
	  else
	    return __auto_bitcast(_mm_abs_epi8(__to_intrin(__x)));
	}
      else
#endif //_GLIBCXX_SIMD_WORKAROUND_PR91533
	return __x._M_data < 0 ? -__x._M_data : __x._M_data;
    }

    // __nearbyint {{{3
    template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
    _GLIBCXX_SIMD_INTRINSIC static _Tp __nearbyint(_Tp __x_) noexcept
    {
      using value_type = typename _TVT::value_type;
      using _V        = typename _TVT::type;
      const _V __x    = __x_;
      const _V __absx = __and(__x, _S_absmask<_V>);
      static_assert(CHAR_BIT * sizeof(1ull) >=
		    std::numeric_limits<value_type>::digits);
      constexpr _V __shifter_abs =
	_V() + (1ull << (std::numeric_limits<value_type>::digits - 1));
      const _V __shifter = __or(__and(_S_signmask<_V>, __x), __shifter_abs);
      _V __shifted = __x + __shifter;
      // how can we stop -fassociative-math to break this pattern?
      //asm("" : "+X"(__shifted));
      __shifted -= __shifter;
      return __absx < __shifter_abs ? __shifted : __x;
    }

    // __rint {{{3
    template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
    _GLIBCXX_SIMD_INTRINSIC static _Tp __rint(_Tp __x) noexcept
    {
      return _SuperImpl::__nearbyint(__x);
    }

    // __trunc {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N>
      __trunc(_SimdWrapper<_Tp, _N> __x)
    {
      using _V        = __vector_type_t<_Tp, _N>;
      const _V __absx = __and(__x._M_data, _S_absmask<_V>);
      static_assert(CHAR_BIT * sizeof(1ull) >=
		    std::numeric_limits<_Tp>::digits);
      constexpr _Tp __shifter = 1ull << (std::numeric_limits<_Tp>::digits - 1);
      _V            __truncated = (__absx + __shifter) - __shifter;
      __truncated -= __truncated > __absx ? _V() + 1 : _V();
      return __absx < __shifter ? __or(__xor(__absx, __x._M_data), __truncated)
				: __x._M_data;
    }

    // __round {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N>
      __round(_SimdWrapper<_Tp, _N> __x)
    {
      using _V        = __vector_type_t<_Tp, _N>;
      const _V __absx = __and(__x._M_data, _S_absmask<_V>);
      static_assert(CHAR_BIT * sizeof(1ull) >=
		    std::numeric_limits<_Tp>::digits);
      constexpr _Tp __shifter = 1ull << (std::numeric_limits<_Tp>::digits - 1);
      _V            __truncated = (__absx + __shifter) - __shifter;
      __truncated -= __truncated > __absx ? _V() + 1 : _V();
      const _V __rounded =
	__or(__xor(__absx, __x._M_data),
	     __truncated + (__absx - __truncated >= _Tp(.5) ? _V() + 1 : _V()));
      return __absx < __shifter ? __rounded : __x._M_data;
    }

    // __floor {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N> __floor(_SimdWrapper<_Tp, _N> __x)
    {
      const auto __y = _SuperImpl::__trunc(__x)._M_data;
      const auto __negative_input = __vector_bitcast<_Tp>(__x._M_data < __vector_broadcast<_N, _Tp>(0));
      const auto __mask = __andnot(__vector_bitcast<_Tp>(__y == __x._M_data), __negative_input);
      return __or(__andnot(__mask, __y), __and(__mask, __y - __vector_broadcast<_N, _Tp>(1)));
    }

    // __ceil {{{3
    template <class _Tp, size_t _N> _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N> __ceil(_SimdWrapper<_Tp, _N> __x)
    {
      const auto __y = _SuperImpl::__trunc(__x)._M_data;
      const auto __negative_input = __vector_bitcast<_Tp>(__x._M_data < __vector_broadcast<_N, _Tp>(0));
      const auto __inv_mask = __or(__vector_bitcast<_Tp>(__y == __x._M_data), __negative_input);
      return __or(__and(__inv_mask, __y),
		  __andnot(__inv_mask, __y + __vector_broadcast<_N, _Tp>(1)));
    }

    // __isnan {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp> __isnan(_SimdWrapper<_Tp, _N> __x)
    {
#if __FINITE_MATH_ONLY__
      [](auto&&){}(__x);
      return {}; // false
#else
      return __cmpunord(__x._M_data, __x._M_data);
#endif
    }

    // __isfinite {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp> __isfinite(_SimdWrapper<_Tp, _N> __x)
    {
#if __FINITE_MATH_ONLY__
      [](auto&&){}(__x);
      return __vector_bitcast<_N>(_Tp()) == __vector_bitcast<_N>(_Tp());
#else
      // if all exponent bits are set, __x is either inf or NaN
      using _I = __int_for_sizeof_t<_Tp>;
      const auto __inf = __vector_bitcast<_I>(
	__vector_broadcast<_N>(std::numeric_limits<_Tp>::infinity()));
      return __vector_bitcast<_Tp>(__inf >
				   (__vector_bitcast<_I>(__x) & __inf));
#endif
    }

    // __isunordered {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp> __isunordered(_SimdWrapper<_Tp, _N> __x,
                                                          _SimdWrapper<_Tp, _N> __y)
    {
        return __cmpunord(__x._M_data, __y._M_data);
    }

    // __signbit {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp> __signbit(_SimdWrapper<_Tp, _N> __x)
    {
      using _I = __int_for_sizeof_t<_Tp>;
      const auto __xx = __vector_bitcast<_I>(__x._M_data);
      return __vector_bitcast<_Tp>(__xx >> std::numeric_limits<_I>::digits);
    }

    // __isinf {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp> __isinf(_SimdWrapper<_Tp, _N> __x)
    {
#if __FINITE_MATH_ONLY__
      [](auto&&){}(__x);
      return {}; // false
#else
      return _SuperImpl::template __equal_to<_Tp, _N>(
	_SuperImpl::__abs(__x),
	__vector_broadcast<_N>(std::numeric_limits<_Tp>::infinity()));
      // alternative:
      // compare to inf using the corresponding integer type
      /*
	 return
	 __vector_bitcast<_Tp>(__vector_bitcast<__int_for_sizeof_t<_Tp>>(__abs(__x)._M_data)
	 ==
	 __vector_bitcast<__int_for_sizeof_t<_Tp>>(__vector_broadcast<_N>(
	 std::numeric_limits<_Tp>::infinity())));
	 */
#endif
    }

    // __isnormal {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp>
      __isnormal(_SimdWrapper<_Tp, _N> __x)
    {
#if __FINITE_MATH_ONLY__
      return _SuperImpl::template __less_equal<_Tp, _N>(
	__vector_broadcast<_N>(std::numeric_limits<_Tp>::min()), _SuperImpl::__abs(__x));
#else
      return __and(
	_SuperImpl::template __less_equal<_Tp, _N>(
	  __vector_broadcast<_N>(std::numeric_limits<_Tp>::min()), _SuperImpl::__abs(__x)),
	_SuperImpl::template __less<_Tp, _N>(
	  _SuperImpl::__abs(__x),
	  __vector_broadcast<_N>(std::numeric_limits<_Tp>::infinity())));
#endif
    }

    // __fpclassify {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static __fixed_size_storage_t<int, _N> __fpclassify(_SimdWrapper<_Tp, _N> __x)
    {
      constexpr auto __fp_normal = __vector_bitcast<_Tp>(
	__vector_broadcast<_N, __int_for_sizeof_t<_Tp>>(FP_NORMAL));
      constexpr auto __fp_nan = __vector_bitcast<_Tp>(
	__vector_broadcast<_N, __int_for_sizeof_t<_Tp>>(FP_NAN));
      constexpr auto __fp_infinite = __vector_bitcast<_Tp>(
	__vector_broadcast<_N, __int_for_sizeof_t<_Tp>>(FP_INFINITE));
      constexpr auto __fp_subnormal = __vector_bitcast<_Tp>(
	__vector_broadcast<_N, __int_for_sizeof_t<_Tp>>(FP_SUBNORMAL));
      constexpr auto __fp_zero = __vector_bitcast<_Tp>(
	__vector_broadcast<_N, __int_for_sizeof_t<_Tp>>(FP_ZERO));

      const auto __tmp =
	_SuperImpl::__abs(__x)._M_data < std::numeric_limits<_Tp>::min()
	  ? (__x._M_data == 0 ? __fp_zero : __fp_subnormal)
	  : __blend(__isinf(__x)._M_data,
		    __blend(__isnan(__x)._M_data, __fp_normal, __fp_nan),
		    __fp_infinite);
      if constexpr (sizeof(_Tp) == sizeof(int))
	{
	  using _FixedInt = __fixed_size_storage_t<int, _N>;
	  const auto __as_int = __vector_bitcast<int, _N>(__tmp);
	  if constexpr (_FixedInt::_S_tuple_size == 1)
	    return {__as_int};
	  else if constexpr (_FixedInt::_S_tuple_size == 2 &&
			     std::is_same_v<
			       typename _FixedInt::_SecondType::_FirstAbi,
			       simd_abi::scalar>)
	    return {__extract<0, 2>(__as_int), __as_int[_N - 1]};
	  else if constexpr (_FixedInt::_S_tuple_size == 2)
	    return {__extract<0, 2>(__as_int), __auto_bitcast(__extract<1, 2>(__as_int))};
	  else
	    __assert_unreachable<_Tp>();
	}
      else if constexpr (_N == 2 && sizeof(_Tp) == 8 &&
			 __fixed_size_storage_t<int, _N>::_S_tuple_size == 2)
	{
	  const auto __aslong = __vector_bitcast<_LLong>(__tmp);
	  return {int(__aslong[0]), {int(__aslong[1])}};
	}
      else if constexpr (sizeof(_Tp) == 8 && sizeof(__tmp) == 32 &&
			 __fixed_size_storage_t<int, _N>::_S_tuple_size == 1)
	{
#if _GLIBCXX_SIMD_X86INTRIN
	  return {_mm_packs_epi32(__intrin_bitcast<__m128i>(__lo128(__tmp)),
				  __intrin_bitcast<__m128i>(__hi128(__tmp)))};
#else  // _GLIBCXX_SIMD_X86INTRIN
	  const auto __aslong = __vector_bitcast<_LLong>(__tmp);
	  return {__make_wrapper<int>(__aslong[0], __aslong[1], __aslong[2], __aslong[3])};
#endif // _GLIBCXX_SIMD_X86INTRIN
	}
      else if constexpr (_N == 2 && sizeof(_Tp) == 8 &&
			 __fixed_size_storage_t<int, _N>::_S_tuple_size == 1)
	{
	  const auto __aslong = __vector_bitcast<_LLong>(__tmp);
	  return {__make_wrapper<int>(__aslong[0], __aslong[1])};
	}
      else
	__assert_unreachable<_Tp>();
    }

    // __increment & __decrement{{{2
    template <class _Tp, size_t _N> _GLIBCXX_SIMD_INTRINSIC static void __increment(_SimdWrapper<_Tp, _N> &__x)
    {
        __x = __x._M_data + 1;
    }
    template <class _Tp, size_t _N> _GLIBCXX_SIMD_INTRINSIC static void __decrement(_SimdWrapper<_Tp, _N> &__x)
    {
        __x = __x._M_data - 1;
    }

    // smart_reference access {{{2
    template <class _Tp, size_t _N, class _U>
    _GLIBCXX_SIMD_INTRINSIC static void __set(_SimdWrapper<_Tp, _N> &__v, int __i, _U &&__x) noexcept
    {
        __v.__set(__i, std::forward<_U>(__x));
    }

    // __masked_assign{{{2
    template <class _Tp, class _K, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static void __masked_assign(_SimdWrapper<_K, _N> __k,
                                                      _SimdWrapper<_Tp, _N> &__lhs,
                                                      __id<_SimdWrapper<_Tp, _N>> __rhs)
    {
        __lhs = __blend(__k._M_data, __lhs._M_data, __rhs._M_data);
    }

    template <class _Tp, class _K, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static void __masked_assign(_SimdWrapper<_K, _N> __k, _SimdWrapper<_Tp, _N> &__lhs,
                                           __id<_Tp> __rhs)
    {
        if (__builtin_constant_p(__rhs) && __rhs == 0 && std::is_same<_K, _Tp>::value) {
            if constexpr (!__is_bitmask(__k)) {
                // the __andnot optimization only makes sense if __k._M_data is a vector register
                __lhs._M_data = __andnot(__k._M_data, __lhs._M_data);
                return;
            } else {
                // for AVX512/__mmask, a _mm512_maskz_mov is best
                __lhs._M_data = __auto_bitcast(__blend(__k, __lhs, __intrinsic_type_t<_Tp, _N>()));
                return;
            }
        }
        __lhs._M_data = __blend(__k._M_data, __lhs._M_data, __vector_broadcast<_N>(__rhs));
    }

    // __masked_cassign {{{2
    template <typename _Op, class _Tp, class _K, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static void
      __masked_cassign(const _SimdWrapper<_K, _N>        __k,
		       _SimdWrapper<_Tp, _N>&            __lhs,
		       const __id<_SimdWrapper<_Tp, _N>> __rhs,
		       _Op                               __op)
    {
      __lhs._M_data =
	__blend(__k._M_data, __lhs._M_data, __op(_SuperImpl{}, __lhs, __rhs));
    }

    template <typename _Op, class _Tp, class _K, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static void
      __masked_cassign(const _SimdWrapper<_K, _N> __k,
		       _SimdWrapper<_Tp, _N>&     __lhs,
		       const __id<_Tp>            __rhs,
		       _Op                        __op)
    {
      __lhs._M_data =
	__blend(__k._M_data, __lhs._M_data,
		__op(_SuperImpl{}, __lhs,
		     _SimdWrapper<_Tp, _N>(__vector_broadcast<_N>(__rhs))));
    }

    // __masked_unary {{{2
    template <template <typename> class _Op, class _Tp, class _K, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N> __masked_unary(const _SimdWrapper<_K, _N> __k,
                                                            const _SimdWrapper<_Tp, _N> __v)
    {
        auto __vv = __make_simd(__v);
        _Op<decltype(__vv)> __op;
        return __blend(__k, __v, __data(__op(__vv)));
    }

    //}}}2
};

// _MaskImplBuiltin {{{1
template <class _Abi> struct _MaskImplBuiltin {
    // member types {{{2
    template <class _Tp> using _TypeTag = _Tp *;
    template <class _Tp>
    using _SimdMember = typename _Abi::template __traits<_Tp>::_SimdMember;
    template <class _Tp>
    using _MaskMember = typename _Abi::template __traits<_Tp>::_MaskMember;

    // __masked_load {{{2
    template <class _Tp, size_t _N, class _F>
    static inline _SimdWrapper<_Tp, _N> __masked_load(_SimdWrapper<_Tp, _N> __merge,
						    _SimdWrapper<_Tp, _N> __mask,
						    const bool*           __mem,
						    _F) noexcept
    {
      // AVX(2) has 32/64 bit maskload, but nothing at 8 bit granularity
      auto __tmp = __wrapper_bitcast<__int_for_sizeof_t<_Tp>>(__merge);
      __bit_iteration(__vector_to_bitset(__mask._M_data).to_ullong(),
		      [&](auto __i) { __tmp.__set(__i, -__mem[__i]); });
      __merge = __wrapper_bitcast<_Tp>(__tmp);
      return __merge;
    }

    // __store {{{2
    template <class _Tp, size_t _N, class _F>
    _GLIBCXX_SIMD_INTRINSIC static void __store(_SimdWrapper<_Tp, _N> __v, bool *__mem, _F) noexcept
    {
      __execute_n_times<_N>([&](auto __i) constexpr { __mem[__i] = __v[__i]; });
    }

    // __masked_store {{{2
    template <class _Tp, size_t _N, class _F>
    static inline void __masked_store(const _SimdWrapper<_Tp, _N> __v, bool *__mem, _F,
                                    const _SimdWrapper<_Tp, _N> __k) noexcept
    {
      __bit_iteration(__vector_to_bitset(__k._M_data).to_ullong(),
		      [&](auto __i) constexpr { __mem[__i] = __v[__i]; });
    }

    // __from_bitset{{{2
    template <size_t _N, class _Tp>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp> __from_bitset(std::bitset<_N> __bits, _TypeTag<_Tp>)
    {
        return __convert_mask<typename _MaskMember<_Tp>::_BuiltinType>(__bits);
    }

    // logical and bitwise operators {{{2
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
      __logical_and(const _SimdWrapper<_Tp, _N>& __x,
		  const _SimdWrapper<_Tp, _N>& __y)
    {
      return __and(__x._M_data, __y._M_data);
    }

    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
      __logical_or(const _SimdWrapper<_Tp, _N>& __x,
		 const _SimdWrapper<_Tp, _N>& __y)
    {
      return __or(__x._M_data, __y._M_data);
    }

    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
      __bit_and(const _SimdWrapper<_Tp, _N>& __x,
	      const _SimdWrapper<_Tp, _N>& __y)
    {
      return __and(__x._M_data, __y._M_data);
    }

    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
      __bit_or(const _SimdWrapper<_Tp, _N>& __x, const _SimdWrapper<_Tp, _N>& __y)
    {
      return __or(__x._M_data, __y._M_data);
    }

    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
      __bit_xor(const _SimdWrapper<_Tp, _N>& __x,
	      const _SimdWrapper<_Tp, _N>& __y)
    {
      return __xor(__x._M_data, __y._M_data);
    }

    // smart_reference access {{{2
    template <class _Tp, size_t _N>
    static void __set(_SimdWrapper<_Tp, _N>& __k, int __i, bool __x) noexcept
    {
      if constexpr (std::is_same_v<_Tp, bool>)
	__k.__set(__i, __x);
      else
	__k._M_data[__i] = __bit_cast<_Tp>(__int_for_sizeof_t<_Tp>(-__x));
    }

    // __masked_assign{{{2
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static void __masked_assign(_SimdWrapper<_Tp, _N> __k, _SimdWrapper<_Tp, _N> &__lhs,
                                           __id<_SimdWrapper<_Tp, _N>> __rhs)
    {
        __lhs = __blend(__k._M_data, __lhs._M_data, __rhs._M_data);
    }

    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static void __masked_assign(_SimdWrapper<_Tp, _N> __k, _SimdWrapper<_Tp, _N> &__lhs, bool __rhs)
    {
        if (__builtin_constant_p(__rhs)) {
            if (__rhs == false) {
                __lhs = __andnot(__k._M_data, __lhs._M_data);
            } else {
                __lhs = __or(__k._M_data, __lhs._M_data);
            }
            return;
        }
        __lhs = __blend(__k, __lhs, __data(simd_mask<_Tp, _Abi>(__rhs)));
    }

    //}}}2
};

//}}}1

#if _GLIBCXX_SIMD_X86INTRIN // {{{
// __x86_simd_impl {{{1
template <class _Abi> struct __x86_simd_impl : _SimdImplBuiltin<_Abi> {
  using _Base = _SimdImplBuiltin<_Abi>;
  template <typename _Tp>
  using _MaskMember = typename _Base::template _MaskMember<_Tp>;
  template <typename _Tp>
  static constexpr size_t _S_full_size = _Abi::template _S_full_size<_Tp>;
  template <typename _Tp>
  static constexpr size_t size = _Abi::template size<_Tp>;

  // __masked_load {{{2
  template <class _Tp, size_t _N, class _U, class _F>
  static inline _SimdWrapper<_Tp, _N>
    __masked_load(_SimdWrapper<_Tp, _N> __merge,
		  _MaskMember<_Tp>      __k,
		  const _U*             __mem,
		  _F) _GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
  {
    static_assert(_N == size<_Tp>);
    if constexpr (std::is_same_v<_Tp, _U> || // no conversion
		  (sizeof(_Tp) == sizeof(_U) &&
		   std::is_integral_v<_Tp> ==
		     std::is_integral_v<_U>) // conversion via bit
					     // reinterpretation
    )
      {
	[[maybe_unused]] const auto __intrin = __to_intrin(__merge);
	constexpr bool              __have_avx512bw_vl_or_zmm =
	  __have_avx512bw_vl || (__have_avx512bw && sizeof(__merge) == 64);
	if constexpr (__have_avx512bw_vl_or_zmm && sizeof(_Tp) == 1)
	  {
	    if constexpr (sizeof(__merge) <= 16)
	      __merge = __vector_bitcast<_Tp, _N>(_mm_mask_loadu_epi8(
		__intrin, _mm_movemask_epi8(__to_intrin(__k)), __mem));
	    else if constexpr (sizeof(__merge) == 32)
	      {
		__merge = __vector_bitcast<_Tp>(_mm256_mask_loadu_epi8(
		  __intrin, _mm256_movemask_epi8(__to_intrin(__k)), __mem));
	      }
	    else if constexpr (sizeof(__merge) == 64)
	      {
		__merge = __vector_bitcast<_Tp>(
		  _mm512_mask_loadu_epi8(__intrin, __k, __mem));
	      }
	    else
	      {
		__assert_unreachable<_Tp>();
	      }
	  }
	else if constexpr (__have_avx512bw_vl_or_zmm && sizeof(_Tp) == 2)
	  {
	    if constexpr (sizeof(__merge) <= 16)
	      __merge = __vector_bitcast<_Tp, _N>(_mm_mask_loadu_epi16(
		__intrin, movemask_epi16(__to_intrin(__k)), __mem));
	    else if constexpr (sizeof(__merge) == 32)
	      {
		__merge = __vector_bitcast<_Tp>(_mm256_mask_loadu_epi16(
		  __intrin, movemask_epi16(__to_intrin(__k)), __mem));
	      }
	    else if constexpr (sizeof(__merge) == 64)
	      {
		__merge = __vector_bitcast<_Tp>(
		  _mm512_mask_loadu_epi16(__intrin, __k, __mem));
	      }
	    else
	      {
		__assert_unreachable<_Tp>();
	      }
	  }
	else if constexpr (__have_avx2 && sizeof(_Tp) == 4 &&
			   std::is_integral_v<_U>)
	  {
	    if constexpr (sizeof(__merge) <= 16)
	      __merge =
		__or(__andnot(__k._M_data, __merge._M_data),
		     __vector_bitcast<_Tp, _N>(_mm_maskload_epi32(
		       reinterpret_cast<const int*>(__mem), __to_intrin(__k))));
	    else if constexpr (sizeof(__merge) == 32)
	      {
		__merge =
		  (~__k._M_data & __merge._M_data) |
		  __vector_bitcast<_Tp>(_mm256_maskload_epi32(
		    reinterpret_cast<const int*>(__mem), __to_intrin(__k)));
	      }
	    else if constexpr (__have_avx512f && sizeof(__merge) == 64)
	      {
		__merge = __vector_bitcast<_Tp>(
		  _mm512_mask_loadu_epi32(__intrin, __k, __mem));
	      }
	    else
	      {
		__assert_unreachable<_Tp>();
	      }
	  }
	else if constexpr (__have_avx && sizeof(_Tp) == 4)
	  {
	    if constexpr (sizeof(__merge) <= 16)
	      __merge = __or(__andnot(__k._M_data, __merge._M_data),
			     __vector_bitcast<_Tp, _N>(_mm_maskload_ps(
			       reinterpret_cast<const float*>(__mem),
			       __intrin_bitcast<__m128i>(__as_vector(__k)))));
	    else if constexpr (sizeof(__merge) == 32)
	      {
		__merge =
		  __or(__andnot(__k._M_data, __merge._M_data),
		       _mm256_maskload_ps(reinterpret_cast<const float*>(__mem),
					  __vector_bitcast<_LLong>(__k)));
	      }
	    else if constexpr (__have_avx512f && sizeof(__merge) == 64)
	      {
		__merge = __vector_bitcast<_Tp>(
		  _mm512_mask_loadu_ps(__intrin, __k, __mem));
	      }
	    else
	      {
		__assert_unreachable<_Tp>();
	      }
	  }
	else if constexpr (__have_avx2 && sizeof(_Tp) == 8 &&
			   std::is_integral_v<_U>)
	  {
	    if constexpr (sizeof(__merge) <= 16)
	      __merge = __or(
		__andnot(__k._M_data, __merge._M_data),
		__vector_bitcast<_Tp, _N>(_mm_maskload_epi64(
		  reinterpret_cast<const _LLong*>(__mem), __to_intrin(__k))));
	    else if constexpr (sizeof(__merge) == 32)
	      {
		__merge =
		  (~__k._M_data & __merge._M_data) |
		  __vector_bitcast<_Tp>(_mm256_maskload_epi64(
		    reinterpret_cast<const _LLong*>(__mem), __to_intrin(__k)));
	      }
	    else if constexpr (__have_avx512f && sizeof(__merge) == 64)
	      {
		__merge = __vector_bitcast<_Tp>(
		  _mm512_mask_loadu_epi64(__intrin, __k, __mem));
	      }
	    else
	      {
		__assert_unreachable<_Tp>();
	      }
	  }
	else if constexpr (__have_avx && sizeof(_Tp) == 8)
	  {
	    if constexpr (sizeof(__merge) <= 16)
	      __merge = __or(__andnot(__k._M_data, __merge._M_data),
			     __vector_bitcast<_Tp, _N>(_mm_maskload_pd(
			       reinterpret_cast<const double*>(__mem),
			       __vector_bitcast<_LLong>(__k))));
	    else if constexpr (sizeof(__merge) == 32)
	      {
		__merge = __or(
		  __andnot(__k._M_data, __merge._M_data),
		  _mm256_maskload_pd(reinterpret_cast<const double*>(__mem),
				     __vector_bitcast<_LLong>(__k)));
	      }
	    else if constexpr (__have_avx512f && sizeof(__merge) == 64)
	      {
		__merge = __vector_bitcast<_Tp>(
		  _mm512_mask_loadu_pd(__intrin, __k, __mem));
	      }
	    else
	      {
		__assert_unreachable<_Tp>();
	      }
	  }
	else
	  {
	    __bit_iteration(__vector_to_bitset(__k._M_data).to_ullong(),
			    [&](auto __i) {
			      __merge.__set(__i, static_cast<_Tp>(__mem[__i]));
			    });
	  }
      }
    /* Very uncertain, that the following improves anything. Needs benchmarking
     * before it's activated.
    else if constexpr (sizeof(_U) <= 8 && // no long double
		       !__converts_via_decomposition_v<
			 _U, _Tp,
			 sizeof(__merge)> // conversion via decomposition
					  // is better handled via the
					  // bit_iteration fallback below
    )
      {
	// TODO: copy pattern from __masked_store, which doesn't resort to
	// fixed_size
	using _A       = simd_abi::deduce_t<_U, _N>;
	using _ATraits = _SimdTraits<_U, _A>;
	using _AImpl   = typename _ATraits::_SimdImpl;
	typename _ATraits::_SimdMember __uncvted{};
	typename _ATraits::_MaskMember __kk;
	if constexpr (__is_fixed_size_abi_v<_A>)
	  {
	    const auto __bitset = __vector_to_bitset(__k._M_data);
	    static_assert(__bitset.size() <= __kk.size());
	    static_assert(__bitset.size() <= sizeof(_ULLong) * CHAR_BIT);
	    __kk =__bitset.to_ullong();
	  }
	else
	  {
	    __kk = __convert_mask<typename _ATraits::_MaskMember>(__k);
	  }
	__uncvted = _AImpl::__masked_load(__uncvted, __kk, __mem, _F());
	_SimdConverter<_U, _A, _Tp, _Abi> __converter;
	_Base::__masked_assign(__k, __merge, __converter(__uncvted));
      }
      */
    else
      __merge = _Base::__masked_load(__merge, __k, __mem, _F());
    return __merge;
  }

    // __masked_store {{{2
    template <class _Tp, size_t _N, class _U, class _F>
    static inline void __masked_store(const _SimdWrapper<_Tp, _N> __v, _U *__mem, _F,
                                    const _MaskMember<_Tp> __k) _GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
    {
      if constexpr (std::is_integral_v<_Tp> && std::is_integral_v<_U> &&
		    sizeof(_Tp) > sizeof(_U) && __have_avx512f &&
		    (sizeof(_Tp) >= 4 || __have_avx512bw) &&
		    (sizeof(__v) == 64 || __have_avx512vl)) {  // truncating store
	[[maybe_unused]] const auto __vi = __to_intrin(__v);
	const auto __kk = [&]() {
	  if constexpr (__is_bitmask_v<decltype(__k)>) {
	    return __k;
	  } else {
	    return __convert_mask<_SimdWrapper<bool, _N>>(__k);
	  }
	}();
	if constexpr (sizeof(_Tp) == 8 && sizeof(_U) == 4) {
	  if constexpr (sizeof(__vi) == 64) {
	    _mm512_mask_cvtepi64_storeu_epi32(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 32) {
	    _mm256_mask_cvtepi64_storeu_epi32(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 16) {
	    _mm_mask_cvtepi64_storeu_epi32(__mem, __kk, __vi);
	  }
	} else if constexpr (sizeof(_Tp) == 8 && sizeof(_U) == 2) {
	  if constexpr (sizeof(__vi) == 64) {
	    _mm512_mask_cvtepi64_storeu_epi16(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 32) {
	    _mm256_mask_cvtepi64_storeu_epi16(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 16) {
	    _mm_mask_cvtepi64_storeu_epi16(__mem, __kk, __vi);
	  }
	} else if constexpr (sizeof(_Tp) == 8 && sizeof(_U) == 1) {
	  if constexpr (sizeof(__vi) == 64) {
	    _mm512_mask_cvtepi64_storeu_epi8(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 32) {
	    _mm256_mask_cvtepi64_storeu_epi8(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 16) {
	    _mm_mask_cvtepi64_storeu_epi8(__mem, __kk, __vi);
	  }
	} else if constexpr (sizeof(_Tp) == 4 && sizeof(_U) == 2) {
	  if constexpr (sizeof(__vi) == 64) {
	    _mm512_mask_cvtepi32_storeu_epi16(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 32) {
	    _mm256_mask_cvtepi32_storeu_epi16(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 16) {
	    _mm_mask_cvtepi32_storeu_epi16(__mem, __kk, __vi);
	  }
	} else if constexpr (sizeof(_Tp) == 4 && sizeof(_U) == 1) {
	  if constexpr (sizeof(__vi) == 64) {
	    _mm512_mask_cvtepi32_storeu_epi8(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 32) {
	    _mm256_mask_cvtepi32_storeu_epi8(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 16) {
	    _mm_mask_cvtepi32_storeu_epi8(__mem, __kk, __vi);
	  }
	} else if constexpr (sizeof(_Tp) == 2 && sizeof(_U) == 1) {
	  if constexpr (sizeof(__vi) == 64) {
	    _mm512_mask_cvtepi16_storeu_epi8(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 32) {
	    _mm256_mask_cvtepi16_storeu_epi8(__mem, __kk, __vi);
	  } else if constexpr (sizeof(__vi) == 16) {
	    _mm_mask_cvtepi16_storeu_epi8(__mem, __kk, __vi);
	  }
	} else {
	  __assert_unreachable<_Tp>();
	}
      } else {
	_Base::__masked_store(__v,__mem,_F(),__k);
      }
    }

    // __multiplies {{{2
    template <typename _V, typename _VVT = _VectorTraits<_V>>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _V
      __multiplies(_V __x, _V __y)
    {
      using _Tp = typename _VVT::value_type;
      if (__builtin_is_constant_evaluated())
	return __as_vector(__x) * __as_vector(__y);
      else if constexpr (sizeof(_Tp) == 1 && sizeof(_V) == 2)
	{
	  const auto __xs = reinterpret_cast<short>(__x._M_data);
	  const auto __ys = reinterpret_cast<short>(__y._M_data);
	  return reinterpret_cast<__vector_type_t<_Tp, 2>>(
	    short(((__xs * __ys) & 0xff) | ((__xs >> 8) * (__ys & 0xff00))));
	}
      else if constexpr (sizeof(_Tp) == 1 && sizeof(_V) == 4 &&
			 _VVT::_S_partial_width == 3)
	{
	  const auto __xi = reinterpret_cast<int>(__x._M_data);
	  const auto __yi = reinterpret_cast<int>(__y._M_data);
	  return reinterpret_cast<__vector_type_t<_Tp, 3>>(
	    ((__xi * __yi) & 0xff) |
	    (((__xi >> 8) * (__yi & 0xff00)) & 0xff00) |
	    ((__xi >> 16) * (__yi & 0xff0000)));
	}
      else if constexpr (sizeof(_Tp) == 1 && sizeof(_V) == 4)
	{
	  const auto __xi = reinterpret_cast<int>(__x._M_data);
	  const auto __yi = reinterpret_cast<int>(__y._M_data);
	  return reinterpret_cast<__vector_type_t<_Tp, 4>>(
	    ((__xi * __yi) & 0xff) |
	    (((__xi >> 8) * (__yi & 0xff00)) & 0xff00) |
	    (((__xi >> 16) * (__yi & 0xff0000)) & 0xff0000) |
	    ((__xi >> 24) * (__yi & 0xff000000u)));
	}
      else if constexpr (sizeof(_Tp) == 1 && sizeof(_V) == 8 && __have_avx2 &&
			 std::is_signed_v<_Tp>)
	return __convert<typename _VVT::type>(
	  __vector_bitcast<short>(_mm_cvtepi8_epi16(__to_intrin(__x))) *
	  __vector_bitcast<short>(_mm_cvtepi8_epi16(__to_intrin(__y))));
      else if constexpr (sizeof(_Tp) == 1 && sizeof(_V) == 8 && __have_avx2 &&
			 std::is_unsigned_v<_Tp>)
	return __convert<typename _VVT::type>(
	  __vector_bitcast<short>(_mm_cvtepu8_epi16(__to_intrin(__x))) *
	  __vector_bitcast<short>(_mm_cvtepu8_epi16(__to_intrin(__y))));
      else if constexpr (sizeof(_Tp) == 1)
	{
	  // codegen of `x*y` is suboptimal (as of GCC 9.0.1)
	  constexpr int _N = sizeof(_V) >= 16 ? _VVT::_S_width / 2 : 8;
	  using _Vs        = __vector_type_t<short, _N>;
	  const _Vs __even =
	    __vector_bitcast<short, _N>(__x) * __vector_bitcast<short, _N>(__y);
	  const _Vs __high_byte = _Vs() - 256;
	  const _Vs __odd       = (__vector_bitcast<short, _N>(__x) >> 8) *
			    (__vector_bitcast<short, _N>(__y) & __high_byte);
	  if constexpr (__have_avx512bw && sizeof(_V) > 2)
	    return __blend(0xaaaa'aaaa'aaaa'aaaaLL,
			   __vector_bitcast<_Tp>(__even),
			   __vector_bitcast<_Tp>(__odd));
	  else if constexpr (__have_sse4_1 && sizeof(_V) > 2)
	    return __intrin_bitcast<typename _VVT::type>(__blend(__high_byte, __even, __odd));
	  else
	    return __intrin_bitcast<typename _VVT::type>(_Vs(__andnot(__high_byte, __even) | __odd));
	}
      else
	return _Base::__multiplies(__x, __y);
    }

    // __divides {{{2
#ifdef _GLIBCXX_SIMD_WORKAROUND_PR90993
    template <typename _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
      __divides(_SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
      if (!__builtin_is_constant_evaluated())
	if constexpr (is_integral_v<_Tp> && sizeof(_Tp) <= 4)
	  { // use divps - codegen of `x/y` is suboptimal (as of GCC 9.0.1)
	    using _Float = conditional_t<sizeof(_Tp) == 4, double, float>;
	    constexpr size_t __n_intermediate =
	      std::min(_N, (__have_avx512f ? 64 : __have_avx ? 32 : 16) /
			     sizeof(_Float));
	    using _FloatV = __vector_type_t<_Float, __n_intermediate>;
	    constexpr size_t __n_floatv = __div_roundup(_N, __n_intermediate);
	    using _R                    = __vector_type_t<_Tp, _N>;
	    const auto __xf = __convert_all<_FloatV, __n_floatv>(__x);
	    constexpr auto __one =
	      ~_Abi::template __implicit_mask<_Tp>() & _Tp(1);
	    const auto __yf =
	      __convert_all<_FloatV, __n_floatv>(__as_vector(__y) | __one);
	    return __call_with_n_evaluations<__n_floatv>(
	      [](auto... __quotients) {
		return __vector_convert<_R>(__quotients...);
	      },
	      [&__xf, &__yf](auto __i) { return __xf[__i] / __yf[__i]; });
	  }
	/* 64-bit int division is potentially optimizable via double division if
	 * the value in __x is small enough and the conversion between
	 * int<->double is efficient enough:
	else if constexpr (is_integral_v<_Tp> && is_unsigned_v<_Tp> &&
			   sizeof(_Tp) == 8)
	  {
	    if constexpr (__have_sse4_1 && sizeof(__x) == 16)
	      {
		if (_mm_test_all_zeros(__x, __m128i{0xffe0'0000'0000'0000ull,
						    0xffe0'0000'0000'0000ull}))
		  {
		    __x._M_data | 0x __vector_convert<__m128d>(__x._M_data)
		  }
	      }
	  }
	  */
      return _Base::__divides(__x, __y);
    }
#endif // _GLIBCXX_SIMD_WORKAROUND_PR90993

    // __bit_shift_left {{{2
    // Notes on UB. C++2a [expr.shift] says:
    // -1- [...] The operands shall be of integral or unscoped enumeration type
    //     and integral promotions are performed. The type of the result is that
    //     of the promoted left operand. The behavior is undefined if the right
    //     operand is negative, or greater than or equal to the width of the
    //     promoted left operand.
    // -2- The value of E1 << E2 is the unique value congruent to E1×2^E2 modulo
    //     2^N, where N is the width of the type of the result.
    //
    // C++17 [expr.shift] says:
    // -2- The value of E1 << E2 is E1 left-shifted E2 bit positions; vacated
    //     bits are zero-filled. If E1 has an unsigned type, the value of the
    //     result is E1 × 2^E2 , reduced modulo one more than the maximum value
    //     representable in the result type. Otherwise, if E1 has a signed type
    //     and non-negative value, and E1 × 2^E2 is representable in the
    //     corresponding unsigned type of the result type, then that value,
    //     converted to the result type, is the resulting value; otherwise, the
    //     behavior is undefined.
    //
    // Consequences:
    // With C++2a signed and unsigned types have the same UB
    // characteristics:
    // - left shift is not UB for 0 <= RHS < max(32, #bits(T))
    //
    // With C++17 there's little room for optimizations because the standard
    // requires all shifts to happen on promoted integrals (i.e. int). Thus,
    // short and char shifts must assume shifts affect bits of neighboring
    // values.
#ifndef _GLIBCXX_SIMD_NO_SHIFT_OPT
    template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
    inline _GLIBCXX_SIMD_CONST static typename _TVT::type
      __bit_shift_left(_Tp __xx, int __y)
    {
      using _V = typename _TVT::type;
      using _U = typename _TVT::value_type;
      _V __x   = __xx;
      [[maybe_unused]] const auto __ix = __to_intrin(__x);
      if (__builtin_is_constant_evaluated())
	return __x << __y;
#if __cplusplus > 201703
      // after C++17, signed shifts have no UB, and behave just like unsigned
      // shifts
      else if constexpr (sizeof(_U) == 1 && is_signed_v<_U>)
	return __vector_bitcast<_U>(
	  __bit_shift_left(__vector_bitcast<make_unsigned_t<_U>>(__x), __y));
#endif
      else if constexpr (sizeof(_U) == 1)
	{
	  // (cf. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83894)
	  if (__builtin_constant_p(__y))
	    {
	      if (__y == 0)
		return __x;
	      else if (__y == 1)
		return __x + __x;
	      else if (__y == 2)
		{
		  __x = __x + __x;
		  return __x + __x;
		}
	      else if (__y > 2 && __y < 8)
		{
		  if constexpr (sizeof(__x) > sizeof(unsigned))
		    {
		      const _UChar __mask = 0xff << __y; // precomputed vector
		      return __vector_bitcast<_U>(
			__vector_bitcast<_UChar>(__vector_bitcast<unsigned>(__x)
						 << __y) &
			__mask);
		    }
		  else
		    {
		      const unsigned __mask =
			(0xff & (0xff << __y)) * 0x01010101u;
		      return reinterpret_cast<_V>(
			static_cast<__int_for_sizeof_t<_V>>(
			  unsigned(reinterpret_cast<__int_for_sizeof_t<_V>>(__x)
				   << __y) &
			  __mask));
		    }
		}
	      else if (__y >= 8 && __y < 32)
		return _V();
	      else
		__builtin_unreachable();
	    }
	  // general strategy in the following: use an sllv instead of sll
	  // instruction, because it's 2 to 4 times faster:
	  else if constexpr (__have_avx512bw_vl && sizeof(__x) == 16)
	    return __vector_bitcast<_U>(_mm256_cvtepi16_epi8(_mm256_sllv_epi16(
	      _mm256_cvtepi8_epi16(__ix), _mm256_set1_epi16(__y))));
	  else if constexpr (__have_avx512bw && sizeof(__x) == 32)
	    return __vector_bitcast<_U>(_mm512_cvtepi16_epi8(_mm512_sllv_epi16(
	      _mm512_cvtepi8_epi16(__ix), _mm512_set1_epi16(__y))));
	  else if constexpr (__have_avx512bw && sizeof(__x) == 64)
	    {
	      const auto __shift = _mm512_set1_epi16(__y);
	      return __vector_bitcast<_U>(
		__concat(_mm512_cvtepi16_epi8(_mm512_sllv_epi16(
			   _mm512_cvtepi8_epi16(__lo256(__ix)), __shift)),
			 _mm512_cvtepi16_epi8(_mm512_sllv_epi16(
			   _mm512_cvtepi8_epi16(__hi256(__ix)), __shift))));
	    }
	  else if constexpr (__have_avx2 && sizeof(__x) == 32)
	    {
#if 1
	      const auto __shift = _mm_cvtsi32_si128(__y);
	      auto __k = _mm256_sll_epi16(_mm256_slli_epi16(~__m256i(), 8), __shift);
	      __k |= _mm256_srli_epi16(__k, 8);
	      return __vector_bitcast<_U>(_mm256_sll_epi32(__ix, __shift) &
					  __k);
#else
	      const _U __k = 0xff << __y;
	      return __vector_bitcast<_U>(__vector_bitcast<int>(__x) << __y) &
		     __k;
#endif
	    }
	  else
	    {
	      const auto __shift = _mm_cvtsi32_si128(__y);
	      auto __k = _mm_sll_epi16(_mm_slli_epi16(~__m128i(), 8), __shift);
	      __k |= _mm_srli_epi16(__k, 8);
	      return __intrin_bitcast<_V>(_mm_sll_epi16(__ix, __shift) & __k);
	    }
	}
      return __x << __y;
    }

    template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
    inline _GLIBCXX_SIMD_CONST static typename _TVT::type
      __bit_shift_left(_Tp __xx, typename _TVT::type __y)
    {
      using _V                         = typename _TVT::type;
      using _U                         = typename _TVT::value_type;
      _V                          __x  = __xx;
      [[maybe_unused]] const auto __ix = __to_intrin(__x);
      [[maybe_unused]] const auto __iy = __to_intrin(__y);
      if (__builtin_is_constant_evaluated())
	return __x << __y;
#if __cplusplus > 201703
      // after C++17, signed shifts have no UB, and behave just like unsigned
      // shifts
      else if constexpr (is_signed_v<_U>)
	return __vector_bitcast<_U>(
	  __bit_shift_left(__vector_bitcast<make_unsigned_t<_U>>(__x),
			   __vector_bitcast<make_unsigned_t<_U>>(__y)));
#endif
      else if constexpr (sizeof(_U) == 1)
	{
	  if constexpr (sizeof __ix == 64 && __have_avx512bw)
	    return __vector_bitcast<_U>(__concat(
	      _mm512_cvtepi16_epi8(
		_mm512_sllv_epi16(_mm512_cvtepu8_epi16(__lo256(__ix)),
				  _mm512_cvtepu8_epi16(__lo256(__iy)))),
	      _mm512_cvtepi16_epi8(
		_mm512_sllv_epi16(_mm512_cvtepu8_epi16(__hi256(__ix)),
				  _mm512_cvtepu8_epi16(__hi256(__iy))))));
	  else if constexpr (sizeof __ix == 32 && __have_avx512bw)
	    return __vector_bitcast<_U>(_mm512_cvtepi16_epi8(_mm512_sllv_epi16(
	      _mm512_cvtepu8_epi16(__ix), _mm512_cvtepu8_epi16(__iy))));
	  else if constexpr (sizeof __x <= 8 && __have_avx512bw_vl)
	    return __intrin_bitcast<_V>(_mm_cvtepi16_epi8(_mm_sllv_epi16(
	      _mm_cvtepu8_epi16(__ix), _mm_cvtepu8_epi16(__iy))));
	  else if constexpr (sizeof __ix == 16 && __have_avx512bw_vl)
	    return __intrin_bitcast<_V>(_mm256_cvtepi16_epi8(_mm256_sllv_epi16(
	      _mm256_cvtepu8_epi16(__ix), _mm256_cvtepu8_epi16(__iy))));
	  else if constexpr (sizeof __ix == 16 && __have_avx512bw)
	    return __intrin_bitcast<_V>(
	      __lo128(_mm512_cvtepi16_epi8(_mm512_sllv_epi16(
		_mm512_cvtepu8_epi16(_mm256_castsi128_si256(__ix)),
		_mm512_cvtepu8_epi16(_mm256_castsi128_si256(__iy))))));
	  else if constexpr (__have_sse4_1 && sizeof(__x) == 16)
	    {
              auto __mask = __vector_bitcast<_U>(__vector_bitcast<short>(__y) << 5);
	      auto __x4   = __vector_bitcast<_U>(__vector_bitcast<short>(__x) << 4);
	      __x4 &= char(0xf0);
              __x = __blend(__mask, __x, __x4);
              __mask += __mask;
	      auto __x2   = __vector_bitcast<_U>(__vector_bitcast<short>(__x) << 2);
	      __x2 &= char(0xfc);
              __x = __blend(__mask, __x, __x2);
              __mask += __mask;
	      auto __x1   = __x + __x;
              __x = __blend(__mask, __x, __x1);
	      return __x & ((__y & char(0xf8)) == 0); // y > 7 nulls the result
	    }
          else if constexpr (sizeof(__x) == 16)
	    {
              auto __mask = __vector_bitcast<_UChar>(__vector_bitcast<short>(__y) << 5);
	      auto __x4   = __vector_bitcast<_U>(__vector_bitcast<short>(__x) << 4);
	      __x4 &= char(0xf0);
              __x = __blend(__vector_bitcast<_SChar>(__mask) < 0, __x, __x4);
              __mask += __mask;
	      auto __x2   = __vector_bitcast<_U>(__vector_bitcast<short>(__x) << 2);
	      __x2 &= char(0xfc);
              __x = __blend(__vector_bitcast<_SChar>(__mask) < 0, __x, __x2);
              __mask += __mask;
	      auto __x1   = __x + __x;
              __x = __blend(__vector_bitcast<_SChar>(__mask) < 0, __x, __x1);
	      return __x & ((__y & char(0xf8)) == 0); // y > 7 nulls the result
	    }
	  else
	    return __x << __y;
	}
      else if constexpr (sizeof(_U) == 2)
	{
	  if constexpr (sizeof __ix == 64 && __have_avx512bw)
	    return __vector_bitcast<_U>(_mm512_sllv_epi16(__ix, __iy));
	  else if constexpr (sizeof __ix == 32 && __have_avx512bw_vl)
	    return __vector_bitcast<_U>(_mm256_sllv_epi16(__ix, __iy));
	  else if constexpr (sizeof __ix == 32 && __have_avx512bw)
	    return __vector_bitcast<_U>(__lo256(_mm512_sllv_epi16(
	      _mm512_castsi256_si512(__ix), _mm512_castsi256_si512(__iy))));
	  else if constexpr (sizeof __ix == 32 && __have_avx2)
	    {
	      const auto __ux = __vector_bitcast<unsigned>(__x);
	      const auto __uy = __vector_bitcast<unsigned>(__y);
	      return __vector_bitcast<_U>(_mm256_blend_epi16(
		__auto_bitcast(__ux << (__uy & 0x0000ffffu)),
		__auto_bitcast((__ux & 0xffff0000u) << (__uy >> 16)), 0xaa));
	    }
	  else if constexpr (sizeof __ix == 16 && __have_avx512bw_vl)
	    return __intrin_bitcast<_V>(_mm_sllv_epi16(__ix, __iy));
	  else if constexpr (sizeof __ix == 16 && __have_avx512bw)
	    return __intrin_bitcast<_V>(__lo128(_mm512_sllv_epi16(
	      _mm512_castsi128_si512(__ix), _mm512_castsi128_si512(__iy))));
	  else if constexpr (sizeof __ix == 16 && __have_avx2)
	    {
	      const auto __ux = __vector_bitcast<unsigned>(__ix);
	      const auto __uy = __vector_bitcast<unsigned>(__iy);
	      return __intrin_bitcast<_V>(_mm_blend_epi16(
		__auto_bitcast(__ux << (__uy & 0x0000ffffu)),
		__auto_bitcast((__ux & 0xffff0000u) << (__uy >> 16)), 0xaa));
	    }
	  else if constexpr (sizeof __ix == 16)
	    {
	      __y += 0x3f8 >> 3;
	      return __x *
		     __intrin_bitcast<_V>(
		       __vector_convert<__vector_type16_t<int>>(
			 __vector_bitcast<float>(
			   __vector_bitcast<unsigned>(__to_intrin(__y))
			   << 23)) |
		       (__vector_convert<__vector_type16_t<int>>(
			  __vector_bitcast<float>(
			    (__vector_bitcast<unsigned>(__to_intrin(__y)) >> 16)
			    << 23))
			<< 16));
	    }
	  else
	    __assert_unreachable<_Tp>();
	}
      else if constexpr (sizeof(_U) == 4 && sizeof __ix == 16 && !__have_avx2)
        // latency is suboptimal, but throughput is at full speedup
	return __intrin_bitcast<_V>(
	  __vector_bitcast<unsigned>(__ix) *
	  __vector_convert<__vector_type16_t<int>>(__vector_bitcast<float>(
	    (__vector_bitcast<unsigned, 4>(__y) << 23) + 0x3f80'0000)));
      else if constexpr (sizeof(_U) == 8 && sizeof __ix == 16 && !__have_avx2)
	{
	  const auto __lo = _mm_sll_epi64(__ix, __iy);
	  const auto __hi = _mm_sll_epi64(__ix, _mm_unpackhi_epi64(__iy, __iy));
	  if constexpr (__have_sse4_1)
	    return __vector_bitcast<_U>(_mm_blend_epi16(__lo, __hi, 0xf0));
	  else
	    return __vector_bitcast<_U>(_mm_move_sd(
	      __vector_bitcast<double>(__hi), __vector_bitcast<double>(__lo)));
	}
      else
	return __x << __y;
    }
#endif // _GLIBCXX_SIMD_NO_SHIFT_OPT

    // __bit_shift_right {{{2
#ifndef _GLIBCXX_SIMD_NO_SHIFT_OPT
    template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
    inline _GLIBCXX_SIMD_CONST static typename _TVT::type
      __bit_shift_right(_Tp __xx, int __y)
    {
      using _V = typename _TVT::type;
      using _U = typename _TVT::value_type;
      _V __x   = __xx;
      [[maybe_unused]] const auto __ix = __to_intrin(__x);
      if (__builtin_is_constant_evaluated())
	return __x >> __y;
      else if (__builtin_constant_p(__y) && std::is_unsigned_v<_U> &&
	       __y >= int(sizeof(_U) * CHAR_BIT))
	return _V();
      else if constexpr (sizeof(_U) == 1 && is_unsigned_v<_U>) //{{{
	return __intrin_bitcast<_V>(__vector_bitcast<_UShort>(__ix) >> __y) &
	       _U(0xff >> __y);
      //}}}
      else if constexpr (sizeof(_U) == 1 && is_signed_v<_U>) //{{{
	return __intrin_bitcast<_V>(
	  (__vector_bitcast<_UShort>(__vector_bitcast<short>(__ix) >> (__y + 8))
	   << 8) |
	  (__vector_bitcast<_UShort>(
	     __vector_bitcast<short>(__vector_bitcast<_UShort>(__ix) << 8) >>
	     __y) >>
	   8));
      //}}}
      // GCC optimizes sizeof == 2, 4, and unsigned 8 as expected
      else if constexpr (sizeof(_U) == 8 && is_signed_v<_U>) //{{{
      {
	if (__y > 32)
	  return (__intrin_bitcast<_V>(__vector_bitcast<int>(__ix) >> 32) &
		  _U(0xffff'ffff'0000'0000ull)) |
		 __vector_bitcast<_U>(__vector_bitcast<int>(
					__vector_bitcast<_ULLong>(__ix) >> 32) >>
				      (__y - 32));
	else
	  return __intrin_bitcast<_V>(__vector_bitcast<_ULLong>(__ix) >> __y) |
		 __vector_bitcast<_U>(
		   __vector_bitcast<int>(__ix & -0x8000'0000'0000'0000ll) >>
		   __y);
      }
      //}}}
      else
	return __x >> __y;
    }

    template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
    inline _GLIBCXX_SIMD_CONST static typename _TVT::type
      __bit_shift_right(_Tp __xx, typename _TVT::type __y)
    {
      using _V                         = typename _TVT::type;
      using _U                         = typename _TVT::value_type;
      _V                          __x  = __xx;
      [[maybe_unused]] const auto __ix = __to_intrin(__x);
      [[maybe_unused]] const auto __iy = __to_intrin(__y);
      if (__builtin_is_constant_evaluated())
	return __x >> __y;
      else if constexpr (sizeof(_U) == 1) //{{{
	{
	  if constexpr (sizeof(__x) <= 8 && __have_avx512bw_vl)
	    return __intrin_bitcast<_V>(_mm_cvtepi16_epi8(
	      is_signed_v<_U> ? _mm_srav_epi16(_mm_cvtepi8_epi16(__ix),
					       _mm_cvtepi8_epi16(__iy))
			      : _mm_srlv_epi16(_mm_cvtepu8_epi16(__ix),
					       _mm_cvtepu8_epi16(__iy))));
	  if constexpr (sizeof(__x) == 16 && __have_avx512bw_vl)
	    return __intrin_bitcast<_V>(_mm256_cvtepi16_epi8(
	      is_signed_v<_U> ? _mm256_srav_epi16(_mm256_cvtepi8_epi16(__ix),
						  _mm256_cvtepi8_epi16(__iy))
			      : _mm256_srlv_epi16(_mm256_cvtepu8_epi16(__ix),
						  _mm256_cvtepu8_epi16(__iy))));
	  else if constexpr (sizeof(__x) == 32 && __have_avx512bw)
	    return __vector_bitcast<_U>(_mm512_cvtepi16_epi8(
	      is_signed_v<_U> ? _mm512_srav_epi16(_mm512_cvtepi8_epi16(__ix),
						  _mm512_cvtepi8_epi16(__iy))
			      : _mm512_srlv_epi16(_mm512_cvtepu8_epi16(__ix),
						  _mm512_cvtepu8_epi16(__iy))));
	  else if constexpr (sizeof(__x) == 64 && is_signed_v<_U>)
	    return __vector_bitcast<_U>(_mm512_mask_mov_epi8(
	      _mm512_srav_epi16(__ix, _mm512_srli_epi16(__iy, 8)),
	      0x5555'5555'5555'5555ull,
	      _mm512_srav_epi16(_mm512_slli_epi16(__ix, 8),
				_mm512_maskz_add_epi8(0x5555'5555'5555'5555ull,
						      __iy,
						      _mm512_set1_epi16(8)))));
	  else if constexpr (sizeof(__x) == 64 && is_unsigned_v<_U>)
	    return __vector_bitcast<_U>(_mm512_mask_mov_epi8(
	      _mm512_srlv_epi16(__ix, _mm512_srli_epi16(__iy, 8)),
	      0x5555'5555'5555'5555ull,
	      _mm512_srlv_epi16(
		_mm512_maskz_mov_epi8(0x5555'5555'5555'5555ull, __ix),
		_mm512_maskz_mov_epi8(0x5555'5555'5555'5555ull, __iy))));
	  /* This has better throughput but higher latency than the impl below
	  else if constexpr (__have_avx2 && sizeof(__x) == 16 &&
			     is_unsigned_v<_U>)
	    {
	      const auto __shorts = __to_intrin(__bit_shift_right(
		__vector_bitcast<_UShort>(_mm256_cvtepu8_epi16(__ix)),
		__vector_bitcast<_UShort>(_mm256_cvtepu8_epi16(__iy))));
	      return __vector_bitcast<_U>(
		_mm_packus_epi16(__lo128(__shorts), __hi128(__shorts)));
	    }
	    */
	  else if constexpr (__have_avx2 && sizeof(__x) > 8)
	    // the following uses vpsr[al]vd, which requires AVX2
	    if constexpr (is_signed_v<_U>)
	      {
		const auto r3 = __vector_bitcast<_UInt>(
				  (__vector_bitcast<int>(__x) >>
				   (__vector_bitcast<_UInt>(__y) >> 24))) &
				0xff000000u;
		const auto r2 =
		  __vector_bitcast<_UInt>(
		    ((__vector_bitcast<int>(__x) << 8) >>
		     ((__vector_bitcast<_UInt>(__y) << 8) >> 24))) &
		  0xff000000u;
		const auto r1 =
		  __vector_bitcast<_UInt>(
		    ((__vector_bitcast<int>(__x) << 16) >>
		     ((__vector_bitcast<_UInt>(__y) << 16) >> 24))) &
		  0xff000000u;
		const auto r0 = __vector_bitcast<_UInt>(
		  (__vector_bitcast<int>(__x) << 24) >>
		  ((__vector_bitcast<_UInt>(__y) << 24) >> 24));
		return __vector_bitcast<_U>(r3 | (r2 >> 8) | (r1 >> 16) |
					    (r0 >> 24));
	      }
	    else
	      {
		const auto r3 = (__vector_bitcast<_UInt>(__x) >>
				 (__vector_bitcast<_UInt>(__y) >> 24)) &
				0xff000000u;
		const auto r2 = ((__vector_bitcast<_UInt>(__x) << 8) >>
				 ((__vector_bitcast<_UInt>(__y) << 8) >> 24)) &
				0xff000000u;
		const auto r1 = ((__vector_bitcast<_UInt>(__x) << 16) >>
				 ((__vector_bitcast<_UInt>(__y) << 16) >> 24)) &
				0xff000000u;
		const auto r0 = (__vector_bitcast<_UInt>(__x) << 24) >>
				((__vector_bitcast<_UInt>(__y) << 24) >> 24);
		return __vector_bitcast<_U>(r3 | (r2 >> 8) | (r1 >> 16) |
					    (r0 >> 24));
	      }
	  else if constexpr (__have_sse4_1 && is_unsigned_v<_U> && sizeof(__x) > 2)
	    {
	      auto __x128 = __vector_bitcast<_U>(__ix);
	      auto __mask =
		__vector_bitcast<_U>(__vector_bitcast<_UShort>(__iy) << 5);
	      auto __x4 = __vector_bitcast<_U>(
		(__vector_bitcast<_UShort>(__x128) >> 4) & _UShort(0xff0f));
	      __x128 = __blend(__mask, __x128, __x4);
	      __mask += __mask;
	      auto __x2 = __vector_bitcast<_U>(
		(__vector_bitcast<_UShort>(__x128) >> 2) & _UShort(0xff3f));
	      __x128 = __blend(__mask, __x128, __x2);
	      __mask += __mask;
	      auto __x1 = __vector_bitcast<_U>(
		(__vector_bitcast<_UShort>(__x128) >> 1) & _UShort(0xff7f));
	      __x128 = __blend(__mask, __x128, __x1);
	      return __intrin_bitcast<_V>(
		__x128 & ((__vector_bitcast<_U>(__iy) & char(0xf8)) ==
			  0)); // y > 7 nulls the result
	    }
	  else if constexpr (__have_sse4_1 && is_signed_v<_U> && sizeof(__x) > 2)
	    {
	      auto __mask =
		__vector_bitcast<_UChar>(__vector_bitcast<_UShort>(__iy) << 5);
	      auto __maskl = [&]() {
		return __vector_bitcast<_UShort>(__mask) << 8;
	      };
	      auto __xh  = __vector_bitcast<short>(__ix);
	      auto __xl  = __vector_bitcast<short>(__ix) << 8;
	      auto __xh4 = __xh >> 4;
	      auto __xl4 = __xl >> 4;
	      __xh       = __blend(__mask, __xh, __xh4);
	      __xl       = __blend(__maskl(), __xl, __xl4);
	      __mask += __mask;
	      auto __xh2 = __xh >> 2;
	      auto __xl2 = __xl >> 2;
	      __xh       = __blend(__mask, __xh, __xh2);
	      __xl       = __blend(__maskl(), __xl, __xl2);
	      __mask += __mask;
	      auto __xh1 = __xh >> 1;
	      auto __xl1 = __xl >> 1;
	      __xh       = __blend(__mask, __xh, __xh1);
	      __xl       = __blend(__maskl(), __xl, __xl1);
	      return __intrin_bitcast<_V>(
		(__vector_bitcast<_U>((__xh & short(0xff00))) |
		 __vector_bitcast<_U>(__vector_bitcast<_UShort>(__xl) >> 8)) &
		((__vector_bitcast<_U>(__iy) & char(0xf8)) ==
		 0)); // y > 7 nulls the result
	    }
	  else if constexpr (is_unsigned_v<_U> && sizeof(__x) > 2) // SSE2
	    {
	      auto __mask =
		__vector_bitcast<_U>(__vector_bitcast<_UShort>(__y) << 5);
	      auto __x4 = __vector_bitcast<_U>(
		(__vector_bitcast<_UShort>(__x) >> 4) & _UShort(0xff0f));
	      __x = __blend(__mask > 0x7f, __x, __x4);
	      __mask += __mask;
	      auto __x2 = __vector_bitcast<_U>(
		(__vector_bitcast<_UShort>(__x) >> 2) & _UShort(0xff3f));
	      __x = __blend(__mask > 0x7f, __x, __x2);
	      __mask += __mask;
	      auto __x1 = __vector_bitcast<_U>(
		(__vector_bitcast<_UShort>(__x) >> 1) & _UShort(0xff7f));
	      __x = __blend(__mask > 0x7f, __x, __x1);
	      return __x & ((__y & char(0xf8)) == 0); // y > 7 nulls the result
	    }
	  else if constexpr (sizeof(__x) > 2) // signed SSE2
	    {
	      static_assert(is_signed_v<_U>);
	      auto __maskh = __vector_bitcast<_UShort>(__y) << 5;
	      auto __maskl = __vector_bitcast<_UShort>(__y) << (5 + 8);
	      auto __xh  = __vector_bitcast<short>(__x);
	      auto __xl  = __vector_bitcast<short>(__x) << 8;
	      auto __xh4 = __xh >> 4;
	      auto __xl4 = __xl >> 4;
	      __xh       = __blend(__maskh > 0x7fff, __xh, __xh4);
	      __xl       = __blend(__maskl > 0x7fff, __xl, __xl4);
	      __maskh += __maskh;
	      __maskl += __maskl;
	      auto __xh2 = __xh >> 2;
	      auto __xl2 = __xl >> 2;
	      __xh       = __blend(__maskh > 0x7fff, __xh, __xh2);
	      __xl       = __blend(__maskl > 0x7fff, __xl, __xl2);
	      __maskh += __maskh;
	      __maskl += __maskl;
	      auto __xh1 = __xh >> 1;
	      auto __xl1 = __xl >> 1;
	      __xh       = __blend(__maskh > 0x7fff, __xh, __xh1);
	      __xl       = __blend(__maskl > 0x7fff, __xl, __xl1);
	      __x        = __vector_bitcast<_U>((__xh & short(0xff00))) |
		    __vector_bitcast<_U>(__vector_bitcast<_UShort>(__xl) >> 8);
	      return __x & ((__y & char(0xf8)) == 0); // y > 7 nulls the result
	    }
	  else
	    return __x >> __y;
	} //}}}
      else if constexpr (sizeof(_U) == 2 && sizeof(__x) >= 4) //{{{
	{
	  [[maybe_unused]] auto __blend_0xaa =
	    [](auto __a, auto __b) {
	      if constexpr (sizeof(__a) == 16)
		return _mm_blend_epi16(__to_intrin(__a), __to_intrin(__b),
				       0xaa);
	      else if constexpr (sizeof(__a) == 32)
		return _mm256_blend_epi16(__to_intrin(__a), __to_intrin(__b),
					  0xaa);
	      else if constexpr (sizeof(__a) == 64)
		return _mm512_mask_blend_epi16(0xaaaa'aaaaU, __to_intrin(__a),
					       __to_intrin(__b));
	      else
		__assert_unreachable<decltype(__a)>();
	    };
	  if constexpr (__have_avx512bw_vl && sizeof(_Tp) <= 16)
	    return __intrin_bitcast<_V>(is_signed_v<_U>
					  ? _mm_srav_epi16(__ix, __iy)
					  : _mm_srlv_epi16(__ix, __iy));
	  else if constexpr (__have_avx512bw_vl && sizeof(_Tp) == 32)
	    return __vector_bitcast<_U>(is_signed_v<_U>
					  ? _mm256_srav_epi16(__ix, __iy)
					  : _mm256_srlv_epi16(__ix, __iy));
	  else if constexpr (__have_avx512bw && sizeof(_Tp) == 64)
	    return __vector_bitcast<_U>(is_signed_v<_U>
					  ? _mm512_srav_epi16(__ix, __iy)
					  : _mm512_srlv_epi16(__ix, __iy));
	  else if constexpr (__have_avx2 && is_signed_v<_U>)
	    return __intrin_bitcast<_V>(
	      __blend_0xaa(((__vector_bitcast<int>(__ix) << 16) >>
			     (__vector_bitcast<int>(__iy) & 0xffffu)) >> 16,
			   __vector_bitcast<int>(__ix) >>
			     (__vector_bitcast<int>(__iy) >> 16)));
	  else if constexpr (__have_avx2 && is_unsigned_v<_U>)
	    return __intrin_bitcast<_V>(
	      __blend_0xaa((__vector_bitcast<_UInt>(__ix) & 0xffffu) >>
			     (__vector_bitcast<_UInt>(__iy) & 0xffffu),
			   __vector_bitcast<_UInt>(__ix) >>
			     (__vector_bitcast<_UInt>(__iy) >> 16)));
	  else if constexpr (__have_sse4_1)
	    {
	      auto __mask = __vector_bitcast<_UShort>(__iy);
	      auto __x128 = __vector_bitcast<_U>(__ix);
	      //__mask *= 0x0808;
	      __mask = (__mask << 3) | (__mask << 11);
	      // do __x128 = 0 where __y[4] is set
	      __x128 = __blend(__mask, __x128, decltype(__x128)());
	      // do __x128 =>> 8 where __y[3] is set
	      __x128 = __blend(__mask += __mask, __x128, __x128 >> 8);
	      // do __x128 =>> 4 where __y[2] is set
	      __x128 = __blend(__mask += __mask, __x128, __x128 >> 4);
	      // do __x128 =>> 2 where __y[1] is set
	      __x128 = __blend(__mask += __mask, __x128, __x128 >> 2);
	      // do __x128 =>> 1 where __y[0] is set
	      return __intrin_bitcast<_V>(__blend(__mask + __mask, __x128, __x128 >> 1));
	    }
	  else
	    {
	      auto __k = __vector_bitcast<_UShort>(__iy) << 11;
	      auto __x128 = __vector_bitcast<_U>(__ix);
	      auto __mask = [](__vector_type16_t<_UShort> __kk) {
		return __vector_bitcast<short>(__kk) < 0;
	      };
	      // do __x128 = 0 where __y[4] is set
	      __x128 = __blend(__mask(__k), __x128, decltype(__x128)());
	      // do __x128 =>> 8 where __y[3] is set
	      __x128 = __blend(__mask(__k += __k), __x128, __x128 >> 8);
	      // do __x128 =>> 4 where __y[2] is set
	      __x128 = __blend(__mask(__k += __k), __x128, __x128 >> 4);
	      // do __x128 =>> 2 where __y[1] is set
	      __x128 = __blend(__mask(__k += __k), __x128, __x128 >> 2);
	      // do __x128 =>> 1 where __y[0] is set
	      return __intrin_bitcast<_V>(__blend(__mask(__k + __k), __x128, __x128 >> 1));
	    }
	} //}}}
      else if constexpr (sizeof(_U) == 4 && !__have_avx2) //{{{
	{
	  if constexpr (is_unsigned_v<_U>)
	    {
	      // x >> y == x * 2^-y == (x * 2^(31-y)) >> 31
	      const __m128 __factor_f = reinterpret_cast<__m128>(
		0x4f00'0000u - (__vector_bitcast<unsigned, 4>(__y) << 23));
	      const __m128i __factor = __builtin_constant_p(__factor_f)
					 ? __to_intrin(__make_vector<int>(
					     __factor_f[0], __factor_f[1],
					     __factor_f[2], __factor_f[3]))
					 : _mm_cvttps_epi32(__factor_f);
	      const auto __r02 = _mm_srli_epi64(_mm_mul_epu32(__ix, __factor), 31);
	      const auto __r13 = _mm_mul_epu32(_mm_srli_si128(__ix, 4),
					       _mm_srli_si128(__factor, 4));
	      if constexpr (__have_sse4_1)
		return __intrin_bitcast<_V>(
		  _mm_blend_epi16(_mm_slli_epi64(__r13, 1), __r02, 0x33));
	      else
		return __intrin_bitcast<_V>(
		  __r02 | _mm_slli_si128(_mm_srli_epi64(__r13, 31), 4));
	    }
	  else
	    {
	      auto __shift = [](auto __a, auto __b) {
		if constexpr (is_signed_v<_U>)
		  return _mm_sra_epi32(__a, __b);
		else
		  return _mm_srl_epi32(__a, __b);
	      };
	      const auto __r0 =
		__shift(__ix, _mm_unpacklo_epi32(__iy, __m128i()));
	      const auto __r1 = __shift(__ix, _mm_srli_epi64(__iy, 32));
	      const auto __r2 =
		__shift(__ix, _mm_unpackhi_epi32(__iy, __m128i()));
	      const auto __r3 = __shift(__ix, _mm_srli_si128(__iy, 12));
	      if constexpr (__have_sse4_1)
		return __intrin_bitcast<_V>(
		  _mm_blend_epi16(_mm_blend_epi16(__r1, __r0, 0x3),
				  _mm_blend_epi16(__r3, __r2, 0x30), 0xf0));
	      else
		return __intrin_bitcast<_V>(_mm_unpacklo_epi64(
		  _mm_unpacklo_epi32(__r0, _mm_srli_si128(__r1, 4)),
		  _mm_unpackhi_epi32(__r2, _mm_srli_si128(__r3, 4))));
	    }
	} //}}}
      else
	return __x >> __y;
    }
#endif // _GLIBCXX_SIMD_NO_SHIFT_OPT

    // compares {{{2
    // __equal_to {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp> __equal_to(
        _SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
      if constexpr (sizeof(__x) == 64) {  // AVX512
	[[maybe_unused]] const auto __xi = __to_intrin(__x);
	[[maybe_unused]] const auto __yi = __to_intrin(__y);
	if constexpr (std::is_floating_point_v<_Tp>) {
	  if constexpr (sizeof(_Tp) == 8) { return _mm512_cmp_pd_mask(__xi, __yi, _CMP_EQ_OQ);
	  } else if constexpr (sizeof(_Tp) == 4) { return _mm512_cmp_ps_mask(__xi, __yi, _CMP_EQ_OQ);
	  } else { __assert_unreachable<_Tp>(); }
	} else {
	  if constexpr (sizeof(_Tp) == 8) { return _mm512_cmpeq_epi64_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 4) { return _mm512_cmpeq_epi32_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 2) { return _mm512_cmpeq_epi16_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 1) { return _mm512_cmpeq_epi8_mask(__xi, __yi);
	  } else { __assert_unreachable<_Tp>(); }
	}
      } else
	return _Base::__equal_to(__x,__y);
    }

    // __not_equal_to {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp> __not_equal_to(
        _SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
      if constexpr (sizeof(__x) == 64) {  // AVX512
	[[maybe_unused]] const auto __xi = __to_intrin(__x);
	[[maybe_unused]] const auto __yi = __to_intrin(__y);
	if constexpr (std::is_floating_point_v<_Tp>) {
	  if constexpr (sizeof(_Tp) == 8) { return _mm512_cmp_pd_mask(__xi, __yi, _CMP_NEQ_UQ);
	  } else if constexpr (sizeof(_Tp) == 4) { return _mm512_cmp_ps_mask(__xi, __yi, _CMP_NEQ_UQ);
	  } else { __assert_unreachable<_Tp>(); }
	} else {
	  if constexpr (sizeof(_Tp) == 8) { return ~_mm512_cmpeq_epi64_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 4) { return ~_mm512_cmpeq_epi32_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 2) { return ~_mm512_cmpeq_epi16_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 1) { return ~_mm512_cmpeq_epi8_mask(__xi, __yi);
	  } else { __assert_unreachable<_Tp>(); }
	}
      } else
	return _Base::__not_equal_to(__x, __y);
    }

    // __less {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp>
      __less(_SimdWrapper<_Tp, _N> __x, _SimdWrapper<_Tp, _N> __y)
    {
      if constexpr (sizeof(__x) == 64)
	{ // AVX512
	  constexpr auto __k1 = _Abi::template __implicit_mask<_Tp>();
	  [[maybe_unused]] const auto __xi = __to_intrin(__x);
	  [[maybe_unused]] const auto __yi = __to_intrin(__y);
	  if constexpr (std::is_same_v<_Tp, float>)
	    return _mm512_mask_cmp_ps_mask(__k1, __xi, __yi, _CMP_LT_OS);
	  else if constexpr (std::is_same_v<_Tp, double>)
	    return _mm512_mask_cmp_pd_mask(__k1, __xi, __yi, _CMP_LT_OS);
	  else if constexpr (std::is_signed_v<_Tp> && sizeof(_Tp) == 1)
	    return _mm512_mask_cmplt_epi8_mask(__k1, __xi, __yi);
	  else if constexpr (std::is_signed_v<_Tp> && sizeof(_Tp) == 2)
	    return _mm512_mask_cmplt_epi16_mask(__k1, __xi, __yi);
	  else if constexpr (std::is_signed_v<_Tp> && sizeof(_Tp) == 4)
	    return _mm512_mask_cmplt_epi32_mask(__k1, __xi, __yi);
	  else if constexpr (std::is_signed_v<_Tp> && sizeof(_Tp) == 8)
	    return _mm512_mask_cmplt_epi64_mask(__k1, __xi, __yi);
	  else if constexpr (std::is_unsigned_v<_Tp> && sizeof(_Tp) == 1)
	    return _mm512_mask_cmplt_epu8_mask(__k1, __xi, __yi);
	  else if constexpr (std::is_unsigned_v<_Tp> && sizeof(_Tp) == 2)
	    return _mm512_mask_cmplt_epu16_mask(__k1, __xi, __yi);
	  else if constexpr (std::is_unsigned_v<_Tp> && sizeof(_Tp) == 4)
	    return _mm512_mask_cmplt_epu32_mask(__k1, __xi, __yi);
	  else if constexpr (std::is_unsigned_v<_Tp> && sizeof(_Tp) == 8)
	    return _mm512_mask_cmplt_epu64_mask(__k1, __xi, __yi);
	  else
	    __assert_unreachable<_Tp>();
	}
      else
	return _Base::__less(__x, __y);
    }

    // __less_equal {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp> __less_equal(_SimdWrapper<_Tp, _N> __x,
                                                                 _SimdWrapper<_Tp, _N> __y)
    {
      if constexpr (sizeof(__x) == 64) {  // AVX512
	[[maybe_unused]] const auto __xi = __to_intrin(__x);
	[[maybe_unused]] const auto __yi = __to_intrin(__y);
	if constexpr (std::is_floating_point_v<_Tp>) {
	  if constexpr (sizeof(_Tp) == 8) { return _mm512_cmp_pd_mask(__xi, __yi, _CMP_LE_OS);
	  } else if constexpr (sizeof(_Tp) == 4) { return _mm512_cmp_ps_mask(__xi, __yi, _CMP_LE_OS);
	  } else { __assert_unreachable<_Tp>(); }
	} else if constexpr (std::is_signed_v<_Tp>) {
	  if constexpr (sizeof(_Tp) == 8) { return _mm512_cmple_epi64_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 4) { return _mm512_cmple_epi32_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 2) { return _mm512_cmple_epi16_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 1) { return _mm512_cmple_epi8_mask(__xi, __yi);
	  } else { __assert_unreachable<_Tp>(); }
	} else {
	  static_assert(std::is_unsigned_v<_Tp>);
	  if constexpr (sizeof(_Tp) == 8) { return _mm512_cmple_epu64_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 4) { return _mm512_cmple_epu32_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 2) { return _mm512_cmple_epu16_mask(__xi, __yi);
	  } else if constexpr (sizeof(_Tp) == 1) { return _mm512_cmple_epu8_mask(__xi, __yi);
	  } else { __assert_unreachable<_Tp>(); }
	}
      } else
	return _Base::__less_equal(__x, __y);
    }

    // negation {{{2
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static constexpr _MaskMember<_Tp> __negate(_SimdWrapper<_Tp, _N> __x) noexcept
    {
      if constexpr (__is_abi<_Abi, simd_abi::_Avx512Abi>()) {
	  return __equal_to(__x, _SimdWrapper<_Tp, _N>());
      } else {
	return _Base::__negate(__x);
      }
    }

    // math {{{2
    using _Base::__abs;
    // __sqrt {{{3
    template <class _Tp, size_t _N> _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N> __sqrt(_SimdWrapper<_Tp, _N> __x)
    {
               if constexpr (__is_sse_ps   <_Tp, _N>()) { return __auto_bitcast(_mm_sqrt_ps(__to_intrin(__x)));
        } else if constexpr (__is_sse_pd   <_Tp, _N>()) { return _mm_sqrt_pd(__x);
        } else if constexpr (__is_avx_ps   <_Tp, _N>()) { return _mm256_sqrt_ps(__x);
        } else if constexpr (__is_avx_pd   <_Tp, _N>()) { return _mm256_sqrt_pd(__x);
        } else if constexpr (__is_avx512_ps<_Tp, _N>()) { return _mm512_sqrt_ps(__x);
        } else if constexpr (__is_avx512_pd<_Tp, _N>()) { return _mm512_sqrt_pd(__x);
        } else { __assert_unreachable<_Tp>(); }
    }

    // __trunc {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N> __trunc(_SimdWrapper<_Tp, _N> __x)
    {
        if constexpr (__is_avx512_ps<_Tp, _N>()) {
            return _mm512_roundscale_ps(__x, 0x0b);
        } else if constexpr (__is_avx512_pd<_Tp, _N>()) {
            return _mm512_roundscale_pd(__x, 0x0b);
        } else if constexpr (__is_avx_ps<_Tp, _N>()) {
            return _mm256_round_ps(__x, 0x3);
        } else if constexpr (__is_avx_pd<_Tp, _N>()) {
            return _mm256_round_pd(__x, 0x3);
        } else if constexpr (__have_sse4_1 && __is_sse_ps<_Tp, _N>()) {
            return __auto_bitcast(_mm_round_ps(__to_intrin(__x), 0x3));
        } else if constexpr (__have_sse4_1 && __is_sse_pd<_Tp, _N>()) {
            return _mm_round_pd(__x, 0x3);
        } else if constexpr (__is_sse_ps<_Tp, _N>()) {
	    auto __truncated =
	      _mm_cvtepi32_ps(_mm_cvttps_epi32(__to_intrin(__x)));
	    const auto __no_fractional_values =
	      __to_intrin(__vector_bitcast<float>(
		__vector_bitcast<int>(__vector_bitcast<_UInt>(__x._M_data) &
				      0x7f800000u) <
		0x4b000000)); // the exponent is so large that no mantissa bits
			      // signify fractional values (0x3f8 + 23*8 =
			      // 0x4b0)
	    return __auto_bitcast(
	      __blend(__no_fractional_values, __to_intrin(__x), __truncated));
	} else {
            return _Base::__trunc(__x);
        }
    }

    // __round {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N>
      __round(_SimdWrapper<_Tp, _N> __x)
    {
      using _V = __vector_type_t<_Tp, _N>;
      _V __truncated;
      if constexpr (__is_avx512_ps<_Tp, _N>())
	__truncated = _mm512_roundscale_ps(__x._M_data, 0x0b);
      else if constexpr (__is_avx512_pd<_Tp, _N>())
	__truncated = _mm512_roundscale_pd(__x._M_data, 0x0b);
      else if constexpr (__is_avx_ps<_Tp, _N>())
	__truncated = _mm256_round_ps(__x._M_data,
			       _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
      else if constexpr (__is_avx_pd<_Tp, _N>())
	__truncated = _mm256_round_pd(__x._M_data,
			       _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
      else if constexpr (__have_sse4_1 && __is_sse_ps<_Tp, _N>())
	__truncated = __auto_bitcast(_mm_round_ps(
	  __to_intrin(__x), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC));
      else if constexpr (__have_sse4_1 && __is_sse_pd<_Tp, _N>())
	__truncated = _mm_round_pd(__x._M_data,
			    _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
      else if constexpr (__is_sse_ps<_Tp, _N>())
	__truncated =
	  __auto_bitcast(_mm_cvtepi32_ps(_mm_cvttps_epi32(__to_intrin(__x))));
      else
	return _Base::__round(__x);

      // x < 0 => truncated <= 0 && truncated >= x => x - truncated <= 0
      // x > 0 => truncated >= 0 && truncated <= x => x - truncated >= 0

      const _V __rounded =
	__truncated +
	(__and(_S_absmask<_V>, __x._M_data - __truncated) >= _Tp(.5)
	   ? __or(__and(_S_signmask<_V>, __x._M_data), _V() + 1)
	   : _V());
      if constexpr(__have_sse4_1)
	return __rounded;
      else
	return __and(_S_absmask<_V>, __x._M_data) < 0x1p23f ? __rounded : __x._M_data;
    }

    // __nearbyint {{{3
    template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
    _GLIBCXX_SIMD_INTRINSIC static _Tp __nearbyint(_Tp __x) noexcept
    {
      if constexpr (_TVT::template __is<float, 16>)
	return _mm512_roundscale_ps(__x, 0x0c);
      else if constexpr (_TVT::template __is<double, 8>)
	return _mm512_roundscale_pd(__x, 0x0c);
      else if constexpr (_TVT::template __is<float, 8>)
	return _mm256_round_ps(__x,
			       _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
      else if constexpr (_TVT::template __is<double, 4>)
	return _mm256_round_pd(__x,
			       _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
      else if constexpr (__have_sse4_1 && _TVT::template __is<float, 4>)
	return _mm_round_ps(__x, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
      else if constexpr (__have_sse4_1 && _TVT::template __is<double, 2>)
	return _mm_round_pd(__x, _MM_FROUND_CUR_DIRECTION | _MM_FROUND_NO_EXC);
      else
	return _Base::__nearbyint(__x);
    }

    // __rint {{{3
    template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
    _GLIBCXX_SIMD_INTRINSIC static _Tp __rint(_Tp __x) noexcept
    {
      if constexpr (_TVT::template __is<float, 16>)
	return _mm512_roundscale_ps(__x, 0x04);
      else if constexpr (_TVT::template __is<double, 8>)
	return _mm512_roundscale_pd(__x, 0x04);
      else if constexpr (_TVT::template __is<float, 8>)
	return _mm256_round_ps(__x, _MM_FROUND_CUR_DIRECTION);
      else if constexpr (_TVT::template __is<double, 4>)
	return _mm256_round_pd(__x, _MM_FROUND_CUR_DIRECTION);
      else if constexpr (__have_sse4_1 && _TVT::template __is<float, 4>)
	return _mm_round_ps(__x, _MM_FROUND_CUR_DIRECTION);
      else if constexpr (__have_sse4_1 && _TVT::template __is<double, 2>)
	return _mm_round_pd(__x, _MM_FROUND_CUR_DIRECTION);
      else
	return _Base::__rint(__x);
    }

    // __floor {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N> __floor(_SimdWrapper<_Tp, _N> __x)
    {
        if constexpr (__is_avx512_ps<_Tp, _N>()) {
            return _mm512_roundscale_ps(__x, 0x09);
        } else if constexpr (__is_avx512_pd<_Tp, _N>()) {
            return _mm512_roundscale_pd(__x, 0x09);
        } else if constexpr (__is_avx_ps<_Tp, _N>()) {
            return _mm256_round_ps(__x, 0x1);
        } else if constexpr (__is_avx_pd<_Tp, _N>()) {
            return _mm256_round_pd(__x, 0x1);
        } else if constexpr (__have_sse4_1 && __is_sse_ps<_Tp, _N>()) {
	    return __auto_bitcast(_mm_floor_ps(__to_intrin(__x)));
	} else if constexpr (__have_sse4_1 && __is_sse_pd<_Tp, _N>()) {
            return _mm_floor_pd(__x);
        } else {
	  return _Base::__floor(__x);
        }
    }

    // __ceil {{{3
    template <class _Tp, size_t _N> _GLIBCXX_SIMD_INTRINSIC static _SimdWrapper<_Tp, _N> __ceil(_SimdWrapper<_Tp, _N> __x)
    {
        if constexpr (__is_avx512_ps<_Tp, _N>()) {
            return _mm512_roundscale_ps(__x, 0x0a);
        } else if constexpr (__is_avx512_pd<_Tp, _N>()) {
            return _mm512_roundscale_pd(__x, 0x0a);
        } else if constexpr (__is_avx_ps<_Tp, _N>()) {
            return _mm256_round_ps(__x, 0x2);
        } else if constexpr (__is_avx_pd<_Tp, _N>()) {
            return _mm256_round_pd(__x, 0x2);
        } else if constexpr (__have_sse4_1 && __is_sse_ps<_Tp, _N>()) {
            return __auto_bitcast(_mm_ceil_ps(__to_intrin(__x)));
        } else if constexpr (__have_sse4_1 && __is_sse_pd<_Tp, _N>()) {
            return _mm_ceil_pd(__x);
        } else {
	  return _Base::__ceil(__x);
        }
    }

    // __signbit {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp> __signbit(_SimdWrapper<_Tp, _N> __x)
    {
        using _I = __int_for_sizeof_t<_Tp>;
        if constexpr (__have_avx512dq && __is_avx512_ps<_Tp, _N>()) {
            return _mm512_movepi32_mask(__vector_bitcast<_LLong>(__x));
        } else if constexpr (__have_avx512dq && __is_avx512_pd<_Tp, _N>()) {
            return _mm512_movepi64_mask(__vector_bitcast<_LLong>(__x));
        } else if constexpr (sizeof(__x) == 64) {
            const auto __signmask = __vector_broadcast<_N>(std::numeric_limits<_I>::min());
            return __equal_to(_SimdWrapper<_I, _N>(__vector_bitcast<_I>(__x._M_data) & __signmask),
                            _SimdWrapper<_I, _N>(__signmask));
        } else {
            const auto __xx = __vector_bitcast<_I>(__x._M_data);
            [[maybe_unused]] constexpr _I __signmask = std::numeric_limits<_I>::min();
            if constexpr ((sizeof(_Tp) == 4 && (__have_avx2 || sizeof(__x) == 16)) ||
                          __have_avx512vl) {
                return __vector_bitcast<_Tp>(__xx >> std::numeric_limits<_I>::digits);
            } else if constexpr ((__have_avx2 || (__have_ssse3 && sizeof(__x) == 16))) {
                return __vector_bitcast<_Tp>((__xx & __signmask) == __signmask);
            } else {  // SSE2/3 or AVX (w/o AVX2)
                constexpr auto __one = __vector_broadcast<_N, _Tp>(1);
                return __vector_bitcast<_Tp>(
                    __vector_bitcast<_Tp>((__xx & __signmask) | __vector_bitcast<_I>(__one))  // -1 or 1
                    != __one);
            }
        }
    }

    // __isnonzerovalue_mask (isnormal | is subnormal == !isinf & !isnan & !is zero) {{{3
    template <class _Tp>
    _GLIBCXX_SIMD_INTRINSIC static auto __isnonzerovalue_mask(_Tp __x)
    {
      using _Traits = _VectorTraits<_Tp>;
      if constexpr (__have_avx512dq_vl)
	{
	  if constexpr (_Traits::template __is<float, 2> ||
			_Traits::template __is<float, 4>)
	    return _knot_mask8(_mm_fpclass_ps_mask(__to_intrin(__x), 0x9f));
	  else if constexpr (_Traits::template __is<float, 8>)
	    return _knot_mask8(_mm256_fpclass_ps_mask(__x, 0x9f));
	  else if constexpr (_Traits::template __is<float, 16>)
	    return _knot_mask16(_mm512_fpclass_ps_mask(__x, 0x9f));
	  else if constexpr (_Traits::template __is<double, 2>)
	    return _knot_mask8(_mm_fpclass_pd_mask(__x, 0x9f));
	  else if constexpr (_Traits::template __is<double, 4>)
	    return _knot_mask8(_mm256_fpclass_pd_mask(__x, 0x9f));
	  else if constexpr (_Traits::template __is<double, 8>)
	    return _knot_mask8(_mm512_fpclass_pd_mask(__x, 0x9f));
	  else
	    __assert_unreachable<_Tp>();
	}
      else
	{
	  using _U            = typename _Traits::value_type;
	  constexpr size_t _N = _Traits::_S_width;
	  const auto       __a =
	    __x * std::numeric_limits<_U>::infinity(); // NaN if __x == 0
	  const auto __b = __x * _U();                 // NaN if __x == inf
	  if constexpr (__have_avx512vl && __is_sse_ps<_U, _N>())
	    {
	      return _mm_cmp_ps_mask(__to_intrin(__a), __to_intrin(__b),
				     _CMP_ORD_Q);
	    }
	  else if constexpr (__have_avx512f && __is_sse_ps<_U, _N>())
	    {
	      return __mmask8(0xf & _mm512_cmp_ps_mask(__auto_bitcast(__a),
						       __auto_bitcast(__b),
						       _CMP_ORD_Q));
	    }
	  else if constexpr (__have_avx512vl && __is_sse_pd<_U, _N>())
	    {
	      return _mm_cmp_pd_mask(__a, __b, _CMP_ORD_Q);
	    }
	  else if constexpr (__have_avx512f && __is_sse_pd<_U, _N>())
	    {
	      return __mmask8(0x3 & _mm512_cmp_pd_mask(__auto_bitcast(__a),
						       __auto_bitcast(__b),
						       _CMP_ORD_Q));
	    }
	  else if constexpr (__have_avx512vl && __is_avx_ps<_U, _N>())
	    {
	      return _mm256_cmp_ps_mask(__a, __b, _CMP_ORD_Q);
	    }
	  else if constexpr (__have_avx512f && __is_avx_ps<_U, _N>())
	    {
	      return __mmask8(_mm512_cmp_ps_mask(
		__auto_bitcast(__a), __auto_bitcast(__b), _CMP_ORD_Q));
	    }
	  else if constexpr (__have_avx512vl && __is_avx_pd<_U, _N>())
	    {
	      return _mm256_cmp_pd_mask(__a, __b, _CMP_ORD_Q);
	    }
	  else if constexpr (__have_avx512f && __is_avx_pd<_U, _N>())
	    {
	      return __mmask8(0xf & _mm512_cmp_pd_mask(__auto_bitcast(__a),
						       __auto_bitcast(__b),
						       _CMP_ORD_Q));
	    }
	  else if constexpr (__is_avx512_ps<_U, _N>())
	    {
	      return _mm512_cmp_ps_mask(__a, __b, _CMP_ORD_Q);
	    }
	  else if constexpr (__is_avx512_pd<_U, _N>())
	    {
	      return _mm512_cmp_pd_mask(__a, __b, _CMP_ORD_Q);
	    }
	  else
	    {
	      __assert_unreachable<_Tp>();
	    }
	}
    }

    // __isfinite {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp>
      __isfinite(_SimdWrapper<_Tp, _N> __x)
    {
#if __FINITE_MATH_ONLY__
      [](auto&&){}(__x);
      return __equal_to(_SimdWrapper<_Tp, _N>(), _SimdWrapper<_Tp, _N>());
#else
      return __cmpord(__x._M_data, __x._M_data * _Tp());
#endif
    }

    // __isinf {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp> __isinf(_SimdWrapper<_Tp, _N> __x)
    {
#if __FINITE_MATH_ONLY__
      [](auto&&){}(__x);
      return {}; // false
#else
      if constexpr (__is_avx512_pd<_Tp, _N>() && __have_avx512dq)
	return _mm512_fpclass_pd_mask(__x, 0x18);
      else if constexpr (__is_avx512_ps<_Tp, _N>() && __have_avx512dq)
	return _mm512_fpclass_ps_mask(__x, 0x18);
      else if constexpr (__have_avx512dq_vl)
	{
	  if constexpr (__is_sse_pd<_Tp, _N>())
	    return __vector_bitcast<double>(
	      _mm_movm_epi64(_mm_fpclass_pd_mask(__x, 0x18)));
	  else if constexpr (__is_avx_pd<_Tp, _N>())
	    return __vector_bitcast<double>(
	      _mm256_movm_epi64(_mm256_fpclass_pd_mask(__x, 0x18)));
	  else if constexpr (__is_sse_ps<_Tp, _N>())
	    return __auto_bitcast(
	      _mm_movm_epi32(_mm_fpclass_ps_mask(__to_intrin(__x), 0x18)));
	  else if constexpr (__is_avx_ps<_Tp, _N>())
	    return __vector_bitcast<float>(
	      _mm256_movm_epi32(_mm256_fpclass_ps_mask(__x, 0x18)));
	  else
	    __assert_unreachable<_Tp>();
	}
      else
	return _Base::__isinf(__x);
#endif
    }

    // __isnormal {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember<_Tp>
      __isnormal(_SimdWrapper<_Tp, _N> __x)
    {
      if constexpr (__have_avx512dq)
	{
	  if constexpr (__have_avx512vl && __is_sse_ps<_Tp, _N>())
	    return __auto_bitcast(_mm_movm_epi32(
	      _knot_mask8(_mm_fpclass_ps_mask(__to_intrin(__x), 0xbf))));
	  else if constexpr (__have_avx512vl && __is_avx_ps<_Tp, _N>())
	    return __vector_bitcast<float>(_mm256_movm_epi32(
	      _knot_mask8(_mm256_fpclass_ps_mask(__x, 0xbf))));
	  else if constexpr (__is_avx512_ps<_Tp, _N>())
	    return _knot_mask16(_mm512_fpclass_ps_mask(__x, 0xbf));
	  else if constexpr (__have_avx512vl && __is_sse_pd<_Tp, _N>())
	    return __vector_bitcast<double>(
	      _mm_movm_epi64(_knot_mask8(_mm_fpclass_pd_mask(__x, 0xbf))));
	  else if constexpr (__have_avx512vl && __is_avx_pd<_Tp, _N>())
	    return __vector_bitcast<double>(_mm256_movm_epi64(
	      _knot_mask8(_mm256_fpclass_pd_mask(__x, 0xbf))));
	  else if constexpr (__is_avx512_pd<_Tp, _N>())
	    return _knot_mask8(_mm512_fpclass_pd_mask(__x, 0xbf));
	  else
	    __assert_unreachable<_Tp>();
	}
      else
	return _Base::__isnormal(__x);
    }

    // __isnan {{{3
    using _Base::__isnan;

    // __fpclassify {{{3
    template <class _Tp, size_t _N>
    _GLIBCXX_SIMD_INTRINSIC static __fixed_size_storage_t<int, _N> __fpclassify(_SimdWrapper<_Tp, _N> __x)
    {
        if constexpr (__is_avx512_pd<_Tp, _N>()) {
            // AVX512 is special because we want to use an __mmask to blend int vectors
            // (coming from double vectors). GCC doesn't allow this combination on the
            // ternary operator. Thus, resort to intrinsics:
            if constexpr (__have_avx512vl) {
                auto &&__b = [](int __y) { return __to_intrin(__vector_broadcast<_N>(__y)); };
                return {_mm256_mask_mov_epi32(
                    _mm256_mask_mov_epi32(
                        _mm256_mask_mov_epi32(__b(FP_NORMAL), __isnan(__x), __b(FP_NAN)),
                        __isinf(__x), __b(FP_INFINITE)),
                    _mm512_cmp_pd_mask(
                        __abs(__x),
                        __vector_broadcast<_N>(std::numeric_limits<double>::min()),
                        _CMP_LT_OS),
                    _mm256_mask_mov_epi32(
                        __b(FP_SUBNORMAL),
                        _mm512_cmp_pd_mask(__x, _mm512_setzero_pd(), _CMP_EQ_OQ),
                        __b(FP_ZERO)))};
            } else {
                auto &&__b = [](int __y) {
                    return _mm512_castsi256_si512(__to_intrin(__vector_broadcast<_N>(__y)));
                };
                return {__lo256(_mm512_mask_mov_epi32(
                    _mm512_mask_mov_epi32(
                        _mm512_mask_mov_epi32(__b(FP_NORMAL), __isnan(__x), __b(FP_NAN)),
                        __isinf(__x), __b(FP_INFINITE)),
                    _mm512_cmp_pd_mask(
                        __abs(__x),
                        __vector_broadcast<_N>(std::numeric_limits<double>::min()),
                        _CMP_LT_OS),
                    _mm512_mask_mov_epi32(
                        __b(FP_SUBNORMAL),
                        _mm512_cmp_pd_mask(__x, _mm512_setzero_pd(), _CMP_EQ_OQ),
                        __b(FP_ZERO))))};
            }
        } else {
	  return _Base::__fpclassify(__x);
        }
    }

    //}}}2
};

// __x86_mask_impl {{{1
template <class _Abi>
struct __x86_mask_impl : _MaskImplBuiltin<_Abi>
{
  using _Base = _MaskImplBuiltin<_Abi>;

  // __masked_load {{{2
  template <class _Tp, size_t _N, class _F>
  static inline _SimdWrapper<_Tp, _N> __masked_load(_SimdWrapper<_Tp, _N> __merge,
						  _SimdWrapper<_Tp, _N> __mask,
						  const bool*           __mem,
						  _F) noexcept
  {
    if constexpr (__is_abi<_Abi, simd_abi::_Avx512Abi>())
      {
	if constexpr (__have_avx512bw_vl)
	  {
	    if constexpr (_N == 8)
	      {
		const auto __a = _mm_mask_loadu_epi8(__m128i(), __mask, __mem);
		return (__merge & ~__mask) | _mm_test_epi8_mask(__a, __a);
	      }
	    else if constexpr (_N == 16)
	      {
		const auto __a = _mm_mask_loadu_epi8(__m128i(), __mask, __mem);
		return (__merge & ~__mask) | _mm_test_epi8_mask(__a, __a);
	      }
	    else if constexpr (_N == 32)
	      {
		const auto __a = _mm256_mask_loadu_epi8(__m256i(), __mask, __mem);
		return (__merge & ~__mask) | _mm256_test_epi8_mask(__a, __a);
	      }
	    else if constexpr (_N == 64)
	      {
		const auto __a = _mm512_mask_loadu_epi8(__m512i(), __mask, __mem);
		return (__merge & ~__mask) | _mm512_test_epi8_mask(__a, __a);
	      }
	    else
	      {
		__assert_unreachable<_Tp>();
	      }
	  }
	else
	  {
	    __bit_iteration(__mask, [&](auto __i) { __merge.__set(__i, __mem[__i]); });
	    return __merge;
	  }
      }
    else if constexpr (__have_avx512bw_vl && _N == 32 && sizeof(_Tp) == 1)
      {
	const auto __k = __convert_mask<_SimdWrapper<bool, _N>>(__mask);
	__merge          = _ToWrapper(
          _mm256_mask_sub_epi8(__vector_bitcast<_LLong>(__merge), __k, __m256i(),
                               _mm256_mask_loadu_epi8(__m256i(), __k, __mem)));
      }
    else if constexpr (__have_avx512bw_vl && _N == 16 && sizeof(_Tp) == 1)
      {
	const auto __k = __convert_mask<_SimdWrapper<bool, _N>>(__mask);
	__merge          = _ToWrapper(
          _mm_mask_sub_epi8(__vector_bitcast<_LLong>(__merge), __k, __m128i(),
                            _mm_mask_loadu_epi8(__m128i(), __k, __mem)));
      }
    else if constexpr (__have_avx512bw_vl && _N == 16 && sizeof(_Tp) == 2)
      {
	const auto __k = __convert_mask<_SimdWrapper<bool, _N>>(__mask);
	__merge          = _ToWrapper(_mm256_mask_sub_epi16(
          __vector_bitcast<_LLong>(__merge), __k, __m256i(),
          _mm256_cvtepi8_epi16(_mm_mask_loadu_epi8(__m128i(), __k, __mem))));
      }
    else if constexpr (__have_avx512bw_vl && _N == 8 && sizeof(_Tp) == 2)
      {
	const auto __k = __convert_mask<_SimdWrapper<bool, _N>>(__mask);
	__merge          = _ToWrapper(_mm_mask_sub_epi16(
          __vector_bitcast<_LLong>(__merge), __k, __m128i(),
          _mm_cvtepi8_epi16(_mm_mask_loadu_epi8(__m128i(), __k, __mem))));
      }
    else if constexpr (__have_avx512bw_vl && _N == 8 && sizeof(_Tp) == 4)
      {
	const auto __k = __convert_mask<_SimdWrapper<bool, _N>>(__mask);
	__merge          = _ToWrapper(_mm256_mask_sub_epi32(
          __vector_bitcast<_LLong>(__merge), __k, __m256i(),
          _mm256_cvtepi8_epi32(_mm_mask_loadu_epi8(__m128i(), __k, __mem))));
      }
    else if constexpr (__have_avx512bw_vl && _N == 4 && sizeof(_Tp) == 4)
      {
	const auto __k = __convert_mask<_SimdWrapper<bool, _N>>(__mask);
	__merge          = _ToWrapper(_mm_mask_sub_epi32(
          __vector_bitcast<_LLong>(__merge), __k, __m128i(),
          _mm_cvtepi8_epi32(_mm_mask_loadu_epi8(__m128i(), __k, __mem))));
      }
    else if constexpr (__have_avx512bw_vl && _N == 4 && sizeof(_Tp) == 8)
      {
	const auto __k = __convert_mask<_SimdWrapper<bool, _N>>(__mask);
	__merge          = _ToWrapper(_mm256_mask_sub_epi64(
          __vector_bitcast<_LLong>(__merge), __k, __m256i(),
          _mm256_cvtepi8_epi64(_mm_mask_loadu_epi8(__m128i(), __k, __mem))));
      }
    else if constexpr (__have_avx512bw_vl && _N == 2 && sizeof(_Tp) == 8)
      {
	const auto __k = __convert_mask<_SimdWrapper<bool, _N>>(__mask);
	__merge          = _ToWrapper(_mm_mask_sub_epi64(
          __vector_bitcast<_LLong>(__merge), __k, __m128i(),
          _mm_cvtepi8_epi64(_mm_mask_loadu_epi8(__m128i(), __k, __mem))));
      }
    else
      {
	return _Base::__masked_load(__merge, __mask, __mem, _F{});
      }
    return __merge;
  }

  // __store {{{2
  template <class _Tp, size_t _N, class _F>
  _GLIBCXX_SIMD_INTRINSIC static void
    __store(_SimdWrapper<_Tp, _N> __v, bool* __mem, _F) noexcept
  {
    if constexpr (__is_abi<_Abi, simd_abi::_SseAbi>())
      {
	if constexpr (_N == 2 && __have_sse2)
	  {
	    const auto __k = __vector_bitcast<int>(__v);
	    __mem[0]       = -__k[1];
	    __mem[1]       = -__k[3];
	  }
	else if constexpr (_N == 4 && __have_sse2)
	  {
	    const unsigned __bool4 =
	      __vector_bitcast<_UInt>(_mm_packs_epi16(
		_mm_packs_epi32(__vector_bitcast<_LLong>(__v), __m128i()),
		__m128i()))[0] &
	      0x01010101u;
	    std::memcpy(__mem, &__bool4, 4);
	  }
	else if constexpr (std::is_same_v<_Tp, float> && __have_mmx)
	  {
	    const __m128 __k = __to_intrin(__v);
	    const __m64  __kk  = _mm_cvtps_pi8(__and(__k, _mm_set1_ps(1.f)));
	    __vector_store<4>(__kk, __mem, _F());
	    _mm_empty();
	  }
	else if constexpr (_N == 8 && __have_sse2)
	  {
	    __vector_store<8>(
	      _mm_packs_epi16(__to_intrin(__vector_bitcast<_UShort>(__v) >> 15),
			      __m128i()),
	      __mem, _F());
	  }
	else if constexpr (_N == 16 && __have_sse2)
	  {
	    __vector_store(__v._M_data & 1, __mem, _F());
	  }
	else
	  {
	    __assert_unreachable<_Tp>();
	  }
      }
    else if constexpr (__is_abi<_Abi, simd_abi::_AvxAbi>())
      {
	if constexpr (_N == 4 && __have_avx)
	  {
	    auto __k = __vector_bitcast<_LLong>(__v);
	    int  __bool4;
	    if constexpr (__have_avx2)
	      {
		__bool4 = _mm256_movemask_epi8(__k);
	      }
	    else
	      {
		__bool4 = (_mm_movemask_epi8(__lo128(__k)) |
			 (_mm_movemask_epi8(__hi128(__k)) << 16));
	      }
	    __bool4 &= 0x01010101;
	    std::memcpy(__mem, &__bool4, 4);
	  }
	else if constexpr (_N == 8 && __have_avx)
	  {
	    const auto __k = __vector_bitcast<_LLong>(__v);
	    const auto __k2 =
	      _mm_srli_epi16(_mm_packs_epi16(__lo128(__k), __hi128(__k)), 15);
	    const auto __k3 = _mm_packs_epi16(__k2, __m128i());
	    __vector_store<8>(__k3, __mem, _F());
	  }
	else if constexpr (_N == 16 && __have_avx2)
	  {
	    const auto __x   = _mm256_srli_epi16(__to_intrin(__v), 15);
	    const auto __bools = _mm_packs_epi16(__lo128(__x), __hi128(__x));
	    __vector_store<16>(__bools, __mem, _F());
	  }
	else if constexpr (_N == 16 && __have_avx)
	  {
	    const auto __bools =
	      1 & __vector_bitcast<_UChar>(_mm_packs_epi16(
		    __lo128(__to_intrin(__v)), __hi128(__to_intrin(__v))));
	    __vector_store<16>(__bools, __mem, _F());
	  }
	else if constexpr (_N == 32 && __have_avx)
	  {
	    __vector_store<32>(1 & __v._M_data, __mem, _F());
	  }
	else
	  {
	    __assert_unreachable<_Tp>();
	  }
      }
    else if constexpr (__is_abi<_Abi, simd_abi::_Avx512Abi>())
      {
	if constexpr (_N == 8)
	  {
	    __vector_store<8>(
#if _GLIBCXX_SIMD_HAVE_AVX512VL && _GLIBCXX_SIMD_HAVE_AVX512BW
	      _mm_maskz_set1_epi8(__v._M_data, 1),
#elif defined __x86_64__
	      __make_wrapper<_ULLong>(
		_pdep_u64(__v._M_data, 0x0101010101010101ULL), 0ull),
#else
	      __make_wrapper<_UInt>(_pdep_u32(__v._M_data, 0x01010101U),
				    _pdep_u32(__v._M_data >> 4, 0x01010101U)),
#endif
	      __mem, _F());
	  }
	else if constexpr (_N == 16 && __have_avx512bw_vl)
	  {
	    __vector_store(_mm_maskz_set1_epi8(__v._M_data, 1), __mem, _F());
	  }
	else if constexpr (_N == 16 && __have_avx512f)
	  {
	    _mm512_mask_cvtepi32_storeu_epi8(
	      __mem, ~__mmask16(), _mm512_maskz_set1_epi32(__v._M_data, 1));
	  }
	else if constexpr (_N == 32 && __have_avx512bw_vl)
	  {
	    __vector_store(_mm256_maskz_set1_epi8(__v._M_data, 1), __mem, _F());
	  }
	else if constexpr (_N == 32 && __have_avx512bw)
	  {
	    __vector_store(__lo256(_mm512_maskz_set1_epi8(__v._M_data, 1)),
			   __mem, _F());
	  }
	else if constexpr (_N == 64 && __have_avx512bw)
	  {
	    __vector_store(_mm512_maskz_set1_epi8(__v._M_data, 1), __mem, _F());
	  }
	else
	  {
	    __assert_unreachable<_Tp>();
	  }
      }
    else
      {
	__assert_unreachable<_Tp>();
      }
  }

  // __masked_store {{{2
  template <class _Tp, size_t _N, class _F>
  static inline void __masked_store(const _SimdWrapper<_Tp, _N> __v,
				  bool*                       __mem,
				  _F,
				  const _SimdWrapper<_Tp, _N> __k) noexcept
  {
    if constexpr (__is_abi<_Abi, simd_abi::_Avx512Abi>())
      {
	if constexpr (_N == 8 && __have_avx512bw_vl)
	  {
	    _mm_mask_cvtepi16_storeu_epi8(__mem, __k,
					  _mm_maskz_set1_epi16(__v, 1));
	  }
	else if constexpr (_N == 8 && __have_avx512vl)
	  {
	    _mm256_mask_cvtepi32_storeu_epi8(__mem, __k,
					     _mm256_maskz_set1_epi32(__v, 1));
	  }
	else if constexpr (_N == 8)
	  {
	    // we rely on __k < 0x100:
	    _mm512_mask_cvtepi32_storeu_epi8(__mem, __k,
					     _mm512_maskz_set1_epi32(__v, 1));
	  }
	else if constexpr (_N == 16 && __have_avx512bw_vl)
	  {
	    _mm_mask_storeu_epi8(__mem, __k, _mm_maskz_set1_epi8(__v, 1));
	  }
	else if constexpr (_N == 16)
	  {
	    _mm512_mask_cvtepi32_storeu_epi8(__mem, __k,
					     _mm512_maskz_set1_epi32(__v, 1));
	  }
	else if constexpr (_N == 32 && __have_avx512bw_vl)
	  {
	    _mm256_mask_storeu_epi8(__mem, __k, _mm256_maskz_set1_epi8(__v, 1));
	  }
	else if constexpr (_N == 32 && __have_avx512bw)
	  {
	    _mm256_mask_storeu_epi8(__mem, __k,
				    __lo256(_mm512_maskz_set1_epi8(__v, 1)));
	  }
	else if constexpr (_N == 64 && __have_avx512bw)
	  {
	    _mm512_mask_storeu_epi8(__mem, __k, _mm512_maskz_set1_epi8(__v, 1));
	  }
	else
	  {
	    __assert_unreachable<_Tp>();
	  }
      }
    else
      {
	_Base::__masked_store(__v, __mem, _F(), __k);
      }
  }

  // logical and bitwise operators {{{2
  template <class _Tp, size_t _N>
  _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
    __logical_and(const _SimdWrapper<_Tp, _N>& __x,
		const _SimdWrapper<_Tp, _N>& __y)
  {
    if constexpr (std::is_same_v<_Tp, bool>)
      {
	if constexpr (__have_avx512dq && _N <= 8)
	  return _kand_mask8(__x._M_data, __y._M_data);
	else if constexpr (_N <= 16)
	  return _kand_mask16(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 32)
	  return _kand_mask32(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 64)
	  return _kand_mask64(__x._M_data, __y._M_data);
	else
	  __assert_unreachable<_Tp>();
      }
    else
      return _Base::__logical_and(__x, __y);
  }

  template <class _Tp, size_t _N>
  _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
    __logical_or(const _SimdWrapper<_Tp, _N>& __x,
	       const _SimdWrapper<_Tp, _N>& __y)
  {
    if constexpr (std::is_same_v<_Tp, bool>)
      {
	if constexpr (__have_avx512dq && _N <= 8)
	  return _kor_mask8(__x._M_data, __y._M_data);
	else if constexpr (_N <= 16)
	  return _kor_mask16(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 32)
	  return _kor_mask32(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 64)
	  return _kor_mask64(__x._M_data, __y._M_data);
	else
	  __assert_unreachable<_Tp>();
      }
    else
      return _Base::__logical_or(__x, __y);
  }

  template <class _Tp, size_t _N>
  _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
    __bit_and(const _SimdWrapper<_Tp, _N>& __x, const _SimdWrapper<_Tp, _N>& __y)
  {
    if constexpr (std::is_same_v<_Tp, bool>)
      {
	if constexpr (__have_avx512dq && _N <= 8)
	  return _kand_mask8(__x._M_data, __y._M_data);
	else if constexpr (_N <= 16)
	  return _kand_mask16(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 32)
	  return _kand_mask32(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 64)
	  return _kand_mask64(__x._M_data, __y._M_data);
	else
	  __assert_unreachable<_Tp>();
      }
    else
      return _Base::__bit_and(__x, __y);
  }

  template <class _Tp, size_t _N>
  _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
    __bit_or(const _SimdWrapper<_Tp, _N>& __x, const _SimdWrapper<_Tp, _N>& __y)
  {
    if constexpr (std::is_same_v<_Tp, bool>)
      {
	if constexpr (__have_avx512dq && _N <= 8)
	  return _kor_mask8(__x._M_data, __y._M_data);
	else if constexpr (_N <= 16)
	  return _kor_mask16(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 32)
	  return _kor_mask32(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 64)
	  return _kor_mask64(__x._M_data, __y._M_data);
	else
	  __assert_unreachable<_Tp>();
      }
    else
      return _Base::__bit_or(__x, __y);
  }

  template <class _Tp, size_t _N>
  _GLIBCXX_SIMD_INTRINSIC static constexpr _SimdWrapper<_Tp, _N>
    __bit_xor(const _SimdWrapper<_Tp, _N>& __x, const _SimdWrapper<_Tp, _N>& __y)
  {
    if constexpr (std::is_same_v<_Tp, bool>)
      {
	if constexpr (__have_avx512dq && _N <= 8)
	  return _kxor_mask8(__x._M_data, __y._M_data);
	else if constexpr (_N <= 16)
	  return _kxor_mask16(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 32)
	  return _kxor_mask32(__x._M_data, __y._M_data);
	else if constexpr (__have_avx512bw && _N <= 64)
	  return _kxor_mask64(__x._M_data, __y._M_data);
	else
	  __assert_unreachable<_Tp>();
      }
    else
      return _Base::__bit_xor(__x, __y);
  }

  //}}}2
};

//}}}1

template <int _Bytes>
struct _MaskImplSse : __x86_mask_impl<simd_abi::_SseAbi<_Bytes>> {};
template <int _Bytes>
struct _SimdImplSse : __x86_simd_impl<simd_abi::_SseAbi<_Bytes>> {};

template <int _Bytes>
struct _MaskImplAvx : __x86_mask_impl<simd_abi::_AvxAbi<_Bytes>> {};
template <int _Bytes>
struct _SimdImplAvx : __x86_simd_impl<simd_abi::_AvxAbi<_Bytes>> {};

template <int _Bytes>
struct _MaskImplAvx512 : __x86_mask_impl<simd_abi::_Avx512Abi<_Bytes>> {};
template <int _Bytes>
struct _SimdImplAvx512 : __x86_simd_impl<simd_abi::_Avx512Abi<_Bytes>> {};

#endif // _GLIBCXX_SIMD_X86INTRIN }}}

#if _GLIBCXX_SIMD_HAVE_NEON // {{{
// _SimdImplNeon {{{
template <int _Bytes>
struct _SimdImplNeon : _SimdImplBuiltin<simd_abi::_NeonAbi<_Bytes>>
{
  using _Base = _SimdImplBuiltin<simd_abi::_NeonAbi<_Bytes>>;
  // math {{{
  // __sqrt {{{
  template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
  _GLIBCXX_SIMD_INTRINSIC static _Tp __sqrt(_Tp __x)
  {
    const auto __intrin = __to_intrin(__x);
    if constexpr (_TVT::template __is<float, 2>)
      return vsqrt_f32(__intrin);
    else if constexpr (_TVT::template __is<float, 4>)
      return vsqrtq_f32(__intrin);
    else if constexpr (_TVT::template __is<double, 1>)
      return vsqrt_f64(__intrin);
    else if constexpr (_TVT::template __is<double, 2>)
      return vsqrtq_f64(__intrin);
    else
      return _Base::__sqrt(__x);
  } // }}}
  // __trunc {{{
  template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
  _GLIBCXX_SIMD_INTRINSIC static _Tp __trunc(_Tp __x)
  {
    const auto __intrin = __to_intrin(__x);
    if constexpr (_TVT::template __is<float, 2>)
      return vrnd_f32(__intrin);
    else if constexpr (_TVT::template __is<float, 4>)
      return vrndq_f32(__intrin);
    else if constexpr (_TVT::template __is<double, 1>)
      return vrnd_f64(__intrin);
    else if constexpr (_TVT::template __is<double, 2>)
      return vrndq_f64(__intrin);
    else
      return _Base::__trunc(__x);
  } // }}}
  // __floor {{{
  template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
  _GLIBCXX_SIMD_INTRINSIC static _Tp __floor(_Tp __x)
  {
    const auto __intrin = __to_intrin(__x);
    if constexpr (_TVT::template __is<float, 2>)
      return vrndm_f32(__intrin);
    else if constexpr (_TVT::template __is<float, 4>)
      return vrndmq_f32(__intrin);
    else if constexpr (_TVT::template __is<double, 1>)
      return vrndm_f64(__intrin);
    else if constexpr (_TVT::template __is<double, 2>)
      return vrndmq_f64(__intrin);
    else
      return _Base::__floor(__x);
  } // }}}
  // __ceil {{{
  template <typename _Tp, typename _TVT = _VectorTraits<_Tp>>
  _GLIBCXX_SIMD_INTRINSIC static _Tp __ceil(_Tp __x)
  {
    const auto __intrin = __to_intrin(__x);
    if constexpr (_TVT::template __is<float, 2>)
      return vrndp_f32(__intrin);
    else if constexpr (_TVT::template __is<float, 4>)
      return vrndpq_f32(__intrin);
    else if constexpr (_TVT::template __is<double, 1>)
      return vrndp_f64(__intrin);
    else if constexpr (_TVT::template __is<double, 2>)
      return vrndpq_f64(__intrin);
    else
      return _Base::__ceil(__x);
  } //}}}
  //}}}
}; // }}}
// _MaskImplNeon {{{
template <int _Bytes>
struct _MaskImplNeon : _MaskImplBuiltin<simd_abi::_NeonAbi<_Bytes>>
{
}; // }}}
#endif // _GLIBCXX_SIMD_HAVE_NEON }}}

/**
 * The fixed_size ABI gives the following guarantees:
 *  - simd objects are passed via the stack
 *  - memory layout of `simd<_Tp, _N>` is equivalent to `std::array<_Tp, _N>`
 *  - alignment of `simd<_Tp, _N>` is `_N * sizeof(_Tp)` if _N is __a power-of-2 value,
 *    otherwise `__next_power_of_2(_N * sizeof(_Tp))` (Note: if the alignment were to
 *    exceed the system/compiler maximum, it is bounded to that maximum)
 *  - simd_mask objects are passed like std::bitset<_N>
 *  - memory layout of `simd_mask<_Tp, _N>` is equivalent to `std::bitset<_N>`
 *  - alignment of `simd_mask<_Tp, _N>` is equal to the alignment of `std::bitset<_N>`
 */
// __autocvt_to_simd {{{
template <class _Tp, bool = std::is_arithmetic_v<__remove_cvref_t<_Tp>>>
struct __autocvt_to_simd {
    _Tp _M_data;
    using _TT = __remove_cvref_t<_Tp>;
    operator _TT() { return _M_data; }
    operator _TT &()
    {
        static_assert(std::is_lvalue_reference<_Tp>::value, "");
        static_assert(!std::is_const<_Tp>::value, "");
        return _M_data;
    }
    operator _TT *()
    {
        static_assert(std::is_lvalue_reference<_Tp>::value, "");
        static_assert(!std::is_const<_Tp>::value, "");
        return &_M_data;
    }

    constexpr inline __autocvt_to_simd(_Tp dd) : _M_data(dd) {}

    template <class _Abi> operator simd<typename _TT::value_type, _Abi>()
    {
        return {__private_init, _M_data};
    }

    template <class _Abi> operator simd<typename _TT::value_type, _Abi> &()
    {
        return *reinterpret_cast<simd<typename _TT::value_type, _Abi> *>(&_M_data);
    }

    template <class _Abi> operator simd<typename _TT::value_type, _Abi> *()
    {
        return reinterpret_cast<simd<typename _TT::value_type, _Abi> *>(&_M_data);
    }
};
template <class _Tp> __autocvt_to_simd(_Tp &&)->__autocvt_to_simd<_Tp>;

template <class _Tp> struct __autocvt_to_simd<_Tp, true> {
    using _TT = __remove_cvref_t<_Tp>;
    _Tp _M_data;
    fixed_size_simd<_TT, 1> _M_fd;

    constexpr inline __autocvt_to_simd(_Tp dd) : _M_data(dd), _M_fd(_M_data) {}
    ~__autocvt_to_simd()
    {
        _M_data = __data(_M_fd).first;
    }

    operator fixed_size_simd<_TT, 1>()
    {
        return _M_fd;
    }
    operator fixed_size_simd<_TT, 1> &()
    {
        static_assert(std::is_lvalue_reference<_Tp>::value, "");
        static_assert(!std::is_const<_Tp>::value, "");
        return _M_fd;
    }
    operator fixed_size_simd<_TT, 1> *()
    {
        static_assert(std::is_lvalue_reference<_Tp>::value, "");
        static_assert(!std::is_const<_Tp>::value, "");
        return &_M_fd;
    }
};

// }}}
// __fixed_size_storage_t<_Tp, _N>{{{1
template <class _Tp, int _N, class _Tuple,
          class _Next = simd<_Tp, _AllNativeAbis::_BestAbi<_Tp, _N>>,
          int _Remain = _N - int(_Next::size())>
struct __fixed_size_storage_builder;

template <class _Tp, int _N>
struct __fixed_size_storage
    : public __fixed_size_storage_builder<_Tp, _N, _SimdTuple<_Tp>> {
};

template <class _Tp, int _N, class... _As, class _Next>
struct __fixed_size_storage_builder<_Tp, _N, _SimdTuple<_Tp, _As...>, _Next, 0> {
    using type = _SimdTuple<_Tp, _As..., typename _Next::abi_type>;
};

template <class _Tp, int _N, class... _As, class _Next, int _Remain>
struct __fixed_size_storage_builder<_Tp, _N, _SimdTuple<_Tp, _As...>, _Next, _Remain> {
    using type = typename __fixed_size_storage_builder<
        _Tp, _Remain, _SimdTuple<_Tp, _As..., typename _Next::abi_type>>::type;
};

// _AbisInSimdTuple {{{1
template <class _Tp> struct _SeqOp;
template <size_t _I0, size_t... _Is> struct _SeqOp<std::index_sequence<_I0, _Is...>> {
    using _FirstPlusOne = std::index_sequence<_I0 + 1, _Is...>;
    using _NotFirstPlusOne = std::index_sequence<_I0, (_Is + 1)...>;
    template <size_t _First, size_t _Add>
    using _Prepend = std::index_sequence<_First, _I0 + _Add, (_Is + _Add)...>;
};

template <class _Tp> struct _AbisInSimdTuple;
template <class _Tp> struct _AbisInSimdTuple<_SimdTuple<_Tp>> {
    using _Counts = std::index_sequence<0>;
    using _Begins = std::index_sequence<0>;
};
template <class _Tp, class _A> struct _AbisInSimdTuple<_SimdTuple<_Tp, _A>> {
    using _Counts = std::index_sequence<1>;
    using _Begins = std::index_sequence<0>;
};
template <class _Tp, class _A0, class... _As>
struct _AbisInSimdTuple<_SimdTuple<_Tp, _A0, _A0, _As...>> {
    using _Counts = typename _SeqOp<typename _AbisInSimdTuple<
        _SimdTuple<_Tp, _A0, _As...>>::_Counts>::_FirstPlusOne;
    using _Begins = typename _SeqOp<typename _AbisInSimdTuple<
        _SimdTuple<_Tp, _A0, _As...>>::_Begins>::_NotFirstPlusOne;
};
template <class _Tp, class _A0, class _A1, class... _As>
struct _AbisInSimdTuple<_SimdTuple<_Tp, _A0, _A1, _As...>> {
    using _Counts = typename _SeqOp<typename _AbisInSimdTuple<
        _SimdTuple<_Tp, _A1, _As...>>::_Counts>::template _Prepend<1, 0>;
    using _Begins = typename _SeqOp<typename _AbisInSimdTuple<
        _SimdTuple<_Tp, _A1, _As...>>::_Begins>::template _Prepend<0, 1>;
};

// __binary_tree_reduce {{{1
template <size_t _Count,
	  size_t _Begin,
	  class _Tp,
	  class... _As,
	  class _BinaryOperation>
auto __binary_tree_reduce(const _SimdTuple<_Tp, _As...>& __tup,
			  const _BinaryOperation&        __binary_op) noexcept
{
  static_assert(_Count > 0);
  if constexpr (_Count == 1)
    return __get_simd_at<_Begin>(__tup);
  else if constexpr (_Count == 2)
    return __binary_op(__get_simd_at<_Begin>(__tup),
		       __get_simd_at<_Begin + 1>(__tup));
  else
    {
      constexpr size_t __left  = __next_power_of_2(_Count) / 2;
      constexpr size_t __right = _Count - __left;
      return __binary_op(
	__binary_tree_reduce<__left, _Begin>(__tup, __binary_op),
	__binary_tree_reduce<__right, _Begin + __left>(__tup, __binary_op));
    }
}

// __vec_to_scalar_reduction {{{1
// This helper function implements the second step in a generic fixed_size reduction.
// -  Input: a tuple of native simd (or scalar) objects of decreasing size.
// - Output: a scalar (the reduction).
// - Approach:
//   1. reduce the first two tuple elements
//      a) If the number of elements differs by a factor of 2, split the first object into
//         two objects of the second type and reduce all three to one object of second
//         type.
//      b) If the number of elements differs by a factor of 4, split the first object into
//         two equally sized objects, reduce, and split to two objects of the second type.
//         Finally, reduce all three remaining objects to one object of second type.
//      c) Otherwise use std::experimental::reduce to reduce both inputs to a scalar, and binary_op to
//         reduce to a single scalar.
//
//      (This optimizes all native cases on x86, e.g. <AVX512, SSE, Scalar>.)
//
//   2. Concate the result of (1) with the remaining tuple elements to recurse into
//      __vec_to_scalar_reduction.
//
//   3. If __vec_to_scalar_reduction is called with a one-element tuple, call std::experimental::reduce to
//      reduce to a scalar and return.
template <class _Tp, class _A0, class _BinaryOperation>
_GLIBCXX_SIMD_INTRINSIC _Tp __vec_to_scalar_reduction(
  const _SimdTuple<_Tp, _A0>& __tup, const _BinaryOperation& __binary_op)
{
    return std::experimental::reduce(simd<_Tp, _A0>(__private_init, __tup.first), __binary_op);
}

template <class _Tp, class _A0, class _A1, class... _As, class _BinaryOperation>
_GLIBCXX_SIMD_INTRINSIC _Tp
			__vec_to_scalar_reduction(const _SimdTuple<_Tp, _A0, _A1, _As...>& __tup,
						  const _BinaryOperation& __binary_op)
{
  using _Simd0 = simd<_Tp, _A0>;
  using _Simd1 = simd<_Tp, _A1>;
  static_assert(_Simd0::size() != _Simd1::size());

  const _Simd0 __left(__private_init, __tup.first);
  [[maybe_unused]] const _Simd1 __right(__private_init, __tup.second.first);
  if constexpr (sizeof(_Simd0) == sizeof(_Simd1) && _A1::_S_is_partial &&
		std::is_same_v<_BinaryOperation, std::plus<>>)
    return reduce(
      __binary_op(__left, _Simd0(__private_init,
				 __and(__tup.second.first._M_data,
				       _A1::template __implicit_mask<_Tp>()))),
      __binary_op);
  else if constexpr (sizeof(_Simd0) == sizeof(_Simd1) && _A1::_S_is_partial &&
		std::is_same_v<_BinaryOperation, std::multiplies<>>)
    return reduce(
      __binary_op(
	__left,
	_Simd0(
	  __private_init,
	  __blend(_A1::template __implicit_mask<_Tp>(),
		  __vector_broadcast<_A1::template _S_full_size<_Tp>>(_Tp(1)),
		  __tup.second.first._M_data))),
      __binary_op);
  else if constexpr (_Simd0::size() == 2 * _Simd1::size() && !_A1::_S_is_partial)
    {
      const auto [__l0, __l1] = split<_Simd1>(__left);
      const _Simd1 __reduced  = __binary_op(__binary_op(__l0, __right), __l1);
      return __vec_to_scalar_reduction(
	__simd_tuple_concat(__make_simd_tuple(__reduced), __tup.second.second),
	__binary_op);
    }
  else if constexpr (_Simd0::size() == 4 * _Simd1::size() && !_A1::_S_is_partial)
    {
      using _SimdIntermed = __deduced_simd<_Tp, _Simd0::size() / 2>;
      const auto [__l0, __l1] = split<_SimdIntermed>(__left);
      const auto [__m0, __m1] = split<_Simd1>(__binary_op(__l0, __l1));
      const _Simd1 __reduced  = __binary_op(__binary_op(__m0, __right), __m1);
      return __vec_to_scalar_reduction(
	__simd_tuple_concat(__make_simd_tuple(__reduced), __tup.second.second),
	__binary_op);
    }
  else // use reduction via scalar
    return __binary_op(
      simd<_Tp, simd_abi::scalar>(reduce(__left, __binary_op)),
      simd<_Tp, simd_abi::scalar>(reduce(__right, __binary_op)))[0];
}

// _SimdImplFixedSize {{{1
// fixed_size should not inherit from _SimdMathFallback in order for
// specializations in the used _SimdTuple Abis to get used
template <int _N> struct _SimdImplFixedSize {
    // member types {{{2
    using _MaskMember = std::bitset<_N>;
    template <class _Tp> using _SimdMember = __fixed_size_storage_t<_Tp, _N>;
    template <class _Tp>
    static constexpr std::size_t _S_tuple_size = _SimdMember<_Tp>::_S_tuple_size;
    template <class _Tp> using _Simd = std::experimental::simd<_Tp, simd_abi::fixed_size<_N>>;
    template <class _Tp> using _TypeTag = _Tp *;

    // broadcast {{{2
    template <class _Tp> static constexpr inline _SimdMember<_Tp> __broadcast(_Tp __x) noexcept
    {
        return _SimdMember<_Tp>::__generate(
            [&](auto __meta) constexpr { return __meta.__broadcast(__x); });
    }

    // __generator {{{2
    template <class _F, class _Tp>
    static constexpr inline _SimdMember<_Tp>
      __generator(_F&& __gen, _TypeTag<_Tp>)
    {
      return _SimdMember<_Tp>::__generate([&__gen](auto __meta) constexpr {
	return __meta.__generator(
	  [&](auto __i) constexpr {
	    return __i < _N ? __gen(_SizeConstant<__meta._S_offset + __i>()) : 0;
	  },
	  _TypeTag<_Tp>());
      });
    }

    // __load {{{2
    template <class _Tp, class _U, class _F>
    static inline _SimdMember<_Tp> __load(const _U *__mem, _F __f,
                                              _TypeTag<_Tp>) _GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
    {
        return _SimdMember<_Tp>::__generate(
            [&](auto __meta) { return __meta.__load(&__mem[__meta._S_offset], __f, _TypeTag<_Tp>()); });
    }

    // __masked_load {{{2
    template <class _Tp, class... _As, class _U, class _F>
    static inline _SimdTuple<_Tp, _As...>
      __masked_load(const _SimdTuple<_Tp, _As...>& __old,
		  const _MaskMember          __bits,
		  const _U*                        __mem,
		  _F __f) _GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
    {
      auto __merge = __old;
      __for_each(__merge, [&](auto __meta, auto& __native) {
	__native = __meta.__masked_load(__native, __meta.__make_mask(__bits),
				  &__mem[__meta._S_offset], __f);
      });
      return __merge;
    }

    // __store {{{2
    template <class _Tp, class _U, class _F>
    static inline void __store(const _SimdMember<_Tp>& __v,
			     _U*                           __mem,
			     _F                            __f,
			     _TypeTag<_Tp>) _GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
    {
      __for_each(__v, [&](auto __meta, auto __native) {
	__meta.__store(__native, &__mem[__meta._S_offset], __f, _TypeTag<_Tp>());
      });
    }

    // __masked_store {{{2
    template <class _Tp, class... _As, class _U, class _F>
    static inline void __masked_store(const _SimdTuple<_Tp, _As...>& __v,
				    _U*                              __mem,
				    _F                               __f,
				    const _MaskMember          __bits)
      _GLIBCXX_SIMD_NOEXCEPT_OR_IN_TEST
    {
      __for_each(__v, [&](auto __meta, auto __native) {
	__meta.__masked_store(__native, &__mem[__meta._S_offset], __f,
			  __meta.__make_mask(__bits));
      });
    }

    // negation {{{2
    template <class _Tp, class... _As>
    static inline _MaskMember
      __negate(const _SimdTuple<_Tp, _As...>& __x) noexcept
    {
        _MaskMember __bits = 0;
        __for_each(__x, [&__bits](auto __meta, auto __native) constexpr {
            __bits |= __meta.__mask_to_shifted_ullong(__meta.__negate(__native));
        });
        return __bits;
    }

    // reductions {{{2
private:
    template <class _Tp, class... _As, class _BinaryOperation, size_t... _Counts,
              size_t... _Begins>
    static inline _Tp __reduce(const _SimdTuple<_Tp, _As...> &__tup,
                           const _BinaryOperation &__binary_op,
                           std::index_sequence<_Counts...>, std::index_sequence<_Begins...>)
    {
      // 1. reduce all tuple elements with equal ABI to a single element in the
      // output tuple
      const auto __reduced_vec = __make_simd_tuple(
	__binary_tree_reduce<_Counts, _Begins>(__tup, __binary_op)...);
      // 2. split and reduce until a scalar results
      return __vec_to_scalar_reduction(__reduced_vec, __binary_op);
    }

public:
    template <class _Tp, class _BinaryOperation>
    static inline _Tp __reduce(const _Simd<_Tp> &__x, const _BinaryOperation &__binary_op)
    {
        using _Ranges = _AbisInSimdTuple<_SimdMember<_Tp>>;
        return _SimdImplFixedSize::__reduce(__x._M_data, __binary_op,
                                              typename _Ranges::_Counts(),
                                              typename _Ranges::_Begins());
    }

    // __min, __max {{{2
    template <typename _Tp, typename... _As>
    static inline constexpr _SimdTuple<_Tp, _As...>
      __min(const _SimdTuple<_Tp, _As...>& __a,
	  const _SimdTuple<_Tp, _As...>& __b)
    {
      return __a.__apply_per_chunk(
	[](auto __impl, auto __aa, auto __bb) constexpr {
	  return __impl.__min(__aa, __bb);
	},
	__b);
    }

    template <typename _Tp, typename... _As>
    static inline constexpr _SimdTuple<_Tp, _As...>
      __max(const _SimdTuple<_Tp, _As...>& __a,
	  const _SimdTuple<_Tp, _As...>& __b)
    {
      return __a.__apply_per_chunk(
	[](auto __impl, auto __aa, auto __bb) constexpr {
	  return __impl.__max(__aa, __bb);
	},
	__b);
    }

    // __complement {{{2
    template <typename _Tp, typename... _As>
    static inline constexpr _SimdTuple<_Tp, _As...>
      __complement(const _SimdTuple<_Tp, _As...>& __x) noexcept
    {
      return __x.__apply_per_chunk([](auto __impl, auto __xx) constexpr {
	return __impl.__complement(__xx);
      });
    }

    // __unary_minus {{{2
    template <typename _Tp, typename... _As>
    static inline constexpr _SimdTuple<_Tp, _As...>
      __unary_minus(const _SimdTuple<_Tp, _As...>& __x) noexcept
    {
      return __x.__apply_per_chunk([](auto __impl, auto __xx) constexpr {
	return __impl.__unary_minus(__xx);
      });
    }

    // arithmetic operators {{{2

#define _GLIBCXX_SIMD_FIXED_OP(name_, op_)                                     \
  template <typename _Tp, typename... _As>                                     \
  static inline constexpr _SimdTuple<_Tp, _As...> name_(                       \
    const _SimdTuple<_Tp, _As...> __x, const _SimdTuple<_Tp, _As...> __y)      \
  {                                                                            \
    return __x.__apply_per_chunk(                                              \
      [](auto __impl, auto __xx, auto __yy) constexpr {                        \
	return __impl.name_(__xx, __yy);                                       \
      },                                                                       \
      __y);                                                                    \
  }

    _GLIBCXX_SIMD_FIXED_OP(__plus, +)
    _GLIBCXX_SIMD_FIXED_OP(__minus, -)
    _GLIBCXX_SIMD_FIXED_OP(__multiplies, *)
    _GLIBCXX_SIMD_FIXED_OP(__divides, /)
    _GLIBCXX_SIMD_FIXED_OP(__modulus, %)
    _GLIBCXX_SIMD_FIXED_OP(__bit_and, &)
    _GLIBCXX_SIMD_FIXED_OP(__bit_or, |)
    _GLIBCXX_SIMD_FIXED_OP(__bit_xor, ^)
    _GLIBCXX_SIMD_FIXED_OP(__bit_shift_left, <<)
    _GLIBCXX_SIMD_FIXED_OP(__bit_shift_right, >>)
#undef _GLIBCXX_SIMD_FIXED_OP

    template <typename _Tp, typename... _As>
    static inline constexpr _SimdTuple<_Tp, _As...>
      __bit_shift_left(const _SimdTuple<_Tp, _As...>& __x, int __y)
    {
      return __x.__apply_per_chunk([__y](auto __impl, auto __xx) constexpr {
	return __impl.__bit_shift_left(__xx, __y);
      });
    }

    template <typename _Tp, typename... _As>
    static inline constexpr _SimdTuple<_Tp, _As...>
      __bit_shift_right(const _SimdTuple<_Tp, _As...>& __x, int __y)
    {
      return __x.__apply_per_chunk([__y](auto __impl, auto __xx) constexpr {
	return __impl.__bit_shift_right(__xx, __y);
      });
    }

    // math {{{2
#define _GLIBCXX_SIMD_APPLY_ON_TUPLE(_RetTp, __name)                           \
  template <typename _Tp, typename... _As, typename... _More>                  \
  static inline __fixed_size_storage_t<_RetTp,                                 \
				       _SimdTuple<_Tp, _As...>::size()>        \
    __##__name(const _SimdTuple<_Tp, _As...>& __x, const _More&... __more)     \
  {                                                                            \
    if constexpr (sizeof...(_More) == 0)                                       \
      {                                                                        \
	if constexpr (is_same_v<_Tp, _RetTp>)                                  \
	  return __x.__apply_per_chunk([](auto __impl, auto __xx) constexpr {  \
	    using _V = typename decltype(__impl)::simd_type;                   \
	    return __data(__name(_V(__private_init, __xx)));                   \
	  });                                                                  \
	else                                                                   \
	  return __optimize_simd_tuple(__x.template __apply_r<_RetTp>(         \
	    [](auto __impl, auto __xx) { return __impl.__##__name(__xx); }));  \
      }                                                                        \
    else if constexpr (is_same_v<_Tp, _RetTp> &&                               \
		       (... &&                                                 \
			std::is_same_v<_SimdTuple<_Tp, _As...>, _More>))       \
      return __x.__apply_per_chunk(                                            \
	[](auto __impl, auto __xx, auto... __pack) constexpr {                 \
	  using _V = typename decltype(__impl)::simd_type;                     \
	  return __data(                                                       \
	    __name(_V(__private_init, __xx), _V(__private_init, __pack)...));  \
	},                                                                     \
	__more...);                                                            \
    else if constexpr (is_same_v<_Tp, _RetTp>)                                 \
      return __x.__apply_per_chunk(                                            \
	[](auto __impl, auto __xx, auto... __pack) constexpr {                 \
	  using _V = typename decltype(__impl)::simd_type;                     \
	  return __data(                                                       \
	    __name(_V(__private_init, __xx), __autocvt_to_simd(__pack)...));   \
	},                                                                     \
	__more...);                                                            \
    else                                                                       \
      __assert_unreachable<_Tp>();                                             \
  }
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, acos)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, asin)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, atan)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, atan2)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, cos)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, sin)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, tan)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, acosh)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, asinh)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, atanh)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, cosh)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, sinh)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, tanh)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, exp)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, exp2)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, expm1)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(int, ilogb)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, log)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, log10)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, log1p)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, log2)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, logb)
    //modf implemented in simd_math.h
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, scalbn) //double scalbn(double x, int exp);
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, scalbln)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, cbrt)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, abs)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fabs)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, pow)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, sqrt)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, erf)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, erfc)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, lgamma)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, tgamma)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, trunc)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, ceil)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, floor)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, nearbyint)

    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, rint)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(long, lrint)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(long long, llrint)

    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, round)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(long, lround)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(long long, llround)

    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, ldexp)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fmod)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, remainder)
    // copysign in simd_math.h
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, nextafter)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fdim)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fmax)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fmin)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(_Tp, fma)
    _GLIBCXX_SIMD_APPLY_ON_TUPLE(int, fpclassify)
#undef _GLIBCXX_SIMD_APPLY_ON_TUPLE

    template <typename _Tp, typename... _Abis>
    static _SimdTuple<_Tp, _Abis...> __remquo(
      const _SimdTuple<_Tp, _Abis...>&                                __x,
      const _SimdTuple<_Tp, _Abis...>&                                __y,
      __fixed_size_storage_t<int, _SimdTuple<_Tp, _Abis...>::size()>* __z)
    {
      return __x.__apply_per_chunk(
	[](auto __impl, const auto __xx, const auto __yy, auto& __zz) {
	  return __impl.__remquo(__xx, __yy, &__zz);
	},
	__y, *__z);
    }

    template <typename _Tp, typename... _As>
    static inline _SimdTuple<_Tp, _As...>
      __frexp(const _SimdTuple<_Tp, _As...>&   __x,
	      __fixed_size_storage_t<int, _N>& __exp) noexcept
    {
      return __x.__apply_per_chunk(
	[](auto __impl, const auto& __a, auto& __b) {
	  return __data(
	    frexp(typename decltype(__impl)::simd_type(__private_init, __a),
		  __autocvt_to_simd(__b)));
	},
	__exp);
    }

    template <typename _Tp, typename... _As>
    static inline __fixed_size_storage_t<int, _N>
      __fpclassify(const _SimdTuple<_Tp, _As...>& __x) noexcept
    {
      return __optimize_simd_tuple(__x.template __apply_r<int>(
	[](auto __impl, auto __xx) { return __impl.__fpclassify(__xx); }));
    }

#define _GLIBCXX_SIMD_TEST_ON_TUPLE_(name_)                                    \
  template <typename _Tp, typename... _As>                                     \
  static inline _MaskMember __##name_(                                   \
    const _SimdTuple<_Tp, _As...>& __x) noexcept                             \
  {                                                                            \
    return __test([](auto __impl, auto __xx) { return __impl.__##name_(__xx); }, \
		__x);                                                          \
  }
    _GLIBCXX_SIMD_TEST_ON_TUPLE_(isinf)
    _GLIBCXX_SIMD_TEST_ON_TUPLE_(isfinite)
    _GLIBCXX_SIMD_TEST_ON_TUPLE_(isnan)
    _GLIBCXX_SIMD_TEST_ON_TUPLE_(isnormal)
    _GLIBCXX_SIMD_TEST_ON_TUPLE_(signbit)
#undef _GLIBCXX_SIMD_TEST_ON_TUPLE_

    // __increment & __decrement{{{2
    template <typename... _Ts>
    _GLIBCXX_SIMD_INTRINSIC static constexpr void
      __increment(_SimdTuple<_Ts...>& __x)
    {
      __for_each(
	__x, [](auto __meta, auto& native) constexpr {
	  __meta.__increment(native);
	});
    }

    template <typename... _Ts>
    _GLIBCXX_SIMD_INTRINSIC static constexpr void
      __decrement(_SimdTuple<_Ts...>& __x)
    {
      __for_each(
	__x, [](auto __meta, auto& native) constexpr {
	  __meta.__decrement(native);
	});
    }

    // compares {{{2
#define _GLIBCXX_SIMD_CMP_OPERATIONS(__cmp)                                    \
  template <typename _Tp, typename... _As>                                     \
  _GLIBCXX_SIMD_INTRINSIC static _MaskMember __cmp(                            \
    const _SimdTuple<_Tp, _As...>& __x, const _SimdTuple<_Tp, _As...>& __y)    \
  {                                                                            \
    return __test([](auto __impl, auto __xx,                                   \
		     auto __yy) { return __impl.__cmp(__xx, __yy); },          \
		  __x, __y);                                                   \
  }
    _GLIBCXX_SIMD_CMP_OPERATIONS(__equal_to)
    _GLIBCXX_SIMD_CMP_OPERATIONS(__not_equal_to)
    _GLIBCXX_SIMD_CMP_OPERATIONS(__less)
    _GLIBCXX_SIMD_CMP_OPERATIONS(__less_equal)
    _GLIBCXX_SIMD_CMP_OPERATIONS(__isless)
    _GLIBCXX_SIMD_CMP_OPERATIONS(__islessequal)
    _GLIBCXX_SIMD_CMP_OPERATIONS(__isgreater)
    _GLIBCXX_SIMD_CMP_OPERATIONS(__isgreaterequal)
    _GLIBCXX_SIMD_CMP_OPERATIONS(__islessgreater)
    _GLIBCXX_SIMD_CMP_OPERATIONS(__isunordered)
#undef _GLIBCXX_SIMD_CMP_OPERATIONS

    // smart_reference access {{{2
    template <typename _Tp, typename... _As, typename _U>
    _GLIBCXX_SIMD_INTRINSIC static void __set(_SimdTuple<_Tp, _As...> &__v, int __i, _U &&__x) noexcept
    {
        __v.__set(__i, std::forward<_U>(__x));
    }

    // __masked_assign {{{2
    template <typename _Tp, typename... _As>
    _GLIBCXX_SIMD_INTRINSIC static void
      __masked_assign(const _MaskMember                __bits,
		    _SimdTuple<_Tp, _As...>&             __lhs,
		    const __id<_SimdTuple<_Tp, _As...>>& __rhs)
    {
      __for_each(__lhs, __rhs,
		 [&](auto __meta, auto& __native_lhs, auto __native_rhs) constexpr {
		   __meta.__masked_assign(__meta.__make_mask(__bits), __native_lhs,
					__native_rhs);
		 });
    }

    // Optimization for the case where the RHS is a scalar. No need to broadcast the
    // scalar to a simd first.
    template <typename _Tp, typename... _As>
    _GLIBCXX_SIMD_INTRINSIC static void
      __masked_assign(const _MaskMember    __bits,
		    _SimdTuple<_Tp, _As...>& __lhs,
		    const __id<_Tp>            __rhs)
    {
      __for_each(__lhs, [&](auto __meta, auto& __native_lhs) constexpr {
	__meta.__masked_assign(__meta.__make_mask(__bits), __native_lhs, __rhs);
      });
    }

    // __masked_cassign {{{2
    template <typename _Op, typename _Tp, typename... _As>
    static inline void __masked_cassign(const _MaskMember              __bits,
					_SimdTuple<_Tp, _As...>&       __lhs,
					const _SimdTuple<_Tp, _As...>& __rhs,
					_Op                            __op)
    {
      __for_each(
	__lhs, __rhs,
	[&](auto __meta, auto& __native_lhs, auto __native_rhs) constexpr {
	  __meta.template __masked_cassign(__meta.__make_mask(__bits),
					   __native_lhs, __native_rhs, __op);
	});
    }

    // Optimization for the case where the RHS is a scalar. No need to broadcast
    // the scalar to a simd first.
    template <typename _Op, typename _Tp, typename... _As>
    static inline void __masked_cassign(const _MaskMember        __bits,
					_SimdTuple<_Tp, _As...>& __lhs,
					const _Tp&               __rhs,
					_Op                      __op)
    {
      __for_each(
	__lhs, [&](auto __meta, auto& __native_lhs) constexpr {
	  __meta.template __masked_cassign(__meta.__make_mask(__bits),
					   __native_lhs, __rhs, __op);
	});
    }

    // __masked_unary {{{2
    template <template <typename> class _Op, typename _Tp, typename... _As>
    static inline _SimdTuple<_Tp, _As...>
      __masked_unary(const _MaskMember         __bits,
		   const _SimdTuple<_Tp, _As...> __v) // TODO: const-ref __v?
    {
      return __v.__apply_wrapped([&__bits](auto __meta, auto __native) constexpr {
	return __meta.template __masked_unary<_Op>(__meta.__make_mask(__bits),
						 __native);
      });
    }

    // }}}2
};

// _MaskImplFixedSize {{{1
template <int _N> struct _MaskImplFixedSize {
    static_assert(sizeof(_ULLong) * CHAR_BIT >= _N,
                  "The fixed_size implementation relies on one "
                  "_ULLong being able to store all boolean "
                  "elements.");  // required in load & store

    // member types {{{2
    using _MaskMember = std::bitset<_N>;
    template <typename _Tp> using _TypeTag = _Tp *;

    // __from_bitset {{{2
    template <typename _Tp>
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember __from_bitset(const _MaskMember &__bs,
                                                     _TypeTag<_Tp>) noexcept
    {
        return __bs;
    }

    // __load {{{2
    template <typename _F> static inline _MaskMember __load(const bool *__mem, _F __f) noexcept
    {
        // TODO: _UChar is not necessarily the best type to use here. For smaller _N _UShort,
        // _UInt, _ULLong, float, and double can be more efficient.
        _ULLong __r = 0;
        using _Vs = __fixed_size_storage_t<_UChar, _N>;
        __for_each(_Vs{}, [&](auto __meta, auto) {
            __r |= __meta.__mask_to_shifted_ullong(
                __meta._S_mask_impl.__load(&__mem[__meta._S_offset], __f, _SizeConstant<__meta.size()>()));
        });
        return __r;
    }

    // __masked_load {{{2
    template <typename _F>
    static inline _MaskMember __masked_load(_MaskMember __merge,
                                               _MaskMember __mask, const bool *__mem,
                                               _F) noexcept
    {
        __bit_iteration(__mask.to_ullong(), [&](auto __i) { __merge[__i] = __mem[__i]; });
        return __merge;
    }

    // __store {{{2
    template <typename _F>
    static inline void __store(_MaskMember __bs, bool *__mem, _F) noexcept
    {
#if _GLIBCXX_SIMD_HAVE_AVX512BW
        const __m512i bool64 = _mm512_movm_epi8(__bs.to_ullong()) & 0x0101010101010101ULL;
        __vector_store<_N>(bool64, __mem, _F());
#elif _GLIBCXX_SIMD_HAVE_BMI2
#ifdef __x86_64__
        __execute_n_times<_N / 8>([&](auto __i) {
            constexpr size_t __offset = __i * 8;
            const _ULLong bool8 =
                _pdep_u64(__bs.to_ullong() >> __offset, 0x0101010101010101ULL);
            std::memcpy(&__mem[__offset], &bool8, 8);
        });
        if (_N % 8 > 0) {
            constexpr size_t __offset = (_N / 8) * 8;
            const _ULLong bool8 =
                _pdep_u64(__bs.to_ullong() >> __offset, 0x0101010101010101ULL);
            std::memcpy(&__mem[__offset], &bool8, _N % 8);
        }
#else   // __x86_64__
        __execute_n_times<_N / 4>([&](auto __i) {
            constexpr size_t __offset = __i * 4;
            const _ULLong __bool4 =
                _pdep_u32(__bs.to_ullong() >> __offset, 0x01010101U);
            std::memcpy(&__mem[__offset], &__bool4, 4);
        });
        if (_N % 4 > 0) {
            constexpr size_t __offset = (_N / 4) * 4;
            const _ULLong __bool4 =
                _pdep_u32(__bs.to_ullong() >> __offset, 0x01010101U);
            std::memcpy(&__mem[__offset], &__bool4, _N % 4);
        }
#endif  // __x86_64__
#elif  _GLIBCXX_SIMD_HAVE_SSE2   // !AVX512BW && !BMI2
        using _V = simd<_UChar, simd_abi::__sse>;
        _ULLong __bits = __bs.to_ullong();
        __execute_n_times<(_N + 15) / 16>([&](auto __i) {
            constexpr size_t __offset = __i * 16;
            constexpr size_t __remaining = _N - __offset;
            if constexpr (__remaining == 1) {
                __mem[__offset] = static_cast<bool>(__bits >> __offset);
            } else if constexpr (__remaining <= 4) {
                const _UInt __bool4 = ((__bits >> __offset) * 0x00204081U) & 0x01010101U;
                std::memcpy(&__mem[__offset], &__bool4, __remaining);
            } else if constexpr (__remaining <= 7) {
                const _ULLong bool8 =
                    ((__bits >> __offset) * 0x40810204081ULL) & 0x0101010101010101ULL;
                std::memcpy(&__mem[__offset], &bool8, __remaining);
            } else if constexpr (__have_sse2) {
                auto __tmp = _mm_cvtsi32_si128(__bits >> __offset);
                __tmp = _mm_unpacklo_epi8(__tmp, __tmp);
                __tmp = _mm_unpacklo_epi16(__tmp, __tmp);
                __tmp = _mm_unpacklo_epi32(__tmp, __tmp);
                _V __tmp2(__tmp);
                __tmp2 &= _V([](auto __j) {
                    return static_cast<_UChar>(1 << (__j % CHAR_BIT));
                });  // mask bit index
                const __m128i __bool16 = __intrin_bitcast<__m128i>(
                    __vector_bitcast<_UChar>(__data(__tmp2 == 0)) +
                    1);  // 0xff -> 0x00 | 0x00 -> 0x01
                if constexpr (__remaining >= 16) {
                    __vector_store<16>(__bool16, &__mem[__offset], _F());
                } else if constexpr (__remaining & 3) {
                    constexpr int to_shift = 16 - int(__remaining);
                    _mm_maskmoveu_si128(__bool16,
                                        _mm_srli_si128(~__m128i(), to_shift),
                                        reinterpret_cast<char *>(&__mem[__offset]));
                } else  // at this point: 8 < __remaining < 16
                    if constexpr (__remaining >= 8) {
                    __vector_store<8>(__bool16, &__mem[__offset], _F());
                    if constexpr (__remaining == 12) {
                        __vector_store<4>(_mm_unpackhi_epi64(__bool16, __bool16),
                                         &__mem[__offset + 8], _F());
                    }
                }
            } else {
                __assert_unreachable<_F>();
            }
        });
#else
        // TODO: _UChar is not necessarily the best type to use here. For smaller _N _UShort,
        // _UInt, _ULLong, float, and double can be more efficient.
        using _Vs = __fixed_size_storage_t<_UChar, _N>;
        __for_each(_Vs{}, [&](auto __meta, auto) {
            __meta._S_mask_impl.__store(__meta.__make_mask(__bs), &__mem[__meta._S_offset], _F());
        });
//#else
        //__execute_n_times<_N>([&](auto __i) { __mem[__i] = __bs[__i]; });
#endif  // _GLIBCXX_SIMD_HAVE_BMI2
    }

    // __masked_store {{{2
    template <typename _F>
    static inline void __masked_store(const _MaskMember __v, bool *__mem, _F,
                                    const _MaskMember __k) noexcept
    {
        __bit_iteration(__k, [&](auto __i) { __mem[__i] = __v[__i]; });
    }

    // logical and bitwise operators {{{2
    _GLIBCXX_SIMD_INTRINSIC static _MaskMember __logical_and(const _MaskMember &__x,
                                                     const _MaskMember &__y) noexcept
    {
        return __x & __y;
    }

    _GLIBCXX_SIMD_INTRINSIC static _MaskMember __logical_or(const _MaskMember &__x,
                                                    const _MaskMember &__y) noexcept
    {
        return __x | __y;
    }

    _GLIBCXX_SIMD_INTRINSIC static _MaskMember __bit_and(const _MaskMember &__x,
                                                 const _MaskMember &__y) noexcept
    {
        return __x & __y;
    }

    _GLIBCXX_SIMD_INTRINSIC static _MaskMember __bit_or(const _MaskMember &__x,
                                                const _MaskMember &__y) noexcept
    {
        return __x | __y;
    }

    _GLIBCXX_SIMD_INTRINSIC static _MaskMember __bit_xor(const _MaskMember &__x,
                                                 const _MaskMember &__y) noexcept
    {
        return __x ^ __y;
    }

    // smart_reference access {{{2
    _GLIBCXX_SIMD_INTRINSIC static void __set(_MaskMember &__k, int __i, bool __x) noexcept
    {
        __k.set(__i, __x);
    }

    // __masked_assign {{{2
    _GLIBCXX_SIMD_INTRINSIC static void __masked_assign(const _MaskMember __k,
                                           _MaskMember &__lhs,
                                           const _MaskMember __rhs)
    {
        __lhs = (__lhs & ~__k) | (__rhs & __k);
    }

    // Optimization for the case where the RHS is a scalar.
    _GLIBCXX_SIMD_INTRINSIC static void __masked_assign(const _MaskMember __k,
                                           _MaskMember &__lhs, const bool __rhs)
    {
        if (__rhs) {
            __lhs |= __k;
        } else {
            __lhs &= ~__k;
        }
    }

    // }}}2
};
// }}}1

// _SimdConverter scalar -> scalar {{{
template <typename _From, typename _To>
struct _SimdConverter<_From,
		      simd_abi::scalar,
		      _To,
		      simd_abi::scalar,
		      std::enable_if_t<!std::is_same_v<_From, _To>>>
{
  _GLIBCXX_SIMD_INTRINSIC _To operator()(_From __a)
  {
    return static_cast<_To>(__a);
  }
};

// }}}
// _SimdConverter "native" -> scalar {{{
template <typename _From, typename _To, typename _Abi>
struct _SimdConverter<_From,
		      _Abi,
		      _To,
		      simd_abi::scalar,
		      std::enable_if_t<!std::is_same_v<_Abi, simd_abi::scalar>>>
{
  using _Arg = typename _Abi::template __traits<_From>::_SimdMember;
  static constexpr size_t _S_n = _Arg::_S_width;

  _GLIBCXX_SIMD_INTRINSIC std::array<_To, _S_n> __all(_Arg __a)
  {
    return __call_with_subscripts(
      __a, make_index_sequence<_S_n>(),
      [&](auto... __values) constexpr -> std::array<_To, _S_n> {
	return {static_cast<_To>(__values)...};
      });
  }
};

// }}}
// _SimdConverter scalar -> "native" {{{
template <typename _From, typename _To, typename _Abi>
struct _SimdConverter<_From,
		      simd_abi::scalar,
		      _To,
		      _Abi,
		      std::enable_if_t<!std::is_same_v<_Abi, simd_abi::scalar>>>
{
  using _Ret = typename _Abi::template __traits<_To>::_SimdMember;

  template <typename... _More>
  _GLIBCXX_SIMD_INTRINSIC constexpr _Ret operator()(_From __a, _More... __more)
  {
    static_assert(sizeof...(_More) + 1 == _Abi::template size<_To>);
    static_assert(std::conjunction_v<std::is_same<_From, _More>...>);
    return __make_vector<_To>(__a, __more...);
  }
};

// }}}
// _SimdConverter "native 1" -> "native 2" {{{
template <typename _From, typename _To, typename _AFrom, typename _ATo>
struct _SimdConverter<
  _From,
  _AFrom,
  _To,
  _ATo,
  std::enable_if_t<!std::disjunction_v<__is_fixed_size_abi<_AFrom>,
				       __is_fixed_size_abi<_ATo>,
				       std::is_same<_AFrom, simd_abi::scalar>,
				       std::is_same<_ATo, simd_abi::scalar>,
				       std::is_same<_From, _To>>>>
{
  using _Arg = typename _AFrom::template __traits<_From>::_SimdMember;
  using _Ret = typename _ATo::template __traits<_To>::_SimdMember;
  using _V   = __vector_type_t<_To, simd_size_v<_To, _ATo>>;

  _GLIBCXX_SIMD_INTRINSIC auto __all(_Arg __a)
  {
    return __convert_all<_V>(__a);
  }

  template <typename... _More>
  _GLIBCXX_SIMD_INTRINSIC _Ret operator()(_Arg __a, _More... __more)
  {
    return __convert<_V>(__a, __more...);
  }
};

// }}}
// _SimdConverter scalar -> fixed_size<1> {{{1
template <typename _From, typename _To>
struct _SimdConverter<_From,
		      simd_abi::scalar,
		      _To,
		      simd_abi::fixed_size<1>,
		      void>
{
  _SimdTuple<_To, simd_abi::scalar> operator()(_From __x)
  {
    return {static_cast<_To>(__x)};
  }
};

// _SimdConverter fixed_size<1> -> scalar {{{1
template <typename _From, typename _To>
struct _SimdConverter<_From,
		      simd_abi::fixed_size<1>,
		      _To,
		      simd_abi::scalar,
		      void>
{
  _GLIBCXX_SIMD_INTRINSIC _To
			  operator()(_SimdTuple<_From, simd_abi::scalar> __x)
  {
    return {static_cast<_To>(__x.first)};
  }
};

// _SimdConverter fixed_size<_N> -> fixed_size<_N> {{{1
template <typename _From, typename _To, int _N>
struct _SimdConverter<_From,
		      simd_abi::fixed_size<_N>,
		      _To,
		      simd_abi::fixed_size<_N>,
		      std::enable_if_t<!std::is_same_v<_From, _To>>>
{
  using _Ret = __fixed_size_storage_t<_To, _N>;
  using _Arg = __fixed_size_storage_t<_From, _N>;

  _GLIBCXX_SIMD_INTRINSIC _Ret operator()(const _Arg& __x)
  {
    if constexpr (std::is_same_v<_From, _To>)
      return __x;

    // special case (optimize) int signedness casts
    else if constexpr (sizeof(_From) == sizeof(_To) &&
		       std::is_integral_v<_From> && std::is_integral_v<_To>)
      return __bit_cast<_Ret>(__x);

    // special case of all ABI tags in _Ret are scalar
    else if constexpr (__is_abi<typename _Ret::_FirstAbi, simd_abi::scalar>())
      {
	return __call_with_subscripts(
	  __x, make_index_sequence<_N>(), [](auto... __values) constexpr->_Ret {
	    return __make_simd_tuple<
	      _To, std::conditional_t<sizeof(__values) == 0, simd_abi::scalar,
				      simd_abi::scalar>...>(
	      static_cast<_To>(__values)...);
	  });
      }

    // from one vector to one vector
    else if constexpr (_Arg::_S_first_size == _Ret::_S_first_size)
      {
	_SimdConverter<_From, typename _Arg::_FirstAbi, _To,
		       typename _Ret::_FirstAbi>
	  __native_cvt;
	if constexpr (_Arg::_S_tuple_size == 1)
	  return {__native_cvt(__x.first)};
	else
	  {
	    constexpr size_t _NRemain = _N - _Arg::_S_first_size;
	    _SimdConverter<_From, simd_abi::fixed_size<_NRemain>, _To,
			   simd_abi::fixed_size<_NRemain>>
	      __remainder_cvt;
	    return {__native_cvt(__x.first), __remainder_cvt(__x.second)};
	  }
      }

    // from one vector to multiple vectors
    else if constexpr (_Arg::_S_first_size > _Ret::_S_first_size)
      {
	const auto __multiple_return_chunks =
	  __convert_all<__vector_type_t<_To, _Ret::_S_first_size>>(__x.first);
	constexpr auto __converted = __multiple_return_chunks.size() *
				     _Ret::_FirstAbi::template size<_To>;
	constexpr auto __remaining = _N - __converted;
	if constexpr (_Arg::_S_tuple_size == 1 && __remaining == 0)
	  return __to_simd_tuple<_To, _N>(__multiple_return_chunks);
	else if constexpr (_Arg::_S_tuple_size == 1)
	  { // e.g. <int, 3> -> <double, 2, 1> or <short, 7> -> <double, 4, 2,
	    // 1>
	    using _RetRem = __remove_cvref_t<decltype(
	      __simd_tuple_pop_front<__multiple_return_chunks.size()>(_Ret()))>;
	    const auto __return_chunks2 =
	      __convert_all<__vector_type_t<_To, _RetRem::_S_first_size>, 0,
			    __converted>(__x.first);
	    constexpr auto __converted2 =
	      __converted + __return_chunks2.size() * _RetRem::_S_first_size;
	    if constexpr (__converted2 == _N)
	      return __to_simd_tuple<_To, _N>(__multiple_return_chunks,
					      __return_chunks2);
	    else
	      {
		using _RetRem2 = __remove_cvref_t<decltype(
		  __simd_tuple_pop_front<__return_chunks2.size()>(_RetRem()))>;
		const auto __return_chunks3 =
		  __convert_all<__vector_type_t<_To, _RetRem2::_S_first_size>,
				0, __converted2>(__x.first);
		constexpr auto __converted3 =
		  __converted2 +
		  __return_chunks3.size() * _RetRem2::_S_first_size;
		if constexpr (__converted3 == _N)
		  return __to_simd_tuple<_To, _N>(__multiple_return_chunks,
						  __return_chunks2, __return_chunks3);
		else
		  {
		    using _RetRem3              = __remove_cvref_t<decltype(
                      __simd_tuple_pop_front<__return_chunks3.size()>(
                        _RetRem2()))>;
		    const auto __return_chunks4 = __convert_all<
		      __vector_type_t<_To, _RetRem3::_S_first_size>, 0,
		      __converted3>(__x.first);
		    constexpr auto __converted4 =
		      __converted3 +
		      __return_chunks4.size() * _RetRem3::_S_first_size;
		    if constexpr (__converted4 == _N)
		      return __to_simd_tuple<_To, _N>(
			__multiple_return_chunks, __return_chunks2,
			__return_chunks3, __return_chunks4);
		    else
		      __assert_unreachable<_To>();
		  }
	      }
	  }
	else
	  {
	    constexpr size_t _NRemain = _N - _Arg::_S_first_size;
	    _SimdConverter<_From, simd_abi::fixed_size<_NRemain>, _To,
			   simd_abi::fixed_size<_NRemain>>
	      __remainder_cvt;
	    return __simd_tuple_concat(
	      __to_simd_tuple<_To, _Arg::_S_first_size>(
		__multiple_return_chunks),
	      __remainder_cvt(__x.second));
	  }
      }

    // from multiple vectors to one vector
    // _Arg::_S_first_size < _Ret::_S_first_size
    // a) heterogeneous input at the end of the tuple (possible with partial
    //    native registers in _Ret)
    else if constexpr (_Ret::_S_tuple_size == 1 && _N % _Arg::_S_first_size != 0)
      {
	static_assert(_Ret::_FirstAbi::_S_is_partial);
	return _Ret{__generate_from_n_evaluations<
	  _N, typename _VectorTraits<typename _Ret::_FirstType>::type>(
	  [&](auto __i) { return static_cast<_To>(__x[__i]); })};
      }
    else
      {
	static_assert(_Arg::_S_tuple_size > 1);
	constexpr auto __n =
	  __div_roundup(_Ret::_S_first_size, _Arg::_S_first_size);
	return __call_with_n_evaluations<__n>(
	  [&__x](auto... __uncvted) {
	    // assuming _Arg Abi tags for all __i are _Arg::_FirstAbi
	    _SimdConverter<_From, typename _Arg::_FirstAbi, _To,
			   typename _Ret::_FirstAbi>
	      __native_cvt;
	    if constexpr (_Ret::_S_tuple_size == 1)
	      return _Ret{__native_cvt(__uncvted...)};
	    else
	      return _Ret{
		__native_cvt(__uncvted...),
		_SimdConverter<
		  _From, simd_abi::fixed_size<_N - _Ret::_S_first_size>, _To,
		  simd_abi::fixed_size<_N - _Ret::_S_first_size>>()(
		  __simd_tuple_pop_front<sizeof...(__uncvted)>(__x))};
	  },
	  [&__x](auto __i) { return __get_tuple_at<__i>(__x); });
      }
  }
};

// _SimdConverter "native" -> fixed_size<_N> {{{1
// i.e. 1 register to ? registers
template <typename _From, typename _A, typename _To, int _N>
struct _SimdConverter<_From,
		      _A,
		      _To,
		      simd_abi::fixed_size<_N>,
		      std::enable_if_t<!__is_fixed_size_abi_v<_A>>>
{
  static_assert(
    _N == simd_size_v<_From, _A>,
    "_SimdConverter to fixed_size only works for equal element counts");

  _GLIBCXX_SIMD_INTRINSIC __fixed_size_storage_t<_To, _N>
			  operator()(typename _SimdTraits<_From, _A>::_SimdMember __x)
  {
    _SimdConverter<_From, simd_abi::fixed_size<_N>, _To,
		   simd_abi::fixed_size<_N>>
      __fixed_cvt;
    return __fixed_cvt(_SimdTuple<_From, _A>{__x});
  }
};

// _SimdConverter fixed_size<_N> -> "native" {{{1
// i.e. ? register to 1 registers
template <typename _From, int _N, typename _To, typename _A>
struct _SimdConverter<_From,
		      simd_abi::fixed_size<_N>,
		      _To,
		      _A,
		      std::enable_if_t<!__is_fixed_size_abi_v<_A>>>
{
  static_assert(
    _N == simd_size_v<_To, _A>,
    "_SimdConverter to fixed_size only works for equal element counts");

  _GLIBCXX_SIMD_INTRINSIC typename _SimdTraits<_To, _A>::_SimdMember
    operator()(__fixed_size_storage_t<_From, _N> __x)
  {
    _SimdConverter<_From, simd_abi::fixed_size<_N>, _To,
		   simd_abi::fixed_size<_N>>
      __fixed_cvt;
    return __fixed_cvt(__x).first;
  }
};

// }}}1
_GLIBCXX_SIMD_END_NAMESPACE
#endif  // __cplusplus >= 201703L
#endif  // _GLIBCXX_EXPERIMENTAL_SIMD_ABIS_H_
// vim: foldmethod=marker sw=2 noet ts=8 sts=2 tw=80
