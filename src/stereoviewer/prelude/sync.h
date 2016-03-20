#ifndef PRELUDE_SYNC_H
#define PRELUDE_SYNC_H

#include "prelude/stdafx.h"
#include "prelude/misc.h"
#include "prelude/log.h"
#include "prelude/sync.h"
#include "prelude/maths.h"

namespace prelude
{

    //
	//
    // scoped handles on lockable types

    #define __SCOPED_LOCK_DEF(prefix) \
        template <typename Lockable> \
        class prefix##scoped_lock \
        { \
        protected: \
            Lockable* lockable; \
        public: \
            prefix##scoped_lock(Lockable& l) : lockable(&l) { l.prefix##lock(); } \
            prefix##scoped_lock(prefix##scoped_lock&& sl) : lockable(sl.lockable) { sl.lockable = NULL; } \
            ~prefix##scoped_lock() { if (lockable != NULL) lockable->unlock(); } \
            prefix##scoped_lock() = delete; \
            prefix##scoped_lock(const prefix##scoped_lock&) = delete; \
            prefix##scoped_lock& operator=(const prefix##scoped_lock&) = delete; \
        };

    __SCOPED_LOCK_DEF( )
    __SCOPED_LOCK_DEF(ro_)
    __SCOPED_LOCK_DEF(rw_)

    #undef __SCOPED_LOCK_DEF

    //
	//
	// mutex class

    class rendezvous;

    template <bool Recursive = true>
	class rmutex
	{
        friend class rendezvous;

    private:
        typedef rmutex<Recursive> self;

	protected:
        QMutex* qm;

        operator QMutex*() { return qm; }

	public:
        typedef scoped_lock<self> scoped;
	
        rmutex() : qm(new QMutex(Recursive ? QMutex::Recursive : QMutex::NonRecursive)) {}

        rmutex(self&& m) : qm(std::move(m.qm))
        {
            m.qm = NULL;
        }

        ~rmutex()
        {
            if (qm != NULL)
            {
                #ifdef QT_DEBUG
                if (qm->tryLock()) qm->unlock();
                else fallback_logger.unexpected_error("invoking destructor on locked rmutex");
                #endif
                delete qm;
            }
        }

        rmutex(const self&) = delete;
        rmutex& operator=(const self&) = delete;

        bool try_lock() { return qm->tryLock(); }
        void lock() { qm->lock(); }
        void unlock() { qm->unlock(); }
    };
    
    typedef rmutex<true> mutex;
    typedef rmutex<false> umutex;

    //
    //
    // multi-reader-single-writer mutex

    template <bool Recursive = true>
    class rw_rmutex
    {
        friend class rendezvous;

    private:
        typedef rw_rmutex<Recursive> self;

    protected:
        QReadWriteLock* qrwl;

        operator QReadWriteLock*() { return qrwl; }

    public:
        typedef ro_scoped_lock<self> ro_scoped;
        typedef rw_scoped_lock<self> rw_scoped;

        rw_rmutex() : qrwl(new QReadWriteLock(Recursive ? QReadWriteLock::Recursive : QReadWriteLock::NonRecursive)) {}

        rw_rmutex(self&& rwm) : qrwl(std::move(rwm.qrwl))
        {
            rwm.qrwl = NULL;
        }

        ~rw_rmutex()
        {
            if (qrwl != NULL)
            {
                #ifdef QT_DEBUG
                if (qrwl->tryLockForWrite()) qrwl->unlock();
                else fallback_logger.unexpected_error("invoking destructor on locked rw_rmutex");
                #endif
                delete qrwl;
            }
        }

        rw_rmutex(const self&) = delete;
        rw_rmutex& operator=(const self&) = delete;

        bool try_ro_lock() { return qrwl->tryLockForRead(); }
        bool try_rw_lock() { return qrwl->tryLockForWrite(); }

        void ro_lock() { qrwl->lockForRead(); }
        void rw_lock() { qrwl->lockForWrite(); }

        void unlock() { qrwl->unlock(); }
    };

    typedef rw_rmutex<true> rw_mutex;
    typedef rw_rmutex<false> rw_umutex;

    //
    // atomic log printer

    namespace log
    {
        class atomic_printer : public printer
        {
        protected:
            mutex m;
            const std::shared_ptr<printer> p;

        public:
            virtual void print_line(const QString& s)
            {
                mutex::scoped l(m);
                p->print_line(s);
            }

            explicit atomic_printer(const std::shared_ptr<printer>& _p) : p(_p) {}
        };
    }

    //
	//
	// simple condition

	class rendezvous
	{
    protected:
        QWaitCondition cond;

	public:
        rendezvous() = default;
        rendezvous(const rendezvous&) = delete;
        rendezvous& operator=(const rendezvous&) = delete;

        struct timed_out : public localized_runtime_error
        {
            explicit timed_out(unsigned long ms)
                : localized_runtime_error(HERE(nfmt<1>("timeout elapsed (%1 ms)") (ms)))
            {}
        };

    protected:
        template <typename Q>
        void wait(Q* q, const option<unsigned long>& ms)
        {
            if (ms)
            {
                if (!cond.wait(q, *ms)) throw timed_out(*ms);
            }
            else cond.wait(q);
        }

    public:
        void wait(umutex& m, const option<unsigned long>& ms = none) { wait(static_cast<QMutex*>(m), ms); }
        void wait(rw_umutex& m, const option<unsigned long>& ms = none) { wait(static_cast<QReadWriteLock*>(m), ms); }

        void signal() { cond.wakeOne(); }
		
        void broadcast() { cond.wakeAll(); }
    };

    //
	//
	// generic synchronized queue class

    template <typename T>
	class sync_queue
	{
	protected:
        mutable umutex m;
		rendezvous rz;
        QQueue<T> q;
        option<size_t> _max_size;

	public:
        typedef T value_type;

        sync_queue() : _max_size(none) {}

        explicit sync_queue(const option<size_t>& __max_size)
            : _max_size(__max_size) {}

        T pop(const option<unsigned long>& ms = none);
        option<T> try_pop(const option<unsigned long>& ms = none);

        template <typename R>
        R apply_to_head(const std::function<R(T&)>& f)
        {
            umutex::scoped l(m);
            return f(q.first());
        }

        template <typename R>
        R apply_to_head(const std::function<R(const T&)>& f) const
        {
            umutex::scoped l(m);
            return f(q.first());
        }

        void push(const T& x);

        size_t size() const
		{
            umutex::scoped l(m);
			return q.size();
        }

        bool empty() const
        {
            umutex::scoped l(m);
            return q.isEmpty();
        }

		void clear()
		{
            umutex::scoped l(m);
			q.clear();
		}

        REF_PROPERTY(max_size, sync_queue<T>, option<size_t>,
        {
            umutex::scoped l(m);
            return _max_size;
        },
        {
            umutex::scoped l(m);
            _max_size = value;
        })
	};

    template <typename T>
    T sync_queue<T>::pop(const option<unsigned long>& ms)
    {
        umutex::scoped sl(m);
        while (q.empty()) rz.wait(m, ms);
        return q.dequeue();
    }

    template <typename T>
    option<T> sync_queue<T>::try_pop(const option<unsigned long>& ms)
    {
        try
        {
            return some(pop(ms));
        }
        catch (rendezvous::timed_out& e)
        {
            return none;
        }
    }

	template <typename T>
    void sync_queue<T>::push(const T& x)
	{
		{
            umutex::scoped sl(m);
            while (_max_size && static_cast<size_t>(q.size()) >= *_max_size) q.dequeue();
            q.enqueue(x);
		}
		rz.signal();
    }

    //
	//
	// generic object synchronizer
  
	template <typename T>
	class sync
	{
        friend class ro_scoped_lock<const sync<T>>;
        friend class rw_scoped_lock<sync<T>>;

	protected:
        mutable rw_mutex m;
        T o;
		
        void ro_lock() const { m.ro_lock(); }
        void rw_lock() const { m.rw_lock(); }
        void unlock() const { m.unlock(); }

	public:
        typedef T locked_type;

        sync() : o() {}
        explicit sync(const T& _o) : m(), o(_o) {}

        sync(sync<T>&& sy) : m(std::move(sy.m)), o(std::move(sy.o)) {}

        sync(const sync<T>& sy) = delete;
        sync<T>& operator=(const sync<T>&) = delete;

        //
        // apply member function family

        template <typename R>
        R apply(const std::function<R(const T&)>& f) const
        {
            rw_mutex::ro_scoped sl(m);
            return f(o);
        }

        template <typename F>
        typename std::result_of<F(const T&)>::type apply(const F& f) const
        {
            typedef typename std::result_of<F(const T&)>::type result_type;
            return apply(std::function<result_type(const T&)>(f));
        }

        template <typename R>
        R apply(const std::function<R(T&)>& f)
        {
            rw_mutex::rw_scoped sl(m);
            return f(o);
        }

        template <typename F>
        typename std::result_of<F(T&)>::type apply(const F& f)
        {
            typedef typename std::result_of<F(T&)>::type result_type;
            return apply(std::function<result_type(T&)>(f));
        }
				
        //
        // other member functions

        void apply_and_set(const std::function<T(T&)>& f)
        {
            rw_mutex::rw_scoped sl(m);
            o = f(o);
        }		

        void set(const T& x)
        {
            rw_mutex::rw_scoped sl(m);
            o = x;
        }
		
        T copy() const
        {
            rw_mutex::ro_scoped sl(m);
            return o;
        }

        class ro_scoped : private ro_scoped_lock<const sync<T>>
		{
            friend class sync<T>;

        private:
            typedef ro_scoped_lock<const sync<T>> super;

        public:
            typedef T locked_type;

            explicit ro_scoped(const sync<T>& sy) : super(sy) {}
            ro_scoped(ro_scoped&& sl) : super(std::move(sl)) {}

            const T& operator*() const { return this->lockable->o; }
            const T* operator->() const { return &operator*(); }

            ro_scoped() = delete;
            ro_scoped(const ro_scoped&) = delete;
            ro_scoped& operator=(const ro_scoped&) = delete;
        };

        struct rw_scoped : private rw_scoped_lock<sync<T>>
        {
            friend class sync<T>;

        private:
            typedef rw_scoped_lock<sync<T>> super;

        public:
            typedef T locked_type;

            explicit rw_scoped(sync<T>& sy) : super(sy) {}
            rw_scoped(rw_scoped&& sl) : super(std::move(sl)) {}

            T& operator*() { return this->lockable->o; }
            T* operator->() { return &operator*(); }

            rw_scoped() = delete;
            rw_scoped(const rw_scoped&) = delete;
            rw_scoped& operator=(const rw_scoped&) = delete;
        };

        ro_scoped ro_atomic() const { return ro_scoped(*this); }
        rw_scoped rw_atomic() { return rw_scoped(*this); }
        ro_scoped atomic() const { return ro_atomic(); }
        rw_scoped atomic() { return rw_atomic(); }
    };

    class sync_flag : public sync<bool>
    {
    public:
        sync_flag() = delete;

        sync_flag(const sync_flag& syf) : sync<bool>(bool(syf)) {}

        sync_flag(bool b) : sync<bool>(b) {}

        sync_flag& operator=(bool b)
        {
            set(b);
            return *this;
        }

        operator bool() const { return copy(); }
        bool operator!() const { return !bool(*this); }
        bool operator&&(bool b) const { return bool(*this) && b; }
        bool operator||(bool b) const { return bool(*this) || b; }
    };

    //
    //
    // condition with data

    template <typename T>
    class gatherer
    {
    public:
        typedef T& result_type;

    protected:
        struct wrapper
        {
            T value;

            wrapper() = delete;
            wrapper& operator=(const wrapper&) = delete;

            explicit wrapper(typename traits::argfwd<T>::type _value)
                : value(_value)
            {}
        };

        umutex m;
        rendezvous rz;
        option<wrapper> w;

    public:
        gatherer() = default;
        gatherer(const gatherer<T>&) = delete;
        gatherer& operator=(const gatherer<T>&) = delete;

        result_type wait(const option<unsigned long>& ms = none)
        {
            umutex::scoped sl(m);
            if (!w) rz.wait(m, ms);
            return w->value;
        }

        void reset()
        {
            umutex::scoped sl(m);
            w = none;
        }

        void broadcast(typename traits::argfwd<T>::type value)
        {
            {
                umutex::scoped sl(m);
                if (w) throw localized_invalid_argument(HERE("object has not been reset before reuse"));
                w = some(wrapper(value));
            }
            rz.broadcast();
        }
    };

    template <>
    class gatherer<void>
    {
    protected:
        umutex m;
        rendezvous rz;
        bool signaled;

    public:
        typedef void result_type;

        gatherer() : signaled(false) {}
        gatherer(const gatherer<void>&) = delete;
        gatherer& operator=(const gatherer<void>&) = delete;

        void wait(const option<unsigned long>& ms = none)
        {
            umutex::scoped sl(m);
            if (!signaled) rz.wait(m, ms);
        }

        void reset()
        {
            umutex::scoped sl(m);
            signaled = false;
        }

        void broadcast()
        {
            {
                umutex::scoped sl(m);
                if (signaled) throw localized_invalid_argument(HERE("object has not been reset before reuse"));
                signaled = true;
            }
            rz.broadcast();
        }
    };

    //
    //
    // generic resource pool

    template <typename T>
    class pool : protected sync_queue< std::tuple< unsigned int, std::shared_ptr<T> > >
    {
    private:
        typedef sync_queue< std::tuple<unsigned int, std::shared_ptr<T> > > super;

    protected:
        typedef unsigned int timestamp_type;
        typedef typename super::value_type queued_type;

    public:
        struct configuration
        {
            option<timestamp_type> expiration_timeout, purge_interval;    // in seconds
            option<size_t> min_size, max_size;
        };

        typedef T value_type;

        class resource : private refcnt
        {
            friend class pool<T>;

        protected:
            const std::shared_ptr< pool<T>* > poolpt;
            const std::shared_ptr<T> pt;

            virtual void dispose() throw ();

            resource(const std::shared_ptr<pool<T>*>& _poolpt, const std::shared_ptr<T>& _pt)
                : poolpt(_poolpt), pt(_pt)
            {}

        public:
            typedef pool<T>::value_type value_type;
            typedef value_type& reference;
            typedef value_type* pointer;

            resource(const resource&) = default;
            resource& operator=(const resource&) = delete;

            ~resource() { unref(); }

            reference operator*() { return *pt; }
            const reference operator*() const { return *pt; }

            pointer operator->() const { return &operator*(); }
        };

        friend class pool<T>::resource;

        class factory
        {
            friend class pool<T>;

        protected:
            virtual std::shared_ptr<T> create() = 0;
            virtual void dispose(T& /*object*/) {}
            virtual void on_acquire(T& /*object*/) {}
            virtual void on_release(T& /*object*/) {}
        };

    protected:
        configuration cfg;
        const std::shared_ptr< pool<T>* > selfpt;
        timestamp_type last_purge;
        factory* const fac;

        static timestamp_type get_now()
        {
            return QDateTime::currentDateTime().toTime_t();
        }

        void enqueue(const std::shared_ptr<T>& pt)
        {
            this->push(std::make_tuple(get_now(), pt));
        }

        void release(resource& res);
        void may_purge();
        void __purge();

    public:
        pool() = delete;
        pool(const pool<T>&) = delete;
        pool<T>& operator=(const pool<T>&) = delete;

        pool(const configuration& cfg, factory* const fac);

        virtual ~pool()
        {
            umutex::scoped sl(this->m);
            *selfpt = NULL;
        }

        void assure(size_t n);
        resource use(const option<unsigned long>& ms = none);
        std::shared_ptr<T> acquire(const option<unsigned long>& ms = none);
        void release(const std::shared_ptr<T>& pt);
        void purge();

        option<std::shared_ptr<T>> try_acquire(const option<unsigned long>& ms = none)
        {
            return trap(acquire)(ms);
        }

        REF_PROPERTY(config, pool<T>, configuration,
        {
            mutex::scoped sl(this->m);
            return cfg;
        },
        {
            mutex::scoped sl(this->m);
            cfg = value;
            this->_max_size = value.max_size;
        })
    };

    template <typename T>
    pool<T>::pool(const configuration& _cfg, factory* const _fac)
        : super(_cfg.max_size),
          cfg(_cfg),
          selfpt(std::make_shared<pool<T>*>(this)),
          last_purge(get_now()),
          fac(_fac)
    {
        if (cfg.min_size) assure(*cfg.min_size);
    }

    template <typename T>
    typename pool<T>::resource pool<T>::use(const option<unsigned long>& ms)
    {
        return resource(selfpt, acquire(ms));
    }

    template <typename T>
    std::shared_ptr<T> pool<T>::acquire(const option<unsigned long>& ms)
    {
        may_purge();
        {
            umutex::scoped sl(this->m);
            const auto&& pt = this->q.empty()
                              && cfg.max_size
                              && static_cast<size_t>(this->q.size()) < *(cfg.max_size)
                                ? fac->create()
                                : std::get<1>(this->pop(ms));
            fac->on_acquire(*pt);
            return pt;
        }
    }

    template <typename T>
    void pool<T>::release(const std::shared_ptr<T>& pt)
    {
        {
            umutex::scoped sl(this->m);
            fac->on_release(*pt);
            enqueue(pt);
        }
        may_purge();
    }

    template <typename T>
    void pool<T>::release(resource& res)
    {
        release(res.pt);
    }

    template <typename T>
    void pool<T>::assure(size_t n)
    {
        {
            umutex::scoped sl(this->m);
            const timestamp_type now = get_now();
            if (cfg.max_size) n = upcrop(n, *cfg.max_size);
            for (auto it = this->q.begin();
                 it != this->q.begin() + upcrop(n, static_cast<size_t>(this->q.size()));
                 ++it)
            {
                std::get<0>(*it) = now;
            }

            int more = static_cast<int>(n) - this->q.size();
            while (more-- > 0)
            {
                enqueue(fac->create());
            }
        }
        may_purge();
    } 

    template <typename T>
    void pool<T>::may_purge()
    {
        umutex::scoped sl(this->m);
        if (cfg.purge_interval && last_purge + *cfg.purge_interval < get_now())
        {
            __purge();
            last_purge = get_now();
        }
    }

    template <typename T>
    void pool<T>::purge()
    {
        umutex::scoped sl(this->m);
        __purge();
    }

    template <typename T>
    void pool<T>::__purge()
    {
        umutex::scoped sl(this->m);
        if (cfg.expiration_timeout)
        {
            const timestamp_type now = get_now();
            QMutableListIterator<queued_type> it(this->q);
            while (it.hasNext())
            {
                queued_type& t = it.next();
                if (now - std::get<0>(t) > *cfg.expiration_timeout)
                {
                    fac->dispose(*std::get<1>(t));
                    it.remove();
                }
            }
        }
    }

    template <typename T>
    void pool<T>::resource::dispose() throw ()
    {
        if (*poolpt != NULL) (*poolpt)->release(*this);
        else fallback_logger.critical("resource cannot be released because source pool doesn't exists any more");
    }

}

#endif // PRELUDE_SYNC_H
