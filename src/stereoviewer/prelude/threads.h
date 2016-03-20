#ifndef PRELUDE_THREADS_H
#define PRELUDE_THREADS_H

#include "prelude/misc.h"
#include "prelude/sync.h"
#include "prelude/log.h"
#include "prelude/traits.h"

namespace prelude
{

    //
    //
    // thread descriptor

    class thread_descriptor
    {
    public:
        typedef QThread::Priority priority_type;
        typedef unsigned int id_type;
        typedef Qt::HANDLE sysid_type;

    protected:
        struct db_type : public QHash<sysid_type, thread_descriptor>
        {
            db_type();
        };

        friend class db_type;

        static sync<db_type> db;

        static id_type generate_logic_id()
        {
            static unsigned int __idcnt = 1;
            return __idcnt++;
        }

        QString _name;
        QThread* qth;
        id_type _id;

    public:
        thread_descriptor(QThread* const _qth, const QString& name);

        static thread_descriptor get_self();

        static thread_descriptor register_named(const sysid_type& sysid, QThread* const qth, const QString& name);
        static thread_descriptor register_unknown(const sysid_type& sysid, QThread* const qth) ;
        static void register_by_sysid(const sysid_type& sysid, const thread_descriptor& td);
        static void unregister_by_sysid(const sysid_type& sysid);
        static option<thread_descriptor> get_registered_by_sysid(const sysid_type& sysid);

        const QString& name() const { return _name; }

        const id_type& id() const { return _id; }

        VALUE_PROPERTY(priority, thread_descriptor, priority_type,
        {
            return qth->priority();
        },
        {
            qth->setPriority(value);
        })

        QString pretty() const { return nfmt<2>("[%1::%2]") (_name) (_id); }

        operator QThread*() const { return qth; }
    };

    //
    // pre-declarations

    template <typename R> class joiner;

    namespace details_thread
    {
        class spooler_qthread;
        template <typename G> class joiner_callback;
    }

    //
    //
    // untyped thread class

    class jthread : protected refcnt
	{
        friend class details_thread::spooler_qthread;

    public:
        enum class finalization_policy { terminate, join };

    protected:
        // internal member data
        details_thread::spooler_qthread* const spoolerqth;
        logger* const l;
        sync<finalization_policy> _at_finalization;

        virtual void dispose() throw ();

    public:
        explicit jthread(logger* const l = &fallback_logger);
        explicit jthread(finalization_policy dpol, logger* const l = &fallback_logger);
        explicit jthread(const QString& name, logger* const _l = &fallback_logger);
        explicit jthread(const QString& name, finalization_policy dpol, logger* const _l = &fallback_logger);

        jthread() = delete;
        jthread& operator=(const jthread& jth) = delete;

        jthread(const jthread& jth)
            : refcnt(jth),
              spoolerqth(jth.spoolerqth),
              l(jth.l),
              _at_finalization(jth._at_finalization.copy())
        {}

        ~jthread() { unref(); }

        static void sleep(unsigned long ms);

        VALUE_PROPERTY(at_finalization, jthread, finalization_policy,
        {
            return _at_finalization.copy();
        },
        {
            _at_finalization.set(value);
        })

        bool is_idle() const;

        const thread_descriptor& descriptor() const;

        void detach(const std::function<void(jthread)>& f);

        //
        // delegate member function family

        template <typename F, typename... Args>
        joiner<typename std::result_of<F(Args...)>::type> delegate(const F& f, Args&&... args);

        template <typename R, typename... Args, typename... Actuals>
        joiner<R> delegate(const std::function<R(Args...)>& f, Actuals&&... args);

        template <typename R, typename... Args, typename... Actuals>
        joiner<R> delegate(R(* const f)(Args...), Actuals&&... args);

        template <typename R, typename C, typename... Args, typename... Actuals>
        joiner<R> delegate(R(C::* const f)(Args...), C* const that, Actuals&&... args);

        //
        // exec member function family

        template <typename F, typename... Args>
        joiner<typename std::result_of<F(Args...)>::type> exec(const F& f, Args&&... args);

        template <typename F, typename... Args>
        option<joiner<typename std::result_of<F(Args...)>::type>> try_exec(const F& f, Args&&... args);        
	};

    //
    // return value encapsulator and joiner

    template <typename R>
    class joiner
    {
        friend class jthread;
        template <typename X> friend class details_thread::joiner_callback;

    protected:
        std::shared_ptr<gatherer<R>> gath;
        thread_descriptor descr;

        explicit joiner(const thread_descriptor& _descr)
            : gath(std::make_shared<gatherer<R>>()), descr(_descr) {}

    public:
        typedef typename gatherer<R>::result_type result_type;

        joiner(const joiner<R>&) = default;

        joiner(joiner<R>&& j) : gath(std::move(j.gath)), descr(std::move(j.descr)) {}

        joiner<R>& operator=(const joiner<R>&) = default;

        joiner<R>& operator=(joiner<R>&& j)
        {
            std::shared_ptr< gatherer<R> >::operator=(std::move(j.gath));
            thread_descriptor::operator=(std::move(j.descr));
            return *this;
        }

        result_type join(const option<unsigned int>& ms = none) const
        {
            return gath->wait(ms);
        }

        const thread_descriptor& source() const { return descr; }
        thread_descriptor& source() { return descr; }

    };

    namespace details_thread
    {
        class exec_callback
        {
        public:
            virtual void exec() = 0;
        };

        //
        // QThread subclass implementing an ever-looping run method

        class spooler_qthread : public QThread
        {
            friend class prelude::jthread;

        protected:            
            struct command
            {
                enum class ty { quit, exec };

                ty type;
                std::shared_ptr<exec_callback> callback;
                QThread* qth;

                QString pretty() const
                {
                    switch (type)
                    {
                        case ty::quit: return "quit";
                        case ty::exec: return "exec";
                        //case ty::quit_as_detached: return "QuitAsDetached";
                        default: std::unexpected();
                    }
                }

                //
                // command constructors

                static command quit()
                {
                    command r;
                    r.type = ty::quit;
                    return r;
                }

                static command exec(const std::shared_ptr<exec_callback>& cb)
                {
                    command r;
                    r.type = ty::exec;
                    r.callback = cb;
                    return r;
                }

                /*static command quit_as_detached(QThread* qth)
                {
                    command r;
                    r.type = ty::quit_as_detached;
                    r.qth = qth;
                    return r;
                }*/
            };

            //
            // internal member data

            logger* const l;
            sync_queue<command> cmds;
            sync_flag idle;
            thread_descriptor descr;

            //
            // constructors

            spooler_qthread() = delete;
            spooler_qthread(const spooler_qthread&) = delete;
            spooler_qthread& operator=(const spooler_qthread&) = delete;

            spooler_qthread(logger* const _l, const QString& name)
                : l(_l), idle(true), descr(this, name)
            {
                setTerminationEnabled(true);
                start();
            }

            void push_cmd(const command& cmd) { cmds.push(cmd); }

            bool is_idle() const
            {
                return idle && cmds.empty();
            }

            virtual void run();

        public:
            static void _sleep(unsigned long ms) { msleep(ms); }            
        };

        template <typename R>
        class joiner_callback : public exec_callback
        {
        private:
            joiner<R> j;
            const std::function<R()> f;

        public:
            virtual void exec() { j.gath->broadcast(f()); }

            joiner_callback(const joiner<R>& _j, const std::function<R()>& _f)
                : j(_j), f(_f)
            {}
        };

        template <>
        class joiner_callback<void> : public exec_callback
        {
        private:
            joiner<void> j;
            const std::function<void()> f;

        public:
            virtual void exec()
            {
                f();
                j.gath->broadcast();
            }

            joiner_callback(const joiner<void>& _j, const std::function<void()>& _f)
                : j(_j), f(_f)
            {}
        };
    }

    template <typename R, typename... Args, typename... Actuals>
    joiner<R> jthread::delegate(const std::function<R(Args...)>& f, Actuals&&... args)
    {
        using namespace details_thread;

        typedef R result_type;

        const joiner<result_type> j(descriptor());
        spoolerqth->push_cmd(
                spooler_qthread::command::exec(
                        std::make_shared<joiner_callback<result_type> >(j, std::bind(f, std::forward<Actuals>(args)...))));
        return j;
    }

    template <typename F, typename... Args>
    joiner<typename std::result_of<F(Args...)>::type>
      jthread::delegate(const F& f, Args&&... args)
    {
        typedef typename std::result_of<F(Args...)>::type result_type;

        return delegate(std::function<result_type(Args...)>(f), std::forward<Args>(args)...);
    }

    template <typename R, typename... Args, typename... Actuals>
    joiner<R> jthread::delegate(R(* const f)(Args...), Actuals&&... args)
    {
        return delegate(std::function<R(Args...)>(f), std::forward<Actuals>(args)...);
    }

    template <typename R, typename C, typename... Args, typename... Actuals>
    joiner<R> jthread::delegate(R(C::* const f)(Args...), C* const that, Actuals&&... args)
    {
        return delegate(std::function<R(C*, Args...)>(f), that, std::forward<Actuals>(args)...);
    }

    template <typename F, typename... Args>
    joiner<typename std::result_of<F(Args...)>::type>
      jthread::exec(const F& f, Args&&... args)
    {
        if (spoolerqth->is_idle()) return delegate(f, std::forward<Args>(args)...);
        else throw localized_runtime_error(HERE(nfmt<1>("thread %1 is currently busy on another task") (descriptor().pretty())));
    }

    template <typename F, typename... Args>
    option<joiner<typename std::result_of<F(Args...)>::type>>
      jthread::try_exec(const F& f, Args&&... args)
    {
        return trap(exec<F, Args...>)(f, std::forward<Args>(args)...);
    }

    //
    //
    // jthread log formatter

    namespace log
    {

        template <typename Formatter>
        class thread_formatter : public Formatter
        {
        public:
            virtual QString format_line(const QString& s)
            {
                const auto&& self = thread_descriptor::get_self();
                return nfmt<2>("%1 %2") (self.pretty()) (Formatter::format_line(s));
            }
        };

    }

    //
    //
    // concurrent function wrapper

    template <typename R>
    class jvalue : protected joiner<R>
    {
    private:
        typedef joiner<R> super;
        typedef jvalue<R> self;

    public:
        typedef typename super::result_type result_type;

        jvalue(const joiner<R>& _j) : super(_j) {}

        jvalue(self&& a) : super(std::move(a)) {}

        jvalue(const self&) = default;
        self& operator=(const self&) = delete;

        result_type operator*() const { return super::join(); }
        operator result_type() const { return operator*(); }
    };

    template <typename F>
    class concurrent_function;

    template <typename R, typename... Args>
    class concurrent_function<R(Args...)>
    {
    private:
        std::function<joiner<R>(Args...)> f;

    public:
        typedef jvalue<R> result_type;

        template <typename X>
        explicit concurrent_function(const X& _f) : f(_f) {}

        result_type operator()(Args&&... args) const
        {
            return jvalue<R>(f(std::forward<Args>(args)...));
        }
    };

    class pooled_concurrence : protected pool<jthread>::factory
    {
    protected:
        typedef pool<jthread> pool_type;

        const QString name;
        const std::shared_ptr<pool_type> thpool;
        logger* const l;

        template <typename F>
        struct concurrent_fun_aux
        {
            friend class pooled_concurrence;

            template <typename... Args>
            struct result
            {
                typedef joiner<typename std::result_of<F(Args...)>::type> type;
            };

        private:
            F f;
            const std::shared_ptr<pool_type> thpool;

            template <typename... Args>
            typename std::result_of<F(Args...)>::type
              exec_fun(pool_type::resource /*res*/, Args&&... args) const
            {
                // argument 'res' needs to be passed by value for triggering thread self-release
                return f(std::forward<Args>(args)...);
            }

            concurrent_fun_aux(const F& _f, const std::shared_ptr<pool_type>& _thpool)
                : f(_f), thpool(_thpool) {}

        public:
            template <typename... Args>
            joiner<typename std::result_of<F(Args...)>::type> operator()(Args&&... args) const
            {
                const auto&& res = thpool->use();
                // using 'delegate' rather than 'exec' avoids exceptions due to the
                // delay possibly introduced by a thread while releasing itself
                return res->delegate(&concurrent_fun_aux::exec_fun<Args...>, this, res, std::forward<Args>(args)...);
            }
        };

        template <typename F>
        concurrent_fun_aux<F> make_concurrent_fun_aux(const F& f)
        {
            return concurrent_fun_aux<F>(f, thpool);
        }

        virtual std::shared_ptr<jthread> create()
        {
            return std::make_shared<jthread>(name, l);
        }

    public:
        typedef pool_type::configuration configuration;

        pooled_concurrence(const QString& _name, const configuration& cfg, logger* _l = &fallback_logger)
            : name(_name),
              thpool(std::make_shared<pool_type>(cfg, static_cast<pool_type::factory*>(this))),
              l(_l)
        {}

        template <typename R, typename... Args>
        concurrent_function<R(Args...)> make(const std::function<R(Args...)>& f)
        {
            return concurrent_function<R(Args...)>(make_concurrent_fun_aux(f));
        }

        template <typename F, typename X>
        concurrent_function<F> make(const X& f)
        {
            return concurrent_function<F>(make_concurrent_fun_aux(f));
        }

        template <typename R, typename... Args>
        concurrent_function<R(Args...)> make(R(*f)(Args...))
        {
            return concurrent_function<R(Args...)>(make_concurrent_fun_aux(f));
        }
    };

}

#endif // PRELUDE_THREADS_H
