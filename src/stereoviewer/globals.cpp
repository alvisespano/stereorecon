#include "globals.h"
#include "config.h"

namespace globals
{

    // loggers

    using namespace prelude::log;

    const std::shared_ptr<prelude::logger> file_logger =
        std::make_shared<text_logger>(std::make_shared<fancy_log_formatter>(),
        std::make_shared<atomic_printer>(std::make_shared<qfile_printer>(::config::log_filename)));

    const std::shared_ptr<prelude::logger> trace_logger =
        std::make_shared<text_logger>(
            std::make_shared<fancy_log_formatter>(),
            std::make_shared<atomic_printer>(std::make_shared<qdebug_printer>()));

    const std::shared_ptr<prelude::logger> default_logger =
        #ifdef QT_DEBUG
        std::make_shared<composed_logger>(
            file_logger,
            trace_logger)
        #else
        trace_logger
        #endif
        ;

    std::shared_ptr<prelude::logger> logger = default_logger;

}
