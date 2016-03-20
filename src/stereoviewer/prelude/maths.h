#ifndef PRELUDE_MATHS_H
#define PRELUDE_MATHS_H

#include "prelude/stdafx.h"

namespace prelude
{
    typedef qreal real;

    //
    // constants

    namespace constants
    {
        static const double pi = 3.14159265358979323846;
    }

    //
    // misc utility functions

    template <typename T>
    inline bool is_within(const T& x, const T& a, const T& b)
    {
        return x >= a && x <= b;
    }

    template <typename T>
    inline bool is_about(const T& x, const T& p, const T& epsilon)
    {
        return is_within(x, p - epsilon, p + epsilon);
    }

    inline double round(double x)
    {
        return x - floor(x) > 0.5 ? ceil(x) : floor(x);
    }

    template <typename T>
    inline const T& crop(const T& x, const T& a, const T& b)
    {
        return x < a ? a : x > b ? b : x;
    }

    template <typename T>
    inline const T& upcrop(const T& x, const T& top)
    {
        return x < top ? x : top;
    }

    template <typename T>
    inline const T& downcrop(const T& x, const T& bottom)
    {
        return x > bottom ? x : bottom;
    }

    inline float degsin(float x)  { return sinf(fmodf(x, 360.) * constants::pi / 180.); }
    inline float degcos(float x)  { return cosf(fmodf(x, 360.) * constants::pi / 180.); }

    real degsinf(const real& a, const real& phi, const real& q, const real& x);
    real degcosf(const real& a, const real& phi, const real& q, const real& x);

    template <typename T>
    void quick_rotate_2d(T& x, T& y, const real& cosa, const real& sina, const real& scale = 1.)
    {
        T x1 = x * cosa - y * sina, y1 = x * sina + y * cosa;
        x = x1 * scale;
        y = y1 * scale;
    }

    template <typename T>
    inline void rotate_2d(T& x, T& y, const real& angle, const real& scale = 1.)
    {
        quick_rotate_2d(x, y, degcos(angle), degsin(angle), scale);
    }

    real rnd(const real& a, const real& b);
    int rnd(int a, int b);

    template <typename T>
    T modulus_2d(const T& x1, const T& y1, const T& x2, const T& y2)
    {
        T dx = x2 - x1, dy = y2 - y1;
        return static_cast<T>(sqrtf(static_cast<real>(dx * dx + dy * dy)));
    }

    template <typename T>
    inline T linear_proj(const T& a, const T& b, const real& x)
    {
        return a + static_cast<T>(static_cast<real>(b - a) * x);
    }

    template <typename T>
    T cubic_proj(const T& a, const T& b, const real& x)
    {
        T mid = b;
        real y = 1 - x;
        real m1 = y*y*y, m2 = 3 * (y*y * x + y * x*x), m3 = x*x*x;
        return static_cast<T>(static_cast<real>(a) * m1 + static_cast<real>(mid) * m2 + static_cast<real>(b) * m3);
    }

    template <typename T, typename S>
    inline S reproj(const T& x, const T& x0, const T& x1, const S& y0, const S& y1)
    {
        return y0 + static_cast<S>(x - x0) * (y1 - y0) / static_cast<S>(x1 - x0);
    }

    template <typename T>
    inline T reproj_omo(const T& x, const T& x0, const T& x1, const T& y0, const T& y1)
    {
        return x0 == y0 && x1 == y1 ? x : reproj<T, T>(x, x0, x1, y0, y1);
    }

}

#endif // PRELUDE_MATHS_H
