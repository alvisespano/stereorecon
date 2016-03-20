#include <QtGui/QApplication>
#include "mainwindow.h"
#include "prelude.h"

#define MAIN_NAMESPACE app

namespace tests
{
    using namespace prelude;

    auto& l = *globals::logger;

    namespace progress
    {
        using namespace prelude;

        void loop(const QString& basetab, int inn, progress_tracker&& t)
        {
            const size_t n = rnd(1, 10);
            //l.msg("looping %d...", n);
            auto stepper = t.section(n);
            for (size_t i = 0; i < n; ++i)
            {
                const QString tab = fmt("%1%2/%3") (basetab) (i + 1) (n);
                l.msg(fmt("%1") (tab));
                if (inn > 0 && rnd(0, inn * 3) < inn) loop(tab + " . ", inn - 1, stepper.sub());
                ++stepper;
            }
        }

        int test()
        {
            qsrand(QTime::currentTime().msec());
            loop("", 3, progress_tracker(std::make_shared<logger_adapter>(&l, "test")));
            return 0;
        }
    }

    namespace threads
    {
        using std::bind;
        using std::function;
        using namespace std::placeholders;

        int triplus(int x, int y, int z)
        {
            using namespace prelude;

            jthread::sleep(rnd(0, 1000));
            int r = x + y + z;
            l.msg("triplus: %d + %d + %d = %d", x, y, z, r);
            return r;
        }

        function<int(int, int)> plus = bind(triplus, 0, _1, _2);

        void plus_in_place(const int& a, int& r) { r += a; }

        int test_jthreads()
        {
            jthread th1("test-th", jthread::finalization_policy::join, &l),
                    th2("test-th", jthread::finalization_policy::join, &l);
            QList<joiner<int>> js;
            int r;
            function<void(const int&, int&)> __plus_in_place = &plus_in_place;
            for (int i = 0; i < 10; ++i)
            {
                auto j1 = th1.delegate(plus, i, i);
                js << j1;
                joiner<void> j2 = th2.delegate(&plus_in_place, std::cref(i), std::ref(r));
                j2.join();
                l.msg("th2 result: r = %d", r);
            }
            foreach (const joiner<int>& j, js)
            {
                const auto& d = j.source();
                l.msg(nfmt<2>("%1 result: %2") (d.pretty()) (j.join()));
            }
            l.msg("adieu");
            return 0;
        }

        class qth : public QThread
        {
            //Q_OBJECT

        public:
            sync_queue<int> syq;

            qth() : QThread(NULL) {}

        protected:
            void run()
            {
                try
                {
                    for (int i = 0; i < 20; ++i)
                    {
                        qDebug("# waiting...");
                        try
                        {
                            int n = syq.pop(some(1000));
                            qDebug("# consumed %d", n);
                        }
                        catch (...)
                        {
                            qDebug("sypop raised");
                            break;
                        }
                        msleep(100);
                    }
                    throw 1;
                }
                catch (...)
                {
                    qDebug("production seems over");
                }
                qDebug("goodbye.");
            }

        };

        int test_qthread()
        {
            qth consumer;
            consumer.start();

            for (int i = 0; i < 10; ++i)
            {
                const int n = rnd(1, 1000);
                qDebug("@ producing %d", n);
                consumer.syq.push(n);
            }
            qDebug("@ joining consumer...");
            consumer.wait();
            qDebug("@ joined");
            return 0;
        }

        joiner<int> my_concurrent_function(int x)
        {
            jthread jth(jthread::finalization_policy::join, &l);
            return jth.delegate(plus, 1, x);
        }

        int test_concurrent_function()
        {
            concurrent_function<int(int)> cf(&my_concurrent_function);
            return cf(2);
        }

        int test_pool_concurrence()
        {
            pooled_concurrence::configuration cfg;
            cfg.expiration_timeout = some(10);
            cfg.purge_interval = some(30);
            cfg.min_size = some(1);
            cfg.max_size = some(QThread::idealThreadCount());
            pooled_concurrence cc("test-concurrence-pool", cfg);

            concurrent_function<int(int, int)>      ccplus       = cc.make(plus);
            concurrent_function<int(int)>           ccplus_10    = cc.make<int(int)>(bind(plus, 10, _1));
            concurrent_function<int(int, int, int)> cctriplus    = cc.make(&triplus);

            int r = cctriplus(ccplus_10(23), 8, ccplus(ccplus_10(35), ccplus_10(11)));
            l.msg("%d", r);
            return 0;
        }
    }

    namespace refcount
    {
        struct counted : private refcnt
        {
            QString s;

            void dispose() throw () { l.msg(nfmt<1>("dispose: %1") (s)); }

            counted(const counted& c) : refcnt(c), s(c.s)
            {
                l.msg(nfmt<2>("copycons: %1#%2") (s) (count()));
            }

            explicit counted(const QString& _s) : s(_s)
            {
                l.msg(nfmt<2>("cons: %1#%2") (s) (count()));
            }

            ~counted()
            {
                l.msg(nfmt<2>("destructor: %1#%2") (s) (count()));
                unref();
                l.msg(nfmt<2>("post-destructor: %1#%2") (s) (count()));
            }

            counted& operator=(const counted& c)
            {
                l.msg(nfmt<4>("assignment: %1#%3 = %2#%4") (s) (c.s) (count()) (c.count()));
                refcnt::operator=(c);
                s = c.s;
                l.msg(nfmt<2>("post-assignment: %1#%2") (s) (count()));
                return *this;
            }
        };

        int test()
        {
            {
                counted a("a"), b("b"), c("c");
                {
                    counted a2 = a, b2 = b;
                    b2 = c;
                }
                b = a;
                c = b;
            }
            return 0;
        }
    }

    int main(int /*argc*/, char* /*argv*/[])
    {
        QVector<int(*)()> fs;
        //fs.append(&progress::test);
        //fs.append(&threads::test_qthread);
        //fs.append(&threads::test_jthreads);
        //fs.append(&threads::test_concurrent_function);
        //fs.append(&threads::test_pool_concurrence);
        fs.append(&refcount::test);

        foreach (int(*f)(), fs)
        {
            try
            {
                int r = f();
                l.msg("test result: %d", r);
            }
            catch (std::exception& e)
            {
                l.fatal_error(nfmt<1>("exception caught: %1") (e.what()));
            }
            catch (...)
            {
                l.fatal_error("unknown exception caught");
            }
        }
        return 0;
    }
}

namespace app
{
    int main(int argc, char *argv[])
    {
        QApplication a(argc, argv);
        main_window w;
        w.show();
        return a.exec();
    }
}

int main(int argc, char *argv[])
{
    #ifndef MAIN_NAMESPACE
    #error !!! MAIN_NAMESPACE must either be defined as 'tests' or 'app' !!!
    #endif
    return MAIN_NAMESPACE::main(argc, argv);
}
