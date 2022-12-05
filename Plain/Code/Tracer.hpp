#ifndef _TRACER_HPP_
#define _TRACER_HPP_

#include "Element.hpp"
#include <Code/IncludeElement.hpp>
#include <Code/TypeElement.hpp>
#include <Code/TypeDefElement.hpp>
#include <Code/NamespaceElement.hpp>

#include <Declaration/BaseDescriptor.hpp>
#include <Declaration/BaseTemplate.hpp>
#include <Declaration/SimpleType.hpp>
#include <Declaration/StructDescriptor.hpp>
#include <Declaration/StructReader.hpp>
#include <Declaration/StructWriter.hpp>
#include <Declaration/CompleteTemplateDescriptor.hpp>
#include <Declaration/Namespace.hpp>

namespace Code
{
  class Tracer:
    public ReferenceCounting::DefaultImpl<>,
    public Code::ElementVisitor
  {
  public:
    void generate(
      std::ostream& out,
      Code::ElementList* elements) noexcept;

    virtual void visit_i(const Code::Element*) noexcept;

    virtual void visit_i(const Code::IncludeElement*) noexcept;

    virtual void visit_i(const Code::NamespaceElement*) noexcept;

    virtual void visit_i(const Code::TypeDefElement*) noexcept;

    virtual void visit_i(const Code::TypeElement*) noexcept;

  protected:
    virtual ~Tracer() noexcept {}

    void generate_descriptor_(
      const Declaration::StructDescriptor* descriptor) noexcept;

    void generate_reader_(
      Declaration::StructReader* reader) noexcept;

    void generate_writer_(
      Declaration::StructWriter* writer) noexcept;

  private:
    std::ostream* out_;
    std::string offset_;
  };

  typedef ReferenceCounting::SmartPtr<Tracer>
    Tracer_var;
}

namespace Code
{
  inline
  void
  Tracer::visit_i(
    const Code::Element*) noexcept
  {}

  inline
  void
  Tracer::visit_i(
    const Code::IncludeElement* elem) noexcept
  {
    *out_ << offset_ << "#include <" << elem->file() << ">" << std::endl;
  }

  inline
  void
  Tracer::visit_i(
    const Code::NamespaceElement* elem) noexcept
  {
    Declaration::Namespace_var namespace_decl = elem->namespace_decl();
    std::string prev_offset;

    if(namespace_decl.in() && namespace_decl->name()[0])
    {
      prev_offset = offset_;
      *out_ << offset_ << "namespace " << namespace_decl->name() << std::endl <<
        offset_ << "{";
      offset_ = offset_ + "  ";
    }
    
    for(Code::ElementList::const_iterator el_it = elem->elements()->begin();
        el_it != elem->elements()->end(); ++el_it)
    {
      visit(*el_it);
    }

    if(namespace_decl.in() && namespace_decl->name()[0])
    {
      offset_ = prev_offset;
      *out_ << offset_ << "}" << std::endl;
    }
  }

  inline
  void
  Tracer::visit_i(
    const Code::TypeDefElement* elem) noexcept
  {
    *out_ << offset_ << "typedef " << elem->base_type()->name() << " " <<
      elem->type_name() << ";" << std::endl;
  }

  inline
  void
  Tracer::visit_i(
    const Code::TypeElement* elem) noexcept
  {
    Declaration::BaseType_var type = elem->type();
    Declaration::BaseDescriptor_var descriptor = type->as_descriptor();

    if(descriptor.in())
    {
      Declaration::StructDescriptor_var struct_descriptor =
        descriptor->as_struct();

      assert(struct_descriptor.in());
      generate_descriptor_(struct_descriptor);
    }
    else
    {
      Declaration::BaseReader_var reader = type->as_reader();
      if(reader.in())
      {
        Declaration::StructReader_var struct_reader =
          reader->as_struct_reader();
        assert(struct_reader.in());
        
        generate_reader_(struct_reader);
      }
      else
      {
        Declaration::BaseWriter_var writer = type->as_writer();
        assert(writer.in());

        Declaration::StructWriter_var struct_writer =
          writer->as_struct_writer();
        assert(struct_writer.in());

        generate_writer_(struct_writer);
      }
    }
  }

  inline
  void
  Tracer::generate_descriptor_(
    const Declaration::StructDescriptor* struct_descriptor) noexcept
  {
    *out_ << std::endl << offset_ <<
      "struct " << struct_descriptor->name() << std::endl <<
      offset_ << "{" << std::endl;

    for(Declaration::StructDescriptor::
        PosedFieldList::const_iterator field_it =
          struct_descriptor->fields()->begin();
        field_it != struct_descriptor->fields()->end(); ++field_it)
    {
      *out_ << offset_ << "  " <<
        (*field_it)->descriptor()->name() << " " << (*field_it)->name() <<
        ";" << std::endl;
    }

    *out_ << offset_ << "};" << std::endl;
  }

  inline
  void
  Tracer::generate_reader_(
    Declaration::StructReader* struct_reader) noexcept
  {
    *out_ << std::endl <<
      offset_ << "reader " << struct_reader->name() << "<" <<
      struct_reader->descriptor()->name() << ">" << std::endl <<
      offset_ << "{" << std::endl;

    for(Declaration::StructReader::
        FieldReaderList::const_iterator field_it =
          struct_reader->fields()->begin();
        field_it != struct_reader->fields()->end(); ++field_it)
    {
      *out_ << offset_ << "  " <<
        (*field_it)->reader()->name() << " " << (*field_it)->name() <<
        ";" << std::endl;
    }

    *out_ << offset_ << "};" << std::endl;
  }

  inline
  void
  Tracer::generate_writer_(
    Declaration::StructWriter* struct_writer) noexcept
  {
    *out_ << std::endl <<
      offset_ << "writer " << struct_writer->name() << "<" <<
      struct_writer->descriptor()->name() << ">" << std::endl <<
      offset_ << "{" << std::endl;

    for(Declaration::StructWriter::
        FieldWriterList::const_iterator field_it =
          struct_writer->fields()->begin();
        field_it != struct_writer->fields()->end(); ++field_it)
    {
      *out_ << offset_ << "  " <<
        (*field_it)->writer()->name() << " " << (*field_it)->name() <<
        ";" << std::endl;
    }

    *out_ << offset_ << "};" << std::endl;
  }
  
  inline
  void
  Tracer::generate(
    std::ostream& out,
    Code::ElementList* elements) noexcept
  {
    out_ = &out;
    
    for(Code::ElementList::const_iterator el_it =
          elements->begin();
        el_it != elements->end(); ++el_it)
    {
      visit(*el_it);
    }

    out_ = 0;
  }
}

#endif /*_TRACER_HPP_*/
