#ifndef CVLAB_STL_FILE_H
#define CVLAB_STL_FILE_H

#include "math3d.h"
#include <string>

namespace cvlab {

   typedef std::vector<triangle> TriangleList;

   class stl_file {
      public:
         stl_file(const std::string& name_) : name(name_) {}
         void save(const std::vector<triangle>* triangles);
         TriangleList *load() const;

      protected:
         const std::string name;
   };

}

#endif // CVLAB_STL_FILE_H
