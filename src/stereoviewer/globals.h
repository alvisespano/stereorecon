#ifndef GLOBALS_H
#define GLOBALS_H

#include "prelude.h"

namespace globals
{

    //
    // logger stuff

    using namespace prelude;

    typedef log::datetime_formatter<log::thread_formatter<log::basic_formatter>> fancy_log_formatter;
    typedef log::basic_formatter compact_log_formatter;

    extern const std::shared_ptr<logger> default_logger;
    extern std::shared_ptr<logger> logger;

}

#endif // GLOBALS_H
