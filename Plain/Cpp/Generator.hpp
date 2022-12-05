#ifndef PLAIN_CPP_GENERATOR_HPP
#define PLAIN_CPP_GENERATOR_HPP

#include <Declaration/SimpleDescriptor.hpp>
#include <Declaration/SimpleReader.hpp>
#include <Declaration/SimpleWriter.hpp>
#include <Declaration/StructDescriptor.hpp>
#include <Declaration/StructReader.hpp>
#include <Declaration/StructWriter.hpp>

#include <Code/Element.hpp>
#include <Code/IncludeElement.hpp>
#include <Code/TypeElement.hpp>
#include <Code/TypeDefElement.hpp>
#include <Code/NamespaceElement.hpp>

namespace Cpp
{
  class Generator: public ReferenceCounting::DefaultImpl<>
  {
  public:
    void generate(
      std::ostream& out,
      std::ostream& out_inl_impl,
      std::ostream& out_cpp,
      Code::ElementList* elements) noexcept;

  protected:
    virtual ~Generator() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<Generator>
    Generator_var;
}

#endif /*PLAIN_CPP_GENERATOR_HPP*/
