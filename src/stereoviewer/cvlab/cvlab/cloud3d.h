#ifndef CVLAB_CLOUD3D_H
#define CVLAB_CLOUD3D_H

#include <list>
#include <vector>
#include <string>
#include "math3d.h"

#define FMT_XYZ		0
#define FMT_VTK		1
#define PLANE_XY	2
#define PLANE_XZ	1
#define PLANE_YZ	0
#define CLIP_FRONT	1
#define CLIP_BACK	2

using std::list;
using std::vector;
using std::string;

#include <fstream>
#include <algorithm> // for min_element() and max_element()

#define NO_COLOR    -1

namespace cvlab {

typedef unsigned char uchar;

/**
 * This class defines a cloud of 3D points. Points can be added or removed,
 * and the cloud can be clipped, moved, visualized, written to file and so on.
 */
class cloud3d {

   public:

      typedef std::list<point3d>::const_iterator const_iterator;

      cloud3d();
      cloud3d(const cloud3d&);
      cloud3d(const vector<point3d>&);
      cloud3d(const string&);

      cloud3d& clear();

      const_iterator begin() const { return points.begin(); }
      const_iterator end() const { return points.end(); }

      unsigned int n_points() const;
      void add_point(const point3d&);
      void add_point(const point3d&,uchar,uchar,uchar);
      void add_point(double,double,double);
      void add_point(double,double,double,uchar,uchar,uchar);

      point3d get_barycenter() const;

      cloud3d& filterOutliersBoundVoxel(int min_count);
      cloud3d& remove_outliers_lof(unsigned int,double);
      cloud3d& keep_biggest_clusters(unsigned int K, unsigned int n=1);
      cloud3d& overlap(const cloud3d&);
      cloud3d& overlap(const vector<point3d>&);

      vector<point3d> getPointsVec() const;
      void assign(const vector<point3d>& pts);
      void assign(const cloud3d&);
      double minX() const;
      double maxX() const;
      double minY() const;
      double maxY() const;
      double minZ() const;
      double maxZ() const;

      void load_from_vtk(const string&);
      void load_from_xyzn(const string&);
      void load_from_ply(const string&);
      void save(const string&,int) const;
      void save_as_text(const string&) const;

      static double squareDist(const point3d& p1, const point3d& p2);

      cloud3d& operator=(const cloud3d& o) {
            assign(o);
            return *this;
      }

      cloud3d& operator=(const vector<point3d>& p) {
          assign(p);
          return *this;
      }

   private:
      list<point3d> points; // a list is used because there can be many removals
      unsigned int _npoints;

      cloud3d& clip_plane(int plane,double D,int where);
      void HOD_global();
};

inline cloud3d& cloud3d::clear() {
    points.clear();
    _npoints = 0;
    return *this;
}

inline unsigned int cloud3d::n_points() const {
   return _npoints;
}

}

#endif // CVLAB_CLOUD3D_H
