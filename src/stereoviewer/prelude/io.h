#ifndef PRELUDE_IO_H
#define PRELUDE_IO_H

#include "prelude/stdafx.h"
#include "prelude/log.h"
#include "prelude/maths.h"

namespace prelude
{

    //
    //
    // misc stuff

    inline QTextStream& uendl(QTextStream& s) { return s << '\n'; }
    QTextStream& ws_or_endl(QTextStream& s);

    //
    //
    // run executables and shell scripts

    class run_config
    {
    protected:
        QString _cmd;

    public:
        QString wdir, prompt;
        QStringList args, env;
        option<QString> stdout_path, stdin_path;
        int start_timeout, exec_timeout, term_timeout, kill_timeout;

        run_config() : wdir(QDir::currentPath()), env(QProcess::systemEnvironment()),
                       start_timeout(10000), exec_timeout(-1), term_timeout(3000), kill_timeout(3000) {}

        REF_PROPERTY(cmd, run_config, QString,
        {
            return _cmd;
        },
        {
            _cmd = value;
            prompt = QFileInfo(value).fileName();
        })

        void prepend_env(const QString& var, const QString& value)
        {
            env.replaceInStrings(QRegExp(nfmt<1>("^%1=(.*)")(var), Qt::CaseInsensitive), nfmt<2>("%1=%2;\\1")(var)(value));
        }
    };

    int run_cmd(logger& l, run_config cfg);
    int run_sh(logger& l, run_config cfg);

    //
    //
    // temporary directories

    class temp_dir : protected refcnt, public QDir
    {
    private:
        void init(const QString& prefix);

    protected:
        option<QString> original_path;
        logger* l;

        virtual void dispose() throw ();

    public:
        temp_dir() : l(&fallback_logger) {}
        explicit temp_dir(const QString& prefix, logger* const _l = &fallback_logger)
            : QDir(QDir::temp()), l(_l)
        {
            init(prefix);
        }

        ~temp_dir() { unref(); }
    };

    //
    //
    // progress tracking

    //
    // adapter to physical device interface & base class

    class progress_adapter
    {   
    public:
        progress_adapter(const progress_adapter&) = delete;
        progress_adapter& operator=(const progress_adapter&) = delete;

        progress_adapter() = default;
        virtual ~progress_adapter() {}
        virtual void start() = 0;
        virtual void finish() = 0;
        virtual void update(double value) = 0;
        virtual double resolution() const = 0;
    };

    class progress_adapter_base : public progress_adapter
    {
    protected:
        double last_value;
        virtual void set(double value) = 0;

    public:
        progress_adapter_base() : last_value(std::numeric_limits<double>::min()) {}

        virtual double resolution() const = 0;

        virtual void update(double value)
        {
            if (!is_about(value, last_value, resolution()))
            {
                set(value);
                last_value = value;
            }
        }

        virtual void start() { update(0.0); }
        virtual void finish() { update(1.0); }
    };

    //
    // adapter composer

    class composed_progress_adapter : public progress_adapter
    {
    protected:
        const std::shared_ptr<progress_adapter> a1, a2;

    public:
        composed_progress_adapter(const std::shared_ptr<progress_adapter>& _a1, const std::shared_ptr<progress_adapter>& _a2)
            : a1(_a1), a2(_a2)
        {}

        virtual void start() { a1->start(); a2->start(); }
        virtual void finish() { a1->finish(); a2->finish(); }
        virtual double resolution() const { return std::min(a1->resolution(), a2->resolution()); }
        virtual void update(double value) { a1->update(value); a2->update(value); }
    };

    //
    // logger adapter

    class logger_adapter : public progress_adapter_base
    {
    protected:
        logger* const l;
        const QString name;
        const double res;

        virtual void set(double value) { l->msg(nfmt<2>("[%1] ------ %2%% ------") (name) (int(value * 100.0))); }

    public:
        logger_adapter(logger* const _l, const QString& _name, double _res = 0.01)
            : l(_l), name(_name), res(_res)
        {}

        virtual double resolution() const { return res; }
    };

    //
    // progress tracker & stepper

    class progress_stepper; // pre-declaration

    class progress_tracker : private refcnt
    {
        friend class progress_stepper;

    protected:
        const std::shared_ptr<progress_adapter> adapter;
        const double from, to;    

        // constructor used by progress_stepper::sub()
        progress_tracker(const progress_tracker& t, double _from, double _to)
            : refcnt(t), adapter(t.adapter), from(_from), to(_to)
        {}

        virtual void dispose() throw ()
        {
            adapter->finish();
        }

    public:
        progress_tracker() = delete;
        progress_tracker(const progress_tracker&) = default;
        progress_tracker& operator=(const progress_tracker&) = delete;

        progress_tracker(const std::shared_ptr<progress_adapter>& _adapter)
            : adapter(_adapter), from(0.0), to(1.0)
        {
            adapter->start();
        }

        ~progress_tracker() { unref(); }

        void set_at(double x)
        {            
            #ifdef QT_DEBUG
            if (!is_within(x, from, to))
            {
                fallback_logger.unexpected_error(HERE(nfmt<3>("%1 is out of range (%2, %3)") (x) (from) (to)));
                x = crop(x, from, to);
            }
            #endif
            adapter->update(x);
        }
        
        progress_stepper section(size_t steps);
    };

    class progress_stepper
    {
        friend class progress_tracker;

    protected:
        progress_tracker t;
        const size_t steps;
        const double inc;
        const int every;
        int evcnt;
        size_t i;

        typedef void(progress_stepper::*increment_type)();
        increment_type increment;

        double at(size_t n) const
        {
            return t.from + inc * double(n);
        }
                
        void skip_increment()
        {
            if (--evcnt <= 0)
            {
                t.set_at(at(i));
                evcnt = every;
            }
        }

        void fast_increment()
        {
            t.set_at(at(i));
        }

        progress_stepper(const progress_tracker& _t, size_t _steps)
            : t(_t), steps(_steps), inc((t.to - t.from) / double(steps)),
              every(round(t.adapter->resolution() / inc)), evcnt(every), i(0),
              increment(every > 1 ? &progress_stepper::skip_increment
                                  : &progress_stepper::fast_increment)
        {}

    public:
        progress_stepper(const progress_stepper&) = delete;
        progress_stepper& operator=(const progress_stepper&) = delete;

        progress_stepper(progress_stepper&& s)
            : t(std::move(s.t)),
              steps(std::move(s.steps)),
              inc(std::move(s.inc)),
              every(std::move(s.every)),
              evcnt(std::move(s.evcnt)),
              i(std::move(i))
        {}

        ~progress_stepper()
        {
            t.set_at(at(steps));
        }

        void operator++()
        {
            ++i;
            (this->*increment)();
        }

        progress_tracker sub(size_t parent_steps = 1) const
        {
            return progress_tracker(t, at(i), at(i + parent_steps));
        }
    };

    inline progress_stepper progress_tracker::section(size_t steps)
    {
        return progress_stepper(*this, steps);
    }

}

#endif // IO_H
