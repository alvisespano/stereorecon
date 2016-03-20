#include "prelude/stdafx.h"
#include "prelude/log.h"
#include "prelude/sync.h"
#include "prelude/threads.h"

namespace prelude
{

    using namespace log;

    text_logger fallback_logger(std::make_shared<datetime_formatter<thread_formatter<basic_formatter> > >(),
                                std::make_shared<atomic_printer>(std::make_shared<qdebug_printer>()));

    void logger::msg(const QString& fmt, ...)
    {
        va_list argptr;
        va_start(argptr, fmt);
        vamsg(fmt, argptr);
        va_end(argptr);
    }

    void logger::critical(const QString& fmt, ...)
    {
        va_list argptr;
        va_start(argptr, fmt);
        vacritical(fmt, argptr);
        va_end(argptr);
    }

    void logger::warn(const QString& fmt, ...)
    {
        va_list argptr;
        va_start(argptr, fmt);
        vawarn(fmt, argptr);
        va_end(argptr);
    }

    void logger::debug(const QString& fmt, ...)
    {
        va_list argptr;
        va_start(argptr, fmt);
        vadebug(fmt, argptr);
        va_end(argptr);
    }

    void logger::fatal_error(const QString& fmt, ...)
    {
        va_list argptr;
        va_start(argptr, fmt);
        vafatal_error(fmt, argptr);
        va_end(argptr);
    }

    void logger::unexpected_error(const QString& fmt, ...)
    {
        va_list argptr;
        va_start(argptr, fmt);
        vaunexpected_error(fmt, argptr);
        va_end(argptr);
    }

    void text_logger::alert(const QString& h, const QString& fmt, ...)
    {
        va_list argptr;
        va_start(argptr, fmt);
        vaalert(h, fmt, argptr);
        va_end(argptr);
    }

    void text_logger::vaalert(const QString& s, const QString& fmt, va_list args)
    {
        vamsg(nfmt<2>("[%1] %2") (s) (fmt), args);
    }

    void text_logger::vamsg(const QString& fmt, va_list args)
    {
        p->print_line(f->format_line(vqsprintf(fmt.toStdString().c_str(), args)));
    }

    void text_logger::vacritical(const QString& fmt, va_list args)
    {
        vaalert("CRITICAL", fmt, args);
    }

    void text_logger::vawarn(const QString& fmt, va_list args)
    {
        vaalert("WARNING", fmt, args);
    }

    void text_logger::vadebug(const QString& fmt, va_list args)
    {
        #ifdef QT_DEBUG
        vaalert("DEBUG", fmt, args);
        #endif
    }

    void text_logger::vafatal_error(const QString& fmt, va_list args)
    {
        vaalert("FATAL ERROR", fmt, args);
    }

    void text_logger::vaunexpected_error(const QString& fmt, va_list args)
    {
        vaalert("UNEXPECTED ERROR", fmt, args);
    }

    namespace log
    {

        qdevice_printer::qdevice_printer(const std::shared_ptr<QIODevice>& _dev)
            : qstream_printer(std::make_shared<QTextStream>(_dev.get())), dev(_dev)
        {
            if (!dev->isOpen())
            {
                if (!dev->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
                    throw localized_runtime_error(HERE(nfmt<1>("cannot open device for output: %1") (dev->errorString())));
                must_be_closed = true;
            }
            else must_be_closed = false;
        }

    }   // namespace log

}
