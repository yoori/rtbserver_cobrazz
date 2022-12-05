#include <Stream/MemoryStream.hpp>
#include <Declaration/SimpleType.hpp>
#include <Declaration/Utils.hpp>

#include "Processor.hpp"

namespace Parsing
{
  Processor::Processor(Code::ElementList* elements_val)
    noexcept
    : global_namespace_(new Declaration::Namespace())
         //    ,
//    elements_(ReferenceCounting::add_ref(elements_val)),
//    declarator_(new Code::Declarator(global_namespace_, elements_))
  {
    current_namespace_ = new Code::NamespaceElement(0, global_namespace_.in());
    elements_val->push_back(current_namespace_);
    Code::Declarator_var(new Code::Declarator(global_namespace_, current_namespace_->elements()));
  }

  void Processor::error(
    unsigned long line,
    unsigned long column,
    const char* message)
    noexcept
  {
    std::cerr << "line " << line << ", column " << column << ": " <<
      message << std::endl;
  }

  Declaration::BaseType_var
  Processor::find_type(const TypeSpecifier& type_specifier) const noexcept
  {
    return current_namespace_->namespace_decl()->find_type(
      type_specifier.full_name().c_str());
  }

  void Processor::open_namespace(
    const char* name)
  {
    Code::NamespaceElement_var new_namespace_elem = new Code::NamespaceElement(
      current_namespace_.in(),
      current_namespace_->namespace_decl()->add_namespace(name));
    current_namespace_->add_element(new_namespace_elem);
    current_namespace_ = new_namespace_elem;
  }

  void Processor::close_namespace()
  {
    current_namespace_ = current_namespace_->owner();
  }

  void Processor::open_descriptor(const char* name)
    /*throw(AlreadyDeclared)*/
  {
    Declaration::BaseType_var type_check =
      current_namespace_->namespace_decl()->find_local_type(name);
    if(type_check.in())
    {
      Stream::Error ostr;
      ostr << "can't declare descriptor with name '" << name << "', it already used";
      throw AlreadyDeclared(ostr);
    }

    descriptor_name_ = name;
    descriptor_fields_ = new Declaration::StructDescriptor::FieldList();
  }

  void
  Processor::add_descriptor_field(
    const TypeSpecifier& field_type_specifier,
    const char* name)
    /*throw(IncorrectType)*/
  {
    descriptor_fields_->push_back(
      Declaration::StructDescriptor::Field_var(
        new Declaration::StructDescriptor::Field(
          resolve_descriptor_(field_type_specifier),
          name)));
  }

  void Processor::close_descriptor() noexcept
  {
    Declaration::StructDescriptor_var new_descriptor(
      new Declaration::StructDescriptor(
        descriptor_name_.c_str(), descriptor_fields_));

    current_namespace_->namespace_decl()->add_type(new_descriptor);
    current_namespace_->add_element(
      Code::Element_var(new Code::TypeElement(new_descriptor)));
  }

  void Processor::open_reader(const char* name, const char* base_type_name)
    /*throw(IncorrectType)*/
  {
    Declaration::BaseDescriptor_var base_descriptor =
      resolve_descriptor_(TypeSpecifier(base_type_name));

    Declaration::StructDescriptor_var struct_descriptor =
      base_descriptor->as_struct();

    if(!struct_descriptor.in())
    {
      Stream::Error ostr;
      ostr << "reader base type '" << base_type_name <<
        "' isn't struct descriptor";
      throw IncorrectType(ostr);
    }

    reader_name_ = name;
    reader_descriptor_ = struct_descriptor;
    reader_fields_ = new Declaration::StructReader::FieldReaderList();
  }

  void Processor::add_reader_field(
    const TypeSpecifier& field_type_specifier, const char* name)
    /*throw(IncorrectType)*/
  {
    Declaration::StructDescriptor::Field_var base_field =
      reader_descriptor_->find_field(name);

    if(!base_field.in())
    {
      Stream::Error ostr;
      ostr << "field '" << name <<
        "' is not declared in '" << reader_descriptor_->name() << "'";
      throw IncorrectType(ostr);
    }

    Declaration::BaseReader_var reader;

    const std::string& field_type_name = field_type_specifier.name;

    if(!field_type_specifier.args.empty())
    {
      // init template reader
      reader = init_template_reader_(
        field_type_specifier,
        base_field->descriptor());
    }
    else if(!field_type_name.empty())
    {
      reader = resolve_reader_(
        TypeSpecifier(field_type_name.c_str()));

      if(::strcmp(reader->descriptor()->name(),
           base_field->descriptor()->name()) != 0)
      {
        Stream::Error ostr;
        ostr << "reader type '" << field_type_name << "' isn't reader of '" <<
          base_field->descriptor()->name() <<
          "', it is reader of '" << reader->descriptor()->name() << "'";
        throw IncorrectType(ostr);
      }
    }
    else
    {
      reader = resolve_auto_reader_(
        base_field->descriptor()->name());
    }

    assert(reader.in());

    reader_fields_->push_back(
      Declaration::StructReader::FieldReader_var(
        new Declaration::StructReader::FieldReader(base_field, reader)));
  }

  void Processor::close_reader() noexcept
  {
    assert(reader_descriptor_.in());
    assert(reader_fields_.in());

    Declaration::StructReader_var new_reader(
      new Declaration::StructReader(
        reader_name_.c_str(),
        reader_descriptor_,
        reader_fields_));

    current_namespace_->namespace_decl()->add_type(new_reader);
    current_namespace_->add_element(
      Code::Element_var(new Code::TypeElement(new_reader)));
  }

  void Processor::create_auto_reader(const char* name, const char* base_type_name)
    /*throw(IncorrectType)*/
  {
    Declaration::BaseDescriptor_var base_descriptor =
      resolve_descriptor_(TypeSpecifier(base_type_name));

    Declaration::StructDescriptor_var struct_descriptor =
      base_descriptor->as_struct();

    if(!struct_descriptor.in())
    {
      Stream::Error ostr;
      ostr << "auto reader base type '" << base_type_name <<
        "' isn't struct descriptor";
      throw IncorrectType(ostr);
    }

    Declaration::StructReader_var result_auto_reader =
      resolve_struct_auto_reader_(struct_descriptor);

    Declaration::StructReader_var result_named_auto_reader(
      new Declaration::StructReader(
        name,
        struct_descriptor,
        result_auto_reader->fields()));

    current_namespace_->namespace_decl()->add_type(result_named_auto_reader);
    current_namespace_->add_element(Code::Element_var(
      new Code::TypeElement(result_named_auto_reader)));
  }

  void Processor::open_writer(const char* name, const char* base_type_name)
    /*throw(IncorrectType)*/
  {
    Declaration::BaseDescriptor_var base_descriptor =
      resolve_descriptor_(TypeSpecifier(base_type_name));

    Declaration::StructDescriptor_var struct_descriptor =
      base_descriptor->as_struct();

    if(!struct_descriptor.in())
    {
      Stream::Error ostr;
      ostr << "reader base type '" << base_type_name <<
        "' isn't struct descriptor";
      throw IncorrectType(ostr);
    }

    writer_name_ = name;
    writer_descriptor_ = struct_descriptor;
    writer_fields_ = new Declaration::StructWriter::FieldWriterList();
  }

  void Processor::add_writer_field(
    const TypeSpecifier& field_type_specifier,
    const char* name,
    const IdentifierList& mapping_specifiers_list)
    /*throw(IncorrectType)*/
  {
    Declaration::StructDescriptor::Field_var base_field =
      writer_descriptor_->find_field(name);

    if(!base_field.in())
    {
      Stream::Error ostr;
      ostr << "field '" << name <<
        "' is not declared in '" << writer_descriptor_->name() << "'";
      throw IncorrectType(ostr);
    }

    Declaration::BaseWriter_var writer;

    const std::string& field_type_name = field_type_specifier.name;

    if(!field_type_specifier.args.empty()) // template
    {
      // init template
      writer = init_template_writer_(
        field_type_specifier,
        base_field->descriptor());
    }
    else if(!field_type_name.empty())
    {
      writer = resolve_writer_(
        TypeSpecifier(field_type_name.c_str()));

      // check that writer is writer of descriptor field type
      if(::strcmp(
           writer->descriptor()->name(),
           base_field->descriptor()->name()) != 0)
      {
        Stream::Error ostr;
        ostr << "writer type '" << field_type_name << "' isn't writer of '" <<
          base_field->descriptor()->name() <<
          "', it is writer of '" << writer->descriptor()->name() << "'";
        throw IncorrectType(ostr);
      }
    }
    else
    {
      writer = resolve_auto_writer_(
        base_field->descriptor()->name());
    }

    Declaration::MappingSpecifierSet mapping_specifiers;
    std::copy(mapping_specifiers_list.begin(),
      mapping_specifiers_list.end(),
      std::inserter(mapping_specifiers, mapping_specifiers.begin()));

    // check mapping specifiers compatibility with writer type
    writer->check_mapping_specifiers(mapping_specifiers);

    writer_fields_->push_back(
      Declaration::StructWriter::FieldWriter_var(
        new Declaration::StructWriter::FieldWriter(
          base_field,
          writer,
          mapping_specifiers)));
  }

  void Processor::close_writer() noexcept
  {
    Declaration::StructWriter_var new_writer(
      new Declaration::StructWriter(
        writer_name_.c_str(),
        writer_descriptor_,
        writer_fields_));

    current_namespace_->namespace_decl()->add_type(new_writer);
    current_namespace_->add_element(
      Code::Element_var(new Code::TypeElement(new_writer)));
  }

  void Processor::create_auto_writer(
    const char* name, const char* base_type_name)
    /*throw(IncorrectType)*/
  {
    Declaration::BaseDescriptor_var base_descriptor =
      resolve_descriptor_(TypeSpecifier(base_type_name));

    Declaration::StructDescriptor_var struct_descriptor =
      base_descriptor->as_struct();

    if(!struct_descriptor.in())
    {
      Stream::Error ostr;
      ostr << "auto writer base type '" << base_type_name <<
        "' isn't struct descriptor";
      throw IncorrectType(ostr);
    }

    Declaration::StructWriter_var result_auto_writer =
      resolve_struct_auto_writer_(struct_descriptor);

    Declaration::StructWriter_var result_named_auto_writer(
      new Declaration::StructWriter(
        name,
        struct_descriptor,
        result_auto_writer->fields()));

    current_namespace_->namespace_decl()->add_type(result_named_auto_writer);
    current_namespace_->add_element(Code::Element_var(
      new Code::TypeElement(result_named_auto_writer)));
  }

  void Processor::clone_type(
    const char* /*new_name*/, const TypeSpecifier& /*base_type*/)
    noexcept
  {
    // TODO
  }

  Declaration::BaseDescriptor_var
  Processor::resolve_descriptor_(const TypeSpecifier& type_specifier)
    /*throw(IncorrectType)*/
  {
    Declaration::BaseType_var type = current_namespace_->namespace_decl()->find_type(
      type_specifier.name.c_str());

    if(!type.in())
    {
      Stream::Error ostr;
      ostr << "can't find type '" << type_specifier.name << "'.";
      throw IncorrectType(ostr);
    }

    if(!type_specifier.args.empty()) // template
    {
      // init template
      return init_template_descriptor_(type_specifier);
    }

    Declaration::BaseDescriptor_var descriptor = type->as_descriptor();

    if(!descriptor.in())
    {
      Stream::Error ostr;
      ostr << "type '" << type_specifier.name << "' isn't descriptor.";
      throw IncorrectType(ostr);
    }

    return descriptor;
  }

  Declaration::BaseReader_var
  Processor::resolve_reader_(
    const TypeSpecifier& type_specifier,
    Declaration::BaseDescriptor* descriptor)
    /*throw(IncorrectType)*/
  {
    Declaration::BaseType_var type = current_namespace_->namespace_decl()->find_type(
      type_specifier.name.c_str());

    if(!type.in())
    {
      Stream::Error ostr;
      ostr << "can't find type '" << type_specifier.name << "'.";
      throw IncorrectType(ostr);
    }

    if(!type_specifier.args.empty())
    {
      return init_template_reader_(type_specifier, descriptor);
    }

    Declaration::BaseReader_var reader = type->as_reader();

    if(!reader.in())
    {
      Stream::Error ostr;
      ostr << "type '" << type_specifier.name << "' isn't reader";
      throw IncorrectType(ostr);
    }

    return reader;
  }

  Declaration::BaseWriter_var
  Processor::resolve_writer_(
    const TypeSpecifier& type_specifier,
    Declaration::BaseDescriptor* descriptor)
    /*throw(IncorrectType)*/
  {
    Declaration::BaseType_var type = current_namespace_->namespace_decl()->find_type(
      type_specifier.name.c_str());

    if(!type.in())
    {
      Stream::Error ostr;
      ostr << "can't find type '" << type_specifier.name << "'.";
      throw IncorrectType(ostr);
    }

    if(!type_specifier.args.empty())
    {
      // init template reader
      return init_template_writer_(type_specifier, descriptor);
    }

    Declaration::BaseWriter_var writer = type->as_writer();

    if(!writer.in())
    {
      Stream::Error ostr;
      ostr << "type '" << type_specifier.name << "' isn't reader";
      throw IncorrectType(ostr);
    }

    return writer;
  }

  Declaration::BaseTemplate_var
  Processor::resolve_template_(const char* name)
    /*throw(IncorrectType)*/
  {
    Declaration::BaseType_var template_type =
      current_namespace_->namespace_decl()->find_type(name);

    if(!template_type.in())
    {
      std::ostringstream ostr;
      ostr << "type '" << name << "' isn't defined";
      throw IncorrectType(ostr.str());
    }

    Declaration::BaseTemplate_var templ = template_type->as_template();
    if(!templ.in())
    {
      std::ostringstream ostr;
      ostr << "type '" << template_type->name() << "' isn't template";
      throw IncorrectType(ostr.str());
    }

    return templ;
  }

  Declaration::StructReader_var
  Processor::resolve_struct_auto_reader_(
    Declaration::StructDescriptor* struct_descriptor)
    /*throw(Exception)*/
  {
    const std::string auto_reader_name = std::string("<") +
      struct_descriptor->name();

    Declaration::BaseType_var auto_reader_type =
      current_namespace_->namespace_decl()->find_type(auto_reader_name.c_str());

    if(auto_reader_type.in())
    {
      Declaration::BaseReader_var auto_reader =
        auto_reader_type->as_reader();

      assert(auto_reader.in()); // user can't declare type with name '<...'

      Declaration::StructReader_var struct_auto_reader =
        auto_reader->as_struct_reader();

      assert(struct_auto_reader.in());

      return struct_auto_reader;
    }

    return generate_auto_reader_(
      auto_reader_name.c_str(), struct_descriptor);
  }

  Declaration::StructWriter_var
  Processor::resolve_struct_auto_writer_(
    Declaration::StructDescriptor* struct_descriptor)
    /*throw(IncorrectType)*/
  {
    const std::string auto_writer_name = std::string(">") +
      struct_descriptor->name();

    Declaration::BaseType_var auto_writer_type =
      current_namespace_->namespace_decl()->find_type(auto_writer_name.c_str());

    if(auto_writer_type.in())
    {
      Declaration::BaseWriter_var auto_writer =
        auto_writer_type->as_writer();

      assert(auto_writer.in()); // user can't declare type with name '>...'

      Declaration::StructWriter_var struct_auto_writer =
        auto_writer->as_struct_writer();

      assert(struct_auto_writer.in());

      return struct_auto_writer;
    }

    return generate_auto_writer_(
      auto_writer_name.c_str(), struct_descriptor);
  }

  Declaration::BaseReader_var
  Processor::resolve_auto_reader_(const char* name)
    /*throw(IncorrectType)*/
  {
    Declaration::BaseType_var type =
      current_namespace_->namespace_decl()->find_type(name);
    assert(type.in());

    Declaration::BaseReader_var reader = type->as_reader();
    if(reader.in())
    {
      return reader;
    }

    Declaration::BaseDescriptor_var base_descriptor =
      type->as_descriptor();

    assert(base_descriptor.in());

    Declaration::StructDescriptor_var struct_descriptor =
      base_descriptor->as_struct();

    if(!struct_descriptor.in())
    {
      Stream::Error ostr;
      ostr << "internal error: automatic reader for type '" << name <<
        "' isn't defined, type traits:";
      type_traits(ostr, type);
      throw IncorrectType(ostr);
    }

    return resolve_struct_auto_reader_(struct_descriptor);
  }

  Declaration::BaseWriter_var
  Processor::resolve_auto_writer_(const char* name)
    /*throw(IncorrectType)*/
  {
    // try resolve base type as reader: simple types
    Declaration::BaseType_var type =
      current_namespace_->namespace_decl()->find_type(name);

    assert(type.in()); // internal method, can be called only for registered types

    Declaration::BaseWriter_var writer = type->as_writer();
    if(writer.in())
    {
      return writer;
    }

    Declaration::BaseDescriptor_var base_descriptor =
      type->as_descriptor();

    assert(base_descriptor.in());

    Declaration::StructDescriptor_var struct_descriptor =
      base_descriptor->as_struct();

    if(!struct_descriptor.in())
    {
      Stream::Error ostr;
      ostr << "internal error: automatic reader for type '" << name <<
        "' isn't defined";
      throw IncorrectType(ostr);
    }

    return resolve_struct_auto_writer_(struct_descriptor);
  }

  Declaration::StructReader_var
  Processor::generate_auto_reader_(
    const char* name,
    Declaration::StructDescriptor* struct_descriptor)
    /*throw(Exception)*/
  {
    Declaration::StructReader::FieldReaderList_var reader_fields =
      new Declaration::StructReader::FieldReaderList();

    Declaration::StructDescriptor::PosedFieldList_var descriptor_fields =
      struct_descriptor->fields();

    for(Declaration::StructDescriptor::PosedFieldList::const_iterator field_it =
          descriptor_fields->begin();
        field_it != descriptor_fields->end(); ++field_it)
    {
      Declaration::BaseReader_var reader =
        resolve_auto_reader_((*field_it)->descriptor()->name());

      reader_fields->push_back(
        Declaration::StructReader::FieldReader_var(
          new Declaration::StructReader::FieldReader(
            *field_it, reader)));
    }

    Declaration::StructReader_var result_auto_reader(
      new Declaration::StructReader(
        name,
        struct_descriptor,
        reader_fields));

    current_namespace_->namespace_decl()->add_type(result_auto_reader);

    return result_auto_reader;
  }

  Declaration::StructWriter_var
  Processor::generate_auto_writer_(
    const char* name,
    Declaration::StructDescriptor* struct_descriptor)
    /*throw(IncorrectType)*/
  {
    Declaration::StructWriter::FieldWriterList_var writer_fields =
      new Declaration::StructWriter::FieldWriterList();

    Declaration::StructDescriptor::PosedFieldList_var descriptor_fields =
      struct_descriptor->fields();

    for(Declaration::StructDescriptor::PosedFieldList::const_iterator field_it =
          descriptor_fields->begin();
        field_it != descriptor_fields->end(); ++field_it)
    {
      Declaration::BaseWriter_var writer =
        resolve_auto_writer_((*field_it)->descriptor()->name());

      writer_fields->push_back(
        Declaration::StructWriter::FieldWriter_var(
          new Declaration::StructWriter::FieldWriter(
            *field_it,
            writer,
            Declaration::MappingSpecifierSet())));
    }

    Declaration::StructWriter_var result_auto_writer(
      new Declaration::StructWriter(
        name,
        struct_descriptor,
        writer_fields));

    current_namespace_->namespace_decl()->add_type(result_auto_writer);

    return result_auto_writer;
  }

  Declaration::CompleteTemplateDescriptor_var
  Processor::init_template_descriptor_(
    const TypeSpecifier& type_specifier,
    DescriptorResolve descriptor_resolving)
    /*throw(IncorrectType)*/
  {
    try
    {
      Declaration::BaseDescriptor_var descriptor =
        resolve_descriptor_(type_specifier.full_name().c_str());

      Declaration::CompleteTemplateDescriptor_var complete_template_descriptor =
        descriptor->as_complete_template();

      if(complete_template_descriptor.in())
      {
        return complete_template_descriptor;
      }
    }
    catch(const IncorrectType&)
    {}

    Declaration::BaseTemplate_var template_type =
      resolve_template_(type_specifier.name.c_str());

    Declaration::BaseDescriptorList arg_descriptors;
    for(TypeSpecifierList::const_iterator arg_it =
          type_specifier.args.begin();
        arg_it != type_specifier.args.end(); ++arg_it)
    {
      try
      {
        Declaration::BaseDescriptor_var arg_descriptor =
          descriptor_resolving == DR_DIRECT ?
          resolve_descriptor_(*arg_it) :
            descriptor_resolving == DR_READER ?
            resolve_reader_(*arg_it)->descriptor() :
            resolve_writer_(*arg_it)->descriptor();

        arg_descriptors.push_back(arg_descriptor);
      }
      catch(const Exception& ex)
      {
        std::ostringstream ostr;
        ostr << "can't initialize template '" <<
          template_type->name() << "' argument: " << ex.what();
        throw IncorrectType(ostr.str());
      }
    }

    try
    {
      Declaration::CompleteTemplateDescriptor_var complete_template_descriptor =
        template_type->complete_template_descriptor(arg_descriptors);

      // add into root namespace, argument type namespaces must resolve collisions
      global_namespace_->add_type(complete_template_descriptor);

      return complete_template_descriptor;
    }
    catch(const Declaration::BaseTemplate::InvalidParam& ex)
    {
      std::ostringstream ostr;
      ostr << "can't initialize template '" << template_type->name() << "'";
      throw IncorrectType(ostr.str());
    }
  }

  Declaration::BaseReader_var
  Processor::init_template_reader_(
    const TypeSpecifier& type_specifier,
    Declaration::BaseDescriptor* descriptor)
    /*throw(IncorrectType)*/
  {
    try
    {
      return resolve_reader_(type_specifier.full_name().c_str());
    }
    catch(const IncorrectType&)
    {}

    Declaration::CompleteTemplateDescriptor_var complete_template_descriptor;

    if(descriptor)
    {
      complete_template_descriptor = descriptor->as_complete_template();

      if(!complete_template_descriptor.in())
      {
        std::ostringstream ostr;
        ostr << "type '" << descriptor->name() << "' isn't template";
        throw IncorrectType(ostr.str());
      }
    }
    else
    {
      complete_template_descriptor = init_template_descriptor_(
        type_specifier,
        DR_READER);
    }

    Declaration::BaseReaderList arg_readers;
    for(TypeSpecifierList::const_iterator arg_it =
          type_specifier.args.begin();
        arg_it != type_specifier.args.end(); ++arg_it)
    {
      try
      {
        arg_readers.push_back(resolve_reader_(*arg_it));
      }
      catch(const Exception& ex)
      {
        std::ostringstream ostr;
        ostr << "can't initialize template reader '" <<
          complete_template_descriptor->name() << "': " << ex.what();
        throw IncorrectType(ostr.str());
      }
    }

    try
    {
      Declaration::BaseReader_var complete_template_reader =
        complete_template_descriptor->complete_template_reader(arg_readers);

      // add into root namespace, argument type namespaces must resolve collisions
      global_namespace_->add_type(complete_template_reader);

      return complete_template_reader;
    }
    catch(const Declaration::CompleteTemplateDescriptor::InvalidParam& ex)
    {
      std::ostringstream ostr;
      ostr << "can't initialize template reader '" << descriptor->name() << "': " <<
        ex.what();
      throw IncorrectType(ostr.str());
    }
  }

  Declaration::BaseWriter_var
  Processor::init_template_writer_(
    const TypeSpecifier& type_specifier,
    Declaration::BaseDescriptor* descriptor)
    /*throw(IncorrectType)*/
  {
    try
    {
      return resolve_writer_(type_specifier.full_name().c_str());
    }
    catch(const IncorrectType&)
    {}

    Declaration::CompleteTemplateDescriptor_var complete_template_descriptor;

    if(descriptor)
    {
      complete_template_descriptor = descriptor->as_complete_template();

      if(!complete_template_descriptor.in())
      {
        std::ostringstream ostr;
        ostr << "type '" << descriptor->name() << "' isn't template";
        throw IncorrectType(ostr.str());
      }
    }
    else
    {
      complete_template_descriptor = init_template_descriptor_(
        type_specifier,
        DR_WRITER);
    }

    Declaration::BaseWriterList arg_writers;
    for(TypeSpecifierList::const_iterator arg_it =
          type_specifier.args.begin();
        arg_it != type_specifier.args.end(); ++arg_it)
    {
      try
      {
        arg_writers.push_back(resolve_writer_(*arg_it));
      }
      catch(const Exception& ex)
      {
        std::ostringstream ostr;
        ostr << "can't initialize template writer '" <<
          complete_template_descriptor->name() << "': " << ex.what();
        throw IncorrectType(ostr.str());
      }
    }

    try
    {
      Declaration::BaseWriter_var complete_template_writer =
        complete_template_descriptor->complete_template_writer(arg_writers);

      global_namespace_->add_type(complete_template_writer);

      return complete_template_writer;
    }
    catch(const Declaration::CompleteTemplateDescriptor::InvalidParam& ex)
    {
      std::ostringstream ostr;
      ostr << "can't initialize template writer '" << descriptor->name() << "': " <<
        ex.what();
      throw IncorrectType(ostr.str());
    }
  }
}
