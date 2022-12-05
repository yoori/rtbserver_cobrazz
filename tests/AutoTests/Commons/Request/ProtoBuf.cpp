
#include "ProtoBuf.hpp"

namespace AutoTest
{
  namespace ProtoBuf
  {
    const FieldDescriptor*
    get_field(
      const Descriptor* descriptor,
      const std::string& name) /*throw(Exception)*/
    {
      const FieldDescriptor* field =
        descriptor->FindFieldByName(name);
      if (!field)
      {
        Stream::Error error;
        error << "Unknown field'" << name << "'";
        throw Exception(error);
      }
      return field;
    }

    void clear(
      Message* message,
      const std::string& name)
    {
      const Descriptor* descriptor = message->GetDescriptor();
      const FieldDescriptor* field = get_field(descriptor, name);
      const Reflection* reflection = message->GetReflection();
      reflection->ClearField(message, field);
    }
      
    bool empty(
      Message* message,
      const std::string& name)
    {
      const Descriptor* descriptor = message->GetDescriptor();
      const FieldDescriptor* field = get_field(descriptor, name);
      const Reflection* reflection = message->GetReflection();
      return
        field->is_repeated()?
          reflection->FieldSize(*message, field) == 0:
            !reflection->HasField(*message, field);
    }
   
    template<> const EnumType::Setter EnumType::setter_ = &Reflection::SetEnum;
    template<> const EnumType::Getter EnumType::getter_ = &Reflection::GetEnum;
    template<> const Int::Setter Int::setter_ = &Reflection::SetInt32;
    template<> const Int::Getter Int::getter_ = &Reflection::GetInt32;
    template<> const UInt::Setter UInt::setter_ = &Reflection::SetUInt32;
    template<> const UInt::Getter UInt::getter_ = &Reflection::GetUInt32;
    template<> const String::Setter String::setter_ = &Reflection::SetString;
    template<> const String::Getter String::getter_ = &Reflection::GetString;
    template<> const IntSeq::Setter IntSeq::setter_ = &Reflection::AddInt32;
    template<> const IntSeq::Getter IntSeq::getter_ = &Reflection::GetRepeatedField<int>;
    template<> const UIntSeq::Setter UIntSeq::setter_ = &Reflection::AddUInt32;
    template<> const UIntSeq::Getter UIntSeq::getter_ = &Reflection::GetRepeatedField<unsigned>;
    template<> const StringSeq::Setter StringSeq::setter_ = &Reflection::AddString;
    template<> const StringSeq::Getter StringSeq::getter_ = &Reflection::GetRepeatedPtrField<std::string>;
  }
}

