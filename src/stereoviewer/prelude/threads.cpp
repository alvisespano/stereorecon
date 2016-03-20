#include "prelude/stdafx.h"
#include "prelude/threads.h"
#include "prelude/log.h"

namespace prelude
{
    //
    //
    // thread descriptor

    sync<thread_descriptor::db_type> thread_descriptor::db;

    thread_descriptor::db_type::db_type()
    {
        auto td = thread_descriptor::register_named(QThread::currentThreadId(), QThread::currentThread(), "main");
    }

    thread_descriptor::thread_descriptor(QThread* const _qth, const QString& name)
        : _name(name), qth(_qth), _id(generate_logic_id())
    {}

    thread_descriptor thread_descriptor::get_self()
    {
        const auto&& sysid = QThread::currentThreadId();
        const auto&& tdo = get_registered_by_sysid(sysid);
        return tdo ? *tdo : thread_descriptor::register_unknown(sysid, QThread::currentThread());
    }

    thread_descriptor thread_descriptor::register_named(const sysid_type& sysid, QThread* const qth, const QString& name)
    {
        const auto&& o = get_registered_by_sysid(sysid);
        if (o) return *o;
        else
        {
            thread_descriptor r(qth, name);
            register_by_sysid(sysid, r);
            return r;
        }
    }

    thread_descriptor thread_descriptor::register_unknown(const sysid_type& sysid, QThread* const qth)
    {
        return register_named(sysid, qth, nfmt<1>("unknown#%1") (reinterpret_cast<unsigned long>(sysid)));
    }

    void thread_descriptor::register_by_sysid(const sysid_type& sysid, const thread_descriptor& td)
    {
        db.atomic()->insert(sysid, td);
    }

    void thread_descriptor::unregister_by_sysid(const sysid_type& sysid)
    {
        db.atomic()->remove(sysid);
    }

    option<thread_descriptor> thread_descriptor::get_registered_by_sysid(const sysid_type& sysid)
    {
        sync<db_type>::ro_scoped sl(db);
        db_type::const_iterator i = sl->find(sysid);
        return i == sl->end() ? none : some(*i);
    }

    //
    //
    // jthread

    using prelude::logger;

    #define CONS(name, pol) \
        spoolerqth(new details_thread::spooler_qthread(_l, name)), \
        l(_l), \
        _at_finalization(pol)

    jthread::jthread(finalization_policy pol, logger* _l)
        : CONS(config::defaults::jthread_name, pol)
    {}

    jthread::jthread(const QString& name, finalization_policy pol, logger* _l)
        : CONS(name, pol)
    {}

    jthread::jthread(logger* _l)
        : CONS(config::defaults::jthread_name, finalization_policy::join)
    {}

    jthread::jthread(const QString& name, logger* _l)
        : CONS(name, finalization_policy::join)
    {}

    void jthread::sleep(unsigned long ms)
    {
        details_thread::spooler_qthread::_sleep(ms);
    }

    bool jthread::is_idle() const { return spoolerqth->is_idle(); }

    const thread_descriptor& jthread::descriptor() const { return spoolerqth->descr; }

    void jthread::dispose() throw ()
    {
        if (spoolerqth == QThread::currentThread())
        {
            l->unexpected_error("self thread expunge detected: quitting now but QThread object will leak");
            spoolerqth->push_cmd(details_thread::spooler_qthread::command::quit());
        }
        else
        {
            if (!is_idle())
            {
                switch (_at_finalization.copy())
                {
                    case finalization_policy::terminate:
                        l->debug(nfmt<1>("thread %1 is still running and will be terminated") (descriptor().pretty()));
                        spoolerqth->terminate();
                        goto wait_and_delete;

                    case finalization_policy::join:
                        l->debug(nfmt<1>("thread %1 is still running and will quit as it finishes its current task. Waiting...") (descriptor().pretty()));
                        break;
                }
            }
            spoolerqth->push_cmd(details_thread::spooler_qthread::command::quit());

        wait_and_delete:
            spoolerqth->wait();
            delete spoolerqth;
        }
    }

    void jthread::detach(const std::function<void(jthread)>& f)
    {
        delegate(f, *this);
    }

    void details_thread::spooler_qthread::run()
    {
        const auto&& sysid = QThread::currentThreadId();

        // register self
        thread_descriptor::register_by_sysid(sysid, descr);

        // execution loop
        bool quit = false;
        while (!quit)
        {
            l->debug("idle.");
            idle.set(true);
            const auto&& cmd = cmds.pop();
            idle.set(false);

            //l->debug(nfmt<1>("command spooled: '%1'") (cmd.pretty()));;

            switch (cmd.type)
            {
                case command::ty::quit:
                    quit = true;
                    break;

                case command::ty::exec:
                    try
                    {
                        cmd.callback->exec();
                    }
                    catch(std::exception& e)
                    {
                        l->warn(nfmt<1>("exception trapped: %1") (e.what()));
                    }
                    catch(...)
                    {
                        l->warn("unknown exception trapped");
                    }
                    break;
            }
        }
        l->debug("goodbye.");

        // unregister self
        thread_descriptor::unregister_by_sysid(sysid);
    }

}
