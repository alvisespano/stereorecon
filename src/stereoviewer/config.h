#ifndef CONFIG_H
#define CONFIG_H

#include "prelude.h"

namespace config
{
    using ::prelude::nfmt;

    extern const char log_filename[];

    //
    // UI configuration

    namespace ui
    {
        extern const int max_image_width, max_image_height;
        extern const double min_view_scale, max_view_scale;

        extern const float tracking_error_threshold;

        extern const QEvent::Type main_window_custom_event_type;

        extern QColor keypoint_color();
        extern const size_t correspondence_palette_dim;
        extern const qreal keypoint_width, correspondence_width;
        extern const int keypoint_z, correspondence_z;
        extern const double keypoint_opacity, correspondence_opacity;

        namespace defaults
        {
            extern const bool rescale_enabled;
            extern const unsigned int rescale_width, rescale_height;
            extern const bool statusbar_logger_enabled;
        }
    }

    //
    // stereoview module configuration

    namespace stereoview
    {
        namespace defaults
        {
            extern const int temp_saved_image_quality;

            namespace sift
            {
                extern float adapt_scale(float x);
                extern float adapt_rotation(float x);
                extern float adapt_descriptor(float x);
            }

            namespace siftpp
            {
                extern float adapt_scale(float x);
                extern float adapt_rotation(float x);
                extern float adapt_descriptor(unsigned int x);

                extern const unsigned int octaves, octave_levels, first_octave;
                extern const double edge_threshold, peak_threshold, magnification_factor;
                extern const bool orientations;
            }

            namespace cvsurf
            {
                extern const double hessian_threshold;
                extern const unsigned int octave_layers, octaves;
                extern float adapt_scale(int size);
                extern float adapt_rotation(float x);
                extern float adapt_descriptor(float component);
            }

            namespace opensurf
            {
                extern const double response_threshold;
                extern float adapt_scale(float x);
                extern float adapt_rotation(float x);
                extern float adapt_descriptor(float x);
            }

            namespace kmatcher
            {
                extern QString pretty_descriptor(float x);

                extern const size_t neighbours, max_iterations;
                extern const double average_payoff_threshold, payoff_alpha, quality_threshold;
            }

            namespace bundler_keymatchfull
            {
                extern QString pretty_descriptor(float x);
            }

            namespace bundler
            {
                extern QString pretty_descriptor(float x);
            }

            namespace pmvs
            {
                extern const unsigned int level;
                extern const unsigned int csize;
                extern const unsigned int wsize;
                extern const double threshold;
                extern const double quad;
                extern const unsigned int max_angle;
            }
        }

        extern const char base_path[];
        extern const nfmt<1> base_fmt;

        namespace tools
        {
            extern const QString base_path;
            extern const nfmt<1> base_fmt;

            extern const QString dos2unix_filename;
        }

        namespace opensurf
        {
            extern const QString base_path;
            extern const nfmt<1> base_fmt;

            extern const char temp_dirname_prefix[];
            extern const QString exe_filename;
        }

        namespace sift
        {
            extern const char temp_dirname_prefix[];
            extern const QString exe_filename;
        }

        namespace siftpp
        {
            extern const char temp_dirname_prefix[];
            extern const QString exe_filename;
        }

        namespace kmatcher
        {
            extern const QString base_path;
            extern const nfmt<1> base_fmt;

            extern const char keylist_filename[];
            extern const char output_matches_filename[];
            extern const nfmt<1> partial_output_matches_filename_fmt;
            extern const QString exe_filename;
        }

        namespace bundler_keymatchfull
        {
            extern const QString base_path;
            extern const nfmt<1> base_fmt;

            extern const char keylist_filename[];
            extern const char output_matches_filename[];
            extern const QString exe_filename;
        }

        namespace bundler
        {
            extern const QString base_path;
            extern const nfmt<1> base_fmt;

            extern const char temp_dirname_prefix[];

            // bundler2pmvs
            extern const QString bundle2pmvs_filename;
            extern const char prep_pmvs_sh_filename[];

            // bundler.exe
            extern const QString exe_filename;
            extern const char out_filename[];
            extern const char out_dirname[];
            extern const QString log_filename;
            extern const QStringList exe_options;
        }

        namespace pmvs
        {
            extern const QString base_path;
            extern const nfmt<1> base_fmt;

            extern const char dll_paths[];
            extern const QString exe_filename;
            extern const char option_filename[];
            extern const unsigned int min_image_num;
        }
    }

}

#endif // CONFIG_H
