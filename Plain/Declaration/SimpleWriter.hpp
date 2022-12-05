#ifndef _SIMPLEWRITER_HPP_
#define _SIMPLEWRITER_HPP_

#include <Stream/MemoryStream.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include "BaseWriter.hpp"

namespace Declaration
{
  class SimpleWriter: public BaseWriter
  {
    /* required override holder_traits, cast_fun */
  public:
    struct CppWriteTraits: public ReferenceCounting::DefaultImpl<>
    {
      CppWriteTraits(
        const char* write_type_reference_val,
        const char* write_type_cast_val,
        const char* write_type_const_reference_val,
        const char* write_type_const_cast_val,
        const char* write_type_val,
        const char* holder_type_name_val,
        const char* field_type_suffix_val = "")
        : write_type_name(write_type_reference_val),
          write_type_cast(write_type_cast_val),
          write_type_const_name(write_type_const_reference_val),
          write_type_const_cast(write_type_const_cast_val),
          write_type(write_type_val),
          holder_type_name(holder_type_name_val),
          field_type_suffix(field_type_suffix_val)
      {}

      // write accessor return type
      std::string write_type_name;
      // for fixed: void* -> write_type_name&
      std::string write_type_cast;

      // const write accessor return type
      std::string write_type_const_name;
      // for fixed: const void* -> const write_type_name&
      std::string write_type_const_cast;

      std::string write_type;

      // for not fixed type: holder type
      std::string holder_type_name;

      std::string field_type_suffix;

    protected:
      virtual ~CppWriteTraits() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<CppWriteTraits>
      CppWriteTraits_var;

    class CppWriteTraitsGenerator: public ReferenceCounting::DefaultImpl<>
    {
    public:
      virtual bool
      check_mapping_specifier(const char* mapping_specifier)
        noexcept = 0;

      virtual CppWriteTraits_var
      generate(const MappingSpecifierSet& mapping_specifiers)
        noexcept = 0;

    protected:
      virtual ~CppWriteTraitsGenerator() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<CppWriteTraitsGenerator>
      CppWriteTraitsGenerator_var;

    SimpleWriter(
      const char* name_val,
      CppWriteTraitsGenerator* cpp_write_traits_generator)
      noexcept;

    virtual SimpleWriter_var as_simple_writer() noexcept;

    virtual void
    check_mapping_specifiers(
      const Declaration::MappingSpecifierSet& mapping_specifiers)
      /*throw(InvalidMappingSpecifier)*/;

    CppWriteTraitsGenerator_var
    cpp_write_traits_generator() const noexcept;

  protected:
    virtual ~SimpleWriter() noexcept {}

  private:
    CppWriteTraitsGenerator_var cpp_write_traits_generator_;
  };

  typedef ReferenceCounting::SmartPtr<SimpleWriter>
    SimpleWriter_var;

  class CppWriteTraitsGeneratorNoSpecifiersImpl:
    public SimpleWriter::CppWriteTraitsGenerator
  {
  public:
    CppWriteTraitsGeneratorNoSpecifiersImpl(
      SimpleWriter::CppWriteTraits* cpp_write_traits)
      : cpp_write_traits_(ReferenceCounting::add_ref(cpp_write_traits))
    {}

    virtual bool
    check_mapping_specifier(const char* /*mapping_specifier*/)
      noexcept
    {
      return false;
    }

    virtual SimpleWriter::CppWriteTraits_var
    generate(const MappingSpecifierSet& /*mapping_specifiers*/)
      noexcept
    {
      return cpp_write_traits_;
    }

  protected:
    virtual ~CppWriteTraitsGeneratorNoSpecifiersImpl() noexcept
    {}

  protected:
    SimpleWriter::CppWriteTraits_var cpp_write_traits_;
  };
}

namespace Declaration
{
  // SimpleWriter impl
  inline
  SimpleWriter::SimpleWriter(
    const char* name_val,
    CppWriteTraitsGenerator* cpp_write_traits_generator)
    noexcept
    : BaseType(name_val),
      BaseWriter(name_val),
      cpp_write_traits_generator_(
        ReferenceCounting::add_ref(cpp_write_traits_generator))
  {}

  inline
  SimpleWriter_var
  SimpleWriter::as_simple_writer() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  inline
  void
  SimpleWriter::check_mapping_specifiers(
    const Declaration::MappingSpecifierSet& mapping_specifiers)
    /*throw(InvalidMappingSpecifier)*/
  {
    for(Declaration::MappingSpecifierSet::const_iterator it =
          mapping_specifiers.begin();
        it != mapping_specifiers.end(); ++it)
    {
      if(!cpp_write_traits_generator_->check_mapping_specifier(it->c_str()))
      {
        Stream::Error ostr;
        ostr << "Incorrect specifier: " << *it;
        throw InvalidMappingSpecifier(ostr);
      }
    }
  }

  inline
  SimpleWriter::CppWriteTraitsGenerator_var
  SimpleWriter::cpp_write_traits_generator() const noexcept
  {
    return cpp_write_traits_generator_;
  }
}

#endif /*_SIMPLEWRITER_HPP_*/
