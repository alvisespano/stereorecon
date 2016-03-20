#ifndef STEREOVIEW_H
#define STEREOVIEW_H

#include <QVector>
#include <QImage>
#include <QHash>
#include <QDir>
#include <cxcore.h>
#include <cv.h>
#include <iterator>
#include <limits>
#include <unordered_map>
#include "config.h"
#include "prelude.h"
#include "cvlab/math3d.h"

using prelude::temp_dir;
using prelude::nfmt;
using prelude::fmt;
using prelude::option;
using prelude::some;
using prelude::none;
using prelude::progress_tracker;
using prelude::logger;
using prelude::unique_id_type;
using prelude::sync;
using prelude::localized_runtime_error;
using prelude::localized_invalid_argument;

class stereoview
{
public:
    stereoview(const stereoview&) = delete;
    stereoview& operator=(const stereoview&) = delete;

    //
    // opaque type definitions

private:
    enum class id_types { image_id };

public:
    typedef unique_id_type<unsigned int, id_types, id_types::image_id> image_id_type;
    typedef QImage image_type;
    typedef int dim_type;
    typedef size_t feature_id_type;

    template <size_t DescriptorDim>
    struct keypoint
    {
        friend class stereoview;

        static const size_t descriptor_dim = DescriptorDim;

        float x, y, scale, rotation, descriptor[descriptor_dim];

        keypoint() = default;

        explicit keypoint(float _x, float _y, float _scale, float _rotation, const float* descr)
            : x(_x), y(_y), scale(_scale), rotation(_rotation)
        {
            std::copy(descr, descr + descriptor_dim, descriptor);
        }

    private:
        option<feature_id_type> optfid;
    };

    typedef keypoint<128> keypoint128;

    class correspondence
    {
    private:
        template <typename Corner>
        correspondence(const Corner& corn, const option<unsigned int>& _group, const option<float>& _err)
            : err(_err), group(_group), x(corn.x), y(corn.y) {}

        option<float> err;

    public:
        option<unsigned int> group;
        float x, y;

        correspondence() = delete;

        template <typename Corner>
        static correspondence original(const Corner& corn, const option<unsigned int>& grp)
        {
            return correspondence(corn, grp, some(-1.0f));
        }

        template <typename Corner>
        static correspondence tracked(const Corner& corn, const option<unsigned int>& grp, const option<float>& err)
        {
            return correspondence(corn, grp, err);
        }

        bool is_original() const { return err ? *err < 0.0f : false; }
        option<float> error() const { return is_original() ? none : err; }
    };


protected:

    //
    // stored image table

    class stored_image
    {
    private:
        QImage qimage;

        static void image_to_gray_raw(const image_type& img, unsigned char* raw);
        static CvImage qt_image_to_cv(const QImage& qimg);

    public:
        option<float> focal_length;
        QVector<keypoint128> keypoints;
        QVector<option<correspondence>> correspondences;
        QVector<feature_id_type> feature_ids;

        stored_image() = delete;

        stored_image(const image_type& img, const option<float>& _focal_length)
                : qimage(img), focal_length(_focal_length)
        {}

        const QImage& unscaled_image() const { return qimage; }

        QImage image(const option<QSize>& sz) const
        {
            return sz && (sz->width() < qimage.width() || sz->height() < qimage.height())
                ? qimage.scaled(*sz, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                : qimage;
        }

        CvImage cvimage(const option<QSize>& sz) const
        {
            return qt_image_to_cv(image(sz));
        }
    };

    //
    //
    // internal member data

    typedef std::list<std::tuple<image_id_type, sync<stored_image>>> stored_image_db_type;

    stored_image_db_type stored_image_db;
    sync<QVector<image_id_type> > origins;
    option<QSize> _rescale;

    //
    //
    // internal API

    template <typename Db>
    static auto find_by_image_id(Db& db, const image_id_type& id) -> decltype(db.begin())
    {
        for (auto it = db.begin(); it != db.end(); ++it)
        {
            if (std::get<0>(*it) == id) return it;
        }
        throw localized_invalid_argument(nfmt<1>("image ID:%1 does not exist") (id));
    }

    stored_image_db_type::iterator find_in_db(const image_id_type& id) { return find_by_image_id(stored_image_db, id); }
    stored_image_db_type::const_iterator find_in_db(const image_id_type& id) const { return find_by_image_id(stored_image_db, id); }

    const sync<stored_image>& access_stored_image(const image_id_type& id) const { return std::get<1>(*find_by_image_id(stored_image_db, id)); }
    sync<stored_image>& access_stored_image(const image_id_type& id) { return std::get<1>(*find_by_image_id(stored_image_db, id)); }

    void add_stored_image(const image_id_type& id,
                          const image_type& img,
                          const option<float>& focal_length);

    template <typename Corner>
    feature_id_type add_feature(const image_id_type& id,
                                const Corner& corn,
                                const option<unsigned int>& grp,
                                stored_image& stimg);

    template <typename Corner>
    void set_correspondence(const feature_id_type& fid,
                            const image_id_type& imgid,
                            const Corner& corn,
                            const option<unsigned int>& grp,
                            const option<float>& err);

public:

    //
    // constructors

    explicit stereoview() = default;

    //
    // image manipulation facilities

    dim_type width() const;
    dim_type height() const;

    QUICK_ATTR_PROPERTY(rescale, stereoview, option<QSize>)

    unsigned int size() const;
    bool is_empty() const;
    void clear();

    image_id_type add_image(const image_type& img, const option<float>& focal_length);
    image_id_type load_image(logger& l, const QString& imgpath, const option<float>& ccd_width = none);
    image_type get_image(const image_id_type& id) const;
    void remove_image(const image_id_type& id);
    void save_image(logger& l, const image_id_type& id, const QString& filepath, int quality) const;

    template <typename ImageIdIterator>
    void save_images(logger& l,
                     progress_tracker&& prog,
                     const ImageIdIterator& from,
                     const ImageIdIterator& to,
                     const QDir& outdir,
                     const nfmt<1>& name,
                     int quality) const;

    //
    // atomic access to stored images

    class stored_image_scoped_lock : public sync<stored_image>::ro_scoped
    {
    private:
        typedef sync<stored_image>::ro_scoped super;

    public:
        typedef super::locked_type locked_type;

        stored_image_scoped_lock(stereoview& parent, const image_id_type& id)
            : super(parent.access_stored_image(id))
        {}
    };

    class stored_image_db_scoped_lock
    {
    private:
        typedef std::list<std::tuple<image_id_type, sync<stored_image>::ro_scoped>> map_type;

        map_type tbl;

    public:
        stored_image_db_scoped_lock() = delete;
        stored_image_db_scoped_lock(const stored_image_db_scoped_lock&) = delete;
        stored_image_db_scoped_lock& operator=(const stored_image_db_scoped_lock&) = delete;

        stored_image_db_scoped_lock(stored_image_db_scoped_lock&& sl) : tbl(std::move(sl.tbl)) {}

        explicit stored_image_db_scoped_lock(const stereoview& parent)
        {
            for (auto it = parent.stored_image_db.begin(); it != parent.stored_image_db.end(); ++it)
            {
                const image_id_type& id = std::get<0>(*it);
                const sync<stored_image>& sy = std::get<1>(*it);
                tbl.push_back(std::make_tuple(id, sync<stored_image>::ro_scoped(sy)));
            }
        }

        const stored_image& locked_stored_image(const image_id_type& id) const
        {
            return *std::get<1>(*stereoview::find_by_image_id(tbl, id));
        }

        typedef map_type::const_iterator const_iterator;

        const_iterator begin() const { return tbl.begin(); }
        const_iterator end() const { return tbl.end(); }
    };

    //
    // feature detection

public:
    void detect_keypoints_opensurf(logger& l,
                                   progress_tracker&& prog,
                                   const image_id_type& id,
                                   double response_threshold = config::stereoview::defaults::opensurf::response_threshold);

    void detect_keypoints_cvsurf(logger& l,
                                 progress_tracker&& prog,
                                 const image_id_type& id,
                                 double hessian_threshold = config::stereoview::defaults::cvsurf::hessian_threshold);

    void detect_keypoints_sift(logger& l, progress_tracker&& prog, const image_id_type& id);

    void detect_keypoints_siftpp(logger& l, progress_tracker&& prog, const image_id_type& id);

    const image_id_type& original_image_of(const feature_id_type& fid) const
    {
        return (*origins.atomic())[fid];
    }

protected:
    void parse_keyfile(logger& l,
                       progress_tracker&& prog,
                       const image_id_type& id,
                       const QString& filepath,
                       const std::function<float(float)>& adapt_scale,
                       const std::function<float(float)>& adapt_rot,
                       const std::function<float(float)>& adapt_descr);

    void parse_siftpp_keyfile(logger& l,
                              progress_tracker&& prog,
                              const image_id_type& id,
                              const QString& filepath,
                              const std::function<float(float)>& adapt_scale,
                              const std::function<float(float)>& adapt_rot,
                              const std::function<float(unsigned int)>& adapt_descr);


    //
    // feature matching

public:
    struct tracking_data
    {
        temp_dir temp;
        QString keylist_filename;
        QVector<image_id_type> image_ids;

        tracking_data() = default;
        tracking_data(const temp_dir& _temp, const QString& _keylist_filename, const QVector<image_id_type>& imgids)
            : temp(_temp), keylist_filename(_keylist_filename), image_ids(imgids)
        {}
    };

    enum class keymatcher_type { kmatcher_single_shot, kmatcher_concurrent, bundler_keymatchfull };

    class kmatcher_options
    {
        friend class stereoview;

    public:
        QString filename;
        size_t neighbours, max_iterations;
        double average_payoff_threshold, payoff_alpha, quality_threshold;

        explicit kmatcher_options()
            : neighbours(config::stereoview::defaults::kmatcher::neighbours),
              max_iterations(config::stereoview::defaults::kmatcher::max_iterations),
              average_payoff_threshold(config::stereoview::defaults::kmatcher::average_payoff_threshold),
              payoff_alpha(config::stereoview::defaults::kmatcher::payoff_alpha),
              quality_threshold(config::stereoview::defaults::kmatcher::quality_threshold)
        {}
    };

    tracking_data track_keypoint_matches(logger& l,
                                         progress_tracker&& p,
                                         const keymatcher_type& ty,
                                         const kmatcher_options& kmopts,
                                         const QVector<image_id_type>& imgids);

    template <typename ImageIdIterator>
    tracking_data track_keypoint_matches(logger& l,
                                         progress_tracker&& p,
                                         const keymatcher_type& ty,
                                         const kmatcher_options& kmopts,
                                         const ImageIdIterator& from,
                                         const ImageIdIterator& to)
    {
        QVector<image_id_type> v;
        for (auto it = from; it != to; ++it) v.append(*it);
        return track_keypoint_matches(l, p, ty, kmopts, v);
    }

protected:
    void parse_keypoint_matches_file(logger& l,
                                     progress_tracker&& prog,
                                     const QVector<image_id_type>& imgids,
                                     const QString& filepath,
                                     const bool group_mode,
                                     const option<unsigned int>& ref = none);

    void generate_keyfiles(logger& l,
                           progress_tracker&& prog,
                           const QVector<image_id_type>& imgids,
                           const QDir& outdir,
                           const QString& listfilepath,
                           const std::function<QString(float)>& pretty_descr) const;

    //
    // bundle adjustment

public:
    struct color_point
    {
        cvlab::point3d point;
        unsigned char r, g, b;
    };

    struct bundler_camera
    {
        image_id_type id;
        double f, k1, k2;
        cvlab::matrix<double> r;
        cvlab::point3d t;

        bundler_camera() : r(3, 3) { r.identity(); }
        explicit bundler_camera(const image_id_type& _id) : id(_id), r(3, 3) { r.identity(); }
    };

    class camera_data
    {
        friend class stereoview;

    private:
        temp_dir tempdir;
        QVector<bundler_camera> bundler_cameras;
        QVector<color_point> bundler_points;

    public:
        camera_data() = default;
        explicit camera_data(const temp_dir& _tempdir) : tempdir(_tempdir) {}

        const QDir& output_dir() const { return tempdir; }
    };

    camera_data bundle_adjustment(logger& l, progress_tracker&& p, const tracking_data& trk);

protected:
    camera_data parse_bundler_out_file(logger& l,
                                       progress_tracker&& parent_prog,
                                       const tracking_data& trk,
                                       const QString& filepath);

    camera_data bundle_adjustment_bundler(logger& l, progress_tracker&& p, const tracking_data& trk);

    //
    // scene reconstruction

public:
    class pmvs_options
    {
        friend class stereoview;

    private:
        void commit(const QDir& dir, unsigned int first_image, unsigned int image_cnt) const;

    public:
        QString filename;
        unsigned int level, csize, wsize, cpu, max_angle;
        double threshold, quad;

        explicit pmvs_options()
            : filename(config::stereoview::pmvs::option_filename),
              level(config::stereoview::defaults::pmvs::level),
              csize(config::stereoview::defaults::pmvs::csize),
              wsize(config::stereoview::defaults::pmvs::wsize),
              cpu(prelude::downcrop(QThread::idealThreadCount(), 1)),
              max_angle(config::stereoview::defaults::pmvs::max_angle),
              threshold(config::stereoview::defaults::pmvs::threshold),
              quad(config::stereoview::defaults::pmvs::quad)
        {}
    };

    class scene_data
    {
        friend class stereoview;

    private:
        QDir outdir;
        QVector<color_point> vertices;

    public:
        scene_data() = default;
        explicit scene_data(const QDir& _outdir) : outdir(_outdir) {}

        const QDir& output_dir() const { return outdir; }
        const QVector<color_point>& mesh() const { return vertices; }
    };

public:
    scene_data parse_ply_file(logger& l, progress_tracker&& prog, const QDir& temp, const QString& filename);
    scene_data recon_scene(logger& l, progress_tracker&& prog, const camera_data& cam, const pmvs_options& opt);

protected:
    scene_data recon_scene_pmvs(logger& l, progress_tracker&& prog, const QDir& temp, const pmvs_options& cfg);

    //
    //
    // VTK file format saver

    class vtk_scene
    {
    private:
        struct triangle_idx {
            cvlab::point3d p0, p1, p2;
            triangle_idx(const cvlab::point3d& a, const cvlab::point3d& b, const cvlab::point3d& c) : p0(a), p1(b), p2(c) {}
        };

        std::list<color_point> points;
        std::list<triangle_idx> triangles;

    public:

        void add_points(const QVector<color_point>& pts)
        {
            points.insert(points.end(), pts.begin(), pts.end());
        }

        void add_triangle(const cvlab::point3d& p0, const cvlab::point3d& p1, const cvlab::point3d& p2)
        {
            triangles.push_back( triangle_idx(p0,p1,p2) );
        }

        void save_as_vtk(logger& l, const QString& fname) const;
    };

    void save_as_vtk(logger& l,
                     const QString& fname,
                     const QVector<color_point>& points,
                     const QVector<bundler_camera>& cameras) const;

    //
    //
    // tracking table

public:

    //
    // tracking table base class

    class tracking_table
    {
        friend class stereoview;

    private:
        stereoview& parent;
        stereoview::stored_image_db_scoped_lock sl;

        explicit tracking_table(stereoview& _parent)
            : parent(_parent), sl(parent)
        {}

    public:
        tracking_table() = delete;
        tracking_table(const tracking_table&) = delete;
        tracking_table& operator=(const tracking_table&) = delete;

        tracking_table(tracking_table&& tt) : parent(tt.parent), sl(std::move(tt.sl)) {}

        const option<correspondence>& operator()(const image_id_type& imgid, const feature_id_type& fid) const
        {
            return sl.locked_stored_image(imgid).correspondences[fid];
        }

        //
        // iterator abstract superclass

        template <typename InnerIterator>
        class const_iterator_base
        {
        protected:
            typedef InnerIterator inner_iterator_type;

            inner_iterator_type it;

            explicit const_iterator_base(const inner_iterator_type& _it)
                    : it(_it) {}

        public:
            typedef std::bidirectional_iterator_tag iterator_category;
            typedef typename std::iterator_traits<inner_iterator_type>::difference_type difference_type;
            typedef const option<stereoview::correspondence> value_type;
            typedef value_type* pointer;
            typedef value_type& reference;

        #define __ITERATOR_MEMBERS_DEF(self, super) \
            typedef super::iterator_category iterator_category; \
            typedef super::difference_type difference_type; \
            typedef super::value_type value_type; \
            typedef super::pointer pointer; \
            typedef super::reference reference; \
            /*self& operator+=(const difference_type& d) { this->it = this->it + d; return *this; } \
            self& operator-=(const difference_type& d) { this->it = this->it - d; return *this; } \
            self operator+(const difference_type& d) const { return self(*this) += d; } \
            self operator-(const difference_type& d) const { return self(*this) -= d; }*/ \
            self& operator++() { ++this->it; return *this; } \
            self operator++(int) { self r(*this); ++this->it; return r; } \
            self& operator--() { --this->it; return *this; } \
            self operator--(int) { self r(*this); --this->it; return r; } \
            bool operator==(const self& that) const { return this->it == that.it; } \
            bool operator!=(const self& that) const { return !operator==(that); }

        };

        //
        // by-image correspondence iterator

        class const_iterator_by_image
            : private const_iterator_base<QVector<option<correspondence>>::const_iterator>
        {
        private:
            typedef const_iterator_by_image self;
            typedef const_iterator_base<QVector<option<correspondence>>::const_iterator> super;

        protected:
            typedef super::inner_iterator_type inner_iterator_type;

            const image_id_type id;
            const inner_iterator_type begin;

        public:
            const_iterator_by_image(const inner_iterator_type& _it,
                                    const image_id_type& _id,
                                    const inner_iterator_type& _begin)
                    : super(_it),
                      id(_id),
                      begin(_begin)
            {}

            __ITERATOR_MEMBERS_DEF(self, super)

            reference operator*() const { return *it; }
            pointer operator->() const { return &(operator*()); }

            feature_id_type feature() const { return this->it - begin; }
            const image_id_type& image() const { return id; }
        };

        const_iterator_by_image begin_by_image(const image_id_type& id) const
        {
            const auto& corr = sl.locked_stored_image(id).correspondences;
            return const_iterator_by_image(corr.begin(), id, corr.begin());
        }

        const_iterator_by_image end_by_image(const image_id_type& id) const
        {
            const auto& corr = sl.locked_stored_image(id).correspondences;
            return const_iterator_by_image(corr.end(), id, corr.begin());
        }

        //
        // by-feature correspondence iterator base

        class const_iterator_by_feature
            : private const_iterator_base<stored_image_db_scoped_lock::const_iterator>
        {
        private:
            typedef const_iterator_by_feature self;
            typedef const_iterator_base<stored_image_db_scoped_lock::const_iterator> super;

        protected:
            typedef super::inner_iterator_type inner_iterator_type;

            const feature_id_type id;

        public:
            const_iterator_by_feature(const feature_id_type& _id,
                                      const inner_iterator_type& _it)
                : super(_it),
                  id(_id)
            {}

            __ITERATOR_MEMBERS_DEF(self, super)

            reference operator*() const { return std::get<1>(*it)->correspondences[id]; }
            pointer operator->() const { return &(operator*()); }

            const feature_id_type& feature() const { return id; }
            const image_id_type& image() const { return std::get<0>(*it); }
        };

        const_iterator_by_feature begin_by_feature(const feature_id_type& id) const
        {
            return const_iterator_by_feature(id, sl.begin());
        }

        const_iterator_by_feature end_by_feature(const feature_id_type& id) const
        {
            return const_iterator_by_feature(id, sl.end());
        }

        #undef __ITERATOR_MEMBERS_DEF
    };

    tracking_table atomic_tracking_table() { return tracking_table(*this); }

};

//
//
// template definitions

//
// internalAPI

template <typename Corner>
stereoview::feature_id_type stereoview::add_feature(const image_id_type& imgid,
                                                    const Corner& corn,
                                                    const option<unsigned int>& grp,
                                                    stored_image& locked_stimg)
{
    for (auto it = stored_image_db.begin(); it != stored_image_db.end(); ++it)
    {
        const image_id_type& id = std::get<0>(*it);
        if (id != imgid) std::get<1>(*it).atomic()->correspondences.append(none);
    }
    locked_stimg.correspondences.append(some(correspondence::original(corn, grp)));
    auto sl = origins.atomic();
    sl->append(imgid);
    const feature_id_type r = sl->size() - 1;
    locked_stimg.feature_ids.append(r);
    return r;
}

template <typename Corner>
void stereoview::set_correspondence(const feature_id_type& fid,
                                    const image_id_type& imgid,
                                    const Corner& corn,
                                    const option<unsigned int>& grp,
                                    const option<float>& err)
{
    access_stored_image(imgid).atomic()->correspondences[fid] = some(correspondence::tracked(corn, grp, err));
}

//
//
// external service/command invokation

namespace details_stereoview
{
    using prelude::run_config;
    using prelude::run_cmd;
    using prelude::run_sh;
    using prelude::logger;

    enum class ext { sh, exe };
    typedef int (*run_type)(logger&, run_config);

    template <ext Ty> run_type run();
    template <> inline run_type run<ext::exe>() { return run_cmd; }
    template <> inline run_type run<ext::sh>() { return run_sh; }

    template <ext Ty>
    void run_external(logger& l, run_config& cfg, const QDir& temp)
    {
        using namespace prelude;

        cfg.wdir = temp.absolutePath();
        int r = run<Ty>()(l, cfg);
        l.msg(nfmt<2>("external command '%1'' returned with exit code %2") (cfg.cmd()) (r));
        if (r != 0)
            throw localized_runtime_error(HERE(nfmt<2>("invokation of external command '%1' failed with exit code %2") (cfg.cmd()) (r)));
    }
}

template <typename ImageIdIterator>
void stereoview::save_images(logger& l, progress_tracker&& prog,
                             const ImageIdIterator& from, const ImageIdIterator& to,
                             const QDir& outdir, const nfmt<1>& name, int quality) const
{
    auto stepper = prog.section(to - from);
    for (ImageIdIterator it = from; it != to; ++it)
    {
        const image_id_type& id = *it;
        save_image(l, id, outdir.absoluteFilePath(name(id)), quality);
        ++stepper;
    }
}

#endif // STEREOVIEW_H
