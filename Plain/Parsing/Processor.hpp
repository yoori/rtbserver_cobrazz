#ifndef PLAIN_PARSING_PROCESSOR_HPP
#define PLAIN_PARSING_PROCESSOR_HPP

#include <sstream>
#include <Declaration/SimpleDescriptor.hpp>
#include <Declaration/StructDescriptor.hpp>
#include <Declaration/SimpleReader.hpp>
#include <Declaration/StructReader.hpp>
#include <Declaration/SimpleWriter.hpp>
#include <Declaration/StructWriter.hpp>
#include <Declaration/Namespace.hpp>

#include <Code/Element.hpp>
#include <Code/NamespaceElement.hpp>
#include <Code/TypeElement.hpp>
#include <Code/TypeDefElement.hpp>
#include <Code/Declarator.hpp>

typedef std::string Identifier;
typedef std::list<Identifier> IdentifierList;

struct TypeSpecifier
{
  TypeSpecifier(const char* name_val) noexcept;

  TypeSpecifier(const char* name_val, const std::list<TypeSpecifier>& args_val)
    noexcept;

  std::string full_name() const noexcept;

  const std::string name;
  const std::list<TypeSpecifier> args;
};

typedef std::list<TypeSpecifier> TypeSpecifierList;

namespace Parsing
{
  class Processor: public ReferenceCounting::DefaultImpl<>
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(AlreadyDeclared, Exception);
    DECLARE_EXCEPTION(IncorrectType, Exception);

  public:
    Processor(Code::ElementList* elements_val) noexcept;

    void error(
      unsigned long line,
      unsigned long column,
      const char* message) noexcept;

    Declaration::Namespace_var
    root_namespace() const noexcept;

    Declaration::BaseType_var
    find_type(const TypeSpecifier& type_name) const noexcept;

    // namespace declaration
    void open_namespace(const char* name);

    void close_namespace();

    // descriptor declaration
    void open_descriptor(const char* name)
      /*throw(AlreadyDeclared)*/;

    void add_descriptor_field(
      const TypeSpecifier& field_type, const char* name)
      /*throw(IncorrectType)*/;

    void close_descriptor() noexcept;

    // reader declaration
    void open_reader(const char* name, const char* base_type_name)
      /*throw(IncorrectType)*/;

    void add_reader_field(
      const TypeSpecifier& field_type, const char* name)
      /*throw(IncorrectType)*/;

    void close_reader() noexcept;

    void create_auto_reader(const char* name, const char* base_type_name)
      /*throw(IncorrectType)*/;

    // writer declaration
    void open_writer(const char* name, const char* base_type_name)
      /*throw(IncorrectType)*/;

    void add_writer_field(
      const TypeSpecifier& field_type,
      const char* name,
      const IdentifierList& mapping_specifiers = IdentifierList())
      /*throw(IncorrectType)*/;

    void close_writer() noexcept;

    void create_auto_writer(const char* name, const char* base_type_name)
      /*throw(IncorrectType)*/;

    void clone_type(const char* new_name, const TypeSpecifier& base_type)
      noexcept;

  protected:
    enum DescriptorResolve
    {
      DR_DIRECT,
      DR_READER,
      DR_WRITER
    };

  protected:
    virtual ~Processor() noexcept {}

    // basic help functions
    Declaration::BaseDescriptor_var
    resolve_descriptor_(const TypeSpecifier& type_specifier)
      /*throw(IncorrectType)*/;

    Declaration::BaseReader_var
    resolve_reader_(
      const TypeSpecifier& type_specifier,
      Declaration::BaseDescriptor* descriptor = 0)
      /*throw(IncorrectType)*/;

    Declaration::BaseWriter_var
    resolve_writer_(
      const TypeSpecifier& type_specifier,
      Declaration::BaseDescriptor* descriptor = 0)
      /*throw(IncorrectType)*/;

    Declaration::BaseTemplate_var
    resolve_template_(const char* name)
      /*throw(IncorrectType)*/;

    // auto types manipulations
    Declaration::StructReader_var
    resolve_struct_auto_reader_(
      Declaration::StructDescriptor* struct_descriptor)
      /*throw(Exception)*/;

    Declaration::StructWriter_var
    resolve_struct_auto_writer_(
      Declaration::StructDescriptor* struct_descriptor)
      /*throw(IncorrectType)*/;

    Declaration::BaseReader_var
    resolve_auto_reader_(const char* name) /*throw(IncorrectType)*/;

    Declaration::BaseWriter_var
    resolve_auto_writer_(const char* name) /*throw(IncorrectType)*/;

    Declaration::StructReader_var
    generate_auto_reader_(
      const char* name,
      Declaration::StructDescriptor* struct_descriptor)
      /*throw(Exception)*/;

    Declaration::StructWriter_var
    generate_auto_writer_(
      const char* name,
      Declaration::StructDescriptor* struct_descriptor)
      /*throw(IncorrectType)*/;

    // template manipulations
    Declaration::CompleteTemplateDescriptor_var
    init_template_descriptor_(
      const TypeSpecifier& type_specifier,
      DescriptorResolve descriptor_resolving = DR_DIRECT)
      /*throw(IncorrectType)*/;

    Declaration::BaseReader_var
    init_template_reader_(
      const TypeSpecifier& type_specifier,
      Declaration::BaseDescriptor* descriptor = 0 // complete template descriptor
      )
      /*throw(IncorrectType)*/;

    Declaration::BaseWriter_var
    init_template_writer_(
      const TypeSpecifier& type_specifier,
      Declaration::BaseDescriptor* descriptor = 0 // complete template descriptor
      )
      /*throw(IncorrectType)*/;

  private:
    /* parsing context */
    Declaration::Namespace_var global_namespace_;
    Code::NamespaceElement_var current_namespace_;
//  Code::ElementList_var elements_;
//  Code::Declarator_var declarator_;

    // descriptor declaration
    std::string descriptor_name_;
    Declaration::StructDescriptor::FieldList_var descriptor_fields_;

    // reader declaration
    std::string reader_name_;
    Declaration::StructDescriptor_var reader_descriptor_;
    Declaration::StructReader::FieldReaderList_var reader_fields_;

    // writer declaration
    std::string writer_name_;
    Declaration::StructDescriptor_var writer_descriptor_;
    Declaration::StructWriter::FieldWriterList_var writer_fields_;
  };

  typedef ReferenceCounting::SmartPtr<Processor> Processor_var;
}

// Inlines
// TypeSpecifier impl
inline
TypeSpecifier::TypeSpecifier(const char* name_val) noexcept
  : name(name_val)
{}

inline
TypeSpecifier::TypeSpecifier(
  const char* name_val, const std::list<TypeSpecifier>& args_val)
  noexcept
  : name(name_val), args(args_val)
{}

inline
std::string
TypeSpecifier::full_name() const noexcept
{
  if(!args.empty())
  {
    std::ostringstream name_ostr;
    name_ostr << name << "<";
    for(std::list<TypeSpecifier>::const_iterator ait = args.begin();
        ait != args.end(); ++ait)
    {
      if(ait != args.begin())
      {
        name_ostr << ",";
      }
      name_ostr << ait->full_name();
    }
    name_ostr << ">";
    return name_ostr.str();
  }

  return name;
}

namespace Parsing
{
  inline
  Declaration::Namespace_var
  Processor::root_namespace() const noexcept
  {
    return global_namespace_;
  }
}

#endif /*PLAIN_PARSING_PROCESSOR_HPP*/
