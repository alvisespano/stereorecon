#ifndef PRELUDE_MISC_H
#define PRELUDE_MISC_H

#include "prelude/stdafx.h"
#include "prelude/traits.h"
#include "prelude/prelude_config.h"

namespace prelude
{

    //
    //
    // properties à la C#

    #define __COMMA__ ,

    #define __PROPERTY_INPLACE_OP(name, getter_ty, setter_ty, op) \
        template <typename T> \
        name##_property_type& operator op(const T& value) \
        { \
            using namespace prelude; \
            typedef traits::type_only<getter_ty>::type gty; \
            typedef traits::type_only<setter_ty>::type sty; \
            parent->set_##name((sty)(gty(parent->get_##name()) op value)); \
            return *this; \
        } \

    #define __PROPERTY_BIN_OP(name, getter_ty, op) \
        template <typename T> \
        auto operator op(const T& value) const -> decltype(parent->get_##name() op value) \
        { \
            return parent->get_##name() op value; \
        } \

    #define __PROPERTY_BODY(name, parent_ty, getter_ty, setter_ty) \
        class name##_property_type \
        { \
        private: \
            parent_ty* const parent; \
        public: \
            name##_property_type() = delete; \
            explicit name##_property_type(parent_ty* const _parent) : parent(_parent) {} \
            name##_property_type& operator=(const name##_property_type& p) { parent->set_##name((setter_ty)p.parent->get_##name()); return *this; } \
            name##_property_type& operator=(const setter_ty& value) { parent->set_##name(value); return *this; } \
            operator getter_ty() const { return parent->get_##name(); } \
            /*template <typename T> operator T() const { return static_cast<T>((parent->get_##name()); } \
            __PROPERTY_INPLACE_OP(name, getter_ty, setter_ty, +=) \
            __PROPERTY_INPLACE_OP(name, getter_ty, setter_ty, -=) \
            __PROPERTY_INPLACE_OP(name, getter_ty, setter_ty, *=) \
            __PROPERTY_INPLACE_OP(name, getter_ty, setter_ty, /=) \
            __PROPERTY_INPLACE_OP(name, getter_ty, setter_ty, %=) \
            __PROPERTY_INPLACE_OP(name, getter_ty, setter_ty, <<=) \
            __PROPERTY_INPLACE_OP(name, getter_ty, setter_ty, >>=) \
            __PROPERTY_BIN_OP(name, getter_ty, +) \
            __PROPERTY_BIN_OP(name, getter_ty, -) \
            __PROPERTY_BIN_OP(name, getter_ty, *) \
            __PROPERTY_BIN_OP(name, getter_ty, /) \
            __PROPERTY_BIN_OP(name, getter_ty, %) \
            __PROPERTY_BIN_OP(name, getter_ty, <<) \
            __PROPERTY_BIN_OP(name, getter_ty, >>) \
            __PROPERTY_BIN_OP(name, getter_ty, >) \
            __PROPERTY_BIN_OP(name, getter_ty, <) \
            __PROPERTY_BIN_OP(name, getter_ty, <=) \
            __PROPERTY_BIN_OP(name, getter_ty, >=) \
            __PROPERTY_BIN_OP(name, getter_ty, ==) \
            __PROPERTY_BIN_OP(name, getter_ty, !=)*/ \
        }; \
        class const_##name##_property_type \
        { \
        private: \
            const parent_ty* const parent; \
        public: \
            const_##name##_property_type() = delete; \
            const_##name##_property_type& operator=(const const_##name##_property_type& p) = delete; \
            explicit const_##name##_property_type(const parent_ty* const _parent) : parent(_parent) {} \
            operator getter_ty() const { return parent->get_##name(); } \
            /*template <typename T> operator T() const { return static_cast<T>((parent->get_##name()); }*/ \
        }; \
        \
        name##_property_type name() \
        { \
            return name##_property_type(this); \
        } \
        \
        const_##name##_property_type name() const \
        { \
            return const_##name##_property_type(this); \
        } \

    #define PROPERTY(name, parent_ty, getter_ty, setter_ty, getter, setter) \
        getter_ty get_##name() const { getter; } \
        void set_##name(const setter_ty& value) { setter; } \
        \
        __PROPERTY_BODY(name, parent_ty, getter_ty, setter_ty)

    #define PROPERTY_DECL(name, parent_ty, getter_ty, setter_ty) \
        getter_ty get_##name() const; \
        void set_##name(const setter_ty& value); \
        \
        __PROPERTY_BODY(name, parent_ty, getter_ty, setter_ty)

    #define PROPERTY_DEF(name, parent_ty, getter_ty, setter_ty, getter, setter) \
        getter_ty parent_ty::get_##name() const { getter; } \
        void parent_ty::set_##name(const setter_ty& value) { setter; } \

    #define VALUE_PROPERTY(name, parent_ty, ty, getter, setter) PROPERTY(name, parent_ty, ty, ty, getter, setter)

    #define REF_PROPERTY(name, parent_ty, ty, getter, setter) PROPERTY(name, parent_ty, const ty&, ty, getter, setter)

    #define ATTR_PROPERTY(name, parent_ty, ty, attr) REF_PROPERTY(name, parent_ty, ty, { return attr; }, { attr = value; })

    #define QUICK_ATTR_PROPERTY(name, parent_ty, ty) ATTR_PROPERTY(name, parent_ty, ty, _##name)

    //
    //
    // function object macros

    #define UNARY_FUNCTION_TYPE(name, rty, ty1, arg1, body) \
        struct name : public std::unary_function<ty1, prelude::traits::type_only<rty>::type> \
        { \
            /*typedef std::unary_function<ty1, rty>::argument_type argument_type; \
            typedef std::unary_function<ty1, rty>::result_type result_type;*/ \
            rty operator()(ty1 arg1) const body \
        }

    #define BINARY_FUNCTION_TYPE(name, rty, ty1, arg1, ty2, arg2, body) \
        struct name : public std::binary_function<ty1, ty2, rty> \
        { \
            /*typedef std::binary_function<ty1, rty>::first_argument_type first_argument_type; \
            typedef std::binary_function<ty1, rty>::second_argument_type second_argument_type; \
            typedef std::binary_function<ty1, rty>::result_type result_type;*/ \
            rty operator()(ty1 arg1, ty2 arg2) const body \
        }

    #define UNARY_PREDICATE_TYPE(name, ty, arg, body) \
        UNARY_FUNCTION_TYPE(name, bool, ty, arg, body)

    #define IDENTITY_FUNCTION_TYPE(name, ty) \
        UNARY_FUNCTION_TYPE(name, ty, const ty&, x, { return x; })

    #define BINARY_PREDICATE_TYPE(name, ty1, arg1, ty2, arg2, body) \
        BINARY_FUNCTION_TYPE(name, bool, ty1, arg1, ty2, arg2, body)

    #define UNARY_FUNCTION_DECL(name, rty, ty1, arg1, body) \
        UNARY_FUNCTION_TYPE(name##_type, rty, ty1, arg1, body) name

    #define BINARY_FUNCTION_DECL(name, rty, ty1, arg1, ty2, arg2, body) \
        static BINARY_FUNCTION_TYPE(name##_type, rty, ty1, arg1, ty2, arg2, body) name

    #define UNARY_PREDICATE_DECL(name, ty, arg, body) \
        static UNARY_PREDICATE_TYPE(name##_type, ty, arg, body) name

    #define IDENTITY_FUNCTION_DECL(name, ty) \
        static IDENTITY_FUNCTION_TYPE(name##_type, ty) name

    #define BINARY_PREDICATE_DECL(name, ty1, arg1, ty2, arg2, body) \
        static BINARY_PREDICATE_TYPE(name##_type, ty1, arg1, ty2, arg2, body) name


    //
    //
    // fuctional utils

    //
    // STL Generators (zerary functions) support

    template <typename R>
    struct generator
    {
        typedef R result_type;
    };

    template<typename T, typename F = T (*)()>
    class pointer_to_generator : public generator<T>
    {
    protected:
        F f;

    public:
        using generator<T>::result_type;

        explicit pointer_to_generator(F _f) : f(_f) {}

        T operator()() const { return f(); }
    };

    template <typename T>
    inline pointer_to_generator<T> ptr_fun(T (*f)())
    {
        return pointer_to_generator<T>(f);
    }

    //
    //
    // apply an n-ary tuple to an n-ary function

    namespace details_apply_unpacked
    {
        template<unsigned int N>
        struct aux
        {
            template <typename F, typename... ArgsT, typename... Args>
            static typename std::result_of<F>::type
              apply_unpacked(const F& f,
                             const std::tuple<ArgsT...>& t,
                             Args&&... args)
            {
                return aux<N - 1>::apply_unpacked(f, t, std::get<N - 1>(t), std::forward<Args>(args)...);
            }
        };

        template<>
        struct aux<0>
        {
            template <typename F, typename... ArgsT, typename... Args>
            static typename std::result_of<F>::type
              apply_unpacked(const F& f,
                             const std::tuple<ArgsT...>&,
                             Args&&... args)
            {
                return f(std::forward<Args>(args)...);
            }
        };
    }

    template <typename F, typename... Args>
    typename std::result_of<F>::type apply_unpacked(const F& f, const std::tuple<Args...>& t)
    {
       return details_apply_unpacked::aux<sizeof...(Args)>::apply_unpacked(f, t);
    }

    //
    //
    // printf-like and formatting facilities

    std::string asprintf(const char* fmt, ...);
    std::string vasprintf(const char* fmt, va_list args);

    QString qsprintf(const char* fmt, ...);
    QString vqsprintf(const char* fmt, va_list args);

    //
    // currying-like format with no checking on application-depth

    class fmt
    {
    protected:
        QString s;

    public:
        typedef fmt result_type;

        fmt() = delete;
        fmt(const char* _s) : s(_s) {}
        fmt(const QString& _s) : s(_s) {}
        fmt(const std::string& _s) : s(_s.c_str()) {}

        template <typename T>
        fmt operator()(const T& x)
        {
            return fmt(s.arg(x));
        }

        fmt operator()(const std::string& str)
        {
            return (*this)(str.c_str());
        }

        operator const QString&() const { return s; }
        operator std::string() const { return s.toStdString(); }
        operator const char*() const { return s.toStdString().c_str(); }
    };

    //
    // currying-like format with templatized application-depth

    template <size_t N>
    class nfmt
    {
    protected:
        const QString s;

    public:
        typedef nfmt<N - 1> result_type;

        nfmt() = delete;
        nfmt(const QString& _s) : s(_s) {}
        nfmt(const char* _s) : s(_s) {}
        nfmt(const std::string& _s) : s(_s.c_str()) {}

        template <typename T>
        result_type operator()(const T& x) const
        {
            return nfmt<N - 1>(s.arg(x));
        }

        result_type operator()(const std::string& str) const
        {
            return (*this)(str.c_str());
        }

        const QString& q_str() const { return s; }
        std::string std_str() const { return s.toStdString(); }
        const char* c_str() const { return std_str().c_str(); }
    };

    template <>
    class nfmt<0>
    {
    protected:
        QString s;

    public:
        nfmt() : s("") {}
        nfmt(const char* _s) : s(_s) {}
        nfmt(const QString& _s) : s(_s) {}
        nfmt(const std::string& _s) : s(_s.c_str()) {}

        const QString& q_str() const { return s; }
        std::string std_str() const { return s.toStdString(); }
        const char* c_str() const { return std_str().c_str(); }

        operator const QString&() const { return q_str(); }
    };

    //
    //
    // iterator utilities

    //
    // iterator wrapper that maps elements by means of a higher-order

    template <typename Iterator, typename UnaryFun>
    class mapping_iterator
        /*: public std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                               typename std::result_of<UnaryFun(typename Iterator::value_type)>::type,
                               typename std::iterator_traits<Iterator>::difference_type,
                               typename std::result_of<UnaryFun(typename Iterator::value_type)>::type*,
                               typename std::result_of<UnaryFun(typename Iterator::value_type)>::type&>*/
    {
    private:
        typedef mapping_iterator<Iterator, UnaryFun> self;

        Iterator it;
        UnaryFun f;

    public:
        typedef typename std::iterator_traits<Iterator>::iterator_category iterator_category;
        typedef typename std::iterator_traits<Iterator>::difference_type difference_type;
        typedef typename std::result_of<UnaryFun(typename Iterator::value_type)>::type reference;
        typedef typename std::remove_reference<reference>::type value_type;
        typedef value_type* pointer;

        explicit mapping_iterator(const Iterator& _it, const UnaryFun& _f) : it(_it), f(_f) {}

        reference operator*() const { return f(*it); }
        pointer operator->() const { return &(operator*()); }

        bool operator==(const self& that) const { return it == that.it; } \
        bool operator!=(const self& that) const { return !operator==(that); } \

        self& operator+=(const difference_type& d) { it += d; return *this; }
        self& operator-=(const difference_type& d) { it -= d; return *this; }

        self operator+(const difference_type& d) const { return self(*this) += d; }
        self operator-(const difference_type& d) const { return self(*this) -= d; }

        difference_type operator-(const self& d) const { return d.it - it; }

        self& operator++() { ++it; return *this; }
        self& operator++(int) { self r(*this); operator++(); return r; }

        self& operator--() { --it; return *this; }
        self& operator--(int) { self r(*this); operator--(); return r; }
    };

    template <typename Iterator, typename UnaryFun>
    inline mapping_iterator<Iterator, UnaryFun> make_mapping_iterator(const Iterator& it, const UnaryFun& f)
    {
        return mapping_iterator<Iterator, UnaryFun>(it, f);
    }

    //
    //
    // fresh numbers

    inline unsigned int fresh_int()
    {
        static unsigned int c = 0;
        return c++;
    }

    //
    //
    // opaque id-type metatype

    template <typename T, typename U, U Unique>
    class unique_id_type
    {
    private:
        typedef unique_id_type<T, U, Unique> self;
        T x;

    public:
        unique_id_type() = default;
        unique_id_type(const T& _x) : x(_x) {}
        unique_id_type(const self&) = default;

        self& operator=(const self&) = default;
        bool operator==(const self& that) const { return x == that.x; }
        bool operator!=(const self& that) const { return x != that.x; }

        operator QString() const { return nfmt<1>("%1") (x); }
        uint hash() const { return qHash(x); }
    };

    template <typename T, typename U, U Unique>
    inline uint qHash(const unique_id_type<T, U, Unique>& uid)
    {
        return uid.hash();
    }

    //
    //
    // useful exceptions & code localization utilities

    extern const nfmt<4> full_localized_fmt;
    extern const nfmt<2> compact_localized_fmt;

    template <typename E>
    class localized_error : public E
    {
    public:
        localized_error(const QString& file, unsigned int line, const QString& func, const QString& msg)
            : E(full_localized_fmt(file)(line)(func)(msg).std_str()) {}

        localized_error(const QString& location, const QString& msg)
            : E(compact_localized_fmt(location)(msg).std_str()) {}

        localized_error(const QString& msg) : E(msg.toStdString()) {}
    };

    typedef localized_error<std::runtime_error> localized_runtime_error;
    typedef localized_error<std::invalid_argument> localized_invalid_argument;

    #define LOCATE_HERE(msg) (::prelude::full_localized_fmt(__FILE__)(__LINE__)(__FUNCTION__)(msg))
    #ifdef QT_DEBUG
    #   define HERE(msg) LOCATE_HERE(msg)
    #else
    #   define HERE(msg) (::prelude::compact_localized_fmt(__FUNCTION__)(msg))
    #endif
    // regexp per rimpiazzare le vecchie occorrenze: localized_([a-z_]+)\(("([^"]+)", )+

    //
    //
    // thread-safe intrusive reference counter

    /*template <typename Derived>
    class refcnt
    {
    protected:
        class expunger
        {
        private:
            Derived* const pt;

        public:
            explicit expunger(Derived* const _pt) : pt(_pt) {}

            expunger(const expunger&) = delete;
            expunger& operator=(const expunger&) = delete;

            ~expunger() { pt->expunge(); }
        };

        std::shared_ptr<expunger> pt;

    public:
        refcnt() = delete;

        refcnt(Derived* const derived) : pt(std::make_shared<expunger>(derived)) {}

        refcnt(refcnt<Derived>&& r) : pt(std::move(r.pt)) {}

        refcnt& operator=(refcnt&& r)
        {
            pt.operator=(std::move(r.pt));
            return *this;
        }

        refcnt(const refcnt& r) = default;
        refcnt& operator=(const refcnt& r) = default;

        auto count() const -> decltype(pt.use_count()) { return pt.use_count(); }
    };*/

    class refcnt
    {
    private:
        QMutex* m;
        size_t* cnt;

        void lock() const { if (m != NULL) m->lock(); }
        void unlock() const { if (m != NULL) m->unlock(); }

        void copy(const refcnt& r);
        void move(refcnt&& r);

    protected:
        virtual void dispose() throw () = 0;

        void unref();
        size_t count() const;

        refcnt() : m(new QMutex(QMutex::NonRecursive)), cnt(new size_t(1)) {}
        refcnt(const refcnt& r) { copy(r); }
        refcnt(refcnt&& r) { move(std::move(r)); }

        refcnt& operator=(const refcnt& r)
        {
            unref();
            copy(r);
            return *this;
        }

        refcnt& operator=(refcnt&& r)
        {
            unref();
            move(std::move(r));
            return *this;
        }
    };

    //
    //
    // ML-like option type

    template <typename T> class option;
    template <typename T> option<T> some(const T& x);

    template <typename T>
    class option
    {
        friend option<T> some<T>(const T& x);
        template <typename U> friend class option;

    public:
        typedef T value_type;

    private:
        T* p;

        void del() { if (p != NULL) delete p; }

        void copy(const option<T>& o) { p = o.p == NULL ? NULL : new T(*o.p); }

        void move(option<T>&& o)
        {
            p = o.p;
            o.p = NULL;
        }

        explicit option(const T& x) : p(new T(x)) {}

    public:
        option() : p(NULL) {}

        option(const option<T>& o) { copy(o); }

        option(option<T>&& o) { move(std::move(o)); }

        ~option() { del(); }

        option<T>& operator=(const option<T>& o)
        {
            del();
            copy(o);
            return *this;
        }

        option<T>& operator=(option<T>&& o)
        {
            del();
            move(std::move(o));
            return *this;
        }

        // method-based API
        bool is_none() const { return p == NULL; }
        bool is_some() const { return !is_none(); }
        T& some() { return *p; }
        const T& some() const { return *p; }

        // operator-based API
        T& operator*() { return some(); }
        const T& operator*() const { return some(); }
        T* operator->() { return p; }
        const T* operator->() const { return p; }
        operator bool() const { return is_some(); }

        template <typename U>
        operator option<U>() const
        {
            return *this ? option<U>(U(**this)) : option<U>();
        }

        //
        // misc facilities

        const T& something(const T& x) const { return *this ? **this : x; }

        template <typename UnaryFunction>
        option<typename std::result_of<UnaryFunction>::type>
          map(const UnaryFunction& f) const;

        template <typename C, typename UnaryMemberFunction>
        option<typename std::result_of<UnaryMemberFunction>::type>
          map(C& o, const UnaryMemberFunction& m) const;

    };

    template <typename T>
    inline option<T> some(const T& x) { return option<T>(x); }

    namespace details_option
    {
        struct polynone
        {
            polynone() = default;
            polynone(const polynone&) = delete;
            polynone& operator=(const polynone&) = delete;

            template <typename T>
            operator option<T>() const { return option<T>(); }
        };
    }

    extern const details_option::polynone none;

    template <typename T>
    template <typename UnaryFunction>
    inline option<typename std::result_of<UnaryFunction>::type>
      option<T>::map(const UnaryFunction& f) const
    {
        return *this ? some(f(**this)) : none;
    }

    template <typename T>
    template <typename C, typename UnaryMemberFunction>
    inline option<typename std::result_of<UnaryMemberFunction>::type>
      option<T>::map(C& o, const UnaryMemberFunction& m) const
    {
        return *this ? some((o.*m)(**this)) : none;
    }

    //
    //
    // lazy object box

    template <typename T>
    class lazy
    {
    private:
        mutable option<T> opt;
        std::function<T()> f;

    public:
        lazy(std::function<T()> _f) : f(_f) {}

        template <typename F>
        lazy(const F& _f) : f(std::function<T()>(_f)) {}

        lazy() = delete;
        lazy(const lazy<T>&) = default;
        lazy<T>& operator=(const lazy<T>&) = default;

    private:
        template <typename This, typename R>
        static R __force(This* _this)
        {
            if (!_this->opt) _this->opt = some(_this->f());
            return *_this->opt;
        }

    public:
        const T& force() const { return __force<__typeof__(this), const T&>(this); }
        T& force() { return __force<__typeof__(this), T&>(this); }

        const T& operator*() const { return force(); }
        T& operator*() { return force(); }
    };

    //
    //
    // functional exception trapper and optionizer

    template <typename R, typename... Args, typename Exception = std::exception>
    std::function<option<R>(Args...)> trap(const std::function<R(Args...)>& f)
    {
        struct optf
        {
            typedef option<R> result_type;

            std::function<R(Args...)> f;

            result_type operator()(Args&&... args) const
            {
                try { return some(f(std::forward<Args>(args)...)); }
                catch (Exception&) { return none; }
            }

            optf(const std::function<R(Args...)>& _f) : f(_f) {}
        };
        return std::function<option<R>(Args...)>(optf(f));
    }

    /*namespace details_trap
    {
        template <typename F, typename Exception>
        class funobj;
    }

    template <typename F, typename Exception = std::exception>
    details_trap::funobj<F, Exception> trap(const F& f);

    namespace details_trap
    {
        template <typename F, typename Exception>
        class funobj
        {
            friend details_trap::funobj<F, Exception> prelude::trap<F>(const F&);

        private:
            F f;

            explicit funobj(const F& _f) : f(_f) {}

        public:
            template <typename... Args>
            struct result
            {
                typedef option<typename std::result_of<F(Args...)>::type> type;
            };

            template <typename... Args>
            typename result<Args...>::type operator()(Args&&... args) const
            {
                try { return some(f(std::forward<Args>(args)...)); }
                catch (Exception&) { return none; }
            }
        };
    }

    template <typename R, typename... Args, typename Exception = std::exception>
    auto trap(const R(*f)(Args...)) -> decltype(trap(std::function<R(Args...)>(f)))
    {
        return trap(std::function<R(Args...)>(f));
    }

    template <typename R, typename C, typename... Args, typename Exception = std::exception>
    auto trap(const R(C::*m)(Args...)) -> decltype(trap(std::function<R(Args...)>(m)))
    {
        return trap(std::function<R(Args...)>(m));
    }*/

}

#endif // PRELUDE_MISC_H

