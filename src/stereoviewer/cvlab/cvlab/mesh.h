/*
  Emanuele Rodol√  <rodola@dsi.unive.it>
*/

#ifndef CVLAB_MESH_H
#define CVLAB_MESH_H

#include <list>
#include <limits>
#include "math3d.h"
#include <stdexcept>

namespace cvlab {

   struct vertex_data {
      oriented_point3d p;
      uint16_t n_tris; // # triangles this vertex belongs to
      std::list<size_t> tri_indices; // triangles this vertex belongs to
   };

   struct triangle_data {
      size_t p0, p1, p2; // indices of the 3 vertices
      normal3d n;
      point3d centroid;
   };

   /**
    * This is used in the kd-tree structure.
    */
   struct mesh_point : public point3d {
      uint32_t vertex_idx;
      double distance;
      mesh_point() : point3d(), vertex_idx(-1), distance(std::numeric_limits<double>::max()) {}
   };


   template <typename T>
   class kdtree;


   /**
    * This class describes a mesh as a collection of oriented vertices and triangles.
    */
   class mesh {

      /**
       * Used for iterating through mesh objects by vertex or by triangle.
       */
      //template <typename Container, typename Data>
      //class mesh_iterator : public std::iterator< std::random_access_iterator_tag, Data > {
      template <typename Container>
      class mesh_iterator {

         private:
            //typedef std::iterator< std::random_access_iterator_tag, Data > super;

            const Container* container;
            mutable size_t idx;

         public:

            /*typedef typename super::pointer pointer;
            typedef typename super::reference reference;
            typedef typename super::value_type value_type;
            typedef typename super::difference_type difference_type;
            typedef typename super::iterator_category iterator_category;*/

            explicit mesh_iterator(const Container* c) : container(c), idx(0) {}

            // default copy constructor and assignment operator are ok

            cvlab::triangle operator*() const {
               const oriented_point3d& p0 = container->vertices[ container->triangles[idx].p0 ].p;
               const oriented_point3d& p1 = container->vertices[ container->triangles[idx].p1 ].p;
               const oriented_point3d& p2 = container->vertices[ container->triangles[idx].p2 ].p;

               return cvlab::triangle(p0,p1,p2,container->triangles[idx].n);
            }

            // prefix
            mesh_iterator& operator++() {
               if (idx+1 > container->triangles.size())
                  throw std::out_of_range("");
               ++idx;
               return *this;
            }

            // postfix
            mesh_iterator& operator++(int) {
               mesh_iterator before(*this);
               ++(*this);
               return before;
            }

            bool operator==(const mesh_iterator& o) const {
               return (container == o.container && idx == o.idx);
            }

            bool operator!=(const mesh_iterator& o) const {
               return !(*this == o);
            }

            // brings the iterator to the last element + 1
            void last() const { idx = container->triangles.size(); }

      }; // mesh_iterator


      public:
         typedef mesh_iterator<mesh> const_iterator_tri;

      private:
         std::vector<vertex_data> vertices;
         std::vector<triangle_data> triangles;
         cvlab::kdtree<mesh_point>* tree;
         mutable uint32_t cur_triangle; // used by get_next_triangle()

      public:
         mesh() : tree(NULL), cur_triangle(0) {}
         virtual ~mesh() throw();

         uint32_t get_n_tris() const { return triangles.size(); }

         void put_vertices(const std::vector<point3d>&);
         void add_triangle(uint32_t p1, uint32_t p2, uint32_t p3);

         double get_resolution() const;

         void calc_normals();
         void flip_normals();

         point3d get_barycenter() const;

         void test_spin();
         void test_spin_fine();

         const_iterator_tri begin_by_triangle() const {
            return const_iterator_tri(this);
         }

         const_iterator_tri end_by_triangle() const {
            const_iterator_tri end(this);
            end.last();
            return end;
         }

         bool get_next_triangle(triangle&) const;
         void first_triangle() const { cur_triangle = 0; }

         void load_range_from_ply(const std::string&, double max_edge_length);
         void save_as_stl(const std::string&) const;

         int nearest_neighbor(const point3d&, double radius) const;

   }; // mesh


   /**
    * Generic pointwise local shape descriptor for meshes.
    */
   class mesh_descriptor {
      protected:
         oriented_point3d point;

      public:
         explicit mesh_descriptor() {}
         mesh_descriptor(const oriented_point3d& p) : point(p) {}
   };


   /**
    *
    * "Spin-Images: A representation for 3-D surface matching", Andrew E. Johnson, PhD Thesis
    */
   class spin_image : public mesh_descriptor {

      protected:

         double bin_size; // shall be a multiple of the resolution of the surface mesh
         uint32_t image_width; // assume square spin images for now
         double support_angle; // max angle (rad) between basis normal and normals of points allowed to contribute
         cvlab::matrix<double> image;

      public:

         explicit spin_image() : mesh_descriptor(), bin_size(0.), image_width(0), support_angle(0.) {}

         spin_image(const oriented_point3d& p, uint32_t w, double bin) : mesh_descriptor(p), bin_size(bin), image_width(w) {
            cvlab::normalize(point.n);

            image.resize(w, w);
            std::fill(image.begin(), image.end(), 0.);
         }

         void set_oriented_point(const oriented_point3d& p) {
            point = p;
            cvlab::normalize(point);
         }

         void set_bin_size(double b) { bin_size = b; }

         void set_support_angle(double a) { support_angle = M_PI * a / 180.; }

         void set_image_width(uint32_t w) {
            image_width = w;
            image.resize(w, w);
            std::fill(image.begin(), image.end(), 0.);
         }

         double get_bin_size() const { return bin_size; }
         uint32_t get_image_width() const { return image_width; }
         double get_support_angle() const { return support_angle * 180 / M_PI; }

         // the amount of space swept out by a spin image
         double get_support_distance() const { return image_width * bin_size; }

         virtual bool accumulate_point(const oriented_point3d& x, double max_distance);
         bool accumulate_cylindric_point(double r, double h);
         //void save(const std::string& fname) const;
   };


   /**
    *
    */
   class fine_spin_image : public spin_image {

      private:

      public:

         explicit fine_spin_image() : spin_image() {}
   };

} // end namespace

#endif // CVLAB_MESH_H
