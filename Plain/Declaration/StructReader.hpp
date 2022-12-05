#ifndef _STRUCTREADER_HPP_
#define _STRUCTREADER_HPP_

#include <list>
#include "StructDescriptor.hpp"
#include "BaseReader.hpp"

namespace Declaration
{
  class StructReader:
    public virtual ReferenceCounting::AtomicImpl,
    public BaseReader
  {
  public:
    class FieldReader: public ReferenceCounting::AtomicImpl
    {
    public:
      FieldReader(
        StructDescriptor::Field* field_val,
        BaseReader* reader_val)
        noexcept;

      const char* name() const noexcept;

      StructDescriptor::Field_var descriptor_field() const noexcept;

      BaseReader_var reader() const noexcept;

    private:
      StructDescriptor::Field_var field_;
      BaseReader_var reader_;
    };

    typedef ReferenceCounting::SmartPtr<FieldReader>
      FieldReader_var;

    struct FieldReaderList:
      public std::list<FieldReader_var>,
      public ReferenceCounting::AtomicImpl
    {
    protected:
      virtual ~FieldReaderList() noexcept {}
    };
    
    typedef ReferenceCounting::SmartPtr<FieldReaderList>
      FieldReaderList_var;

  public:
    StructReader(
      const char* name_val,
      BaseDescriptor* descriptor_val,
      FieldReaderList* fields_val);

    FieldReaderList_var fields() const;

    virtual BaseDescriptor_var descriptor() noexcept;

    virtual StructReader_var as_struct_reader() noexcept;

  private:
    BaseDescriptor_var descriptor_;
    FieldReaderList_var fields_;
  };

  typedef ReferenceCounting::SmartPtr<StructReader>
    StructReader_var;
}

namespace Declaration
{
  inline
  StructReader::FieldReader::FieldReader(
    StructDescriptor::Field* field_val,
    BaseReader* reader_val)
    noexcept
    : field_(ReferenceCounting::add_ref(field_val)),
      reader_(ReferenceCounting::add_ref(reader_val))
  {}

  inline
  const char*
  StructReader::FieldReader::name() const noexcept
  {
    return field_->name();
  }

  inline
  StructDescriptor::Field_var
  StructReader::FieldReader::descriptor_field() const noexcept
  {
    return field_;
  }

  inline
  BaseReader_var
  StructReader::FieldReader::reader() const noexcept
  {
    return reader_;
  }

  inline
  StructReader::StructReader(
    const char* name_val,
    BaseDescriptor* descriptor_val,
    FieldReaderList* fields_val)
    : BaseType(name_val),
      BaseReader(name_val),
      descriptor_(ReferenceCounting::add_ref(descriptor_val)),
      fields_(ReferenceCounting::add_ref(fields_val))
  {}

  inline
  BaseDescriptor_var
  StructReader::descriptor() noexcept
  {
    return descriptor_;
  }

  inline
  StructReader::FieldReaderList_var
  StructReader::fields() const
  {
    return fields_;
  }

  inline
  StructReader_var
  StructReader::as_struct_reader() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }
}

#endif /*_STRUCTREADER_HPP_*/
