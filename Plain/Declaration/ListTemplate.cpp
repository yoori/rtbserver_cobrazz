#include <stdint.h>
#include <iostream>
#include <sstream>
#include "../Cpp/Config.hpp"
#include "SimpleType.hpp"
#include "StructDescriptor.hpp"
#include "ListTemplate.hpp"

namespace Declaration
{
  /**
   * complete_template_descriptor:
   *   for simple type:
   *     BaseType ->
   *       descriptor -> as_complete_template, as_simple
   *       reader -> as_simple_reader
   *       writer -> as_simple_writer
   *   for struct type:
   *     BaseType ->
   *       descriptor -> as_complete_template
   *       writer -> as_simple_writer (for protected holder generation)
   */

  namespace
  {
    const char* VECTOR_SPECIFIER = "cpp_vector";

    //
    // traits generator for arrays that can be accessed by simple way
    // without mediators, and with fixed size record
    //
    class CppWriteTraitsGeneratorSimpleArrayImpl:
      public SimpleWriter::CppWriteTraitsGenerator
    {
    public:
      CppWriteTraitsGeneratorSimpleArrayImpl(
        const char* holder_type,
        const char* read_type_cast,
        const char* write_type_cast,
        unsigned long fixed_size)
        : cpp_list_write_traits_(
          generate_cpp_write_traits_(
            "SimpleList",
            holder_type,
            read_type_cast,
            write_type_cast,
            fixed_size)),
          cpp_vector_write_traits_(
            generate_cpp_write_traits_(
              "SimpleVector",
              holder_type,
              read_type_cast,
              write_type_cast,
              fixed_size))
      {}

      virtual bool
      check_mapping_specifier(const char* mapping_specifier)
        noexcept
      {
        return ::strcmp(mapping_specifier, VECTOR_SPECIFIER) == 0;
      }

      virtual SimpleWriter::CppWriteTraits_var
      generate(const MappingSpecifierSet& mapping_specifiers)
        noexcept
      {
        MappingSpecifierSet::const_iterator specifier_it =
          mapping_specifiers.find(VECTOR_SPECIFIER);
        if(specifier_it != mapping_specifiers.end())
        {
          return cpp_vector_write_traits_;
        }

        return cpp_list_write_traits_;
      }

    protected:
      virtual ~CppWriteTraitsGeneratorSimpleArrayImpl() noexcept
      {}

      static
      SimpleWriter::CppWriteTraits_var
      generate_cpp_write_traits_(
        const char* base_type,
        const char* holder_type,
        const char* read_type_cast,
        const char* write_type_cast,
        unsigned long fixed_size)
        noexcept
      {
        std::ostringstream cpp_write_type_name;
        cpp_write_type_name << "PlainTypes::" << base_type << "<" <<
          holder_type << ", " <<
          read_type_cast << ", " <<
          write_type_cast << ", " <<
          fixed_size << ">";

        return new SimpleWriter::CppWriteTraits(
          (cpp_write_type_name.str() + "&").c_str(),
          "",
          (std::string("const ") + cpp_write_type_name.str() + "&").c_str(),
          "",
          cpp_write_type_name.str().c_str(),
          cpp_write_type_name.str().c_str(),
          "_Container");
      }

    protected:
      SimpleWriter::CppWriteTraits_var cpp_list_write_traits_;
      SimpleWriter::CppWriteTraits_var cpp_vector_write_traits_;
    };

    class CppWriteTraitsGeneratorArrayImpl:
      public SimpleWriter::CppWriteTraitsGenerator
    {
    public:
      CppWriteTraitsGeneratorArrayImpl(const char* holder_type)
        : cpp_list_write_traits_(
            generate_cpp_write_traits_("List", holder_type)),
          cpp_vector_write_traits_(
            generate_cpp_write_traits_("Vector", holder_type))
      {}

      virtual bool
      check_mapping_specifier(const char* mapping_specifier)
        noexcept
      {
        return ::strcmp(mapping_specifier, VECTOR_SPECIFIER) == 0;
      }

      virtual SimpleWriter::CppWriteTraits_var
      generate(const MappingSpecifierSet& mapping_specifiers)
        noexcept
      {
        MappingSpecifierSet::const_iterator specifier_it =
          mapping_specifiers.find(VECTOR_SPECIFIER);
        if(specifier_it != mapping_specifiers.end())
        {
          return cpp_vector_write_traits_;
        }

        return cpp_list_write_traits_;
      }

    protected:
      virtual ~CppWriteTraitsGeneratorArrayImpl() noexcept
      {}

      static
      SimpleWriter::CppWriteTraits_var
      generate_cpp_write_traits_(
        const char* base_type,
        const char* holder_type)
        noexcept
      {
        std::ostringstream cpp_write_type_name;
        cpp_write_type_name << "PlainTypes::" << base_type <<
          "<" << holder_type << ">";

        return new SimpleWriter::CppWriteTraits(
          (cpp_write_type_name.str() + "&").c_str(),
          "",
          (std::string("const ") + cpp_write_type_name.str() + "&").c_str(),
          "",
          cpp_write_type_name.str().c_str(),
          cpp_write_type_name.str().c_str(),
          "_Container");
      }

    protected:
      SimpleWriter::CppWriteTraits_var cpp_list_write_traits_;
      SimpleWriter::CppWriteTraits_var cpp_vector_write_traits_;
    };
  };

  /* ArrayCompleteTemplateDescriptor */
  class ArrayCompleteTemplateDescriptor:
    public virtual ReferenceCounting::DefaultImpl<>,
    public CompleteTemplateDescriptor
  {
  public:
    ArrayCompleteTemplateDescriptor(
      const char* name,
      const BaseDescriptorList& args,
      unsigned long header_size)
      noexcept;

    virtual bool is_fixed() const noexcept;

    virtual SizeType fixed_size() const noexcept;

  protected:
    virtual ~ArrayCompleteTemplateDescriptor() noexcept {}

    virtual BaseReader_var
    create_template_reader_(const BaseReaderList& args)
      /*throw(InvalidParam)*/;

    virtual BaseWriter_var
    create_template_writer_(const BaseWriterList& args)
      /*throw(InvalidParam)*/;

    virtual BaseReader_var
    create_array_reader_(BaseReader* arg_reader)
      /*throw(InvalidParam)*/;

    virtual BaseWriter_var
    create_array_writer_(BaseWriter* arg_writer)
      /*throw(InvalidParam)*/;

  private:
    const unsigned long header_size_;
  };

  /* ArrayReader */
  class ArrayReader:
    public virtual ReferenceCounting::AtomicImpl,
    public virtual SimpleReader
  {
  public:
    ArrayReader(
      const char* name,
      BaseDescriptor* descriptor,
      const SimpleReader::CppReadTraits& cpp_read_traits);

    virtual BaseDescriptor_var descriptor() noexcept;

  protected:
    virtual ~ArrayReader() noexcept {}

  private:
    BaseDescriptor_var descriptor_;
  };

  /* ArrayWriter */
  class ArrayWriter:
    public virtual ReferenceCounting::AtomicImpl,
    public virtual SimpleWriter
  {
  public:
    ArrayWriter(
      const char* name,
      BaseDescriptor* descriptor,
      SimpleWriter::CppWriteTraitsGenerator* cpp_write_traits_generator);

    virtual BaseDescriptor_var descriptor() noexcept;

    virtual void
    check_mapping_specifiers(const Declaration::MappingSpecifierSet&)
      /*throw(InvalidMappingSpecifier)*/;

  protected:
    virtual ~ArrayWriter() noexcept {}

  private:
    BaseDescriptor_var descriptor_;
  };

  /* SimpleArrayType */
  class SimpleArrayType:
    public virtual ReferenceCounting::DefaultImpl<>,
    public virtual SimpleType,
    public virtual ArrayCompleteTemplateDescriptor
  {
  public:
    SimpleArrayType(
      const char* name,
      bool is_fixed_val,
      SizeType fixed_size_val,
      const Declaration::SimpleReader::CppReadTraits& cpp_read_traits,
      Declaration::SimpleWriter::CppWriteTraitsGenerator* cpp_write_traits_generator,
      const BaseDescriptorList& args)
      noexcept;

    virtual CompleteTemplateDescriptor_var
    as_complete_template() noexcept;

    bool is_fixed() const noexcept;

    SizeType fixed_size() const noexcept;

  protected:
    virtual ~SimpleArrayType() noexcept {};
  };

  /* StructArrayType */
  class StructArrayType:
    public virtual ReferenceCounting::DefaultImpl<>,
    public virtual SimpleWriter,
    public virtual ArrayCompleteTemplateDescriptor
  {
  public:
    StructArrayType(
      const char* name,
      Declaration::SimpleWriter::CppWriteTraitsGenerator* cpp_write_traits_generator,
      const BaseDescriptorList& args,
      unsigned long fixed_size_val)
      noexcept;

    virtual CompleteTemplateDescriptor_var
    as_complete_template() noexcept;

    bool is_fixed() const noexcept;

    SizeType fixed_size() const noexcept;

    virtual BaseDescriptor_var descriptor() noexcept
    {
      return ReferenceCounting::add_ref(this);
    };

  protected:
    virtual ~StructArrayType() noexcept {};
  };

  /**
   * Implementations
   */
  void
  init_cpp_reader_mapping(
    std::string& read_type_name,
    std::string& result_init_read_type_fun,
    unsigned long element_fixed_size,
    const char* element_read_type_name,
    const char* element_read_cast_fun,
    bool check_element_bounds)
  {
    std::ostringstream read_type_name_ostr;
    read_type_name_ostr << "PlainTypes::ConstVector<" <<
      "PlainTypes::ReadPolicy::" <<
      (check_element_bounds ? "CheckBounds" : "NoCheck") << "<" <<
        element_read_type_name << ", " <<
        element_read_cast_fun << " >, " << // c-tor as caster
      element_fixed_size << ">";

    read_type_name = read_type_name_ostr.str();

    std::ostringstream init_read_type_fun_ostr;
    init_read_type_fun_ostr <<
      "PlainTypes::init_const_vector<" <<
      read_type_name << " >";

    result_init_read_type_fun = init_read_type_fun_ostr.str();
  }

  /*
   * ArrayCompleteTemplateDescriptor
   */
  ArrayCompleteTemplateDescriptor::ArrayCompleteTemplateDescriptor(
    const char* name,
    const BaseDescriptorList& args,
    unsigned long header_size)
    noexcept
    : BaseType(name),
      BaseDescriptor(name),
      CompleteTemplateDescriptor(name, args),
      header_size_(header_size)
  {}

  bool ArrayCompleteTemplateDescriptor::is_fixed() const noexcept
  {
    return false;
  }

  SizeType
  ArrayCompleteTemplateDescriptor::fixed_size() const noexcept
  {
    return header_size_;
  }

  BaseReader_var
  ArrayCompleteTemplateDescriptor::create_template_reader_(
    const BaseReaderList& args)
    /*throw(InvalidParam)*/
  {
    return create_array_reader_(*args.begin());
  }

  BaseWriter_var
  ArrayCompleteTemplateDescriptor::create_template_writer_(
    const BaseWriterList& args)
    /*throw(InvalidParam)*/
  {
    return create_array_writer_(*args.begin());
  }

  BaseReader_var
  ArrayCompleteTemplateDescriptor::create_array_reader_(
    BaseReader* arg_reader)
    /*throw(InvalidParam)*/
  {
    std::string cpp_read_type_name;
    std::string cpp_init_read_type_fun;

    init_cpp_reader_mapping(
      cpp_read_type_name,
      cpp_init_read_type_fun,
      arg_reader->descriptor()->fixed_size(),
      arg_reader->name(),
      (std::string("PlainTypes::") +
        (arg_reader->descriptor()->is_fixed() ? "ctor_read_cast" :
           "ctor_with_bounds_read_cast") + "<" +
        arg_reader->name() + " >").c_str(),
      !arg_reader->descriptor()->is_fixed());

    return new ArrayReader(
      (std::string("list<") + arg_reader->name() + ">").c_str(),
      arg_reader->descriptor(),
      Declaration::SimpleReader::CppReadTraits(
        cpp_read_type_name.c_str(),
        cpp_init_read_type_fun.c_str(),
        true, // init with using size
        cpp_read_type_name.c_str(),
        "_Container"));
  }

  BaseWriter_var
  ArrayCompleteTemplateDescriptor::create_array_writer_(
    BaseWriter* arg_writer)
    /*throw(InvalidParam)*/
  {
    return new ArrayWriter(
      (std::string("list<") + arg_writer->name() + ">").c_str(),
      arg_writer->descriptor(),
      SimpleWriter::CppWriteTraitsGenerator_var(
        new CppWriteTraitsGeneratorArrayImpl(arg_writer->name())));
  }

  /* ArrayReader impl */
  ArrayReader::ArrayReader(
    const char* name,
    BaseDescriptor* descriptor,
    const SimpleReader::CppReadTraits& cpp_read_traits)
    : BaseType(name),
      BaseReader(name),
      SimpleReader(name, cpp_read_traits),
      descriptor_(ReferenceCounting::add_ref(descriptor))
  {}

  BaseDescriptor_var
  ArrayReader::descriptor() noexcept
  {
    return descriptor_;
  }

  // ArrayWriter impl
  ArrayWriter::ArrayWriter(
    const char* name,
    BaseDescriptor* descriptor,
    SimpleWriter::CppWriteTraitsGenerator* cpp_write_traits_generator)
    : BaseType(name),
      SimpleWriter(name, cpp_write_traits_generator),
      descriptor_(ReferenceCounting::add_ref(descriptor))
  {}

  BaseDescriptor_var
  ArrayWriter::descriptor() noexcept
  {
    return descriptor_;
  }

  void
  ArrayWriter::check_mapping_specifiers(
    const Declaration::MappingSpecifierSet& mapping_specifiers)
    /*throw(InvalidMappingSpecifier)*/
  {
    for(Declaration::MappingSpecifierSet::const_iterator it =
          mapping_specifiers.begin();
        it != mapping_specifiers.end(); ++it)
    {
      if(!cpp_write_traits_generator()->check_mapping_specifier(it->c_str()))
      {
        Stream::Error ostr;
        ostr << "Incorrect specifier: " << *it;
        throw InvalidMappingSpecifier(ostr);
      }
    }
  }

  // SimpleArrayType impl
  SimpleArrayType::SimpleArrayType(
    const char* name,
    bool is_fixed_val,
    SizeType fixed_size_val,
    const Declaration::SimpleReader::CppReadTraits& cpp_read_traits,
    Declaration::SimpleWriter::CppWriteTraitsGenerator* cpp_write_traits_generator,
    const BaseDescriptorList& args)
    noexcept
    : BaseType(name),
      BaseDescriptor(name),
      BaseReader(name),
      SimpleType(
        name,
        is_fixed_val,
        fixed_size_val,
        cpp_read_traits,
        cpp_write_traits_generator),
      ArrayCompleteTemplateDescriptor(name, args, fixed_size_val)
  {}

  CompleteTemplateDescriptor_var
  SimpleArrayType::as_complete_template() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  bool SimpleArrayType::is_fixed() const noexcept
  {
    return ArrayCompleteTemplateDescriptor::is_fixed();
  }

  SizeType
  SimpleArrayType::fixed_size() const noexcept
  {
    return ArrayCompleteTemplateDescriptor::fixed_size();
  }

  StructArrayType::StructArrayType(
    const char* name,
    Declaration::SimpleWriter::CppWriteTraitsGenerator* cpp_write_traits_generator,
    const BaseDescriptorList& args,
    unsigned long header_size)
    noexcept
    : BaseType(name),
      SimpleWriter(name, cpp_write_traits_generator),
      BaseDescriptor(name),
      ArrayCompleteTemplateDescriptor(name, args, header_size)
  {}

  CompleteTemplateDescriptor_var
  StructArrayType::as_complete_template() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  bool StructArrayType::is_fixed() const noexcept
  {
    return ArrayCompleteTemplateDescriptor::is_fixed();
  }

  SizeType
  StructArrayType::fixed_size() const noexcept
  {
    return ArrayCompleteTemplateDescriptor::fixed_size();
  }

  /* BaseArrayTemplate */
  BaseArrayTemplate::BaseArrayTemplate(
    const char* name,
    unsigned long header_size) noexcept
    : BaseTemplate(name, 1),
      header_size_(header_size)
  {}

  CompleteTemplateDescriptor_var
  BaseArrayTemplate::create_template_descriptor_(
    const char* /*name*/,
    const BaseDescriptorList& args) const
    /*throw(InvalidParam)*/
  {
    Declaration::SimpleDescriptor_var arg_simple_descriptor =
      (*args.begin())->as_simple();

    if(arg_simple_descriptor.in())
    {
      // if type is simple create BaseType with reader/writer
      return create_array_simple_type_(
        arg_simple_descriptor);
    }

    return create_array_struct_type_(
      *args.begin());
  }

  CompleteTemplateDescriptor_var
  BaseArrayTemplate::create_array_simple_type_(
    BaseDescriptor* descriptor) const
    noexcept
  {
    BaseReader_var reader = descriptor->as_reader();
    assert(reader.in());

    BaseWriter_var writer = descriptor->as_writer();
    assert(writer.in());

    SimpleReader_var simple_reader = reader->as_simple_reader();
    assert(simple_reader.in());

    SimpleWriter_var simple_writer = writer->as_simple_writer();
    assert(simple_writer.in());

    std::string cpp_read_type_name;
    std::string cpp_init_read_type_fun;

    init_cpp_reader_mapping(
      cpp_read_type_name,
      cpp_init_read_type_fun,
      descriptor->fixed_size(),
      simple_reader->cpp_read_traits().read_type_name.c_str(),
      simple_reader->cpp_read_traits().read_type_cast.c_str(),
      !descriptor->is_fixed());

    std::ostringstream cpp_write_type_name;

    std::string cpp_base_write_type_name_suffix = "List";

    /*
    if(mapping_specifiers.find(CPP_VECTOR) != mapping_specifiers.end())
    {
      cpp_base_write_type_name_suffix = "Vector";
    }
    else
    {
      cpp_base_write_type_name_suffix = "List";
    }
    */

    SimpleWriter::CppWriteTraits_var cpp_write_traits =
      simple_writer->cpp_write_traits_generator()->generate(
        MappingSpecifierSet());

    if(descriptor->is_fixed())
    {
      cpp_write_type_name << "PlainTypes::Simple" <<
        cpp_base_write_type_name_suffix << "<" <<
        cpp_write_traits->holder_type_name << ", " <<
        simple_reader->cpp_read_traits().read_type_cast << ", " <<
        cpp_write_traits->write_type_cast << ", " <<
        descriptor->fixed_size() << ">";
    }
    else
    {
      cpp_write_type_name << "PlainTypes::" <<
        cpp_base_write_type_name_suffix << "<" <<
        cpp_write_traits->holder_type_name << ">";
    }

    BaseDescriptorList args;
    args.push_back(ReferenceCounting::add_ref(descriptor));

    SimpleWriter::CppWriteTraits_var arg_cpp_write_traits =
      simple_writer->cpp_write_traits_generator()->generate(
        MappingSpecifierSet());

    return new SimpleArrayType(
      (std::string(name()) + "<" + descriptor->name() + ">").c_str(),
      false,
      header_size_,
      Declaration::SimpleReader::CppReadTraits(
        cpp_read_type_name.c_str(),
        cpp_init_read_type_fun.c_str(),
        true, // init with using size
        cpp_read_type_name.c_str(),
        "_Container"),
      descriptor->is_fixed() ?
        SimpleWriter::CppWriteTraitsGenerator_var(
          new CppWriteTraitsGeneratorSimpleArrayImpl(
            arg_cpp_write_traits->holder_type_name.c_str(),
            simple_reader->cpp_read_traits().read_type_cast.c_str(),
            arg_cpp_write_traits->write_type_cast.c_str(),
            descriptor->fixed_size())) :
      SimpleWriter::CppWriteTraitsGenerator_var(
        new CppWriteTraitsGeneratorArrayImpl(
          arg_cpp_write_traits->holder_type_name.c_str())),
      args);
  }

  CompleteTemplateDescriptor_var
  BaseArrayTemplate::create_array_struct_type_(
    BaseDescriptor* descriptor) const
    noexcept
  {
    StructDescriptor_var struct_descriptor = descriptor->as_struct();
    assert(struct_descriptor.in());

    std::ostringstream cpp_holder_type_name_str;
    cpp_holder_type_name_str <<
      struct_descriptor->name() << Cpp::PROTECTED_WRITER_SUFFIX;
    std::string cpp_holder_type_name = cpp_holder_type_name_str.str();

    BaseDescriptorList args;
    args.push_back(ReferenceCounting::add_ref(descriptor));

    return new StructArrayType(
      (std::string(name()) + "<" + descriptor->name() + ">").c_str(),
      SimpleWriter::CppWriteTraitsGenerator_var(
        new CppWriteTraitsGeneratorArrayImpl(
          cpp_holder_type_name.c_str())),
      args,
      header_size_);
  }

  /* ArrayTemplate */
  ArrayTemplate::ArrayTemplate() noexcept
    : BaseArrayTemplate("array", sizeof(uint32_t) * 2)
  {}

  /* CompatibilityListTemplate */
  CompatibilityListTemplate::CompatibilityListTemplate() noexcept
    : BaseArrayTemplate("list", sizeof(uint32_t) * 3)
  {}
}
