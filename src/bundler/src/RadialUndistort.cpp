/* RadialUndistort.cpp */
/* Undo radial distortion */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <string.h>

#include "sfm.h"

#include "color.h"
#include "image.h"
#include "resample.h"
#include "util.h"

// #include "jpegcvt.h"
#include "LoadJPEG.h"

typedef struct
{
    int image;
    int key;
    double x;
    double y;
} view_t;

typedef struct 
{
    double pos[3];
    double color[3];
    std::vector<view_t> views;
} point_t;

void ReadBundleFile(char *bundle_file, 
                    std::vector<camera_params_t> &cameras,
                    std::vector<point_t> &points)
{
    FILE *f = fopen(bundle_file, "r");
    if (f == NULL) {
	printf("Error opening file %s for reading\n", bundle_file);
	return;
    }

    int num_images, num_points;
    double bundle_version;

    char first_line[256];
    fgets(first_line, 256, f);
    if (first_line[0] == '#') {
        double version;
        sscanf(first_line, "# Bundle file v%lf", &version);

        bundle_version = version;
        printf("[ReadBundleFile] Bundle version: %0.3f\n", version);

        fscanf(f, "%d %d\n", &num_images, &num_points);
    } else if (first_line[0] == 'v') {
        double version;
        sscanf(first_line, "v%lf", &version);
        bundle_version = version;
        printf("[ReadBundleFile] Bundle version: %0.3f\n", version);

        fscanf(f, "%d %d\n", &num_images, &num_points);
    } else {
        bundle_version = 0.1;
        sscanf(first_line, "%d %d\n", &num_images, &num_points);
    }

    printf("[ReadBundleFile] Reading %d images and %d points...\n",
	   num_images, num_points);

    /* Read cameras */
    for (int i = 0; i < num_images; i++) {
	double focal_length, k0, k1;
	double R[9];
	double t[3];
        
        if (bundle_version < 0.2) {
            /* Focal length */
            fscanf(f, "%lf\n", &focal_length);
        } else {
            fscanf(f, "%lf %lf %lf\n", &focal_length, &k0, &k1);
        }

	/* Rotation */
	fscanf(f, "%lf %lf %lf\n%lf %lf %lf\n%lf %lf %lf\n", 
	       R+0, R+1, R+2, R+3, R+4, R+5, R+6, R+7, R+8);
	/* Translation */
	fscanf(f, "%lf %lf %lf\n", t+0, t+1, t+2);

        camera_params_t cam;

        cam.f = focal_length;
        cam.k[0] = k0;
        cam.k[1] = k1;
        memcpy(cam.R, R, sizeof(double) * 9);
        memcpy(cam.t, t, sizeof(double) * 3);

        /* Flip the scene if needed */
        if (bundle_version < 0.3) {
            R[2] = -R[2];
            R[5] = -R[5];
            R[6] = -R[6];
            R[7] = -R[7];
            t[2] = -t[2];
        }

        cameras.push_back(cam);
    }

    /* Read points */
    for (int i = 0; i < num_points; i++) {
	point_t pt;

	/* Position */
	fscanf(f, "%lf %lf %lf\n", 
	       pt.pos + 0, pt.pos + 1, pt.pos + 2);

	/* Color */
	fscanf(f, "%lf %lf %lf\n", 
	       pt.color + 0, pt.color + 1, pt.color + 2);

	int num_visible;
	fscanf(f, "%d", &num_visible);

	for (int j = 0; j < num_visible; j++) {
	    int image, key;
	    fscanf(f, "%d %d", &image, &key);

            double x, y;
            if (bundle_version >= 0.3)
                fscanf(f, "%lf %lf", &x, &y);

            view_t view;
            view.image = image;
            view.key = key;
            view.x = x;
            view.y = y;

            pt.views.push_back(view);
	}

        if (bundle_version < 0.3)
            pt.pos[2] = -pt.pos[2];

        if (num_visible > 0) {
            points.push_back(pt);
        }
    }

    fclose(f);
}

void ReadListFile(char *list_file, std::vector<std::string> &files)
{
    FILE *f = fopen(list_file, "r");
    
    char buf[256];
    while (fgets(buf, 256, f)) {
        if (buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = 0;

        char *space = strchr(buf, ' ');
        if (space) *space = 0;

        files.push_back(std::string(buf));
    }

    fclose(f);
}

void WriteBundleFile(const char *bundle_file, 
                     const std::vector<camera_params_t> &cameras,
                     const std::vector<point_t> &points)
{
    FILE *f = fopen(bundle_file, "w");
    if (f == NULL) {
	printf("Error opening file %s for reading\n", bundle_file);
	return;
    }

    int num_images = cameras.size();
    int num_points = points.size();

    int *remap = new int[num_images];

    /* Count the number of good images */
    int num_good_images = 0;
    for (int i = 0; i < num_images; i++) {
        remap[i] = -1;

        if (cameras[i].f == 0)
            continue;
        
        remap[i] = num_good_images;

        num_good_images++;
    }

    printf("[WriteBundleFile] Writing %d images and %d points...\n",
	   num_good_images, num_points);

    fprintf(f, "# Bundle file v0.3\n");
    fprintf(f, "%d %d\n", num_good_images, num_points);    

    /* Write cameras */
    for (int i = 0; i < num_images; i++) {
        if (cameras[i].f == 0)
            continue;

        /* Focal length */
        fprintf(f, "%lf 0.0 0.0\n", cameras[i].f);

	/* Rotation */
        const double *R = cameras[i].R;
	fprintf(f, "%lf %lf %lf\n%lf %lf %lf\n%lf %lf %lf\n", 
                R[0], R[1], R[2], R[3], R[4], R[5], R[6], R[7], R[8]);

	/* Translation */
        const double *t = cameras[i].t;
	fprintf(f, "%lf %lf %lf\n", t[0], t[1], t[2]);
    }
    
    /* Write points */
    for (int i = 0; i < num_points; i++) {
	/* Position */
        const double *pos = points[i].pos;
	fprintf(f, "%lf %lf %lf\n", pos[0], pos[1], pos[2]);

	/* Color */
        const double *color = points[i].color;
	fprintf(f, "%d %d %d\n", 
                iround(color[0]), iround(color[1]), iround(color[2]));

	int num_visible;
        num_visible = points[i].views.size();
	fprintf(f, "%d", num_visible);

	for (int j = 0; j < num_visible; j++) {
	    int view = points[i].views[j].image;
            int key = points[i].views[j].key;
            double x = points[i].views[j].x;
            double y = points[i].views[j].y;

	    fprintf(f, " %d %d %0.2f %0.2f", remap[view], key, x, y);
	}

        fprintf(f, "\n");
    }

    fclose(f);
}

void UndistortImage(const std::string &in, 
                    const camera_params_t &camera,
                    const std::string &out)
{ 
    printf("Undistorting image %s\n", in.c_str());
    fflush(stdout);

    img_t *img = LoadJPEG(in.c_str());
    int w = img->w;
    int h = img->h;
    
    img_t *img_out = img_new(w, h);
   
    double f2_inv = 1.0 / (camera.f * camera.f);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            double x_c = x - 0.5 * w;
            double y_c = y - 0.5 * h;
            
            double r2 = (x_c * x_c + y_c * y_c) * f2_inv;
            double factor = 1.0 + camera.k[0] * r2 + camera.k[1] * r2 * r2;
            
            x_c *= factor;
            y_c *= factor;

            x_c += 0.5 * w;
            y_c += 0.5 * h;
            
            fcolor_t c;
            if (x_c >= 0 && x_c < w - 1 && y_c >= 0 && y_c < h - 1) {
                c = pixel_lerp(img, x_c, y_c);
            } else {
                c = fcolor_new(0.0, 0.0, 0.0);
            }
            
            img_set_pixel(img_out, x, y, 
                          iround(c.r), iround(c.g), iround(c.b));
        }
    }

    // img_write_bmp_file(img_out, (char *) out.c_str());
    WriteJPEG(img_out, (char *) out.c_str());

    img_free(img);
    img_free(img_out);
}

void UndistortImages(const std::vector<std::string> &files, 
                     const std::vector<camera_params_t> &cameras)
{
    int num_files = (int) files.size();
    assert(files.size() == cameras.size());

    for (int i = 0; i < num_files; i++) {
        if (cameras[i].f == 0.0)
            continue;

        std::string in = files[i];
        // std::string out = in;
        // int len = out.length();
        
        // out[len-3] = 'b';
        // out[len-2] = 'm';
        // out[len-1] = 'p';

        std::string out = in.substr(0, in.length() - 3).append("rd.jpg");

        UndistortImage(in, cameras[i], out);
    }
}

void WriteNewFiles(const std::vector<std::string> &files, 
                   const std::vector<camera_params_t> &cameras,
                   const std::vector<point_t> &points)
{
    FILE *f = fopen("list.rd.txt", "w");

    int num_files = (int) files.size();
    for (int i = 0; i < num_files; i++) {
        if (cameras[i].f == 0.0)
            continue;

        fprintf(f, "%s\n", files[i].c_str());
    }

    fclose(f);

    WriteBundleFile("bundle.rd.out", cameras, points);
}

int main(int argc, char **argv) 
{
    if (argc != 3) {
        printf("Usage: %s <list.txt> <bundle.out>\n", argv[0]);
        return 1;
    }
    
    char *list_file = argv[1];
    char *bundle_file = argv[2];

    /* Read the bundle file */
    std::vector<camera_params_t> cameras;
    std::vector<point_t> points;
    std::vector<std::string> files;

    ReadListFile(list_file, files);
    ReadBundleFile(bundle_file, cameras, points);

    WriteNewFiles(files, cameras, points);
    UndistortImages(files, cameras);

    return 0;
}
