/*
  Emanuele Rodol√  <rodola@dsi.unive.it>
*/

#include "mesh.h"
#include "kdtree.h"
#include "stl_file.h"
#include "exceptions.h"
#include "../string_utilities.h"
#include <fstream>

// These are included for debugging purposes
//#include <CImg.h>
#include "cv.h"
#include "cloud3d.h"

using std::vector;
using std::string;
using std::cout;
using std::endl;

#define INVALID_VERTEX_INDEX  -1

namespace cvlab {

   mesh::~mesh() throw() {
      if (tree)
         delete tree;
   }





   // ______________________________________________TEST________________________________________________
   void mesh::test_spin() {

      const size_t selected = 500;
      const vertex_data& P = vertices.at(selected);

      const double resolution = get_resolution();
      const double bin_size = 1.0 * resolution;
      const uint32_t image_width = 200;

      spin_image spin(P.p, image_width, bin_size);
      spin.set_support_angle(60.);

      for (unsigned int k=0; k<vertices.size(); ++k) {
         if (k==selected)
            continue;
         spin.accumulate_point(vertices[k].p, 500. * resolution);
      }

      //spin.save("spin.bmp");
   }
   // ______________________________________________TEST________________________________________________
   void mesh::test_spin_fine() {

      save_as_stl("bunny.stl");

      using std::cout;
      using std::endl;
      using std::flush;

      const vertex_data& P = vertices.at(11000);

      const double resolution = get_resolution();
      const double bin_size = resolution / 4.;
      const uint32_t image_width = 70;

      spin_image spin(P.p, image_width, bin_size);
      spin.set_support_angle(60.);

      const double start_radius = resolution / 4.;
      const double max_radius = 10 * resolution;
      const double radius_step = resolution / 4.;
      const uint8_t n_angles = 100;
      const double angle_step = 2.*M_PI / n_angles;

      CvMat* R = cvCreateMat(3,3,CV_32FC1);

      // Rotation matrix
      point3d c3 = P.p.n;
      cvlab::normalize(c3);
      point3d c1(1., 1., - (c3.x + c3.y) / c3.z);
      cvlab::normalize(c1);
      point3d c2 = cvlab::cross_product(c3,c1);
      cvlab::normalize(c2);
      cvmSet(R,0,0,c1.x);
      cvmSet(R,1,0,c1.y);
      cvmSet(R,2,0,c1.z);
      cvmSet(R,0,1,c2.x);
      cvmSet(R,1,1,c2.y);
      cvmSet(R,2,1,c2.z);
      cvmSet(R,0,2,c3.x);
      cvmSet(R,1,2,c3.y);
      cvmSet(R,2,2,c3.z);

      // Test the rotation matrix
      /*std::cout << "Det: " << cvDet(R) << std::endl;
      CvMat* Rt = cvCreateMat(3,3,CV_32FC1);
      cvTranspose(R,Rt);
      cout << "transposed:" << endl;
      for(int y=0;y<3;++y){for(int x=0;x<3;++x){std::cout << cvmGet(Rt,y,x) << " ";}std::cout << std::endl;}
      cvInvert(R,Rt);
      cout << "inverted:" << endl;
      for(int y=0;y<3;++y){for(int x=0;x<3;++x){std::cout << cvmGet(Rt,y,x) << " ";}std::cout << std::endl;}
      cvReleaseMat(&Rt);*/

      // Translation
      point3d T = P.p;

      cvlab::cloud3d cloud;

      for (double angle = 0; angle < 2.*M_PI; angle += angle_step) {
         for (double r = start_radius; r < max_radius; r += radius_step) {

            // Point along the circle in a standard frame centered on (0,0,0)
            point3d orig( r*std::cos(angle), r*std::sin(angle), 0.);

            // Apply the rigid transformation to bring it over the reference point tangent plane
            point3d transformed;
            transformed.x = T.x + orig.x * cvmGet(R,0,0) + orig.y * cvmGet(R,0,1) + orig.z * cvmGet(R,0,2);
            transformed.y = T.y + orig.x * cvmGet(R,1,0) + orig.y * cvmGet(R,1,1) + orig.z * cvmGet(R,1,2);
            transformed.z = T.z + orig.x * cvmGet(R,2,0) + orig.y * cvmGet(R,2,1) + orig.z * cvmGet(R,2,2);

            cloud.add_point(transformed);

            // Finally project it onto the mesh.
            // First find the nearest neighbor mesh vertex.

            int nearest_idx = nearest_neighbor(transformed,resolution*10);
            if (nearest_idx == INVALID_VERTEX_INDEX) {
               cout << "!" << flush;
               continue;
            }

            const vertex_data& nearest = vertices.at(nearest_idx);
            int n_intersections = 0;

            if (nearest.n_tris == 0) // outlier vertex
               continue;

            // Now check all the triangles which the selected nearest vertex belongs to.
            // Only one of these is the triangle onto which the transformed point will be projected.

            point3d projected;

            std::list<size_t>::const_iterator cur_tri = nearest.tri_indices.begin(), it_end = nearest.tri_indices.end();
            for (; cur_tri != it_end; ++cur_tri) {

               const triangle_data& tri = triangles[*cur_tri];
               const point3d& p0 = vertices[tri.p0].p;
               const point3d& p1 = vertices[tri.p1].p;
               const point3d& p2 = vertices[tri.p2].p;

               point3d direction = P.p.n; // project along the reference normal
               bool found_intersection = cvlab::findLineTriangleIntersection(transformed,direction,p0,p1,p2,projected);

               // DEBUG
               if (found_intersection) {
                  ++n_intersections;
                  //cout << "+" << flush;
                  cloud.add_point(projected);
               }

            } // next nearest triangle

            // DEBUG
            if (n_intersections > 1)
               cout << "x" << flush;
            else if (n_intersections == 0)
               cout << "?" << flush;

            // Orthogonal distance to the reference tangent plane
            double h = cvlab::dist(projected, transformed);

            // Now (r,h) are the cylindric coordinates to use in the spin-image.
            spin.accumulate_cylindric_point(r,h);

         } // next radius step
      } // next angle step

      cvReleaseMat(&R);

      cloud.save("cloud",FMT_VTK);
      //spin.save("spin.bmp");

   }
   // ______________________________________________TEST________________________________________________




   /**
    * The given oriented point is used for accumulation only if it has a compatible normal,
    * that is a normal within the support angle of the spin image, and if it is not too far.
    * The contribution of the point is bilinearly interpolated to the four surrounding bins.
    *
    * @param x Point to use for accumulation in the spin image.
    * @param max_distance Maximum Euclidean distance between the reference and the given point.
    * @return Returns TRUE if the point was accumulated in the current spin image, FALSE otherwise.
    */
   bool spin_image::accumulate_point(const oriented_point3d& x, double max_distance) {

      normal3d xn( cvlab::get_normalize(x.n) );

      if (std::acos( cvlab::dot_product(point.n, xn) ) < support_angle && cvlab::dist(point,x) < max_distance) {

         // cylindric coordinates (r,h)
         double r, h;
         point3d d = x - point;
         h = cvlab::dot_product(point.n, d);
         r = std::sqrt( cvlab::dot_product(d, d) - h*h);

         // image coordinates (j,i)
         uint32_t i = floor( (image_width*bin_size/2. - h) / bin_size ); //FIXME: is it really image_width*bin_size?
         uint32_t j = floor( r / bin_size );

         if (j > image_width || i > image_width) // assuming square image
            return false;

         //FIXME: something goes wrong here and we get negative valued bins...

         // bilinear weights
         /*double a = r - j * bin_size;
         double b = h - i * bin_size;

         image(i,j) += (1-a)*(1-b);
         image(i+1,j) += a*(1-b);
         image(i,j+1) += (1-a)*b;
         image(i+1,j+1) += a*b;*/

         image(i,j) += 1;

         return true;
      }

      return false;
   }

   bool spin_image::accumulate_cylindric_point(double r, double h) {

      // image coordinates (j,i)
      uint32_t i = floor( (image_width*bin_size/2. - h) / bin_size ); //FIXME: is it really image_width*bin_size?
      uint32_t j = floor( r / bin_size );

      if (j > image_width || i > image_width) // assuming square image
         return false;

      //FIXME: something goes wrong here and we get negative valued bins...

      // bilinear weights
      /*double a = r - j * bin_size;
      double b = h - i * bin_size;

      image(i,j) += (1-a)*(1-b);
      image(i+1,j) += a*(1-b);
      image(i,j+1) += (1-a)*b;
      image(i+1,j+1) += a*b;*/

      image(i,j) += 1;

      return true;
   }

   /**
    * Save the spin image to disk in BMP format.
    * The spin image is normalized and inverted for better visualization.
    *
    * @param fname The output filename.
    */
   /*void spin_image::save(const std::string& fname) const {

      cimg_library::CImg<unsigned char> cimg(image_width,image_width,1,1);

      double maxp = *(std::max_element(image.begin(), image.end()));

      for (uint32_t y=0; y<image_width; ++y) {
         for (uint32_t x=0; x<image_width; ++x) {
            cimg(x,y) = 255 - (255 * image(y,x) / maxp);
         }
      }
      cimg.save(fname.c_str());
   }*/

   /**
    * Read a triangle from the mesh and move the triangle pointer one step forward.
    *
    * @param tri Output triangle.
    * @return Returns FALSE if the end of the mesh has been reached, TRUE otherwise.
    */
   bool mesh::get_next_triangle(triangle& tri) const {
      if (cur_triangle == triangles.size())
         return false;

      const triangle_data& cur_tri = triangles[cur_triangle];

      tri.p0 = vertices[cur_tri.p0].p;
      tri.p1 = vertices[cur_tri.p1].p;
      tri.p2 = vertices[cur_tri.p2].p;
      tri.n = cur_tri.n;

      ++cur_triangle;

      return true;
   }

   /**
    * Calculates the barycenter (centroid) of the current mesh and returns it.
    *
    * @return The current mesh barycenter is returned.
    * @throw std::logic_error
    */
   point3d mesh::get_barycenter() const {
      if (vertices.empty())
         throw std::logic_error("Cannot calculate barycenter for an empty mesh.");

      point3d centroid;
      uint32_t npts = vertices.size();

      for (uint32_t k=0; k<npts; ++k)
         centroid += vertices[k].p;

      return centroid / double(npts);
   }

   /**
    * Get an estimate of the resolution of the current mesh.
    * The estimate is calculated as the median edge length throughout the mesh.
    *
    * @return The resolution estimate is returned.
    */
   double mesh::get_resolution() const {

      double res; // the estimated mesh resolution

      uint32_t ntris = triangles.size();

      if (ntris == 0)
         return 0.;

      uint32_t nedges = ntris * 3;
      std::vector<double> lengths(nedges);

      // FIXME: We are counting shared edges more than once...
      for (uint32_t k=0; k<ntris; ++k) {

         const point3d& p0 = vertices[ triangles[k].p0 ].p;
         const point3d& p1 = vertices[ triangles[k].p1 ].p;
         const point3d& p2 = vertices[ triangles[k].p2 ].p;

         lengths[3*k] = cvlab::dist(p0,p1);
         lengths[3*k+1] = cvlab::dist(p2,p1);
         lengths[3*k+2] = cvlab::dist(p0,p2);

      } // next triangle

      std::sort(lengths.begin(),lengths.end());

      if (nedges % 2 == 0)
         res = (lengths[nedges/2] + lengths[nedges/2-1]) / 2.;
      else
         res = lengths[ (nedges-1) / 2 ];

      return res;
   }

   /**
    * Loads a rangemap from an ASCII ply file in range grid format, and construct
    * a mesh out of it. The range grid is tessellated using edges with a given
    * maximum length.
    *
    * @param fname Filename of the ply rangemap.
    * @param max_edge_length Maximum edge length allowed during mesh tessellation.
    * @throw cvlab::file_not_found, std::runtime_error
    */
   void mesh::load_range_from_ply(const string& fname, double max_edge_length) {

      using namespace strutils;

      cout << "Loading range map from " << fname << "..." << endl;

      std::ifstream file_in(fname.c_str(), std::ios::in);

      if (!file_in.is_open() || file_in.fail())
         throw file_not_found("Cannot read "+fname);

      string line;
      unsigned int cur_l=0;
      vector<string> tokens;

      uint32_t n_points = 0;
      uint32_t n_cols = 0, n_rows = 0;

      // Read the number of points, width and height of the range grid

      do {
         tokens.clear();
         std::getline(file_in, line);
         tokenize_str(line, tokens, " ");
         ++cur_l;

         if (tokens.size() == 3) {

            if (tokens[1]=="num_cols")
               n_cols = convert_str<uint32_t>(tokens[2]);

            else if (tokens[1]=="num_rows")
               n_rows = convert_str<uint32_t>(tokens[2]);

            else if (tokens[0]=="element" && tokens[1]=="vertex") {
               n_points = convert_str<uint32_t>(tokens[2]);

               do {
                  tokens.clear();
                  std::getline(file_in,line);
                  tokenize_str(line, tokens, " ");
                  ++cur_l;
               } while(tokens[0] != "end_header");

               break;
            }
         }
      } while (!file_in.eof());

      if (n_cols == 0 || n_rows == 0 || n_points == 0)
         throw std::runtime_error("Can't parse range grid in "+fname);

      // We are now at the right position to start reading in vertices

      cout << "\trange grid " << n_cols << "x" << n_rows << endl;
      cout << "\t" << n_points << " points... " << std::flush;

      cur_l = 0;
      vector<point3d> verts(n_points);

      while (cur_l<n_points){
         tokens.clear();
         std::getline(file_in, line);
         tokenize_str(line, tokens, " ");
         verts[cur_l] = point3d(
               convert_str<double>(tokens[0]),
               - convert_str<double>(tokens[1]), // we negate the y coordinate because tipically the range grid is left-handed
               convert_str<double>(tokens[2]));
         ++cur_l;
      }

      put_vertices(verts);
      cout << "loaded." << endl;

      // The range grid written in the ply file is loaded into a matrix of
      // indices referring to the elements loaded into the vertices structure.

      cout << "\ttessellating... " << std::flush;

      matrix<int> rangegrid(n_cols,n_rows);

      for (uint32_t cur_row=0; cur_row<n_rows; ++cur_row) {
         for (uint32_t cur_col=0; cur_col<n_cols; ++cur_col){
            tokens.clear();
            std::getline(file_in, line);
            tokenize_str(line, tokens, " ");

            if (tokens.size()>2)
               throw std::runtime_error("The current range grid is not orthographic.");

            rangegrid.at(cur_row,cur_col) = convert_str<uint32_t>(tokens[0]) == 0 ? INVALID_VERTEX_INDEX : convert_str<int32_t>(tokens[1]);
         }
      }

      file_in.close();

      // Actual tessellation of the range map takes place here

      uint32_t ntris = 0;

      for (uint32_t cur_row=0; cur_row<n_rows-1; ++cur_row) {
         for (uint32_t cur_col=0; cur_col<n_cols-1; ++cur_col){

            //cout << "\r\ttessellating... " << ntris << std::flush;

            //  p0--,p1
            //   | / |
            //  p3'--p2

            int p0 = rangegrid(cur_row,cur_col);
            int p1 = rangegrid(cur_row,cur_col+1);
            int p2 = rangegrid(cur_row+1,cur_col+1);
            int p3 = rangegrid(cur_row+1,cur_col);

            double dist01, dist03, dist12, dist13, dist23;
            double max_dist;

            if(p0!=INVALID_VERTEX_INDEX && p1!=INVALID_VERTEX_INDEX && p3!=INVALID_VERTEX_INDEX) {

               dist01 = cvlab::dist(vertices.at(p0).p,vertices.at(p1).p);
               dist03 = cvlab::dist(vertices.at(p0).p,vertices.at(p3).p);
               dist13 = cvlab::dist(vertices.at(p1).p,vertices.at(p3).p);

               if (dist01 > dist13)
                  max_dist = dist01 > dist03 ? dist01 : dist03;
               else
                  max_dist = dist13 > dist03 ? dist13 : dist03;

               if (max_dist<=max_edge_length) {
                  add_triangle(p0,p1,p3);
                  ++ntris;
               }
            }

            if(p1!=INVALID_VERTEX_INDEX && p2!=INVALID_VERTEX_INDEX && p3!=INVALID_VERTEX_INDEX) {

               dist12 = cvlab::dist(vertices.at(p1).p,vertices.at(p2).p);
               dist13 = cvlab::dist(vertices.at(p1).p,vertices.at(p3).p);
               dist23 = cvlab::dist(vertices.at(p2).p,vertices.at(p3).p);

               if (dist12 > dist13)
                  max_dist = dist12 > dist23 ? dist12 : dist23;
               else
                  max_dist = dist13 > dist23 ? dist13 : dist23;

               if (max_dist<=max_edge_length) {
                  add_triangle(p1,p2,p3);
                  ++ntris;
               }
            }

         } // next col
      } // next row

      cout << ntris << " triangles." << endl;

      // Vertex / triangle normals are finally calculated and assigned to each vertex / triangle

      cout << "\tnormals... " << std::flush;
      calc_normals();
      cout << "done." << endl;

      cout << "done." << endl;
   }

   /**
    *
    *
    * @param points
    */
   void mesh::put_vertices(const vector<point3d>& points) {

      uint32_t n_points = points.size();
      vertices.resize(n_points);

      // Put the points in the appropriate structure for the kd-tree
      // and in the vertices collection for future indexed reference

      vector<mesh_point> mesh_points(n_points);
      for (uint32_t k=0; k<n_points; ++k) {
         // indicized vertices
         vertex_data v;
         v.p = oriented_point3d(points[k]);
         v.n_tris = 0;
         vertices[k] = v;

         // for the kd-tree
         mesh_points[k].x = points[k].x;
         mesh_points[k].y = points[k].y;
         mesh_points[k].z = points[k].z;
         mesh_points[k].vertex_idx = k;
      }

      tree = new kdtree<mesh_point>(mesh_points);
   }

   /**
    * Adds a triangle to the mesh whose 3 vertices are indicized by the given indices.
    *
    * @param p0 Index of the first vertex in the triangle.
    * @param p1 Index of the second vertex in the triangle.
    * @param p2 Index of the third vertex in the triangle.
    */
   void mesh::add_triangle(size_t p0, size_t p1, size_t p2) {

      triangle_data td;

      td.p0 = p0;
      td.p1 = p1;
      td.p2 = p2;

      const point3d& p3d1 = vertices.at(p0).p;
      const point3d& p3d2 = vertices.at(p1).p;
      const point3d& p3d3 = vertices.at(p2).p;

      td.centroid = point3d( p3d1 + p3d2 + p3d3 );
      td.centroid /= 3;

      td.n = normal3d( cross_product(p3d2-p3d1, p3d3-p3d1) );

      triangles.push_back(td);
      uint32_t tri_idx = triangles.size() - 1;

      vertices.at(p0).tri_indices.push_back(tri_idx);
      vertices.at(p0).n_tris++;
      vertices.at(p1).tri_indices.push_back(tri_idx);
      vertices.at(p1).n_tris++;
      vertices.at(p2).tri_indices.push_back(tri_idx);
      vertices.at(p2).n_tris++;
   }

   /**
    * Calculates and assigns unit-length normals at each vertex and triangle of the mesh.
    */
   void mesh::calc_normals() {
      
      uint32_t n_points = vertices.size();
      for (uint32_t k = 0; k < n_points; ++k) {

         std::list<size_t>::const_iterator it = vertices[k].tri_indices.begin(), it_end = vertices[k].tri_indices.end();
         for (; it!=it_end; ++it) {

            triangle_data& tri = triangles[ *it ];
            const oriented_point3d& p0 = vertices[tri.p0].p;
            const oriented_point3d& p1 = vertices[tri.p1].p;
            const oriented_point3d& p2 = vertices[tri.p2].p;

            tri.n = cross_product(p2-p0, p1-p0);
            vertices[k].p.n += tri.n;

            tri.n /= cvlab::magnitude(tri.n);

         } // next triangle

         double mag = cvlab::magnitude(vertices[k].p.n);
         vertices[k].p.n /= (mag != 0. ? mag : 1.); // some vertices might be outliers

      } // next vertex

   }

   /**
    * Flip triangle and vertex normals for the current mesh.
    */
   void mesh::flip_normals() {
      uint32_t n_points = vertices.size();
      for (uint32_t k = 0; k < n_points; ++k)
         vertices[k].p.n *= -1.;

      uint32_t n_tris = triangles.size();
      for (uint32_t k = 0; k < n_tris; ++k)
         triangles[k].n *= -1.;
   }

   /**
    * Look for the nearest neighbor mesh vertex of a given point.
    * Of course, the given point is not needed to be itself a vertex in the mesh.
    *
    * @param p Input point for which a nearest neighbor is requested.
    * @param radius Radius of the cube-like neighborhood to consider.
    * @return An index in the vertices structure pointing to the nearest neighbor found.
    */
   int mesh::nearest_neighbor(const point3d& p, double radius) const {
      std::vector<mesh_point> ret;
      tree->neighbors(p, 1, radius, ret);

      if (ret.empty())
         return INVALID_VERTEX_INDEX;

      return ret.at(0).vertex_idx;
   }

   /**
    * Save the current mesh to disk in STL binary format.
    *
    * @param fname Destination filename.
    */
   void mesh::save_as_stl(const std::string& fname) const {
      stl_file out(fname);

      cout << "Saving to " << fname << "... " << std::flush;

      uint32_t n_tris = triangles.size();
      std::vector<triangle> tris(n_tris);

      for (uint32_t k=0; k<n_tris; ++k){
         tris[k] = triangle(
               vertices.at(triangles[k].p0).p,
               vertices.at(triangles[k].p1).p,
               vertices.at(triangles[k].p2).p,
               triangles[k].n);
      }

      out.save(&tris);

      cout << "done." << endl;
   }

}
