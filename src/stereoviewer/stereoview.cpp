#include <stdexcept>
#include <QDir>
#include <QFileDialog>
#include <QtGlobal>
#include "prelude.h"
#include "stereoview.h"
#include "cvlab/math3d.h"
#include "exif.h"
#include <fstream>

using prelude::localized_invalid_argument;
using prelude::localized_runtime_error;
//using prelude::ws_or_endl;
using prelude::uendl;
using prelude::fresh_int;
using prelude::fmt;

//
//
// image manipulation

void stereoview::clear()
{
    stored_image_db.clear();
    origins.atomic()->clear();
}

stereoview::dim_type stereoview::width() const
{
    return stored_image_db.empty() ? 0 : std::get<1>(*stored_image_db.begin()).atomic()->unscaled_image().width();
}

stereoview::dim_type stereoview::height() const
{
    return stored_image_db.empty() ? 0 : std::get<1>(*stored_image_db.begin()).atomic()->unscaled_image().height();
}

unsigned int stereoview::size() const
{
    return stored_image_db.size();
}

bool stereoview::is_empty() const
{
    return stored_image_db.empty();
}

stereoview::image_type stereoview::get_image(const image_id_type& id) const
{
    return access_stored_image(id).atomic()->image(rescale());
}

stereoview::image_id_type stereoview::add_image(const image_type& img, const option<float>& focal_length)
{
    if (!stored_image_db.empty() && (img.width() != width() || img.height() != height()))
        throw localized_invalid_argument(HERE(nfmt<4>("image size is %1 x %2 while expected size is %3 x %4") (img.width()) (img.height()) (width()) (height())));
    const unsigned int id = fresh_int();
    add_stored_image(id, img, focal_length);
    return id;
}

static option<float> extract_focal_length_from_image(logger& l, const QString& path, float ccd_width)
{
    //return some((3310.4 + 3325.5) / 2.); // dino ring
    return some(810.); // stereo experiments (ueye 640x512)

    float r;
    const auto&& c = extract_focal_length(path.toStdString().c_str(), &r, ccd_width);
    switch (c)
    {
    case FOCAL_LENGTH_OK:
        l.msg(nfmt<3>("EXIF chunk specifies focal length = %2 / CCD width = %3 for image file '%1'") (path) (r) (ccd_width));
        return some(r);
    case FOCAL_LENGTH_INVALID_EXIF:
        l.critical(nfmt<1>("invalid EXIF chunk in image file '%1'") (path));
        return none;
    case FOCAL_LENGTH_NO_CCD_DATA:
        l.critical(nfmt<1>("cannot find CCD data in image '%1'") (path));
        return none;
    default:
        l.unexpected_error(HERE(nfmt<1>("unknown return code %1") (c)));
        return none;
    }
}

stereoview::image_id_type stereoview::load_image(logger& l, const QString& path, const option<float>& ccd_width)
{
    QImage img(path);
    if (img.isNull())
        throw localized_runtime_error(HERE(nfmt<1>("file %1 is not of image type") (path)));
    else
        return add_image(img, extract_focal_length_from_image(l, path, ccd_width.something(0)));
}

void stereoview::remove_image(const image_id_type& id)
{
    stored_image_db.erase(find_in_db(id));
}

void stereoview::stored_image::image_to_gray_raw(const image_type& img, unsigned char* raw)
{
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            *raw++ = qGray(img.pixel(x, y));
}

CvImage stereoview::stored_image::qt_image_to_cv(const QImage& qimg)
{
    CvImage cvimg(cvSize(qimg.width(), qimg.height()), IPL_DEPTH_8U, 1);
    image_to_gray_raw(qimg, cvimg.data());
    return cvimg;
}

void stereoview::save_image(logger& l, const image_id_type& id, const QString& filepath, int quality) const
{
    const option<QSize>& rs = rescale();
    const auto stimg = access_stored_image(id).atomic();
    const auto&& qimg = stimg->image(rs);

    l.msg(nfmt<5>("saving image ID:%1 with quality %2 at %3x%4 to file %5...") (id) (quality) (qimg.width()) (qimg.height()) (filepath));
    if (!qimg.save(filepath, NULL, quality))
        l.msg(nfmt<0>("ERROR: cannot save image to disk."));
}

//
//
// stored image db access

void stereoview::add_stored_image(const image_id_type& id, const image_type& img, const option<float>& focal_length)
{
    stored_image_db.push_back(std::make_tuple(id, sync<stored_image>(stored_image(img, focal_length))));
}

//
//
//
// feature detection

void stereoview::detect_keypoints_opensurf(logger& l, progress_tracker&& prog, const image_id_type& id, double response_threshold)
{
    temp_dir temp(config::stereoview::opensurf::temp_dirname_prefix);
    auto stepper = prog.section(3);

    const QString&& imgfilepath = temp.absoluteFilePath(nfmt<1>("%1.pgm") (id));
    save_image(l, id, imgfilepath, config::stereoview::defaults::temp_saved_image_quality);
    ++stepper;

    const QString&& keyfilepath = temp.absoluteFilePath(nfmt<1>("%1.key") (id));

    // run opensurf
    {
        using namespace ::details_stereoview;

        run_config cfg;
        cfg.cmd() = config::stereoview::opensurf::exe_filename;
        cfg.args << imgfilepath << keyfilepath << fmt("%1") (response_threshold);
        run_external<ext::exe>(l, cfg, temp);
    }
    ++stepper;

    parse_keyfile(l, stepper.sub(), id, keyfilepath,
                  &config::stereoview::defaults::opensurf::adapt_scale,
                  &config::stereoview::defaults::opensurf::adapt_rotation,
                  &config::stereoview::defaults::opensurf::adapt_descriptor);
}

void stereoview::detect_keypoints_sift(logger& l, progress_tracker&& prog, const image_id_type& id)
{
    temp_dir temp(config::stereoview::sift::temp_dirname_prefix);
    auto stepper = prog.section(3);

    const QString&& imgfilepath = temp.absoluteFilePath(nfmt<1>("%1.pgm") (id));
    save_image(l, id, imgfilepath, config::stereoview::defaults::temp_saved_image_quality);
    ++stepper;

    const QString&& keyfilepath = temp.absoluteFilePath(nfmt<1>("%1.key") (id));

    // run sift
    {
        using namespace ::details_stereoview;

        run_config cfg;
        cfg.cmd() = config::stereoview::sift::exe_filename;
        cfg.stdin_path = some(imgfilepath);
        cfg.stdout_path = some(keyfilepath);
        run_external<ext::exe>(l, cfg, temp);
    }
    ++stepper;

    parse_keyfile(l, stepper.sub(), id, keyfilepath,
                  &config::stereoview::defaults::sift::adapt_scale,
                  &config::stereoview::defaults::sift::adapt_rotation,
                  &config::stereoview::defaults::sift::adapt_descriptor);
}

void stereoview::detect_keypoints_siftpp(logger& l, progress_tracker&& prog, const image_id_type& id)
{
    temp_dir temp(config::stereoview::siftpp::temp_dirname_prefix);
    auto stepper = prog.section(3);

    const QString&& imgfilepath = temp.absoluteFilePath(nfmt<1>("%1.pgm") (id));
    save_image(l, id, imgfilepath, config::stereoview::defaults::temp_saved_image_quality);
    ++stepper;

    const QString&& keyfilepath = temp.absoluteFilePath(nfmt<1>("%1.key") (id));

    // run sift
    {
        using namespace ::details_stereoview;

        run_config cfg;
        cfg.cmd() = config::stereoview::siftpp::exe_filename;
        cfg.args << "--verbose"
                 << "--octaves" << QString::number(config::stereoview::defaults::siftpp::octaves)
                 << "--levels" << QString::number(config::stereoview::defaults::siftpp::octave_levels)
                 << "--first-octave" << QString::number(config::stereoview::defaults::siftpp::first_octave)
               //<< "--edge-thresh" << QString::number(config::stereoview::defaults::siftpp::edge_threshold)
                 << "--peak-thresh" << QString::number(config::stereoview::defaults::siftpp::peak_threshold)
               //<< "--magnif" << QString::number(config::stereoview::defaults::siftpp::magnification_factor)
                 ;
        if (config::stereoview::defaults::siftpp::orientations) cfg.args << "--orientations";
        cfg.args << imgfilepath;
        run_external<ext::exe>(l, cfg, temp);
    }
    ++stepper;

    // SIFT++ produces .sift files instead of Lowe's .key format
    const QString siftfilename(keyfilepath.left(keyfilepath.length() - 3) + "sift");

    parse_siftpp_keyfile(l, stepper.sub(), id, siftfilename,
                  &config::stereoview::defaults::siftpp::adapt_scale,
                  &config::stereoview::defaults::siftpp::adapt_rotation,
                  &config::stereoview::defaults::siftpp::adapt_descriptor);
}

void stereoview::detect_keypoints_cvsurf(logger&, progress_tracker&&, const image_id_type& id, double hessian_threshold)
{
    auto sl = access_stored_image(id).atomic();
    auto& stimg = *sl;
    const auto&& cvimg = stimg.cvimage(rescale());
    CvMemStorage* storage = cvCreateMemStorage(0);
    if (storage == NULL) throw localized_runtime_error(HERE("cvCreateMemStorage() returned NULL pointer"));
    try
    {
        CvSeq* keypoints = NULL, *descriptors = NULL;
        CvSURFParams params;
        params.extended = 1;
        params.hessianThreshold = hessian_threshold;
        params.nOctaveLayers = config::stereoview::defaults::cvsurf::octave_layers;
        params.nOctaves = config::stereoview::defaults::cvsurf::octaves;
        cvExtractSURF(static_cast<const IplImage*>(cvimg), NULL, &keypoints, &descriptors, storage, params);

        stimg.keypoints.clear();
        for (int i = 0; i < keypoints->total; ++i)
        {
            auto p = reinterpret_cast<const CvSURFPoint*>(cvGetSeqElem(keypoints, i));
            auto d = reinterpret_cast<const float*>(cvGetSeqElem(descriptors, i));

            keypoint128 k(p->pt.x,
                          p->pt.y,
                          config::stereoview::defaults::cvsurf::adapt_scale(p->size),
                          config::stereoview::defaults::cvsurf::adapt_rotation(p->dir),
                          d);
            for (size_t i = 0; i < keypoint128::descriptor_dim; ++i)
            {
                k.descriptor[i] = config::stereoview::defaults::cvsurf::adapt_descriptor(k.descriptor[i]);
            }
            stimg.keypoints.append(k);
        }
    }
    catch (...)
    {
        cvReleaseMemStorage(&storage);
        throw;
    }
    cvReleaseMemStorage(&storage);
}

//
//
// parsing tools

static QString read_file_for_parsing(const QString& filepath)
{
    QFile f(filepath);
    if (!f.open(QFile::ReadOnly | QIODevice::Text))
        throw localized_runtime_error(HERE(nfmt<1>("cannot open file for reading: %1") (QFileInfo(f).absoluteFilePath())));
    QTextStream s(&f);
    return s.readAll().simplified();
}

static bool is_parse_stream_past_end(const QTextStream& s, const QString& filepath)
{
    switch (s.status())
    {
        case QTextStream::Ok:
            break;
        case QTextStream::ReadPastEnd:
            return true;
        case QTextStream::ReadCorruptData:
            throw localized_runtime_error(HERE(nfmt<1>("stream corrupted while parsing file '%1'") (filepath)));
    }
    return false;
}

static void check_parse_stream_not_past_end(const QTextStream& s, const QString& filepath)
{
    if (is_parse_stream_past_end(s, filepath))
        throw localized_runtime_error(HERE(nfmt<1>("unexpected end of stream reached while parsing file '%1'") (filepath)));
}

//
// parse generic keyfile

void stereoview::parse_keyfile(logger& l,
                               progress_tracker&& prog,
                               const image_id_type& id,
                               const QString& filepath,
                               const std::function<float(float)>& adapt_scale,
                               const std::function<float(float)>& adapt_rot,
                               const std::function<float(float)>& adapt_descr)
{
    QString&& ins = read_file_for_parsing(filepath);
    QTextStream s(&ins);

    unsigned int expected_keypoints, descr_dim;
    s >> expected_keypoints >> descr_dim;
    check_parse_stream_not_past_end(s, filepath);

    if (descr_dim != keypoint128::descriptor_dim)
        throw localized_runtime_error(HERE(nfmt<2>("descriptor dimension in keyfile '%1' is %2 while 128 was expected") (filepath) (descr_dim)));

    l.msg(nfmt<3>("parsing keyfile '%1': %2 keypoints with %3-dimesional descriptor...")
          (filepath) (expected_keypoints) (descr_dim));

    {
        auto sl = access_stored_image(id).atomic();
        auto& stimg = *sl;
        stimg.keypoints.clear();
        auto stepper = prog.section(expected_keypoints);
        for (size_t i = 0; i < expected_keypoints; ++i)
        {
            keypoint128 kp;
            float scale, rot;

            s >> kp.y >> kp.x >> scale >> rot;
            kp.scale = adapt_scale(scale);
            kp.rotation = adapt_rot(rot);

            for (size_t di = 0; di < descr_dim; ++di)
            {
                float x;
                s >> x;
                kp.descriptor[di] = adapt_descr(x);
            }

            check_parse_stream_not_past_end(s, filepath);
            stimg.keypoints.append(kp);
            ++stepper;
        }
    }
}

void stereoview::parse_siftpp_keyfile(logger& l,
                               progress_tracker&&,
                               const image_id_type& id,
                               const QString& filepath,
                               const std::function<float(float)>& adapt_scale,
                               const std::function<float(float)>& adapt_rot,
                               const std::function<float(unsigned int)>& adapt_descr)
{
    QString&& ins = read_file_for_parsing(filepath);
    QTextStream s(&ins);
    l.msg(nfmt<1>("parsing SIFT++ keyfile '%1' with 128-dimesional descriptor...") (filepath));

    auto sl = access_stored_image(id).atomic();
    auto& stimg = *sl;

    // get the number of features
    unsigned int n_keyp = 0;
    {
        std::ifstream siftfile(filepath.toStdString().c_str());
        std::string dump;
        while(getline(siftfile,dump)) { ++n_keyp; }
        siftfile.close();
    }

    // parse the .sift file
    stimg.keypoints.clear();
    for (size_t cnt = 0; cnt < n_keyp; ++cnt)
    {
        if (is_parse_stream_past_end(s, filepath))
        {
            l.msg(HERE(nfmt<2>("end of stream reached while parsing '%1': %2 keypoints stored") (filepath) (cnt)));
            return;
        }

        keypoint128 kp;
        float scale, rot;

        s >> kp.x >> kp.y >> scale >> rot;
        kp.scale = adapt_scale(scale);
        kp.rotation = adapt_rot(rot);

        for (size_t di = 0; di < 128; ++di)
        {
            unsigned int x;
            s >> x;
            kp.descriptor[di] = adapt_descr(x);
        }
        stimg.keypoints.append(kp);
    } // next feature
}

//
//
// feature tracking

void stereoview::generate_keyfiles(logger& l,
                                   progress_tracker&& prog,
                                   const QVector<image_id_type>& imgids,
                                   const QDir& outdir,
                                   const QString& listfilepath,
                                   const std::function<QString(float)>& pretty_descr) const
{
    QFile listf(listfilepath);
    if (!listf.open(QFile::WriteOnly | QIODevice::Text | QFile::Truncate))
        throw localized_runtime_error(HERE(nfmt<1>("cannot open keyfile list file for writing: %1") (QFileInfo(listf).absoluteFilePath())));
    QTextStream ls(&listf);

    auto imgstepper = prog.section(imgids.size());
    foreach (const auto& id, imgids)
    {
        auto sl = access_stored_image(id).atomic();
        const auto& kps = sl->keypoints;
        if (kps.isEmpty())
            throw localized_invalid_argument(HERE(nfmt<1>("keypoints of image ID:%1 must be detected for tracking to make effect")(id)));
        const QString keyfilename = nfmt<1>("%1.key") (id);
        QFile f(outdir.absoluteFilePath(keyfilename));
        if(!f.open(QFile::WriteOnly | QIODevice::Text | QFile::Truncate))
            throw localized_runtime_error(HERE(nfmt<1>("cannot open keyfile for writing: %1") (QFileInfo(f).absoluteFilePath())));

        l.msg(nfmt<2>("generating keyfile '%1' (%2 keypoints)...") (f.fileName()) (kps.size()));
        QTextStream s(&f);
        s << kps.size() << " " << keypoint128::descriptor_dim << uendl;

        auto kpstepper = imgstepper.sub().section(kps.size());
        foreach (const auto& kp, kps)
        {
            s << kp.y << " " << kp.x << " " << kp.scale << " " << kp.rotation;
            for (size_t i = 0; i < keypoint128::descriptor_dim; ++i)
            {
                // follow indentation
                if (i % 20 == 0) s << uendl;
                s << " " << pretty_descr(kp.descriptor[i]);
            }
            s << uendl;
            ++kpstepper;
        }
        ls << "./" << keyfilename << uendl;
        ++imgstepper;
    }
}

void stereoview::parse_keypoint_matches_file(logger& l,
                                             progress_tracker&& prog,
                                             const QVector<image_id_type>& imgids,
                                             const QString& filepath,
                                             const bool group_mode,
                                             const option<unsigned int>& refimgi) // reference image index
{
    QString&& ins = read_file_for_parsing(filepath);
    QTextStream s(&ins);

    const size_t expected_matches = refimgi ? imgids.size() - 1 : imgids.size() * (imgids.size() - 1) / 2;
    l.msg(nfmt<2>("parsing KeyMatcher output file '%1': up to %2 match sets expected...") (filepath) (expected_matches));

    auto stepper = prog.section(expected_matches);
    for (size_t i = 0; i < expected_matches; ++i)
    {
        unsigned int img1, img2, kpcnt;
        s >> img1 >> img2 >> kpcnt;

        if (is_parse_stream_past_end(s, filepath))
        {
            l.msg(nfmt<2>("end of stream reached while parsing file '%1': %2 match sets stored total") (filepath) (i));
            return;
        }

        /*if (img1 == 0 && img2 == 0)
            return;*/

        if (img1 >= static_cast<unsigned int>(imgids.size()) || img2 >= static_cast<unsigned int>(imgids.size()))
            throw localized_runtime_error(HERE(nfmt<2>("unexpected match set between non existent images #%1,%2") (img1) (img2)));

        if (refimgi && (img1 != *refimgi && img2 != *refimgi))
            throw localized_runtime_error(HERE(nfmt<3>("unexpected match set between images #%1,%2 with reference image being #%1") (img1) (img2) (*refimgi)));

        l.msg(nfmt<3>("storing %1 keypoint correspondences for match set between images #%2,%3...") (kpcnt) (img1) (img2));
        const auto& img1id = imgids[img1],
                  & img2id = imgids[img2];

        // atomic inner loop dealing with each match set
        {
            auto sl1 = access_stored_image(img1id).atomic(),
                 sl2 = access_stored_image(img2id).atomic();
            auto& stimg1 = *sl1,
                  stimg2 = *sl2;
            for (unsigned int j = 0; j < kpcnt; ++j)
            {
                unsigned int kp1, kp2;
                s >> kp1 >> kp2;

                if (kp1 >= static_cast<unsigned int>(stimg1.keypoints.size()))
                    throw localized_runtime_error(HERE(nfmt<2>("left-side keypoint #%1 is out of range [0..%2]") (kp1) (stimg1.keypoints.size() - 1)));
                if (kp2 >= static_cast<unsigned int>(stimg2.keypoints.size()))
                    throw localized_runtime_error(HERE(nfmt<2>("right-side keypoint #%1 is out of range [0..%2]") (kp2) (stimg2.keypoints.size() - 1)));

                option<unsigned int> group;
                if (group_mode)
                {
                    unsigned int g;
                    s >> g;
                    group = some(g);
                }

                check_parse_stream_not_past_end(s, filepath);

                auto& ofid = stimg1.keypoints[kp1].optfid;
                if (!ofid) ofid = some(add_feature(img1id, stimg1.keypoints[kp1], group, stimg1));
                set_correspondence(*ofid, img2id, stimg2.keypoints[kp2], group, none);
            }
        }
        ++stepper;
    }
}

stereoview::tracking_data stereoview::track_keypoint_matches(logger& l,
                                                             progress_tracker&& prog,
                                                             const keymatcher_type& ty,
                                                             const kmatcher_options& kmopts,
                                                             const QVector<image_id_type>& imgids)
{
    temp_dir temp(config::stereoview::bundler::temp_dirname_prefix, &l);
    auto outstepper = prog.section(2);
    QString keylist_filename;

    // generate key files
    {
        std::function<QString(float)> pretty_descr;
        switch (ty)
        {
            case keymatcher_type::kmatcher_single_shot:
            case keymatcher_type::kmatcher_concurrent:
                pretty_descr = &config::stereoview::defaults::kmatcher::pretty_descriptor;
                keylist_filename = config::stereoview::kmatcher::keylist_filename;
                break;
            case keymatcher_type::bundler_keymatchfull:
                pretty_descr = &config::stereoview::defaults::bundler_keymatchfull::pretty_descriptor;
                keylist_filename = config::stereoview::bundler_keymatchfull::keylist_filename;
                break;
        }
        generate_keyfiles(l, outstepper.sub(), imgids, temp, temp.absoluteFilePath(keylist_filename), pretty_descr);
        ++outstepper;
    }

    // invoke keymatcher
    switch (ty)
    {
        case keymatcher_type::kmatcher_single_shot:
        case keymatcher_type::bundler_keymatchfull:
        {
            auto stepper = outstepper.sub().section(2);
            QString outfilename;

            // run kmatcher in single-shot mode
            {
                using namespace ::details_stereoview;

                run_config cfg;
                switch (ty)
                {
                    case keymatcher_type::kmatcher_single_shot:
                        outfilename = config::stereoview::kmatcher::output_matches_filename;
                        cfg.cmd() = config::stereoview::kmatcher::exe_filename;
                        cfg.args << keylist_filename
                                 << outfilename
                                 << fmt("%1") (width())
                                 << fmt("%1") (kmopts.neighbours)
                                 << fmt("%1") (kmopts.max_iterations)
                                 << fmt("%1") (kmopts.average_payoff_threshold)
                                 << fmt("%1") (kmopts.payoff_alpha)
                                 << fmt("%1") (kmopts.quality_threshold);
                        break;

                    case keymatcher_type::bundler_keymatchfull:
                        outfilename = config::stereoview::bundler_keymatchfull::output_matches_filename;
                        cfg.cmd() = config::stereoview::bundler_keymatchfull::exe_filename;
                        cfg.args << keylist_filename
                                 << outfilename;
                        break;

                    default: std::unexpected();
                }
                run_external<ext::exe>(l, cfg, temp);
            }
            ++stepper;

            // parse partial matches file
            switch (ty)
            {
                case keymatcher_type::kmatcher_single_shot:
                    parse_keypoint_matches_file(l, stepper.sub(), imgids, temp.absoluteFilePath(fmt("%1.grp")(outfilename)), true, none);
                    break;
                case keymatcher_type::bundler_keymatchfull:
                    parse_keypoint_matches_file(l, stepper.sub(), imgids, temp.absoluteFilePath(outfilename), false, none);
                    break;

                default: std::unexpected();
            }
        }
        break;

        case keymatcher_type::kmatcher_concurrent:
        {
            // open global output file
            const QString goutfilepath = temp.absoluteFilePath(config::stereoview::kmatcher::output_matches_filename);
            QFile goutf(goutfilepath);
            if (!goutf.open(QFile::WriteOnly | QIODevice::Text | QFile::Truncate))
                throw localized_runtime_error(HERE(nfmt<1>("cannot open keymatcher global output file for writing: %1") (QFileInfo(goutf).absoluteFilePath())));
            QTextStream gouts(&goutf);

            // iterate kmatcher invokations
            auto stepper = outstepper.sub().section(imgids.size());
            for (int i = 0; i < imgids.size(); ++i)
            {
                using namespace ::details_stereoview;

                const QString outfilename = config::stereoview::kmatcher::partial_output_matches_filename_fmt(i);

                // run kmatcher for image #i
                {
                    run_config cfg;
                    cfg.cmd() = config::stereoview::kmatcher::exe_filename;
                    cfg.args << keylist_filename
                             << outfilename
                             << fmt("%1") (width())
                             << fmt("%1") (kmopts.neighbours)
                             << fmt("%1") (kmopts.max_iterations)
                             << fmt("%1") (kmopts.average_payoff_threshold)
                             << fmt("%1") (kmopts.payoff_alpha)
                             << fmt("%1") (kmopts.quality_threshold)
                             << fmt("%1") (i);
                    run_external<ext::exe>(l, cfg, temp);
                }

                // parse & merge partial matches file
                {
                    const QString outfilepath = temp.absoluteFilePath(fmt("%1.grp")(outfilename));
                    parse_keypoint_matches_file(l, stepper.sub(), imgids, outfilepath, true, some(i));

                    l.debug(nfmt<2>("merging partial matches file '%1' to global output matches file '%2'...") (outfilepath) (goutfilepath));
                    QFile inf(outfilepath);
                    if (!inf.open(QFile::ReadOnly | QIODevice::Text))
                        throw localized_runtime_error(HERE(nfmt<1>("cannot open keypoint matches file for reading: %1") (QFileInfo(inf).absoluteFilePath())));
                    QTextStream ins(&inf);
                    while (!ins.atEnd())
                    {
                        gouts << ins.readLine() << endl;
                    }
                    ++stepper;
                }
            }
        }
        break;
    }
    return tracking_data(temp, keylist_filename, imgids);
}

//
//
// bundle adjustment

stereoview::camera_data stereoview::parse_bundler_out_file(logger& l, progress_tracker&& prog, const tracking_data& trk, const QString& filepath)
{
    const temp_dir& temp = trk.temp;
    camera_data camdata(temp);

    QString&& ins = read_file_for_parsing(temp.absoluteFilePath(filepath));
    QTextStream s(&ins);

    {
        QString trash;
        s >> trash >> trash >> trash >> trash;
    }
    unsigned int num_cameras, num_points;
    s >> num_cameras >> num_points;
    l.msg(nfmt<3>("parsing Bundler output file '%1': %2 cameras and %3 points expected...") (filepath) (num_cameras) (num_points));
    check_parse_stream_not_past_end(s, filepath);

    auto outstepper = prog.section(2);

    // read cameras intrinsics and extrinsics
    {
        auto stepper = outstepper.sub().section(num_cameras);

        for (size_t i = 0; i < num_cameras; ++i)
        {
            bundler_camera cam(trk.image_ids[i]);
            {
                s >> cam.f >> cam.k1 >> cam.k2;

                {
                    auto& r = cam.r;
                    s >> r(0, 0) >> r(0, 1) >> r(0, 2);
                    s >> r(1, 0) >> r(1, 1) >> r(1, 2);
                    s >> r(2, 0) >> r(2, 1) >> r(2, 2);
                }

                s >> cam.t.x >> cam.t.y >> cam.t.z;
            }
            check_parse_stream_not_past_end(s, filepath);

            camdata.bundler_cameras.append(cam);
            ++stepper;
        }
        l.msg(nfmt<1>("stored %1 bundler cameras") (num_cameras));
    }

    // read colored 3d points
    {
        auto stepper = outstepper.sub().section(num_points);

        for (size_t i = 0; i < num_points; ++i)
        {
            color_point p;
            {
                s >> p.point.x >> p.point.y >> p.point.z;
                unsigned int r, g, b;
                s >> r >> g >> b;
                p.r = r;
                p.g = g;
                p.b = b;

                // skip viewlists
                {
                    unsigned int n;
                    s >> n;
                    QString trash;
                    for (; n > 0; --n)
                    {
                        s >> trash >> trash >> trash >> trash;
                    }
                }
            }
            check_parse_stream_not_past_end(s, filepath);
            camdata.bundler_points.append(p);
            ++stepper;
        }
        l.msg(nfmt<1>("stored %1 bundler points") (num_points));
    }

    return camdata;
}

stereoview::camera_data stereoview::bundle_adjustment_bundler(logger& l, progress_tracker&& prog, const tracking_data& trk)
{
    auto temp = trk.temp;
    auto stepper = prog.section(4);

    // regenerate key files
    generate_keyfiles(l, stepper.sub(), trk.image_ids, trk.temp, trk.temp.absoluteFilePath(trk.keylist_filename), &config::stereoview::defaults::bundler::pretty_descriptor);
    ++stepper;

    // setup dirs and files
    temp.mkdir(config::stereoview::bundler::out_dirname);
    temp.remove("constraints.txt");
    temp.remove("pairwise_scores.txt");

    save_images(l, stepper.sub(), trk.image_ids.begin(), trk.image_ids.end(), temp, nfmt<1>("%1.jpg"), config::stereoview::defaults::temp_saved_image_quality);
    ++stepper;

    // generate 'list.txt' file
    {
        const char* listfilename = "list.txt";
        QFile f(temp.absoluteFilePath(listfilename));
        if (!f.open(QFile::WriteOnly | QIODevice::Text | QFile::Truncate))
            throw localized_runtime_error(HERE(nfmt<1>("cannot open image list file for writing: %1") (QFileInfo(f).absoluteFilePath())));
        QTextStream s(&f);
        foreach (const auto& id, trk.image_ids)
        {
            const auto& fl = access_stored_image(id).ro_atomic()->focal_length;
            if (fl) s << nfmt<2>("./%1.jpg 0 %2") (id) (*fl) << uendl;
            else s << nfmt<1>("./%1.jpg") (id) << uendl;
        }
    }
    ++stepper;

    // run Bundler
    {
        using namespace ::details_stereoview;

        const nfmt<1> out_fmt = nfmt<2>("%1/%2") (config::stereoview::bundler::out_dirname);

        // run Bundler program
        run_config cfg;
        cfg.cmd() = config::stereoview::bundler::exe_filename;
        cfg.args << "list.txt" << config::stereoview::bundler::exe_options;
        cfg.stdout_path = some(temp.absoluteFilePath(out_fmt(config::stereoview::bundler::log_filename)));
        run_external<ext::exe>(l, cfg, temp);

        camera_data&& camdata = parse_bundler_out_file(l, prog, trk, out_fmt(config::stereoview::bundler::out_filename));
        for (auto it = camdata.bundler_cameras.begin(); it != camdata.bundler_cameras.end(); ++it)
        {
            using namespace cvlab;

            bundler_camera& cam = *it;
            point3d& t = cam.t;
            matrix<double>& r = cam.r;

            r = transpose(r);
            rotate(t, r);
            t *= -1;
        }
        save_as_vtk(l, temp.absoluteFilePath(out_fmt("out.vtk")), camdata.bundler_points, camdata.bundler_cameras);

        camdata.tempdir.cd(config::stereoview::bundler::out_dirname);
        return camdata;
    }
}

stereoview::camera_data stereoview::bundle_adjustment(logger& l, progress_tracker&& prog, const tracking_data& trk)
{
    return bundle_adjustment_bundler(l, prog, trk);
}


//
//
// scene reconstruction

stereoview::scene_data stereoview::recon_scene(logger& l, progress_tracker&& prog, const camera_data& cam, const pmvs_options& opt)
{
    QDir d(cam.tempdir);
    d.cdUp();
    return recon_scene_pmvs(l, prog, d, opt);
}

void stereoview::pmvs_options::commit(const QDir& dir, unsigned int first_image, unsigned int image_cnt) const
{
    QFile f(nfmt<2>("%1/%2") (dir.absolutePath()) (filename));
    if (!f.open(QFile::WriteOnly | QIODevice::Text))
        throw localized_runtime_error(HERE(nfmt<1>("cannot open PMVS2 options file for writing: %1") (QFileInfo(f).absoluteFilePath())));
    QTextStream s(&f);
    s << "level " << level << uendl;
    s << "csize " << csize << uendl;
    s << "threshold " << threshold << uendl;
    s << "wsize " << wsize << uendl;
    s << "minImageNum " << config::stereoview::pmvs::min_image_num << uendl;
    s << "CPU " << cpu << uendl;
    s << "useVisData 0" << uendl;
    s << "sequence -1" << uendl;
    s << "timages -1 " << first_image << ' ' << image_cnt << uendl;
    s << "oimages 0" << uendl;
    s << "quad " << quad << endl;
    s << "maxAngle " << max_angle << endl;
}

stereoview::scene_data stereoview::parse_ply_file(logger& l, progress_tracker&& prog, const QDir& d, const QString& filename)
{
    scene_data scenedata(d);
    auto filepath = d.absoluteFilePath(filename);
    QString&& ins = read_file_for_parsing(filepath);
    QTextStream s(&ins);

    unsigned int num_vertex;
    {
        QString trash;
        s >> trash >> trash >> trash >> trash >> trash >> trash;
        s >> num_vertex;
    }

    for (QString str; str != "end_header"; s >> str);

    l.msg(nfmt<2>("parsing PMVS output file '%1': %2 vertex expected...") (filepath) (num_vertex));
    auto stepper = prog.section(num_vertex);
    for (size_t i = 0; i < num_vertex; ++i)
    {
        color_point p;
        {
            QString trash;
            unsigned int r, g, b;
            s >> p.point.x >> p.point.y >> p.point.z
              >> trash >> trash >> trash
              >> r >> g >> b;
            p.r = r;
            p.g = g;
            p.b = b;
        }
        check_parse_stream_not_past_end(s, filepath);
        scenedata.vertices.append(p);
        ++stepper;
    }
    l.msg(nfmt<1>("stored %1 vertex") (num_vertex));

    return scenedata;
}

stereoview::scene_data stereoview::recon_scene_pmvs(logger& l, progress_tracker&& prog, const QDir& temp, const pmvs_options& opt)
{
    using namespace ::details_stereoview;
    auto stepper = prog.section(4);

    // run Bundle2PMVS
    {
        run_config cfg;
        cfg.cmd() = config::stereoview::bundler::bundle2pmvs_filename;
        cfg.args << "list.txt" << nfmt<2>("%1/%2") (config::stereoview::bundler::out_dirname) (config::stereoview::bundler::out_filename);
        run_external<ext::exe>(l, cfg, temp);
        ++stepper;
    }

    // convert generated prep-PMVS shell script into Unix text-file format
    {
        run_config cfg;
        cfg.cmd() = config::stereoview::tools::dos2unix_filename;
        cfg.args << config::stereoview::bundler::prep_pmvs_sh_filename;
        run_external<ext::exe>(l, cfg, temp);
        ++stepper;
    }

    // run PMVS-prep shell script
    {
        run_config cfg;
        cfg.cmd() = config::stereoview::bundler::prep_pmvs_sh_filename;
        cfg.prepend_env("PATH", QDir(config::stereoview::bundler::base_path).absolutePath());
        run_external<ext::sh>(l, cfg, temp);
        ++stepper;
    }

    // run PMVS
    {
        QDir ptemp = temp;
        ptemp.cd("pmvs");
        run_config cfg;

        ptemp.cd("visualize");
        const unsigned int cnt = ptemp.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::NoSort).size();
        if (cnt == 0) throw localized_runtime_error(HERE("empty image directory for PMVS"));
        ptemp.cdUp();

        cfg.cmd() = config::stereoview::pmvs::exe_filename;
        cfg.args << "./" << opt.filename;
        cfg.prepend_env("PATH", config::stereoview::pmvs::dll_paths);
        opt.commit(ptemp, 0, cnt);
        run_external<ext::exe>(l, cfg, ptemp);

        ptemp.cd("models");
        const scene_data&& scene = parse_ply_file(l, prog, ptemp, nfmt<1>("%1.ply") (opt.filename));

        // save VTK output file
        save_as_vtk(l, ptemp.absoluteFilePath(nfmt<1>("%1.vtk") (opt.filename)), scene.vertices, QVector<bundler_camera>());
        return scene;
    }
}

//
//
//
// VTK stuff

void stereoview::vtk_scene::save_as_vtk(logger& l, const QString& fname) const
{
    l.msg(nfmt<1>("saving VTK file: %1...") (fname));

    QFile modelf(fname);
    if (!modelf.open(QFile::WriteOnly | QIODevice::Text | QFile::Truncate))
        throw localized_runtime_error(HERE(nfmt<1>("cannot open VTK file for writing: %1") (QFileInfo(modelf).absoluteFilePath())));
    QTextStream model(&modelf);

    model << "# vtk DataFile Version 2.0\ncvlab\nASCII\nDATASET POLYDATA" << uendl;
    model << "POINTS " << points.size() + 3 * triangles.size() << " float" << uendl;

    // write xyz data of the points first
    if (!points.empty())
    {
        std::list<color_point>::const_iterator it(points.begin()), last_pt(points.end());
        for (; it != last_pt; ++it)
        {
            model << it->point << uendl;
        }
    }

    // write xyz data of the triangles
    if (!triangles.empty())
    {
        std::list<triangle_idx>::const_iterator it_tri(triangles.begin()), last_tri(triangles.end());
        for(; it_tri != last_tri; ++it_tri)
        {
            model << it_tri->p0 << uendl << it_tri->p1 << uendl << it_tri->p2 << uendl;
        }
    }

    // write the vertices
    if (!points.empty())
    {
        model << "VERTICES " << points.size() << " " << 2 * points.size() << uendl;
        for (uint32_t k=0; k<points.size(); ++k)
        {
            model << "1 " << k << uendl;
        }
    }

    // write the triangles
    if (!triangles.empty())
    {
        model << "POLYGONS " << triangles.size() << " " << triangles.size() * 4 << uendl;
        uint32_t tri = points.size();
        for (uint32_t k=0; k<triangles.size(); ++k, tri+=3)
        {
            model << "3 " << tri << " " << tri + 1 << " " << tri + 2 << uendl;
        }
    }

    // write the colors - all the triangles are assigned white colors
    model << "\nPOINT_DATA " << points.size() + 3 * triangles.size() << "\nVECTORS cvlab_colors unsigned_char" << uendl;
    if (!points.empty())
    {
        std::list<color_point>::const_iterator it(points.begin()), last_pt(points.end());
        for (; it!=last_pt; ++it)
        {
            const auto& cp = *it;
            model << (unsigned int)cp.r << " " << (unsigned int)cp.g << " " << (unsigned int)cp.b << uendl;
        }
    }
    if (!triangles.empty())
    {
        for (uint32_t k=0; k<triangles.size(); ++k)
        {
            model << "255 255 255\n255 255 255\n255 255 255" << uendl;
        }
    }

    model << uendl;
}

inline void correct_point3d_for_bundler(cvlab::point3d& p)
{
    p.y = -p.y;
    p.z = -p.z;
}

void stereoview::save_as_vtk(logger& l,
        const QString&                 fname,
        const QVector<color_point>&    points,
        const QVector<bundler_camera>& cameras
        ) const
{
    vtk_scene scene;

    const uint32_t w = width(), h = height();
    const cvlab::point2d center(w / 2., h / 2.);

    scene.add_points(points);

    for (int k = 0; k < cameras.size(); ++k) {

        const bundler_camera& cam = cameras[k];

        cvlab::point3d tl( -center.x / cam.f, -center.y / cam.f, 1. );
        cvlab::point3d tr( (w - center.x) / cam.f, tl.y, 1. );
        cvlab::point3d bl( tl.x, (h - center.y) / cam.f, 1. );
        cvlab::point3d br( tr.x, bl.y, 1. );

        correct_point3d_for_bundler(tl);
        correct_point3d_for_bundler(tr);
        correct_point3d_for_bundler(bl);
        correct_point3d_for_bundler(br);

        cvlab::rotate_translate(tl, cam.r, cam.t);
        cvlab::rotate_translate(tr, cam.r, cam.t);
        cvlab::rotate_translate(bl, cam.r, cam.t);
        cvlab::rotate_translate(br, cam.r, cam.t);

        scene.add_triangle(tl, tr, bl);
        scene.add_triangle(tr, br, bl);

        {
            const cvlab::point3d& c = cam.t;
            scene.add_triangle(c, tl, tr);
            scene.add_triangle(c, tl, bl);
            scene.add_triangle(c, tr, br);
            scene.add_triangle(c, bl, br);
        }

    } // next camera

    scene.save_as_vtk(l, fname);
}
