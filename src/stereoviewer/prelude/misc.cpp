#include "prelude/stdafx.h"
#include "prelude/misc.h"

namespace prelude
{
    using std::string;

    //
    // refcnt

    void refcnt::copy(const refcnt& r)
    {
        r.lock();
        m = r.m;
        cnt = r.cnt;
        if (cnt != NULL) ++(*cnt);
        r.unlock();
    }

    void refcnt::move(refcnt&& r)
    {
        r.lock();
        m = r.m;
        cnt = r.cnt;
        r.m = NULL;
        r.cnt = NULL;
        r.unlock();
    }

    void refcnt::unref()
    {
        lock();
        if (cnt != NULL && --(*cnt) == 0)
        {
            dispose();
            delete cnt;
            cnt = NULL;
            unlock();
            delete m;
            m = NULL;
        }
        else unlock();
    }

    size_t refcnt::count() const
    {
        lock();
        const size_t r = cnt != NULL ? *cnt : 0;
        unlock();
        return r;
    }

    //
    // option

    const details_option::polynone none;

    //
    // fmt

    const nfmt<4> full_localized_fmt("[%1:%2] [%3] %4");
    const nfmt<2> compact_localized_fmt("[%1] %2");

    string vasprintf(const char* fmt, va_list args)
    {
        char buff[1024];
        int len = ::vsprintf(buff, fmt, args);
        std::stringstream ss;
        ss << "invalid format: " << fmt;
        if(len < 0) throw localized_invalid_argument(HERE(ss.str().c_str()));
        return string(buff, len);
    }

    string asprintf(const char* fmt, ...)
    {
        va_list argptr;
        va_start(argptr, fmt);
        string r = vasprintf(fmt, argptr);
        va_end(argptr);
        return r;
    }

    QString vqsprintf(const char* fmt, va_list args)
    {
        return QString().vsprintf(fmt, args);
    }

}
