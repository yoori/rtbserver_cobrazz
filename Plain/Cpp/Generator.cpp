#include "Config.hpp"
#include "DescriptorGenerator.hpp"
#include "ReaderGenerator.hpp"
#include "WriterGenerator.hpp"

#include "Generator.hpp"

namespace Cpp
{
  /* DeclareGenerator */
  class DeclareGenerator: public Code::ElementVisitor
  {
  public:
    DeclareGenerator(std::ostream& out, std::ostream& out_cpp) noexcept;

    virtual ~DeclareGenerator() noexcept {}

    virtual void visit_i(const Code::Element*) noexcept {};

    virtual void visit_i(const Code::IncludeElement*) noexcept;

    virtual void visit_i(const Code::NamespaceElement*) noexcept;

    virtual void visit_i(const Code::TypeDefElement*) noexcept;

    virtual void visit_i(const Code::TypeElement*) noexcept;

  private:
    std::ostream& out_;
    std::ostream& out_cpp_;
    std::string offset_;
  };
    
  /* ImplGenerator */
  class ImplGenerator: public Code::ElementVisitor
  {
  public:
    ImplGenerator(std::ostream& out, std::ostream& out_cpp) noexcept;

    virtual ~ImplGenerator() noexcept {}

    virtual void visit_i(const Code::Element*) noexcept {};

    virtual void visit_i(const Code::IncludeElement*) noexcept {};

    virtual void visit_i(const Code::NamespaceElement*) noexcept;

    virtual void visit_i(const Code::TypeDefElement*) noexcept {};

    virtual void visit_i(const Code::TypeElement*) noexcept;

  private:
    std::ostream& out_;
    std::ostream& out_cpp_;
    std::string offset_;
  };

  namespace Utils
  {
    bool is_struct(Declaration::BaseDescriptor* descriptor)
      noexcept
    {
      return Declaration::StructDescriptor_var(
        descriptor->as_struct()).in();
      
    }
  }

  /* DeclareGenerator implementation */
  DeclareGenerator::DeclareGenerator(
    std::ostream& out,
    std::ostream& out_cpp) noexcept
    : out_(out),
      out_cpp_(out_cpp)
  {}

  void
  DeclareGenerator::visit_i(const Code::IncludeElement* elem) noexcept
  {
    out_ << "#include <" << elem->file() << ">" << std::endl;
  }

  void
  DeclareGenerator::visit_i(const Code::NamespaceElement* elem) noexcept
  {
    Declaration::Namespace_var namespace_decl = elem->namespace_decl();
    std::string prev_offset;

    if(namespace_decl.in() && namespace_decl->name()[0])
    {
      prev_offset = offset_;
      out_ << offset_ << "namespace " << namespace_decl->name() << std::endl <<
        offset_ << "{" << std::endl;
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
      out_ << offset_ << "}" << std::endl;
    }
  }

  void
  DeclareGenerator::visit_i(const Code::TypeDefElement* elem) noexcept
  {
    out_ << offset_ << "typedef " << elem->base_type()->name() << " " <<
      elem->type_name() << ";" << std::endl;
  }

  void
  DeclareGenerator::visit_i(
    const Code::TypeElement* elem) noexcept
  {
    Declaration::BaseType_var type = elem->type();
    Declaration::BaseDescriptor_var descriptor = type->as_descriptor();

    if(descriptor.in())
    {
      Declaration::StructDescriptor_var struct_descriptor =
        descriptor->as_struct();

      assert(struct_descriptor.in());

      DescriptorGenerator(out_, out_cpp_, offset_.c_str()).generate_decl(
        struct_descriptor);
    }
    else
    {
      Declaration::BaseReader_var reader = type->as_reader();
      if(reader.in())
      {
        Declaration::StructReader_var struct_reader =
          reader->as_struct_reader();

        assert(struct_reader.in());

        ReaderGenerator(out_, offset_.c_str()).generate_decl(
          struct_reader);
      }
      else
      {
        Declaration::BaseWriter_var writer = type->as_writer();
        assert(writer.in());

        Declaration::StructWriter_var struct_writer =
          writer->as_struct_writer();

        assert(struct_writer.in());

        WriterGenerator(out_, out_cpp_, offset_.c_str()).generate_decl(
          struct_writer);
      }
    }
  }

  /* Generator::ImplGenerator */
  ImplGenerator::ImplGenerator(
    std::ostream& out,
    std::ostream& out_cpp) noexcept
    : out_(out),
      out_cpp_(out_cpp)
  {}

  void
  ImplGenerator::visit_i(const Code::NamespaceElement* elem) noexcept
  {
    Declaration::Namespace_var namespace_decl = elem->namespace_decl();
    std::string prev_offset;

    if(namespace_decl.in() && namespace_decl->name()[0])
    {
      prev_offset = offset_;
      out_ << offset_ << "namespace " << namespace_decl->name() << std::endl <<
        offset_ << "{" << std::endl;
      out_cpp_ << offset_ << "namespace " << namespace_decl->name() << std::endl <<
        offset_ << "{" << std::endl;
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
      out_ << offset_ << "}" << std::endl;
      out_cpp_ << offset_ << "}" << std::endl;
    }
  }

  void
  ImplGenerator::visit_i(const Code::TypeElement* elem) noexcept
  {
    Declaration::BaseType_var type = elem->type();
    Declaration::BaseDescriptor_var descriptor = type->as_descriptor();

    if(descriptor.in())
    {
      Declaration::StructDescriptor_var struct_descriptor =
        descriptor->as_struct();

      assert(struct_descriptor.in());

      DescriptorGenerator(out_, out_cpp_, offset_.c_str()).generate_impl(
        struct_descriptor);
    }
    else
    {
      Declaration::BaseReader_var reader = type->as_reader();
      if(reader.in())
      {
        Declaration::StructReader_var struct_reader =
          reader->as_struct_reader();

        assert(struct_reader.in());

        ReaderGenerator(out_, offset_.c_str()).generate_impl(
          struct_reader);
      }
      else
      {
        Declaration::BaseWriter_var writer = type->as_writer();
        assert(writer.in());
        Declaration::StructWriter_var struct_writer =
          writer->as_struct_writer();

        assert(struct_writer.in());

        WriterGenerator(out_, out_cpp_, offset_.c_str()).generate_impl(
          struct_writer);
      }
    }
  }

  /* Generator */
  void Generator::generate(
    std::ostream& out,
    std::ostream& out_inl_impl,
    std::ostream& out_cpp,
    Code::ElementList* elements) noexcept
  {
    if(INCLUDE_LIST[0])
    {
      out << INCLUDE_LIST << std::endl << std::endl;
    }

    DeclareGenerator declare_generator(out, out_cpp);

    for(Code::ElementList::const_iterator el_it = elements->begin();
        el_it != elements->end(); ++el_it)
    {
      declare_generator.visit(*el_it);
    }

    out << std::endl;

    ImplGenerator impl_generator(out_inl_impl, out_cpp);
    
    for(Code::ElementList::const_iterator el_it = elements->begin();
        el_it != elements->end(); ++el_it)
    {
      impl_generator.visit(*el_it);
    }

    out_inl_impl << std::endl;
  }
}
