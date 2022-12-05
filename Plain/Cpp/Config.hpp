#ifndef PLAIN_CPP_CONFIG_HPP
#define PLAIN_CPP_CONFIG_HPP

namespace Cpp
{
  const char BASE_SUFFIX[] = "_Base";
  const char PROTECTED_WRITER_SUFFIX[] = "_ProtectedWriter";
  const char DEFAULT_BUFFERS_SUFFIX[] = "_DefaultBuffers";
  const char FIELD_OFFSET_SUFFIX[] = "_OFFSET";
  const char FIXED_BUFFER_PREFIX[] = "fixed_buf_";

  const char INCLUDE_LIST[] =
    "#include <string>\n"
    "#include <Plain/Base.hpp>\n"
    "#include <Plain/String.hpp>\n"
    "#include <Plain/ConstVector.hpp>\n"
    "#include <Plain/List.hpp>\n"
    "#include <Plain/Vector.hpp>\n"
    "#include <Plain/Buffer.hpp>\n";
}

#endif /*PLAIN_CPP_CONFIG_HPP*/
