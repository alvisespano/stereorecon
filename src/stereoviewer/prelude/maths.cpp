#include "prelude/stdafx.h"
#include "prelude/maths.h"

namespace prelude
{

    real degsinf(const real& a, const real& phi, const real& q, const real& x)
    {
        return a * degsin(x + phi) + q;
    }

    real degcosf(const real& a, const real& phi, const real& q, const real& x)
    {
        return a * degcos(x + phi) + q;
    }

    real rnd(const real& a, const real& b)
    {
        return real(rand()) / (real(RAND_MAX) + 1) * (b - a) + a;
    }

    int rnd(int a, int b)
    {
        return qrand() % (b - a + 1) + a;
    }

}
