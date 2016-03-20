#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
#include <QGraphicsScene>
#include <QDir>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QCoreApplication>
#include "prelude.h"
#include "stereoview.h"
#include "globals.h"
#include "glscene.h"
#include "stereographicsview.h"

using prelude::jthread;

namespace Ui
{
    class MainWindow;
}

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    main_window(QWidget *parent = 0);
    ~main_window();

    void show_msg(QString s);
    void warn_box(QString s);
    void warn_box(const nfmt<1>& f, const std::exception& ex);

protected:
    virtual void customEvent(QEvent* ev);

private:
    // UI components
    Ui::MainWindow *ui;
    StereoGraphicsScene scene;

    // stereoview instance
    stereoview sv;

    //
    // image view database

    struct image_spec
    {
        stereoview::image_id_type id;
        QGraphicsItem* image_item;
        QGraphicsItemGroup* feature_item_group;
        QGraphicsItemGroup* correspondence_item_group;
        QList<QGraphicsItemGroup*> prev_item_groups;

        image_spec(const stereoview::image_id_type& _id)
            : id(_id),
              image_item(NULL),
              feature_item_group(NULL),
              correspondence_item_group(NULL)
        {}

        image_spec() = delete;

        ~image_spec()
        {
            foreach (QGraphicsItemGroup* g,  prev_item_groups)
            {
                delete g;
            }
        }
    };

    QList<image_spec> imgspecs;
    QVector<QColor> palette;

    stereoview::camera_data camdata;
    stereoview::tracking_data trkdata;

    UNARY_FUNCTION_DECL(id_of_spec, const stereoview::image_id_type&, const image_spec&, spec, { return spec.id; });

    //
    // main-thread-only internal API

    void replace_item_group(size_t i, QGraphicsItemGroup*& group, const QList<QGraphicsItem*>& items, int z, double opa);
    void load_images(const QDir& dir, const QStringList& filenames);
    void load_and_display_images(const QDir& dir, const QStringList& filenames);
    void set_best_scale_factor(QGraphicsItem* item);

    //
    // multi-thread internal API

    void __display_images();
    void __display_features(size_t i);
    void __display_keypoints(size_t i, const QColor& color);
    void __display_correspondences(size_t i);

    void display_images() { delegate_as_event(&main_window::__display_images, this); }
    void display_features(size_t i) { delegate_as_event(&main_window::__display_features, this, i); }
    void display_keypoints(size_t i, const QColor& color) { delegate_as_event(&main_window::__display_keypoints, this, i, color); }
    void display_correspondences(size_t i) { delegate_as_event(&main_window::__display_correspondences, this, i); }

    progress_tracker start_progress_tracker(const QString& s);
    void set_statusbar_logger_enabled(bool b);

    void copy_output_dir_contents(const QDir& srcdir, const QStringList& filters = QStringList());

    typedef void (main_window::*keypoint_detector_type)(logger&, progress_tracker&&, const stereoview::image_id_type&);

    void detect_keypoints_by_index(int i, const char* name, keypoint_detector_type f, progress_tracker&& prog, const QColor& color);
    void detect_keypoints(const char* name, keypoint_detector_type f, const QColor& color);
    void async_detect_keypoints(const char* name, keypoint_detector_type f);
    void detect_keypoints_opensurf(logger& l, progress_tracker&& p, const stereoview::image_id_type& id);
    void detect_keypoints_opencv(logger& l, progress_tracker&& p, const stereoview::image_id_type& id);
    void detect_keypoints_sift(logger& l, progress_tracker&& p, const stereoview::image_id_type& id);
    void detect_keypoints_siftpp(logger& l, progress_tracker&& p, const stereoview::image_id_type& id);

    void match_keypoints(const stereoview::keymatcher_type& ty);

    void jthread_detach_handler(jthread self);

    void set_bar_at(int x);
    void test_bar_update();

    //
    // external events

protected:
    void scale_corresponding_match_graphics_items(MatchGraphicsItem* item, double opa, int z, const QPen& pen);

public:
    void match_item_hover_enter(MatchGraphicsItem* item);
    void match_item_hover_leave(MatchGraphicsItem* item);

private slots:
    // Stereoview menu
    void on_actionSIFTpp_Keypoints_activated();
    void on_actionBundler_Keymatcher_activated();
    void on_actionKmatcher_Concurrent_activated();
    void on_actionKmatcher_Single_Shot_activated();
    void on_actionMulti_threaded_Progress_Bar_activated();
    void on_actionAsync_Bundler_activated();
    //void on_actionOpen_PLY_file_activated();
    void on_hBox_valueChanged(int );
    void on_wBox_valueChanged(int );
    void on_rescaleCheckBox_toggled(bool checked);
    void on_actionBundler_activated();
    void on_actionPMVS2_activated();
    void on_actionOpenSURF_Keypoints_activated();
    void on_actionOpenCV_SURF_Keypoints_activated();
    void on_actionSIFT_Keypoints_activated();

    // Options menu
    void on_actionStatusBarLogEnabled_toggled(bool);

    // File menu
    void on_actionClear_activated();
    void on_actionOpenFiles_activated();
    void on_actionOpenDir_activated();

    //
    // custom event system

public:
    class custom_event : public QEvent
    {
        friend class main_window;

    private:
        std::function<void()> f;

        template <typename F>
        explicit custom_event(const F& _f)
            : QEvent(config::ui::main_window_custom_event_type), f(_f)
        {}

        void exec() { f(); }
    };

    template <typename... Args, typename... Actuals>
    void delegate_as_event(const std::function<void(Args...)>& f, Actuals&&... args)
    {
        using namespace prelude;

        if (QThread::currentThread() == this->thread()) f(std::forward<Actuals>(args)...);
        else qApp->postEvent(this, new custom_event(std::bind(f, std::forward<Actuals>(args)...)));
    }

    template <typename... Args, typename... Actuals>
    void delegate_as_event(void(* const f)(Args...), Actuals&&... args)
    {
        delegate_as_event(std::function<void(Args...)>(f), std::forward<Actuals>(args)...);
    }

    template <typename C, typename... Args, typename... Actuals>
    void delegate_as_event(void(C::* const m)(Args...), C* that, Actuals&&... args)
    {
        delegate_as_event(std::function<void(C*, Args...)>(m), that, std::forward<Actuals>(args)...);
    }

    template <typename... Args, typename... Actuals>
    void delegate_as_event(void(main_window::* const m)(Args...), Actuals&&... args)
    {
        delegate_as_event(m, this, std::forward<Actuals>(args)...);
    }
};

#endif // MAINWINDOW_H
