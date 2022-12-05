#ifndef _STRUCTDESCRIPTOR_HPP_
#define _STRUCTDESCRIPTOR_HPP_

#include <list>
#include <cstring>

#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include "BaseDescriptor.hpp"

namespace Declaration
{
  class StructDescriptor:
    public virtual ReferenceCounting::AtomicImpl,
    public virtual BaseDescriptor
  {
  public:
    class Field: public ReferenceCounting::DefaultImpl<>
    {
    public:
      Field(BaseDescriptor* descriptor_val, const char* name_val)
        /*throw(eh::Exception)*/;
      
      Field(const Field& other)
        /*throw(eh::Exception)*/;
      
      const char* name() const noexcept;

      BaseDescriptor_var descriptor() const noexcept;

    protected:
      virtual ~Field() noexcept {}
      
    private:
      BaseDescriptor_var descriptor_;
      std::string name_;
    };

    typedef ReferenceCounting::SmartPtr<Field>
      Field_var;

    struct FieldList: public std::list<Field_var>,
      public ReferenceCounting::DefaultImpl<>
    {
    protected:
      virtual ~FieldList() noexcept {}
    };
    
    typedef ReferenceCounting::SmartPtr<FieldList>
      FieldList_var;

    class PosedField: public Field
    {
    public:
      PosedField(const Field& field_val, unsigned long pos_val)
        noexcept;

      unsigned long pos() const noexcept;

    protected:
      virtual ~PosedField() noexcept {}
      
    private:
      unsigned long pos_;
    };

    typedef ReferenceCounting::SmartPtr<PosedField>
      PosedField_var;

    struct PosedFieldList: public std::list<PosedField_var>,
      public ReferenceCounting::DefaultImpl<>
    {
    protected:
      virtual ~PosedFieldList() noexcept {}
    };
    
    typedef ReferenceCounting::SmartPtr<PosedFieldList>
      PosedFieldList_var;

  public:
    StructDescriptor(
      const char* name_val,
      FieldList* fields_val) noexcept;

    PosedFieldList_var fields() const noexcept;

    PosedField_var find_field(const char* name) const noexcept;

    virtual bool is_fixed() const noexcept;

    virtual SizeType fixed_size() const noexcept;
    
    virtual StructDescriptor_var as_struct() noexcept;

  protected:
    virtual ~StructDescriptor() noexcept {}

  private:
    PosedFieldList_var fields_;
    bool is_fixed_;
    SizeType fixed_size_;
  };

  typedef ReferenceCounting::SmartPtr<StructDescriptor>
    StructDescriptor_var;
}

namespace Declaration
{
  inline
  StructDescriptor::Field::Field(
    BaseDescriptor* descriptor_val,
    const char* name_val)
    /*throw(eh::Exception)*/
    : descriptor_(ReferenceCounting::add_ref(descriptor_val)),
      name_(name_val)
  {}

  inline
  StructDescriptor::Field::Field(
    const Field& other)
    /*throw(eh::Exception)*/
    : ReferenceCounting::Interface(),
      ReferenceCounting::DefaultImpl<>(),
      descriptor_(other.descriptor_),
      name_(other.name_)
  {}

  inline
  const char*
  StructDescriptor::Field::name() const
    noexcept
  {
    return name_.c_str();
  }

  inline
  StructDescriptor::PosedField::PosedField(
    const StructDescriptor::Field& field_val,
    unsigned long pos_val)
    noexcept
    : StructDescriptor::Field(field_val),
      pos_(pos_val)
  {}

  inline
  unsigned long
  StructDescriptor::PosedField::pos() const
    noexcept
  {
    return pos_;
  }

  inline
  BaseDescriptor_var
  StructDescriptor::Field::descriptor() const
    noexcept
  {
    return descriptor_;
  }
  
  inline
  StructDescriptor::StructDescriptor(
    const char* name_val,
    FieldList* fields_val)
    noexcept
    : BaseType(name_val),
      BaseDescriptor(name_val),
      fields_(new PosedFieldList()),
      is_fixed_(true)
  {
    unsigned long pos = 0;
    for(FieldList::const_iterator fit = fields_val->begin();
        fit != fields_val->end(); ++fit)
    {
      is_fixed_ &= (*fit)->descriptor()->is_fixed();
      fields_->push_back(PosedField_var(new PosedField(*(*fit), pos)));
      pos += (*fit)->descriptor()->fixed_size();
    }

    fixed_size_ = pos;
  }

  inline
  StructDescriptor::PosedFieldList_var
  StructDescriptor::fields() const noexcept
  {
    return fields_;
  }

  inline
  StructDescriptor::PosedField_var
  StructDescriptor::find_field(const char* name) const noexcept
  {
    for(StructDescriptor::PosedFieldList::const_iterator fit =
          fields_->begin();
        fit != fields_->end(); ++fit)
    {
      if(strcmp(name, (*fit)->name()) == 0)
      {
        return *fit;
      }
    }

    return StructDescriptor::PosedField_var();
  }
  
  inline
  bool
  StructDescriptor::is_fixed() const noexcept
  {
    return is_fixed_;
  }

  inline
  SizeType
  StructDescriptor::fixed_size() const noexcept
  {
    return fixed_size_;
  }

  inline
  StructDescriptor_var
  StructDescriptor::as_struct() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }
}

#endif /*_STRUCTDESCRIPTOR_HPP_*/
