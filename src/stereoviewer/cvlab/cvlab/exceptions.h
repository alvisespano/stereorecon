#ifndef CVLAB_EXCEPTIONS_H
#define CVLAB_EXCEPTIONS_H

#include <stdexcept>

namespace cvlab {

   class null_pointer : public std::runtime_error {
      public:
      null_pointer(const std::string& msg) : std::runtime_error("Exception null_pointer caught: "+msg) {}
      null_pointer() : std::runtime_error("Exception null_pointer caught.") {}
   };

   class file_not_found : public std::runtime_error {
      public:
      file_not_found(const std::string& msg) : std::runtime_error("Exception file_not_found caught: "+msg) {}
      file_not_found() : std::runtime_error("Exception file_not_found caught.") {}
   };

}

#endif // CVLAB_EXCEPTIONS_H
