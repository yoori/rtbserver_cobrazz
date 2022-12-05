#include <tests/AutoTests/Commons/Reflection.hpp>

UnitDescriptor* base_unit_descriptor = 0;

UnitDescriptor::UnitDescriptor (const char* c,
  const int g,
  AutoTestSerialize s,
  const char* n,
  BaseUnitConstructor* co)
  :category(c), group(g), serialize(s), name(n), constructor(co)
{
  next = base_unit_descriptor;
  base_unit_descriptor = this;
  dirty = false;
}

std::ostream&
operator<< (std::ostream& strm, const UnitDescriptor& descriptor)
{
  strm << descriptor.category << '.' << descriptor.name;
  return strm;
}
