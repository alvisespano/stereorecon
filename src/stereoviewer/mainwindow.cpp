#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QAbstractButton>
#include <QDialog>
#include <QHBoxLayout>
#include <algorithm>
#include "config.h"
#include "globals.h"
#include "prelude.h"
#include "cvlab/math3d.h"

using namespace cvlab;
using namespace std::placeholders;
using namespace prelude::log;
using prelude::fmt;
using prelude::make_mapping_iterator;
using prelude::joiner;
using prelude::jthread;
using prelude::progress_adapter;
using prelude::progress_adapter_base;
using prelude::composed_progress_adapter;
using prelude::logger_adapter;
using prelude::text_logger;
using prelude::composed_logger;

//
//
//
// Qt main window widgets printers

template <typename QW>
class main_window_printer_base : public stubbing_printer<QW*>
{
protected:
    main_window* const mw;
    virtual void print(const QString& s) = 0;
    main_window_printer_base(main_window* _mw, QW* qw) : stubbing_printer<QW*>(qw), mw(_mw) {}

public:
    virtual void print_line(const QString& s)
    {
        mw->delegate_as_event(&main_window_printer_base::print, this, s);
    }
};

class qstatusbar_printer : public main_window_printer_base<QStatusBar>
{
protected:
    virtual void print(const QString& s) { this->object->showMessage(s); }

public:
    qstatusbar_printer(main_window* mw, QStatusBar* bar)
        : main_window_printer_base<QStatusBar>(mw, bar) {}
};

class qconsole_printer : public main_window_printer_base<QPlainTextEdit>
{
protected:
    virtual void print(const QString& s) { this->object->appendPlainText(s); }

public:
    qconsole_printer(main_window* mw, QPlainTextEdit* e)
        : main_window_printer_base<QPlainTextEdit>(mw, e) {}
};

//
//
//
// QProgressBar progress adapter

class progressbar_adapter : public progress_adapter_base,
                            public std::enable_shared_from_this<progressbar_adapter>
{
protected:
    main_window* const mw;
    QProgressBar* const bar;

    int min() const { return bar->minimum(); }
    int max() const { return bar->maximum(); }

    typedef progressbar_adapter self;

    static void __start(const std::shared_ptr<self>& _this)
    {
        _this->bar->setValue(_this->min());
        _this->bar->show();
    }

    static void __finish(const std::shared_ptr<self>& _this)
    {
        _this->bar->setValue(_this->max());
        _this->bar->hide();
    }

    static void __set(const std::shared_ptr<self>& _this, double x)
    {
        using prelude::reproj;
        _this->bar->setValue(reproj(x, 0.0, 1.0, _this->min(), _this->max()));
    }

    virtual void set(double x) { mw->delegate_as_event(&self::__set, this->shared_from_this(), x); }

public:
    progressbar_adapter(main_window* _mw, QProgressBar* _bar) : mw(_mw), bar(_bar) {}

    virtual double resolution() const { return 1.0 / (max() - min()); }
    virtual void start() { mw->delegate_as_event(&self::__start, this->shared_from_this()); }
    virtual void finish() { mw->delegate_as_event(&self::__finish, this->shared_from_this()); }
};

//
//
//
// main window implementation

/*template <typename Q, typename X>
class qt_settable
{
private:
    Q* const q;

public:
    explicit qt_settable(Q* const _q) : q(_q) {}

    qt_settable<Q, X>& operator=(const X& x)
    {
        q->setValue(x);
        return *this;
    }

    operator X() const
    {
        return q->value();
    }
};

template <typename Q>
auto qtval(Q* const q) -> qt_settable<Q, decltype(q->value())>
{
    return qt_settable<Q, decltype(q->value())>(q);
}*/

main_window::main_window(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    globals::logger->msg("setting up UI...");
    ui->setupUi(this);

    // inner widgets setup
    {
        statusBar()->clearMessage();
        ui->progressBar->hide();
        ui->imagesView->setScene(&scene);
        ui->imagesView->setRenderHint(QPainter::Antialiasing);
    }

    // generate palette
    for (size_t i = 0; i < config::ui::correspondence_palette_dim; ++i)
    {
        palette.append(QColor(qrand() % 0xff, qrand() % 0xff, qrand() % 0xff));
    }

    // console logger composition
    {
        auto&& console_logger = std::make_shared<text_logger>(
                                    std::make_shared<globals::fancy_log_formatter>(),
                                    std::make_shared<qconsole_printer>(this, ui->console));
        globals::logger = std::make_shared<composed_logger>(console_logger, globals::logger);
    }

    // default values
    {
        // kmatcher options
        ui->neighboursBox->setValue(config::stereoview::defaults::kmatcher::neighbours);
        ui->maxItersBox->setValue(config::stereoview::defaults::kmatcher::max_iterations);
        ui->avgPayoffDoubleBox->setValue(config::stereoview::defaults::kmatcher::average_payoff_threshold);
        ui->payoffAlphaDoubleBox->setValue(config::stereoview::defaults::kmatcher::payoff_alpha);
        ui->qualityDoubleBox->setValue(config::stereoview::defaults::kmatcher::quality_threshold);

        // SURF options
        ui->hessianBox->setValue(config::stereoview::defaults::cvsurf::hessian_threshold);
        ui->responseDoubleBox->setValue(config::stereoview::defaults::opensurf::response_threshold);

        // PMVS2 options
        ui->levelBox->setValue(config::stereoview::defaults::pmvs::level);
        ui->csizeBox->setValue(config::stereoview::defaults::pmvs::csize);
        ui->wsizeBox->setValue(config::stereoview::defaults::pmvs::wsize);
        ui->thresholdDoubleBox->setValue(config::stereoview::defaults::pmvs::threshold);
        ui->quadDoubleBox->setValue(config::stereoview::defaults::pmvs::quad);
        ui->maxAngleBox->setValue(config::stereoview::defaults::pmvs::max_angle);

        // viewer options
        ui->rescaleCheckBox->setChecked(config::ui::defaults::rescale_enabled);
        ui->wBox->setValue(config::ui::defaults::rescale_width);
        ui->hBox->setValue(config::ui::defaults::rescale_height);

        // menus
        ui->actionStatusBarLogEnabled->setChecked(config::ui::defaults::statusbar_logger_enabled);
    }

    // trigger options & flags
    {
        set_statusbar_logger_enabled(config::ui::defaults::statusbar_logger_enabled);
        sv.rescale() = config::ui::defaults::rescale_enabled ? some(QSize(config::ui::defaults::rescale_width, config::ui::defaults::rescale_height)) : none;
    }

    globals::logger->msg("UI setup done");
}

main_window::~main_window()
{
    delete ui;
}

void main_window::load_images(const QDir& dir, const QStringList& filenames)
{
    auto stepper = start_progress_tracker(__func__).section(filenames.size());
    unsigned int i = 0;
    const int nfiles = filenames.size();
    QStringList fnames = filenames; // Qt recommends iterating over a copy
    fnames.sort();

    foreach (const auto& s, fnames)
    {
        const auto&& path = dir.absoluteFilePath(s);
        globals::logger->msg(nfmt<3>("loading image #%1 of %2 '%3'...") (i) (nfiles) (path));
        imgspecs.append(sv.load_image(*globals::logger, path));
        globals::logger->msg(nfmt<2>("image stored at %1x%2") (sv.width()) (sv.height()));
        ++stepper;
        ++i;
    }
}

void main_window::load_and_display_images(const QDir& dir, const QStringList& filenames)
{
    try
    {
        load_images(dir, filenames);
        display_images();
    }
    catch (std::exception& e)
    {
        warn_box("exception caught while loading images: %1", e);
    }
}

//
//
//
// external events

void main_window::scale_corresponding_match_graphics_items(MatchGraphicsItem* item, double opa, int z, const QPen& pen)
{
    const stereoview::feature_id_type&& fid = item->feature_id;
    foreach (const image_spec& spec, imgspecs)
    {
        if (spec.correspondence_item_group != NULL)
        {
            foreach (QGraphicsItem* const _item, spec.correspondence_item_group->childItems())
            {
                MatchGraphicsItem* const item = dynamic_cast<MatchGraphicsItem* const>(_item);
                if (item->feature_id == fid)
                {
                    item->setOpacity(opa);
                    item->setZValue(z);
                    item->setPen(pen);
                }
            }
        }
    }
}

void main_window::match_item_hover_enter(MatchGraphicsItem* item)
{
    scale_corresponding_match_graphics_items(item, 1.0, 1000, QPen(QColor(0xff, 0xff, 0xff), config::ui::correspondence_width * 1.2));
}

void main_window::match_item_hover_leave(MatchGraphicsItem* item)
{
    scale_corresponding_match_graphics_items(item, config::ui::correspondence_opacity, config::ui::correspondence_z, item->original_pen);
}

//
//
//
// display function family

void main_window::set_best_scale_factor(QGraphicsItem* item)
{
    auto* iv = ui->imagesView;
    if (item != NULL)
    {
        iv->fitInView(item, Qt::KeepAspectRatioByExpanding);
        iv->centerOn(item);
    }
}

void main_window::__display_images()
{
    globals::logger->msg(nfmt<1>("displaying %1 images...") (sv.size()));
    qreal xpos = 0.0;
    for (auto it = imgspecs.begin(); it != imgspecs.end(); ++it)
    {
        image_spec& spec = *it;
        const auto&& img = sv.get_image(spec.id);
        QGraphicsItem*& item = spec.image_item;
        if (item != NULL) scene.removeItem(item);
        item = scene.addPixmap(QPixmap::fromImage(img));
        item->setPos(xpos, 0.0);
        item->setZValue(0);
        xpos += img.width() + 1;
    }
    if (!imgspecs.empty()) set_best_scale_factor(imgspecs.begin()->image_item);
}

void main_window::__display_keypoints(size_t imgi, const QColor& color)
{
    QPen pen(color, config::ui::keypoint_width);
    auto& spec = imgspecs[imgi];
    QList<QGraphicsItem*> items;
    {
        stereoview::stored_image_scoped_lock stimg(sv, spec.id);
        const auto& keypoints = stimg->keypoints;
        for (int i = 0; i < keypoints.size(); ++i)
        {
            const auto& k = keypoints[i];
            QGraphicsItem* item = scene.addEllipse(k.x - 2.0, k.y - 2.0, 4.0, 4.0, pen);
            item->setToolTip(nfmt<5>("keypoint #%1 x:%2 y:%3 scale:%4 rot:%5") (i) (int(k.x)) (int(k.y)) (k.scale) (k.rotation));
            items.append(item);
        }
    }
    replace_item_group(imgi, spec.feature_item_group, items, config::ui::keypoint_z, config::ui::keypoint_opacity);
}

void main_window::__display_correspondences(size_t i)
{
    auto& spec = imgspecs[i];
    const auto& id = spec.id;
    size_t cnt = 0;
    QList<QGraphicsItem*> items;
    auto tt = sv.atomic_tracking_table();
    for (auto it = tt.begin_by_image(id); it != tt.end_by_image(id); ++it)
    {
        if (*it)
        {
            const auto& c = **it;
            const auto& col = palette[(c.group ? *c.group : it.feature()) % config::ui::correspondence_palette_dim];
            QPen pen(col, config::ui::correspondence_width);
            const stereoview::feature_id_type&& feat = it.feature();
            MatchGraphicsItem* item = new MatchGraphicsItem(this, feat, c.x - 4.0, c.y - 4.0, 8.0, 8.0, pen);
            item->setZValue(cnt);
            QString occurs;
            for (auto fit = tt.begin_by_feature(feat); fit != tt.end_by_feature(feat); ++fit)
            {
                const QString s = fmt("[%1]") (fit.image());
                if (*fit) occurs.append(s);
            }
            item->setToolTip(nfmt<7>("feature #%1 x:%2 y:%3 group:%4 err:%5 original:#%6 occurs:%7")
                (feat)
                (int(c.x))
                (int(c.y))
                (c.group ? fmt("%1")(*c.group) : fmt("none"))
                (c.error() ? fmt("%1")(*(c.error())) : fmt("none"))
                (sv.original_image_of(feat))
                (occurs)
                );
            items.append(item);
            ++cnt;
        }
    }
    auto*& group = spec.correspondence_item_group;
    replace_item_group(i, group, items, config::ui::correspondence_z, config::ui::correspondence_opacity);
    group->setAcceptHoverEvents(true);
    group->setHandlesChildEvents(false);
    spec.feature_item_group->setOpacity(config::ui::keypoint_opacity * 0.4);

    if (items.empty())
        ui->menuBundler->setEnabled(false);
    else
        ui->menuBundler->setEnabled(true);
}


void main_window::replace_item_group(size_t i, QGraphicsItemGroup*& group, const QList<QGraphicsItem*>& items, int z, double opa)
{
    auto& spec = imgspecs[i];
    if (group != NULL)
    {
        spec.prev_item_groups.append(group);
        globals::logger->debug(nfmt<2>("dimming %1 old item groups over image #%2") (spec.prev_item_groups.size()) (i));
        foreach (auto* g, spec.prev_item_groups)
        {
            const auto&& o = g->opacity() / 2.0;
            if (o < 0.1)
            {
                scene.removeItem(g);
                scene.destroyItemGroup(g);
            }
            else
            {
                g->setOpacity(o);
            }
        }
    }
    group = scene.createItemGroup(items);
    const auto* imgitem = spec.image_item;
    group->setPos(imgitem->pos());
    group->setZValue(imgitem->zValue() + z);
    group->setOpacity(opa);
    globals::logger->msg(nfmt<2>("displayed %1 new items over image #%2") (group->childItems().size()) (i));
}

//
//
//
// message boxes & progress bar

static void __warn_box(QWidget* parent, const QString& s)
{
    QMessageBox::warning(parent, "Warning!", nfmt<1>("%1.\nClick 'Ok' to continue.") (s));
}

void main_window::warn_box(QString s)
{
    globals::logger->warn("%s", s.toStdString().c_str());
    s[0] = s[0].toUpper();
    delegate_as_event(&__warn_box, this, s);
}

void main_window::warn_box(const nfmt<1>& f, const std::exception& e)
{
    warn_box(f(e.what()));
}

void main_window::show_msg(QString s)
{
    s[0] = s[0].toUpper();
    delegate_as_event(&QStatusBar::showMessage, ui->statusBar, s, 0);
}

progress_tracker main_window::start_progress_tracker(const QString& s)
{
    auto adapter = std::make_shared<composed_progress_adapter>(
            std::make_shared<logger_adapter>(globals::logger.get(), s, 0.10),
            std::make_shared<progressbar_adapter>(this, ui->progressBar));
    return progress_tracker(adapter);
}

//
//
//
// jthread management

static void dispose_jthread(jthread jth)
{
    //globals::logger->debug(HERE(nfmt<1>("disposing thread %1") (jth.descriptor().pretty())));
}

void main_window::jthread_detach_handler(jthread self)
{
    //globals::logger->debug(HERE(nfmt<1>("thread %1 has been detached") (self.descriptor().pretty())));
    delegate_as_event(&dispose_jthread, self);
}

template <typename T>
class jthread_set
{
protected:
    QList<joiner<T>> js;
    main_window* const mw;

public:
    explicit jthread_set(main_window* const _mw)
        : mw(_mw)
    {}

    QVector<T> join() const
    {
        QVector<T> r(js.size());
        std::transform(js.being(), js.end(), r.begin(), &joiner<T>::join);
        return r;
    }

    template <typename F, typename... Args>
    void spawn(const QString& name, const F& f, Args&&... args)
    {
        jthread jth(name);
        js.append(jth.exec(f, std::forward<Args>(args)...));
        jth.detach(std::bind(&main_window::jthread_detach_handler, mw, _1));
    }
};

//
//
//
// File menu

void main_window::on_actionOpenDir_activated()
{
    const auto&& dirname = QFileDialog::getExistingDirectory(this,
                            tr("Select image directory..."),
                            "",
                            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dirname.isEmpty())
    {
        QDir dir(dirname);
        globals::logger->msg(nfmt<1>("user selected image directory: %1") (dir.absolutePath()));
        load_and_display_images(dir, dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::NoSort));
    }
}

void main_window::on_actionOpenFiles_activated()
{
    const auto&& names = QFileDialog::getOpenFileNames(this,
                            tr("Select image file(s)..."),
                            "",
                            tr("Image Files (*.png *.jpg *.bmp *.gif)"));
    if (!names.isEmpty())
        load_and_display_images(QDir(), names);
}

void main_window::on_actionClear_activated()
{
    sv.clear();
    imgspecs.clear();
    scene.clear();
    globals::logger->msg("cleared all image and stereoview data");
}

//
//
//
// Options menu

static std::shared_ptr<logger> old_logger;

void main_window::set_statusbar_logger_enabled(bool b)
{
    if (b)
    {
        old_logger = globals::logger;
        auto&& statusbar_logger = std::make_shared<text_logger>(
                                    std::make_shared<globals::compact_log_formatter>(),
                                    std::make_shared<qstatusbar_printer>(this, ui->statusBar));
        globals::logger = std::make_shared<composed_logger>(statusbar_logger, old_logger);
    }
    else globals::logger = old_logger;
}

void main_window::on_actionStatusBarLogEnabled_toggled(bool b)
{
    set_statusbar_logger_enabled(b);
    globals::logger->msg("StatusBar logger %s", b ? "enabled" : "disabled");
}

void main_window::copy_output_dir_contents(const QDir& srcdir, const QStringList& filters)
{
    const auto&& dstdirname = QFileDialog::getExistingDirectory(this,
                            tr("Select destination directory for output files..."),
                            "",
                            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dstdirname.isEmpty())
    {
        QDir dstdir(dstdirname);
        globals::logger->msg(nfmt<1>("user selected destination directory: %1") (dstdir.absolutePath()));
        bool overwrite = false, all = false;
        foreach (const auto& srcfinfo, srcdir.entryInfoList(filters, QDir::NoDotAndDotDot | QDir::Files, QDir::NoSort))
        {
            const QString&& srcfname = srcfinfo.fileName(),
                         && srcfpath = srcfinfo.absoluteFilePath(),
                         && dstfpath = dstdir.absoluteFilePath(srcfname);
            QFile srcf(srcfpath);
            const bool exists = QFileInfo(dstfpath).exists();
            if (exists && !all)
            {
                const QString s = nfmt<1>("File '%1' already exists. Overwrite it?") (dstfpath);
                const auto&& b = QMessageBox::question(this, "Alert!", s, QMessageBox::Yes | QMessageBox::YesAll | QMessageBox::NoAll | QMessageBox::No, QMessageBox::No);
                switch (b)
                {
                    case QMessageBox::Yes:      overwrite = true; break;
                    case QMessageBox::YesAll:   overwrite = all = true; break;
                    case QMessageBox::NoAll:    overwrite = false; all = true; break;
                    case QMessageBox::No:       overwrite = false; break;
                    default:                    std::unexpected();
                }
            }
            if (exists && overwrite)
            {
                if (!dstdir.remove(dstfpath))
                {
                    warn_box(nfmt<1>("cannot delete file '%1'") (dstfpath));
                    continue;
                }
            }
            if (!exists || overwrite)
            {
                globals::logger->msg(nfmt<2>("copying output file '%1' to '%2'...") (srcfname) (dstfpath));
                if (!srcf.copy(dstfpath))
                    warn_box(nfmt<2>("cannot copy file '%1' to '%2'") (srcfpath) (dstfpath));
            }
        }
    }
}

//
//
//
// scene reconstruction

void main_window::on_actionPMVS2_activated()
{
    try
    {
        stereoview::pmvs_options opt;
        opt.level = ui->levelBox->value();
        opt.csize = ui->csizeBox->value();
        opt.wsize = ui->wsizeBox->value();
        opt.threshold = ui->thresholdDoubleBox->value();
        opt.quad = ui->quadDoubleBox->value();
        opt.max_angle = ui->maxAngleBox->value();

        stereoview::scene_data scenedata = sv.recon_scene(*globals::logger, start_progress_tracker(__func__), camdata, opt);
        copy_output_dir_contents(scenedata.output_dir());
        globals::logger->msg("scene reconstruction done.");
    }
    catch (std::exception& e)
    {
        warn_box("exception trapped while reconstructing scene: %1", e);
    }
    catch (...)
    {
        warn_box("unknown exception trapped while reconstructing scene");
    }
}

//
//
//
// bundle adjustment

void main_window::on_actionBundler_activated()
{
    try
    {
        camdata = sv.bundle_adjustment(*globals::logger, start_progress_tracker(__func__), trkdata);
        copy_output_dir_contents(camdata.output_dir());
        globals::logger->msg("bundle adjustment done.");
    }
    catch (std::exception& e)
    {
        warn_box("exception trapped while reconstructing cameras: %1", e);
    }
    catch (...)
    {
        warn_box("unknown exception trapped while reconstructing cameras");
    }
}

void main_window::on_actionAsync_Bundler_activated()
{
    using prelude::jthread;
    using prelude::joiner;

    jthread th("bundler", globals::logger.get());
    joiner<stereoview::camera_data> j = th.exec(&stereoview::bundle_adjustment, &sv,
                                                std::ref(*globals::logger),
                                                std::move(start_progress_tracker(__func__)),
                                                std::cref(trkdata));
    globals::logger->msg("async bundle adjustment started...");
    camdata = j.join();
}

//
//
//
// keypoint matching

void main_window::match_keypoints(const stereoview::keymatcher_type& ty)
{
    try
    {
        stereoview::kmatcher_options kmopts;
        kmopts.neighbours = ui->neighboursBox->value();
        kmopts.max_iterations = ui->maxItersBox->value();
        kmopts.average_payoff_threshold = ui->avgPayoffDoubleBox->value();
        kmopts.payoff_alpha = ui->payoffAlphaDoubleBox->value();
        kmopts.quality_threshold = ui->qualityDoubleBox->value();

        trkdata = sv.track_keypoint_matches(*globals::logger, start_progress_tracker(__func__),
                        ty,
                        kmopts,
                        make_mapping_iterator(imgspecs.begin(), id_of_spec),
                        make_mapping_iterator(imgspecs.end(), id_of_spec));

        for (int i = 0; i < imgspecs.size(); ++i)
        {
            display_correspondences(i);
        }
        globals::logger->msg("keymatching done.");
    }
    catch (std::exception& e)
    {
        warn_box("exception trapped during keymatch: %1", e);
    }
    catch (...)
    {
        warn_box("unknown exception during keymatch");
    }
}

void main_window::on_actionKmatcher_Single_Shot_activated()
{
    match_keypoints(stereoview::keymatcher_type::kmatcher_single_shot);
}

void main_window::on_actionKmatcher_Concurrent_activated()
{
    match_keypoints(stereoview::keymatcher_type::kmatcher_concurrent);
}

void main_window::on_actionBundler_Keymatcher_activated()
{
    match_keypoints(stereoview::keymatcher_type::bundler_keymatchfull);
}

//
//
//
// keypoint/feature detection

void main_window::detect_keypoints_by_index(int i, const char* name, keypoint_detector_type f, progress_tracker&& prog, const QColor& color)
{
    //const auto& id = imgspecs[i].id;
    globals::logger->msg(nfmt<2>("detecting %1 keypoints from image #%2...") (name) (i));
    //(this->*f)(*globals::logger, prog, id);
    //globals::logger->msg(nfmt<3>("detected %1 %2 keypoints from image #%3") (stereoview::stored_image_scoped_lock(sv, id)->keypoints.size()) (name) (id));
    //display_keypoints(i, color);
}

void main_window::detect_keypoints(const char* name, keypoint_detector_type f, const QColor& color)
{
    try
    {
        auto outstepper = start_progress_tracker(name).section(imgspecs.size());
        jthread_set<void> jset(this);
        for (int i = 0; i < imgspecs.size(); ++i)
        {
            jset.spawn(nfmt<1>("detector#%1") (i),
                       &main_window::detect_keypoints_by_index, this,
                       i, name, f, outstepper.sub(), color);

            //detect_keypoints_sift(*globals::logger, outstepper.sub(), imgspecs[i].id);
            globals::logger->msg("WARNING: ALWAYS CALLING SIFT++");
            detect_keypoints_siftpp(*globals::logger, outstepper.sub(), imgspecs[i].id);
            globals::logger->msg(nfmt<3>("detected %1 %2 keypoints from image #%3")
                                 (stereoview::stored_image_scoped_lock(sv, imgspecs[i].id)->keypoints.size())
                                 (name)
                                 (imgspecs[i].id));
            display_keypoints(i, color);

            ++outstepper;
        }
        //jset.join();
    }
    catch (std::exception& e)
    {
        warn_box(nfmt<2>("exception trapped while detecting %1 keypoints: %2") (name), e);
    }
    catch (...)
    {
        warn_box(nfmt<1>("unknown exception trapped while detecting %1 keypoints") (name));
    }
}

void main_window::async_detect_keypoints(const char* name, keypoint_detector_type f)
{
    jthread jth("detector-spawner", globals::logger.get());
    jth.delegate(&main_window::detect_keypoints, this, name, f, config::ui::keypoint_color());
    jth.detach(std::bind(&main_window::jthread_detach_handler, this, _1));
}

void main_window::detect_keypoints_opencv(logger& l, progress_tracker&& p, const stereoview::image_id_type& id)
{
    sv.detect_keypoints_cvsurf(l, p, id, ui->hessianBox->value());
}

void main_window::detect_keypoints_opensurf(logger& l, progress_tracker&& p, const stereoview::image_id_type& id)
{
    sv.detect_keypoints_opensurf(l, p, id, ui->responseDoubleBox->value());
}

void main_window::detect_keypoints_sift(logger& l, progress_tracker&& p, const stereoview::image_id_type& id)
{
    sv.detect_keypoints_sift(l, p, id);
}

void main_window::detect_keypoints_siftpp(logger& l, progress_tracker&& p, const stereoview::image_id_type& id)
{
    sv.detect_keypoints_siftpp(l, p, id);
}

void main_window::on_actionOpenSURF_Keypoints_activated()
{
    async_detect_keypoints("OpenSURF", &main_window::detect_keypoints_opensurf);
}

void main_window::on_actionOpenCV_SURF_Keypoints_activated()
{
    async_detect_keypoints("OpenCV-SURF", &main_window::detect_keypoints_opencv);
}

void main_window::on_actionSIFT_Keypoints_activated()
{
    async_detect_keypoints("SIFT", &main_window::detect_keypoints_sift);
}

void main_window::on_actionSIFTpp_Keypoints_activated()
{
    async_detect_keypoints("SIFT++", &main_window::detect_keypoints_siftpp);
}

//
//
//
// flags & control widgets

void main_window::on_rescaleCheckBox_toggled(bool b)
{
    sv.rescale() = b ? some(QSize(ui->wBox->value(), ui->hBox->value())) : none;
}

void main_window::on_wBox_valueChanged(int w)
{
    const option<QSize>& rs = sv.rescale();
    if (rs) sv.rescale() = some(QSize(w, rs->height()));
}

void main_window::on_hBox_valueChanged(int h)
{
    const option<QSize>& rs = sv.rescale();
    if (rs) sv.rescale() = some(QSize(rs->width(), h));
}

//
//
//
// custom event handler

void main_window::customEvent(QEvent* _ev)
{
    try
    {
        custom_event* ev = dynamic_cast<custom_event*>(_ev);
        try
        {
            ev->exec();
        }
        catch (std::exception& e)
        {
            globals::logger->warn("exception caught while processing main_window_event: %s", e.what());
        }
        catch (...)
        {
            globals::logger->warn("unknown exception caught while processing main_window_event");
        }
    }
    catch (std::bad_cast& e)
    {
        globals::logger->unexpected_error(HERE(nfmt<2>("event object has type %1: %2") (typeid(_ev).name()) (e.what())));
    }
}

//
//
//
// test stuff

void main_window::set_bar_at(int x)
{
    globals::logger->msg("setting bar at %d", x);
    ui->progressBar->show();
    ui->progressBar->setValue(x);
}

void main_window::test_bar_update()
{
    for (int i = 0; i < 100; i += 10)
    {
        globals::logger->msg("posting event: set_bar_at(%d)", i);
        delegate_as_event(&main_window::set_bar_at, this, i);
        jthread::sleep(100);
    }
}

void main_window::on_actionMulti_threaded_Progress_Bar_activated()
{
    jthread jth("bar-updater", jthread::finalization_policy::join, globals::logger.get());
    jth.exec(&main_window::test_bar_update, this);
    jth.detach(std::bind(&main_window::jthread_detach_handler, this, _1));
    globals::logger->debug("adios");
}

