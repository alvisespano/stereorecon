#include "cloud3d.h"

#include <functional>
#include <iomanip> // for setprecision()
#include <sstream>
#include <stdexcept>
#include <iostream>
#include "exceptions.h"
#include "../string_utilities.h"

using std::cout;
using std::endl;

namespace cvlab {

cloud3d::cloud3d() : _npoints(0) {}

/**
 * Populates the current point cloud with a given vector of 3D points.
 * The previous state is permanently erased.
 *
 * @param pts The vector of 3D points for populating the point cloud.
 */
void cloud3d::assign(const vector<point3d>& pts) {
   points.clear();
   _npoints = pts.size();
   points.resize(_npoints);
   std::copy(pts.begin(),pts.end(),points.begin());
}

void cloud3d::assign(const cloud3d& o) {
    _npoints = o._npoints;
    points.clear();
    points.insert(points.end(),o.points.begin(),o.points.end());
}

cloud3d::cloud3d(const vector<point3d>& pts) {
   assign(pts);
}

cloud3d::cloud3d(const cloud3d& o) {
    assign(o);
}

void cloud3d::add_point(const point3d& p) {
   points.push_back(p);
   ++_npoints;
}

void cloud3d::add_point(double x, double y, double z) {
   points.push_back(point3d(x,y,z));
   ++_npoints;
}

string ntos2(float f) {
   std::ostringstream s;
   s << std::fixed << std::setprecision(10) << f;
   return s.str();
}

/**
 *
 * @return
 * @throw std::logic_error
 */
point3d cloud3d::get_barycenter() const {
   if (points.empty())
      throw std::logic_error("Cannot calculate barycenter for an empty cloud.");

   point3d centroid;

   list<point3d>::const_iterator it(points.begin()), it_end(points.end());
   while(it!=it_end){
      centroid += *it;
      ++it;
   }

   return centroid / double(points.size());
}

/**
 * Merge the current point cloud with a given one.
 *
 * @param c The input point cloud to add to the current one.
 * @return A reference to the current object, which now includes the new point cloud.
 */
cloud3d& cloud3d::overlap(const cloud3d& c) {
   /*list<point3d> l(c.getPointsList());
   list<point3d>::const_iterator it(l.begin()), it_end(l.end());
   while(it!=it_end){
      points.push_back(*it);
      ++it;
   }*/
   points.insert(points.end(),c.points.begin(),c.points.end());
   _npoints = points.size();
   return *this;
}

cloud3d& cloud3d::overlap(const vector<point3d>& p) {
   vector<point3d>::const_iterator it(p.begin()), it_end(p.end());
   while(it!=it_end){
      points.push_back(*it);
      ++it;
   }
   _npoints += p.size();
   return *this;
}

/**
 * Copy the point cloud to a vector of 3D points and return this vector.
 *
 * @return The vector of 3D points contained in the point cloud.
 */
vector<point3d> cloud3d::getPointsVec() const {
   vector<point3d> vec(_npoints);
   list<point3d>::const_iterator it(points.begin()), it_end(points.end());
   unsigned int k=0;
   while (it!=it_end) {
       vec[k++] = point3d(it->x,it->y,it->z);
       ++it;
   }
   return vec;
}

/**
 *  Writes the current cloud of points to disk.
 *
 * @param fname Destination filename. The extension is not needed since it will be added by the function depending on the requested format.
 * @param format Desired file format. Currently it can be either FMT_XYZ or FMT_VTK.
 */
void cloud3d::save(const string& fname, int format) const {

   string ext;
   if (format==FMT_XYZ)
      ext.assign(".xyz");
   else if (format==FMT_VTK)
      ext.assign(".vtk");
   else
      fprintf(stderr,"Error: unknown format.\n");

   cout << "Saving to " << fname << ext << "..." << std::flush;

   std::ofstream model((fname+ext).c_str(), std::ios::out);
   list<point3d>::const_iterator it(points.begin()), it_end(points.end());

   if (format==FMT_VTK) {
      model << "# vtk DataFile Version 2.0\nloop\nASCII\nDATASET UNSTRUCTURED_GRID" << endl;
      model << "POINTS " << _npoints << " float" << endl;

      // write xyz data
      while(it!=it_end) {
         model << std::fixed << std::setprecision(10) << it->x << " " << it->y << " " << it->z << endl;
         ++it;
      }

      model << "CELLS " << _npoints << " " << 2*_npoints << endl;

      // write cell data
      for (unsigned int i=0; i<_npoints; ++i)
         model << "1 " << i << "\n";

      // write cell types
      model << "CELL_TYPES " << _npoints << endl;
      for (unsigned int i=0; i<_npoints; ++i)
         model << "1 \n";

      // write z scalar values
      model << "\nPOINT_DATA " << _npoints << "\nVECTORS ScanColors unsigned_char" << endl;
      it = points.begin();
      while (it!=it_end){
         //model << (unsigned int)it->r << " " << (unsigned int)it->g << " " << (unsigned int)it->b << "\n";
         model << 255 << " " << 255 << " " << 255 << "\n";
         ++it;
      }
      model << endl;
   }

   else if (format==FMT_XYZ) {
      while (it!=it_end) {
         model << it->x << " " << it->y << " " << it->z << endl;
         ++it;
      }
   }

   else
      fprintf(stderr,"Error: unknown format.\n");


   model.close();

   cout << " done." << endl;
}

/**
 * As for now, the expected file format is a PLY containing only vertices, one line per vertex.
 *
 * @param fname Input filename of the .ply file.
 * @throw cvlab::file_not_found
 */
void cloud3d::load_from_ply(const string& fname) {

   using namespace strutils;

   cout << "Loading point cloud from " << fname << "... " << std::flush;

   std::ifstream file(fname.c_str(), std::ios::in);
   if (!file.is_open() || file.fail())
      throw cvlab::file_not_found("Cannot read "+fname);

   string line;
   vector<string> tokens;
   _npoints = 0;

   // Read in the number of points

   do {
      tokens.clear();
      std::getline(file, line);
      tokenize_str(line, tokens, " ");

      if (tokens.size() == 3) {

         if (tokens[0]=="element" && tokens[1]=="vertex") {
            _npoints = convert_str<uint32_t>(tokens[2]);

            do {
               tokens.clear();
               std::getline(file,line);
               tokenize_str(line, tokens, " ");
            } while(tokens[0] != "end_header");

            break;
         }
      }
   } while (!file.eof());

   if (_npoints == 0)
      throw std::runtime_error("Can't load any point "+fname);

   // We are now at the right position to start reading in vertices

   uint32_t cur_l = 0;
   while (cur_l<_npoints){
      tokens.clear();
      std::getline(file, line);
      tokenize_str(line, tokens, " ");
      points.push_back( point3d(
            convert_str<double>(tokens[0]),
            convert_str<double>(tokens[1]),
            convert_str<double>(tokens[2])) );
      ++cur_l;
   }

   file.close();

   cout << "done." << endl;
}

double cloud3d::squareDist(const point3d& p1, const point3d& p2) {
   double x = p1.x - p2.x;
   double y = p1.y - p2.y;
   double z = p1.z - p2.z;
   return ((x*x) + (y*y) + (z*z));
}

string to_string(const point3d& p) {
    string ret(ntos2(p.x));
    ret += "+" + ntos2(p.y);
    ret += "+" + ntos2(p.z);
    return ret;
}

}
