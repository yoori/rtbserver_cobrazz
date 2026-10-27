#include <algorithm>
#include <ostream>
#include <vector>

#include <Declaration/SimpleWriter.hpp>

#include "Config.hpp"
#include "Utils.hpp"
#include "CustomWriterGenerator.hpp"

namespace Cpp
{
  namespace
  {
    std::string custom_name(const std::string& name)
    {
      const std::string protected_suffix = PROTECTED_WRITER_SUFFIX;
      if (name.size() >= protected_suffix.size() &&
        name.compare(name.size() - protected_suffix.size(), protected_suffix.size(),
          protected_suffix) == 0)
      {
        return name.substr(0, name.size() - protected_suffix.size()) + "_CustomProtectedWriter";
      }

      const std::string suffix = "Writer";
      if (name.size() >= suffix.size() &&
        name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
      {
        return name.substr(0, name.size() - suffix.size()) + "CustomWriter";
      }

      return name + "CustomWriter";
    }

    std::string writer_type(const std::string& holder, unsigned long fixed_size)
    {
      return "typename PlainTypes::WriterType<WriterTypes, " + holder + ", " +
        std::to_string(fixed_size) + ">::Type";
    }

    struct Field
    {
      std::string name;
      std::string holder;
      Declaration::SimpleWriter::CppWriteTraits_var traits;
      unsigned long fixed_size;
      bool fixed;
      bool exposed;
    };
  }

  CustomWriterGenerator::CustomWriterGenerator(std::ostream& out, const char* offset)
    : out_(out), offset_(offset)
  {}

  void CustomWriterGenerator::generate(Declaration::StructDescriptor* descriptor)
  {
    generate_(custom_name(std::string(descriptor->name()) + PROTECTED_WRITER_SUFFIX),
      descriptor, nullptr);
  }

  void CustomWriterGenerator::generate(Declaration::StructWriter* writer)
  {
    Declaration::StructDescriptor_var descriptor = writer->descriptor()->as_struct();
    generate_(custom_name(writer->name()), descriptor, writer->fields());
  }

  void CustomWriterGenerator::generate_(const std::string& name,
    Declaration::StructDescriptor* descriptor, Declaration::StructWriter::FieldWriterList* fields)
  {
    std::vector<Field> members;
    for (const auto& field : *descriptor->fields())
    {
      Declaration::BaseDescriptor_var field_descriptor = field->descriptor();
      Declaration::StructDescriptor_var structure = field_descriptor->as_struct();
      Declaration::BaseWriter_var field_writer = field_descriptor->as_writer();
      Declaration::MappingSpecifierSet mapping;
      Field member;
      member.name = field->name();
      member.fixed_size = field_descriptor->fixed_size();
      member.fixed = field_descriptor->is_fixed() && !structure.in();
      member.exposed = false;
      std::string holder = Utils::HolderTypeOfField()(field);
      if (fields)
      {
        for (const auto& exposed_field : *fields)
        {
          if (member.name == exposed_field->name())
          {
            member.exposed = true;
            field_writer = exposed_field->writer();
            mapping = exposed_field->mapping_specifiers();
            holder = Utils::HolderTypeOfField()(exposed_field);
            break;
          }
        }
      }

      if (field_writer.in())
      {
        Declaration::SimpleWriter_var simple = field_writer->as_simple_writer();
        if (simple.in())
        {
          member.traits = simple->cpp_write_traits_generator()->generate(mapping);
        }
      }

      if (!member.fixed)
      {
        member.holder = writer_type(holder, member.fixed_size);
      }

      members.push_back(std::move(member));
    }

    const std::string base = std::string(descriptor->name()) + BASE_SUFFIX;
    const std::string type = name + "<WriterTypes>";
    const auto line = [this](const std::string& value)
    {
      out_ << offset_ << value << '\n';
    };
    const auto method = [&](const std::string& result, const std::string& signature)
    {
      line("");
      line("template<typename WriterTypes>");
      if (!result.empty())
      {
        line(result);
      }
      line(type + "::" + signature);
    };

    line("");
    line("template<typename WriterTypes>");
    line("class " + name + ": public " + base);
    line("{");
    line("public:");
    line("  using Context = typename WriterTypes::Context;");
    for (const auto& member : members)
    {
      if (!member.fixed)
      {
        line("  using " + member.name + "_Type = " + member.holder + ";");
        if (member.exposed && member.traits.in() &&
          !member.traits->field_type_suffix.empty())
        {
          line("  using " + member.name + member.traits->field_type_suffix + " = " +
            member.name + "_Type;");
        }
      }
    }
    std::string has_dynamic_data = "false";
    std::string requires_dynamic_end = "false";
    for (const auto& member : members)
    {
      if (!member.fixed)
      {
        has_dynamic_data += " || " + member.name + "_Type::HAS_DYNAMIC_DATA";
        requires_dynamic_end += " || " + member.name + "_Type::REQUIRES_DYNAMIC_END";
      }
    }
    line("  static constexpr bool HAS_DYNAMIC_DATA = " + has_dynamic_data + ";");
    line("  static constexpr bool REQUIRES_DYNAMIC_END = " + requires_dynamic_end + ";");
    line("");
    line("  explicit " + name + "(Context& context, bool init_defaults = false);");
    line("  " + name + "(Context& context, const void* buf, unsigned long size);");
    line("  " + name + "(Context& context, const PlainTypes::ConstBuf& buf);");
    line("  " + name + "& operator=(const PlainTypes::ConstBuf& buf);");
    line("  Context& context() const noexcept;");
    line("  void init_default() noexcept;");
    line("  void init(const void* buf, unsigned long size);");
    line("  const char* init(const void* buf, unsigned long size,");
    line("    const char* dynamic_buf_end);");
    line("  void unsafe_init(const void* buf, unsigned long size);");
    line("  unsigned long size() const;");
    line("  unsigned long dyn_size_() const;");
    line("  void save(void* buf, unsigned long size) const;");
    line("  void save_(void* fixed_buf, void* dyn_buf) const;");
    line("  void swap(" + name + "& other);");
    for (const auto& member : members)
    {
      if (member.exposed)
      {
        const std::string write = member.fixed ? member.traits->write_type_name :
          member.name + "_Type&";
        const std::string read = member.fixed ? member.traits->write_type_const_name :
          "const " + member.name + "_Type&";
        line("  " + write + " " + member.name + "() noexcept;");
        line("  " + read + " " + member.name + "() const noexcept;");
      }
    }
    line("");
    line("private:");
    line("  Context* context_;");
    for (const auto& member : members)
    {
      if (!member.fixed)
      {
        line("  " + member.name + "_Type " + member.name + "_;");
      }
      else
      {
        // Scalar accessors return references, so each scalar needs suitably aligned storage.
        line("  alignas(std::uint64_t) std::array<unsigned char, " +
          std::to_string(member.fixed_size) + "> " + member.name + "_{};");
      }
    }
    line("};");

    method("", name + "(Context& context, bool)");
    line("  : context_(&context)");
    for (const auto& member : members)
    {
      if (!member.fixed)
      {
        line("  , " + member.name + "_(context)");
      }
    }
    line("{}");
    method("", name + "(Context& context, const void* buf, unsigned long size)");
    line("  : " + name + "(context)");
    line("{");
    line("  init(buf, size);");
    line("}");
    method("", name + "(Context& context, const PlainTypes::ConstBuf& buf)");
    line("  : " + name + "(context, buf.get(), buf.size())");
    line("{}");
    method(type + "&", "operator=(const PlainTypes::ConstBuf& buf)");
    line("{");
    line("  init(buf.get(), buf.size());");
    line("  return *this;");
    line("}");
    method("typename " + type + "::Context&", "context() const noexcept");
    line("{");
    line("  return *context_;");
    line("}");
    method("void", "init_default() noexcept");
    line("{");
    for (const auto& member : members)
    {
      if (!member.fixed)
      {
        line("  " + member.name + "_.init_default();");
      }
      else
      {
        line("  " + member.name + "_.fill(0);");
      }
    }
    line("}");
    method("void", "init(const void* buf, unsigned long size)");
    line("{");
    line("  if constexpr (REQUIRES_DYNAMIC_END)");
    line("  {");
    line("    init(buf, size, static_cast<const char*>(buf) + size);");
    line("  }");
    line("  else");
    line("  {");
    line("    if (size < FIXED_SIZE)");
    line("    {");
    line("      throw PlainTypes::CorruptedStruct(\"CustomWriter: truncated fixed buffer\");");
    line("    }");
    for (const auto& member : members)
    {
      if (!member.fixed)
      {
        line("    " + member.name + "_.init(static_cast<const char*>(buf) + " + member.name +
          "_OFFSET, size - " + member.name + "_OFFSET);");
      }
      else
      {
        line("    std::memcpy(" + member.name + "_.data(), static_cast<const char*>(buf) + " +
          member.name + "_OFFSET, " + std::to_string(member.fixed_size) + ");");
      }
    }
    line("  }");
    line("}");
    method("const char*", "init(const void* buf, unsigned long size,");
    line("  const char* dynamic_buf_end)");
    line("{");
    line("  if (size < FIXED_SIZE)");
    line("  {");
    line("    throw PlainTypes::CorruptedStruct(\"CustomWriter: truncated fixed buffer\");");
    line("  }");
    line("  const char* begin = static_cast<const char*>(buf);");
    line("  const char* dynamic_end = dynamic_buf_end;");
    line("  if (dynamic_end < begin || dynamic_end > begin + size)");
    line("  {");
    line("    throw PlainTypes::CorruptedStruct(\"CustomWriter: invalid dynamic boundary\");");
    line("  }");
    line("");
    for (std::size_t reverse_index = members.size(); reverse_index > 0; --reverse_index)
    {
      const auto& member = members[reverse_index - 1];
      if (!member.fixed)
      {
        line("  if constexpr (" + member.name + "_Type::HAS_DYNAMIC_DATA)");
        line("  {");
        line("    dynamic_end = " + member.name + "_.init(begin + " + member.name +
          "_OFFSET, size - " + member.name + "_OFFSET, dynamic_end);");
        line("  }");
        line("  else");
        line("  {");
        line("    " + member.name + "_.init(begin + " + member.name +
          "_OFFSET, size - " + member.name + "_OFFSET);");
        line("  }");
      }
      else
      {
        line("  std::memcpy(" + member.name + "_.data(), begin + " +
          member.name + "_OFFSET, " + std::to_string(member.fixed_size) + ");");
      }
    }
    line("  if constexpr (HAS_DYNAMIC_DATA)");
    line("  {");
    line("    return dynamic_end;");
    line("  }");
    line("  return begin + FIXED_SIZE;");
    line("}");
    method("void", "unsafe_init(const void* buf, unsigned long size)");
    line("{");
    line("  init(buf, size);");
    line("}");
    method("unsigned long", "size() const");
    line("{");
    line("  return FIXED_SIZE + dyn_size_();");
    line("}");
    method("unsigned long", "dyn_size_() const");
    line("{");
    line("  unsigned long result = 0;");
    for (const auto& member : members)
    {
      if (!member.fixed)
      {
        line("  result += " + member.name + "_.dyn_size_();");
      }
    }
    line("  return result;");
    line("}");
    method("void", "save(void* buf, unsigned long size) const");
    line("{");
    line("  if (size < this->size())");
    line("  {");
    line("    throw PlainTypes::CorruptedStruct(\"CustomWriter: output buffer is too small\");");
    line("  }");
    line("");
    line("  save_(buf, static_cast<char*>(buf) + FIXED_SIZE);");
    line("}");
    method("void", "save_(void* fixed_buf, void* dyn_buf) const");
    line("{");
    line("  std::memset(fixed_buf, 0, FIXED_SIZE);");
    const auto is_dynamic = [](const Field& field) { return !field.fixed; };
    if (std::any_of(members.begin(), members.end(), is_dynamic))
    {
      line("  char* dynamic = static_cast<char*>(dyn_buf);");
    }
    else
    {
      line("  (void) dyn_buf;");
    }
    for (const auto& member : members)
    {
      if (!member.fixed)
      {
        line("  " + member.name + "_.save_(static_cast<char*>(fixed_buf) + " +
          member.name + "_OFFSET, dynamic);");
        line("  dynamic += " + member.name + "_.dyn_size_();");
      }
      else
      {
        line("  std::memcpy(static_cast<char*>(fixed_buf) + " + member.name + "_OFFSET, " +
          member.name + "_.data(), " + std::to_string(member.fixed_size) + ");");
      }
    }
    line("}");
    method("void", "swap(" + name + "& other)");
    line("{");
    line("  std::swap(*this, other);");
    line("}");
    for (const auto& member : members)
    {
      if (!member.exposed)
      {
        continue;
      }

      for (bool constant : {false, true})
      {
        std::string result;
        std::string expression;
        if (member.fixed)
        {
          result = constant ? member.traits->write_type_const_name : member.traits->write_type_name;
          const auto& cast = constant ? member.traits->write_type_const_cast :
            member.traits->write_type_cast;
          expression = cast + "(" + member.name + "_.data())";
          if (constant && member.traits->holder_type_name == "char")
          {
            expression = "static_cast<char>(" + member.name + "_[0])";
          }
        }
        else
        {
          result = (constant ? "const typename " : "typename ") + type + "::" +
            member.name + "_Type&";
          expression = member.name + "_";
        }
        method(result, member.name + "()" + (constant ? " const" : "") + " noexcept");
        line("{");
        line("  return " + expression + ";");
        line("}");
      }
    }
  }
}
