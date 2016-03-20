#include "prelude/stdafx.h"
#include "prelude/prelude_config.h"
#include "prelude/misc.h"
#include "prelude/io.h"

namespace prelude
{
    QTextStream& ws_or_endl(QTextStream& s)
    {
        while (!s.atEnd())
        {
            s.skipWhiteSpace();
            qint64 pos = s.pos();
            char c;
            s >> c;
            switch (c)
            {
                case '\n':
                case '\r': continue;
                default:   s.seek(pos); return s;
            }
        }
        return s;
    }

    static QString pretty_process_error(const QProcess& p)
    {
        return p.errorString().toLower();
    }

    static void trim_eol(char* const& line, const long len)
    {
        char* c = &line[len - 1];
        if (*c == '\n')
        {
            *c = '\0';
            if (*--c == '\r') *c = '\0';
        }
    }

    int run_cmd(logger& l, run_config cfg)
    {
        cfg.prepend_env("PATH", config::cygwin_paths);

        QProcess child;
        child.setWorkingDirectory(cfg.wdir);
        if (cfg.stdin_path) child.setStandardInputFile(*cfg.stdin_path);
        if (cfg.stdout_path) child.setStandardOutputFile(*cfg.stdout_path);
        else
        {
            child.setReadChannelMode(QProcess::MergedChannels);
            child.setReadChannel(QProcess::StandardOutput);
        }
        child.setEnvironment(cfg.env);

        l.msg(nfmt<3>("launching command from directory '%1': \"%2\" <%3>")
            (cfg.wdir)
            (cfg.cmd())
            (cfg.args.join("> <"))
            /*(cfg.env.filter(QRegExp("^PATH=(.*)")).first())*/);
        child.start(cfg.cmd(), cfg.args);
        if (!child.waitForStarted(cfg.start_timeout))
            throw localized_invalid_argument(HERE(nfmt<2>("cannot launch cmd '%1': %2") (cfg.cmd()) (pretty_process_error(child))));

        if (cfg.stdout_path)
        {
            l.msg(nfmt<3>("cmd '%1' stdout has been redirected to '%2'. Waiting for process to finish (timeout = %3 ms)...") (cfg.cmd()) (*cfg.stdout_path) (cfg.exec_timeout));
            child.waitForFinished(cfg.exec_timeout);
        }
        else
        {
            static const qint64 maxlen = 256;
            char line[maxlen];
            l.msg(nfmt<1>("cmd '%1' stdout and stderr follows:") (cfg.cmd()));

            while (child.waitForReadyRead(cfg.exec_timeout))
            {
                while (child.canReadLine())
                {
                    const qint64 len = child.readLine(line, maxlen);
                    if (len <= 0) break;
                    trim_eol(line, len);
                    l.msg(nfmt<3>("(%1) $ %2%3") (cfg.prompt) (line) (len >= maxlen ? " [...]" : ""));
                }
            }
        }

        switch (child.state())
        {
            case QProcess::NotRunning:
            {
                const int ec = child.exitCode();
                switch (child.exitStatus())
                {
                    case QProcess::NormalExit:  return ec;
                    case QProcess::CrashExit:   throw localized_runtime_error(HERE(nfmt<2>("cmd '%1' crashed: %2") (cfg.cmd()) (pretty_process_error(child))));
                }
            }
            default:
            {
                l.warn(nfmt<3>("cmd '%1' is still running but timeout has expired or some error occured: %2. Delaying process termination (%3 ms)...") (cfg.cmd()) (pretty_process_error(child)) (cfg.term_timeout));
                if (!child.waitForFinished(cfg.term_timeout))
                {
                    l.warn(nfmt<2>("cmd '%1' doesn't seem to finish: forcing termination (timeout = %2 ms)...") (cfg.cmd()) (cfg.kill_timeout));
                    child.terminate();
                    if (!child.waitForFinished(cfg.kill_timeout))
                    {
                        l.warn(nfmt<1>("cmd '%1' doesn't respond to termination signal: forcing kill...") (cfg.cmd()));
                        child.kill();
                    }
                }
                throw localized_runtime_error(HERE(nfmt<2>("cmd '%1' execution failed: %2") (cfg.cmd()) (pretty_process_error(child))));
            }
        }
    }

    int run_sh(logger& l, run_config cfg)
    {
        const QString old_prompt = cfg.prompt;
        cfg.args.insert(0, cfg.cmd());
        cfg.cmd() = config::bash_filename;
        cfg.prompt = old_prompt;
        return run_cmd(l, cfg);
    }

    void temp_dir::init(const QString& prefix)
    {
        QString sub;
        do sub = nfmt<2>("%1_%2") (prefix) (qrand());
        while (exists(sub));
        if (!mkdir(sub))
            throw localized_runtime_error(__func__, nfmt<2>("cannot create temporary directory %1/%2") (absolutePath()) (sub));
        cd(sub);
        original_path = some(absolutePath());
    }

    void temp_dir::dispose() throw ()
    {
        if (original_path)
        {
            l->debug(nfmt<1>("temporary directory '%1' shall be removed manually") (*original_path));
            /*cd(original);
            l->debug(nfmt<1>("removing temporary directory '%1'...") (absolutePath()));
            foreach (const auto& s, entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs, QDir::NoSort))
            {
                if (!remove(s)) l->warn(nfmt<2>("cannot remove file '%1' in temporary directory '%2'") (s) (absolutePath()));
            }
            const auto&& old = absolutePath();
            cdUp();
            if (!rmdir(old)) l->warn(nfmt<1>("cannot remove temporary directory '%1'") (old));*/
        }
    }

}
