#ifndef PRELUDE_TRAITS_H
#define PRELUDE_TRAITS_H

#include "prelude/stdafx.h"

namespace prelude { namespace traits
{

    template <typename T>
    struct type_only
    {
        typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type type;
    };    

    //
    // argument forwarder

    template <typename T>
    struct argfwd
    {
        typedef const T& type;  // fallback case: T is by-value
    };

    template <typename T>
    struct argfwd<T&>
    {
        typedef T& type;
    };

    template <typename T>
    struct argfwd<const T&>
    {
        typedef const T& type;
    };

    template <typename T>
    struct argfwd<const T>
    {
        typedef const T& type;
    };

}}

#endif // PRELUDE_TRAITS_H

