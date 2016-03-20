
#include "config.h"

namespace config
{
    using ::prelude::nfmt;
    using ::prelude::fmt;

    const char log_filename[] = "log.txt";

    //
    // UI configuration

    namespace ui
    {
        const int max_image_width = 1000, max_image_height = 1000;
        const double min_view_scale = 0.2, max_view_scale = 4.0;

        const float tracking_error_threshold = 4000.0f;

        const QEvent::Type main_window_custom_event_type = static_cast<QEvent::Type>(QEvent::registerEventType());

        QColor keypoint_color()
        {
            using prelude::rnd;
            static int last_hue_band;
            static const int hue_bands = 12;
            int hue_band = rnd(0, hue_bands - 1);
            if (hue_band == last_hue_band) hue_band = (hue_band + 1) % hue_bands;
            last_hue_band = hue_band;
            return QColor::fromHsv(hue_band * 360 / hue_bands, rnd(0xcc, 0xff), rnd(0xdd, 0xff));
        }

        const size_t correspondence_palette_dim = 5000u;
        const qreal keypoint_width = 1.7, correspondence_width = 3.5;
        const int keypoint_z = 10, correspondence_z = 20;
        const double keypoint_opacity = 0.8, correspondence_opacity = 0.87;

        namespace defaults
        {
            const bool rescale_enabled = false;
            const unsigned int rescale_width = 640, rescale_height = 480;

            const bool statusbar_logger_enabled = true;
        }
    }

    //
    // stereoview module configuration

    namespace stereoview
    {
        namespace defaults
        {
            const int temp_saved_image_quality = 100;

            namespace sift
            {
                float adapt_scale(float x) { return x; }
                float adapt_rotation(float x) { return x; }
                float adapt_descriptor(float x) { return x; }
            }

            namespace siftpp
            {
                float adapt_scale(float x) { return x; }
                float adapt_rotation(float x) { return x; }
                float adapt_descriptor(unsigned int x) { return x; }

                const unsigned int octaves = 3,
                                   octave_levels = 3,
                                   first_octave = -1;

                const double edge_threshold = 6.0,
                             peak_threshold = 1.0,
                             magnification_factor = 1.0;

                const bool orientations = true;
            }

            namespace cvsurf
            {
                const double hessian_threshold = 300;
                const unsigned int octave_layers = 4u, octaves = 3u;

                float adapt_scale(int x) { return float(x); }
                float adapt_rotation(float x) { return x / 180.0f * prelude::constants::pi; }
                float adapt_descriptor(float x) { return x; }
            }

            namespace opensurf
            {
                const double response_threshold = 0.0001;

                float adapt_scale(float x) { return x; }
                float adapt_rotation(float x) { return x - prelude::constants::pi; }
                float adapt_descriptor(float x) { return x; /*(x + 1.0f) * 100.0f;*/ }
            }

            namespace kmatcher
            {
                QString pretty_descriptor(float x)
                {
                    return fmt("%1") (x);
                }

                const size_t neighbours = 1u,
                             max_iterations = 15u;

                const double average_payoff_threshold = 0.6,
                             payoff_alpha = 0.001,
                             quality_threshold = 0.3;
            }

            namespace bundler_keymatchfull
            {
                QString pretty_descriptor(float x)
                {
                    return bundler::pretty_descriptor(x);
                }
            }

            namespace bundler
            {
                QString pretty_descriptor(float x)
                {
                    return fmt("%1") (static_cast<unsigned short>(x));
                }
            }

            namespace pmvs
            {
                const unsigned int level = 0u;
                const unsigned int csize = 2u;
                const unsigned int wsize = 9u;
                const double threshold = 0.8;
                const double quad = 2.5;
                const unsigned int max_angle = 5u;
            }
        }

        static inline nfmt<1> make_base_fmt(const QString& base_path)
        {
            return nfmt<2>("%1/%2") (base_path);
        }

        //const char base_path[] = "C:/Documents and Settings/Manta/Documenti/Unive/stereorecon/src";
        //const char base_path[] = "C:/Documents and Settings/rodola/Desktop/workspace/stereorecon/src";
        const char base_path[] = "../..";

        const nfmt<1> base_fmt = make_base_fmt(base_path);

        namespace tools
        {
            const QString base_path = stereoview::base_fmt("tools");
            const nfmt<1> base_fmt = make_base_fmt(base_path);

            const QString dos2unix_filename = base_fmt("dos2unix.exe");
        }

        namespace opensurf
        {
            const QString base_path = stereoview::base_fmt("letsurfagain/release");
            const nfmt<1> base_fmt = make_base_fmt(base_path);

            const char temp_dirname_prefix[] = "stereoview_opensurf";
            const QString exe_filename = base_fmt("letsurfagain.exe");
        }

        namespace sift
        {
            const char temp_dirname_prefix[] = "stereoview_sift";
            const QString exe_filename = tools::base_fmt("siftWin32.exe");
        }

        namespace siftpp
        {
            const char temp_dirname_prefix[] = "stereoview_siftpp";
            const QString exe_filename = tools::base_fmt("sift.exe");
        }

        namespace kmatcher
        {
            const QString base_path = stereoview::base_fmt("kmatcher/release");
            const nfmt<1> base_fmt = make_base_fmt(base_path);

            const char keylist_filename[] = "list_keys.txt";
            const char output_matches_filename[] = "matches.init.txt";
            const nfmt<1> partial_output_matches_filename_fmt = "matches_%1.txt";

            const QString exe_filename = base_fmt("kmatcher.exe");
        }

        namespace bundler
        {
            const QString base_path = stereoview::base_fmt("bundler/bin");
            const nfmt<1> base_fmt = make_base_fmt(base_path);

            const char temp_dirname_prefix[] = "stereoview_recon";
            const int temp_file_quality = 100;

            // bundler2pmvs
            const QString bundle2pmvs_filename = base_fmt("Bundle2PMVS.exe");
            const char prep_pmvs_sh_filename[] = "./prep_pmvs.sh";

            // bundler.exe
            const QString exe_filename = base_fmt("bundler.exe");
            const char out_filename[] = "bundle.out";
            const char out_dirname[] = "bundle";
            const QString log_filename = "out.log";
            const QStringList exe_options = QStringList()
                << "--match_table" << kmatcher::output_matches_filename
                << "--output" << out_filename
                //<< "--output_all" << "bundle_"
                << "--output_dir" << out_dirname
                << "--use_focal_estimate"
                << "--constrain_focal"
                << "--constrain_focal_weight" << "0.0001"
                << "--variable_focal_length"
                << "--estimate_distortion"
                //<< "--ray_angle_threshold" << "2.0"
                //<< "--projection_estimation_threshold" << "4"
                //<< "--min_proj_error_threshold" << "8"
                //<< "--max_proj_error_threshold" << "16"
                //<< "--slow_bundle"
                << "--run_bundle";

            // DINO da 20 a 37
        }

        // list.txt --match_table matches.init.txt --output bundle.out --output_dir bundle --use_focal_estimate --constrain_focal --constrain_focal_weight 0.0001 --variable_focal_length --estimate_distortion --run_bundle

        namespace bundler_keymatchfull
        {
            const QString base_path = bundler::base_path;
            const nfmt<1> base_fmt = make_base_fmt(base_path);

            const char keylist_filename[] = "list_keys.txt";
            const char output_matches_filename[] = "matches.init.txt";

            const QString exe_filename = base_fmt("KeyMatchFull.exe");
        }

        namespace pmvs
        {
            const QString base_path = stereoview::base_fmt("pmvs-2/program/main");
            const nfmt<1> base_fmt = make_base_fmt(base_path);

            const char dll_paths[] = "C:/cygwin/lib/lapack";
            const QString exe_filename = base_fmt("pmvs2.exe");
            const char option_filename[] = "option.txt";
            const unsigned int min_image_num = 2u;
        }
    }

}
