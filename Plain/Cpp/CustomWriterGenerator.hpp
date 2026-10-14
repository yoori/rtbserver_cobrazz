#pragma once

#include <iosfwd>
#include <string>
#include <Declaration/StructDescriptor.hpp>
#include <Declaration/StructWriter.hpp>

namespace Cpp
{
  class CustomWriterGenerator
  {
  public:
    CustomWriterGenerator(std::ostream& out, const char* offset);

    void generate(Declaration::StructDescriptor* descriptor);
    void generate(Declaration::StructWriter* writer);

  private:
    void generate_(const std::string& name, Declaration::StructDescriptor* descriptor,
      Declaration::StructWriter::FieldWriterList* fields);

    std::ostream& out_;
    std::string offset_;
  };
}
