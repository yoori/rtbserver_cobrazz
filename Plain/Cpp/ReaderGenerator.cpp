#include "Config.hpp"
#include "ReaderGenerator.hpp"

namespace Cpp
{
  ReaderGenerator::ReaderGenerator(
    std::ostream& out, const char* offset)
    noexcept
    : out_(out),
      offset_(offset)
  {}

  void ReaderGenerator::generate_decl(
    Declaration::StructReader* struct_reader)
    noexcept
  {
    out_ << offset_ << "/* " << struct_reader->name() <<
      " reader declaration */" << std::endl;

    Declaration::BaseDescriptor_var descriptor =
      struct_reader->descriptor();

    Declaration::StructReader::FieldReaderList_var fields =
      struct_reader->fields();

    out_ << offset_ << "class " << struct_reader->name() <<
        ": public " << descriptor->name() << BASE_SUFFIX << "," <<
        std::endl <<
      offset_ << "  public PlainTypes::ConstBuf" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "public:" << std::endl;

    for(Declaration::StructReader::FieldReaderList::const_iterator fit =
          fields->begin();
        fit != fields->end(); ++fit)
    {
      std::string read_type;
      std::string field_type_suffix;
      Declaration::BaseReader_var field_reader = (*fit)->reader();
      Declaration::SimpleReader_var simple_field_reader =
        field_reader->as_simple_reader();
      if(simple_field_reader.in())
      {
        read_type = simple_field_reader->cpp_read_traits().read_type;
        field_type_suffix = simple_field_reader->cpp_read_traits().field_type_suffix;
      }
      else
      {
        Declaration::StructReader_var struct_field_reader =
          field_reader->as_struct_reader();
        if(struct_field_reader.in())
        {
          read_type = struct_field_reader->name();
          field_type_suffix = "_Reader";
        }
        else
        {
          assert(0);
        }
      }

      if(!field_type_suffix.empty())
      {
        out_ << std::endl <<
          offset_ << "  typedef " << read_type << " " << (*fit)->name() <<
            field_type_suffix << ";" << std::endl;
      }
    }

    out_ << offset_ << "public: " << std::endl <<
      offset_ << "  " << struct_reader->name() <<
        "(const void* buf, unsigned size);" << std::endl;

    for(Declaration::StructReader::FieldReaderList::const_iterator fit =
          fields->begin();
        fit != fields->end(); ++fit)
    {
      std::string ret_type_name;
      Declaration::BaseReader_var field_reader = (*fit)->reader();
      Declaration::SimpleReader_var simple_field_reader =
        field_reader->as_simple_reader();
      if(simple_field_reader.in())
      {
        ret_type_name = simple_field_reader->cpp_read_traits().read_type_name;
      }
      else
      {
        Declaration::StructReader_var struct_field_reader =
          field_reader->as_struct_reader();
        if(struct_field_reader.in())
        {
          ret_type_name = struct_field_reader->name();
        }
        else
        {
          assert(0);
        }
      }

      out_ << std::endl <<
        offset_ << "  " << ret_type_name << " " << (*fit)->name() <<
          "() const;" << std::endl;
    }

    out_ << offset_ << "};" << std::endl << std::endl;
  }

  void ReaderGenerator::generate_impl(
    Declaration::StructReader* struct_reader)
    noexcept
  {
    out_ << offset_ << "/* reader " << struct_reader->name() <<
      " */" << std::endl;

    generate_ctor_impl_(struct_reader);

    generate_field_funs_impl_(struct_reader);
  }

  void
  ReaderGenerator::generate_ctor_impl_(
    const Declaration::StructReader* struct_reader)
    noexcept
  {
    out_ << offset_ << "inline" << std::endl <<
      offset_ << struct_reader->name() << "::" <<
      struct_reader->name() << "(const void* buf, unsigned buf_size)" << std::endl <<
      offset_ << "  : PlainTypes::ConstBuf(buf, buf_size)" << std::endl <<
      offset_ << "{}" << std::endl;
  }

  void
  ReaderGenerator::generate_field_funs_impl_(
    const Declaration::StructReader* struct_reader)
    noexcept
  {
    Declaration::StructReader::FieldReaderList_var fields =
      struct_reader->fields();

    for(Declaration::StructReader::FieldReaderList::
          const_iterator fit = fields->begin();
        fit != fields->end(); ++fit)
    {
      std::string ret_type_name, ret_cast_call;
      bool cast_with_size = false;

      Declaration::SimpleReader_var simple_reader =
        (*fit)->reader()->as_simple_reader();

      if(simple_reader.in())
      {
        ret_type_name = simple_reader->cpp_read_traits().read_type_name;
        ret_cast_call = simple_reader->cpp_read_traits().read_type_cast;
        cast_with_size = simple_reader->cpp_read_traits().read_type_cast_with_size;
      }
      else
      {
        Declaration::StructDescriptor_var struct_descriptor =
          (*fit)->reader()->descriptor()->as_struct();

        if(struct_descriptor.in())
        {
          ret_type_name = (*fit)->reader()->name();
          ret_cast_call = ret_type_name;
          cast_with_size = true;
        }
        else
        {
          assert(0);
        }
      }

      out_ << offset_ << "inline" << std::endl <<
        offset_ << ret_type_name << std::endl <<
        offset_ << struct_reader->name() << "::" <<
          (*fit)->name() << "() const" << std::endl <<
        offset_ << "{" << std::endl <<
        offset_ << "  return " << ret_cast_call <<
          "(static_cast<const void*>(buf_ + " <<
          (*fit)->name() << FIELD_OFFSET_SUFFIX << ")";

      if(cast_with_size)
      {
        out_ << ", buf_size_ - " << (*fit)->name() << FIELD_OFFSET_SUFFIX;
      }

      out_ << ");" << std::endl <<
        offset_ << "}" << std::endl << std::endl;
    }
  }
}


