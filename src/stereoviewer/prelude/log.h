#ifndef PRELUDE_LOG_H
#define PRELUDE_LOG_H

#include "prelude/stdafx.h"
#include "prelude/prelude_config.h"
#include "prelude/misc.h"

namespace prelude
{

    //
    //
    // logger interface

    class logger
    {
    public:
        virtual void msg(const QString& fmt, ...);
        virtual void critical(const QString& fmt, ...);
        virtual void debug(const QString& fmt, ...);
        virtual void warn(const QString& fmt, ...);
        virtual void fatal_error(const QString& fmt, ...);
        virtual void unexpected_error(const QString& fmt, ...);

        virtual void vamsg(const QString& fmt, va_list args) = 0;
        virtual void vacritical(const QString& fmt, va_list args) = 0;
        virtual void vadebug(const QString& fmt, va_list args) = 0;
        virtual void vawarn(const QString& fmt, va_list args) = 0;
        virtual void vafatal_error(const QString& fmt, va_list args) = 0;
        virtual void vaunexpected_error(const QString& fmt, va_list args) = 0;

        class stream : public std::ostringstream
        {
            friend class logger;

        protected:
            typedef void(logger::*prompt_type)(const QString& fmt, ...);
            logger* parent;
            prompt_type m;

            explicit stream(logger* _parent, const prompt_type& _m)
                    : parent(_parent), m(_m) {}

        public:
            stream() : std::ostringstream(), parent(NULL) {}

            stream(const stream& that)
                : std::ostringstream(), parent(that.parent), m(that.m)
            {
                operator<<(that.str());
            }

            ~stream() { if (parent != NULL) (parent->*m)("%s", str().c_str()); }

            stream& operator=(const stream& that)
            {
                parent = that.parent;
                m = that.m;
                clear();
                operator<<(that.str());
                return *this;
            }

            stream& operator<<(const char* s) { std::operator<<(*this, s); return *this; }
            stream& operator<<(const std::string& s) { std::operator<<(*this, s); return *this; }
            stream& operator<<(const QString& s) { return operator<<(s.toStdString()); }

            // quickly stub all other member operator<<() definitions
            template <typename T>
            stream& operator<<(const T& x) { std::ostringstream::operator<<(x); return *this; }
        };

        stream msg() { return stream(this, &logger::msg); }
        stream critical() { return stream(this, &logger::critical); }
        stream warn() { return stream(this, &logger::warn); }
        stream debug() { return stream(this, &logger::debug); }
        stream fatal_error() { return stream(this, &logger::fatal_error); }
        stream unexpected_error() { return stream(this, &logger::unexpected_error); }
    };

    // pre-declaration
    namespace log
    {
        class formatter;
        class printer;
    }

    class text_logger : public logger
    {
    protected:
        const std::shared_ptr<log::formatter> f;
        const std::shared_ptr<log::printer> p;

    public:
        text_logger(const std::shared_ptr<log::formatter>& _f, const std::shared_ptr<log::printer>& _p) : f(_f), p(_p) {}

        virtual void alert(const QString& header, const QString& fmt, ...);
        virtual void vaalert(const QString& header, const QString& fmt, va_list args);
        virtual void vamsg(const QString& fmt, va_list args);
        virtual void vacritical(const QString& fmt, va_list args);
        virtual void vadebug(const QString& fmt, va_list args);
        virtual void vawarn(const QString& fmt, va_list args);
        virtual void vafatal_error(const QString& fmt, va_list args);
        virtual void vaunexpected_error(const QString& fmt, va_list args);
    };

    //
    // dummy null logger

    class null_logger : public logger
    {
    public:
        virtual void vamsg(const QString& , va_list ) {}
        virtual void vacritical(const QString& , va_list ) {}
        virtual void vadebug(const QString& , va_list ) {}
        virtual void vawarn(const QString& , va_list ) {}
        virtual void vafatal_error(const QString& , va_list ) {}
        virtual void vaunexpected_error(const QString& , va_list ) {}
    };

    //
    // compose two loggers

    class composed_logger : public logger
    {
    protected:
         const std::shared_ptr<logger> l1, l2;

    public:
        composed_logger(const std::shared_ptr<logger>& _logger1, const std::shared_ptr<logger>& _logger2)
            : l1(_logger1), l2(_logger2) {}

        composed_logger(const composed_logger&) = delete;
        composed_logger& operator=(const composed_logger&) = delete;

        const std::shared_ptr<logger>& first() const { return l1; }
        const std::shared_ptr<logger>& second() const { return l2; }

        #define VAFUN(name) \
            virtual void va##name(const QString& fmt, va_list args) \
            { \
                first()->va##name(fmt, args); \
                second()->va##name(fmt, args); \
            }

        VAFUN(msg)
        VAFUN(debug)
        VAFUN(warn)
        VAFUN(critical)
        VAFUN(fatal_error)
        VAFUN(unexpected_error)
        #undef VAFUN
    };

    namespace log
    {

        //
        //
        // formatters

        class formatter
        {
        public:
            virtual QString format_line(const QString& s) = 0;
        };

        //
        // basic identity formatter

        class basic_formatter : public formatter
        {
        public:
            virtual QString format_line(const QString& s)
            {
                return s;
            }
        };

        //
        // datetime formatter

        template <typename Formatter>
        class datetime_formatter : public Formatter
        {
        public:
            virtual QString format_line(const QString& s)
            {
                const auto&& now = QDateTime::currentDateTime();
                return nfmt<2>("[%1] %2") (now.toString("ddd dd/MM/yyyy hh:mm:ss")) (Formatter::format_line(s));
            }
        };

        //
        //
        // printers

        class printer
        {
        public:
            virtual void print_line(const QString& s) = 0;
        };

        //
        // base class for creating custom stubbing printers

        template <typename O>
        class stubbing_printer : public printer
        {
        protected:
            O object;
            explicit stubbing_printer(O o) : object(o) {}
        };

        //
        // C++ stdlib ostream printer

        class ostream_printer : public stubbing_printer<std::ostream&>
        {
        public:
            virtual void print_line(const QString& s)
            {
                this->object << s.toStdString() << std::endl << std::flush;
            }

            explicit ostream_printer(std::ostream& os) : stubbing_printer<std::ostream&>(os) {}

            virtual ~ostream_printer() { this->object.flush(); }
        };

        //
        // Qt TextStream printer

        class qstream_printer : public stubbing_printer< const std::shared_ptr<QTextStream> >
        {
        public:
            virtual void print_line(const QString& s)
            {
                *(this->object) << s << endl;
            }

            explicit qstream_printer(const std::shared_ptr<QTextStream>& qts)
                : stubbing_printer< const std::shared_ptr<QTextStream> >(qts) {}

            virtual ~qstream_printer() { this->object->flush(); }
        };

        //
        // Qt IODevice printer

        class qdevice_printer : public qstream_printer
        {
        protected:
            const std::shared_ptr<QIODevice> dev;
            bool must_be_closed;

        public:
            explicit qdevice_printer(const std::shared_ptr<QIODevice>& _dev);
            virtual ~qdevice_printer() { if (dev->isOpen() && must_be_closed) dev->close(); }
        };

        //
        // Qt File printer

        class qfile_printer : public qdevice_printer
        {
        public:
            explicit qfile_printer(const QString& name)
                : qdevice_printer(std::make_shared<QFile>(name)) {}
        };

        //
        // Qt Debug console printer

        class qdebug_printer : public printer
        {
        public:
            virtual void print_line(const QString& s) { qDebug("%s", s.toStdString().c_str()); }
        };

    }   // namespace log

    //
    //
    //
    // library-wide static logger object

    extern text_logger fallback_logger;

}

#endif // PRELUDE_LOG_H
