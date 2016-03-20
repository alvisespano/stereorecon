#ifndef STRING_UTILITIES_H
#define STRING_UTILITIES_H

#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>

namespace strutils {

   using std::string;
   using std::vector;

   void tokenize_str(const string& str, vector<string>& tokens, const string& delimiters);

   template<class T>
   T convert_str(const string& s)
   {
      std::istringstream i(s);
      T x;
      if (!(i >> x))
         throw std::invalid_argument("Exception: convert_str()\n");
      return x;
   }

}

#endif // STRING_UTILITIES_H
