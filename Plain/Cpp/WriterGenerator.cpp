#include "Config.hpp"
#include "Utils.hpp"

#include "WriterGenerator.hpp"

namespace Cpp
{
  struct OnlyFixedSumOps
  {
    template<typename FieldType>
    void process_non_fixed_or_complex(const FieldType&) const
    {}
  };

  struct FieldBuffersDeclOps: public OnlyFixedSumOps
  {
    FieldBuffersDeclOps(std::ostream& out, const char* offset)
      : out_(out), offset_(offset)
    {}

    void process_fixed_sum(
      unsigned long fixed_buf_i,
      unsigned long fixed_buf_size,
      const char*) const
    {
      out_ << offset_ << "static const unsigned char DEFAULT_FIXED_BUF_" <<
        fixed_buf_i << "[" << fixed_buf_size << "];" << std::endl;
    }

  private:
    std::ostream& out_;
    std::string offset_;
  };

  struct FieldBuffersImplOps: public OnlyFixedSumOps
  {
    FieldBuffersImplOps(
      std::ostream& out, const char* offset, const char* class_name)
      : out_(out), offset_(offset), class_name_(class_name)
    {}

    void process_fixed_sum(
      unsigned long fixed_buf_i,
      unsigned long fixed_buf_size,
      const char*) const
    {
      out_ << offset_ << "const unsigned char " <<
        class_name_ << "::DEFAULT_FIXED_BUF_" <<
        fixed_buf_i << "[" << fixed_buf_size << "] = { ";
      for(unsigned long i = 0; i < fixed_buf_size; ++i)
      {
        if(i != 0) out_ << ", ";
        out_ << "0";
      }
      out_ << " };" << std::endl << std::endl;
    }

  private:
    std::ostream& out_;
    std::string offset_;
    std::string class_name_;
  };

  struct FieldBuffersSwapImplOps: public OnlyFixedSumOps
  {
    FieldBuffersSwapImplOps(
      std::ostream& out, const char* offset)
      : out_(out), offset_(offset)
    {}

    void process_fixed_sum(
      unsigned long fixed_buf_i,
      unsigned long fixed_buf_size,
      const char*) const
    {
      out_ << offset_ << "  ::memcpy(swap_buf_, "
          "right." << FIXED_BUFFER_PREFIX << fixed_buf_i << "_, " <<
          fixed_buf_size << ");" << std::endl <<
        offset_ << "  ::memcpy(right." <<
          FIXED_BUFFER_PREFIX << fixed_buf_i << "_, " <<
          FIXED_BUFFER_PREFIX << fixed_buf_i << "_, " <<
          fixed_buf_size << ");" << std::endl <<
        offset_ << "  ::memcpy(" <<
          FIXED_BUFFER_PREFIX << fixed_buf_i << "_, " <<
          "swap_buf_, " <<
          fixed_buf_size << ");" << std::endl;
    }

  private:
    std::ostream& out_;
    std::string offset_;
  };

  struct FieldBuffersMaxOps: public OnlyFixedSumOps
  {
    FieldBuffersMaxOps()
      : max_fixed_buf_size_(0)
    {}

    void process_fixed_sum(
      unsigned long,
      unsigned long fixed_buf_size,
      const char*) const
    {
      max_fixed_buf_size_ = std::max(max_fixed_buf_size_, fixed_buf_size);
    }

    unsigned long value() const
    {
      return max_fixed_buf_size_;
    }

  private:
    mutable unsigned long max_fixed_buf_size_;
  };

  template<typename TypeOfFieldOpType>
  struct FieldDeclOps
  {
    FieldDeclOps(
      std::ostream& out,
      const char* offset,
      Declaration::StructWriter::FieldWriterList* field_writers = 0,
      const TypeOfFieldOpType& field_type_op =
        TypeOfFieldOpType())
      : out_(out),
        offset_(offset),
        field_writers_(ReferenceCounting::add_ref(field_writers)),
        field_type_op_(field_type_op)
    {
      if(field_writers_.in())
      {
        field_writer_it_ = field_writers_->begin();

        while(field_writer_it_ != field_writers_->end() &&
          (*field_writer_it_)->descriptor()->is_fixed() &&
          !(*field_writer_it_)->descriptor()->as_struct())
        {
          ++field_writer_it_;
        }
      }
    }

    void process_fixed_sum(
      unsigned long fixed_buf_i,
      unsigned long fixed_buf_size,
      const char*) const
    {
      out_ << offset_ << "unsigned char " << FIXED_BUFFER_PREFIX <<
        fixed_buf_i << "_[" << fixed_buf_size << "];" << std::endl;
    }

    void process_non_fixed_or_complex(
      const Declaration::StructDescriptor::PosedField* field) const
    {
      if(field_writers_.in() &&
         field_writer_it_ != field_writers_->end() &&
         ::strcmp(field->name(), (*field_writer_it_)->name()) == 0)
      {
        out_ << offset_ << field_type_op_(*field_writer_it_) << " " <<
          field->name() << "_; // have accessor" << std::endl;

        // move to next not fixed or complex field
        do
        {
          ++field_writer_it_;
        }
        while(field_writer_it_ != field_writers_->end() &&
          (*field_writer_it_)->descriptor()->is_fixed() &&
          !(*field_writer_it_)->descriptor()->as_struct());
      }
      else
      {
        out_ << offset_ << field_type_op_(field) << " " <<
          field->name() << "_;" << std::endl;
      }
    }

  private:
    std::ostream& out_;
    const std::string offset_;
    Declaration::StructWriter::FieldWriterList_var field_writers_;
    mutable Declaration::StructWriter::FieldWriterList::const_iterator
      field_writer_it_;
    TypeOfFieldOpType field_type_op_;
  };

  struct FieldInitDefaultImplOps
  {
    FieldInitDefaultImplOps(std::ostream& out, const char* offset)
      : out_(out), offset_(offset)
    {}

    void process_fixed_sum(
      unsigned long fixed_buf_i,
      unsigned long fixed_buf_size,
      const char*) const
    {
      out_ << offset_ << "::memcpy(fixed_buf_" << fixed_buf_i <<
        "_, DEFAULT_FIXED_BUF_" << fixed_buf_i << ", " <<
        fixed_buf_size << ");" << std::endl;
    }

    template<typename FieldType>
    void process_non_fixed_or_complex(const FieldType& field) const
    {
      out_ << offset_ << field->name() << "_.init_default();" <<
        std::endl;
    }

  private:
    std::ostream& out_;
    std::string offset_;
  };

  struct FieldInitImplOps
  {
    FieldInitImplOps(std::ostream& out,
      const char* offset)
      : out_(out), offset_(offset)
    {}

    void process_fixed_sum(
      unsigned long fixed_buf_i,
      unsigned long fixed_buf_size,
      const char* first_fixed_field_name)
      const
    {
      out_ << offset_ << "::memcpy(fixed_buf_" << fixed_buf_i << "_, "
        "buf_ptr + " << first_fixed_field_name <<
        FIELD_OFFSET_SUFFIX << ", " <<
        fixed_buf_size << ");" << std::endl;
    }

    template<typename FieldType>
    void process_non_fixed_or_complex(const FieldType& field) const
    {
      out_ << offset_ << field->name() << "_.init(" <<
        "buf_ptr + " << field->name() << FIELD_OFFSET_SUFFIX << ", "
        "size);" << std::endl;
    }

  private:
    std::ostream& out_;
    std::string offset_;
  };

  struct FieldSaveImplOps
  {
    FieldSaveImplOps(std::ostream& out, const char* offset)
      : out_(out), offset_(offset)
    {}

    void process_fixed_sum(
      unsigned long fixed_buf_i,
      unsigned long fixed_buf_size,
      const char* first_fixed_field_name) const
    {
      out_ << offset_ << "::memcpy("
        "buf_ptr + " << first_fixed_field_name << FIELD_OFFSET_SUFFIX << ", " <<
        FIXED_BUFFER_PREFIX << fixed_buf_i << "_, " <<
        fixed_buf_size << ");" << std::endl;
    }

    template<typename FieldType>
    void process_non_fixed_or_complex(const FieldType& field) const
    {
      out_ << offset_ << field->name() << "_.save_("
        "buf_ptr + " << field->pos() << ", " <<
        (!field->descriptor()->is_fixed() ? "dyn_buf_ptr" : "0") << ");" <<
        std::endl;

      if(!field->descriptor()->is_fixed())
      {
        out_ << offset_ << "dyn_buf_ptr += " << field->name() << "_.dyn_size_();" <<
          std::endl;
      }
    }

  private:
    std::ostream& out_;
    const std::string offset_;
  };

  /* WriterGenerator */
  WriterGenerator::WriterGenerator(
    std::ostream& out_hpp,
    std::ostream& out_cpp,
    const char* offset)
    noexcept
    : out_(out_hpp),
      out_cpp_(out_cpp),
      offset_(offset)
  {}

  /* WriterGenerator: buffers generation */
  void
  WriterGenerator::generate_default_buffers_decl(
    Declaration::StructDescriptor* struct_descriptor)
    noexcept
  {
    Declaration::StructDescriptor::PosedFieldList_var fields =
      struct_descriptor->fields();

    out_ << offset_ << "struct " << struct_descriptor->name() <<
        DEFAULT_BUFFERS_SUFFIX << std::endl <<
      offset_ << "{" << std::endl;

    Utils::fetch_fields_with_fixed_sum(
      *fields, FieldBuffersDeclOps(out_, (offset_ + "  ").c_str()));

    out_ << offset_ << "};" << std::endl << std::endl;
  }

  void
  WriterGenerator::generate_default_buffers_impl(
    Declaration::StructDescriptor* struct_descriptor)
    noexcept
  {
    Declaration::StructDescriptor::PosedFieldList_var fields =
      struct_descriptor->fields();

    std::string class_name = std::string(struct_descriptor->name()) +
      DEFAULT_BUFFERS_SUFFIX;

    Utils::fetch_fields_with_fixed_sum(
      *fields, FieldBuffersImplOps(
        out_cpp_, offset_.c_str(), class_name.c_str()));
  }

  /* protected writer generation */
  void WriterGenerator::generate_protected_decl(
    Declaration::StructDescriptor* struct_descriptor)
    noexcept
  {
    Declaration::StructDescriptor::PosedFieldList_var fields =
      struct_descriptor->fields();

    std::string class_name = std::string(struct_descriptor->name()) +
      PROTECTED_WRITER_SUFFIX;

    out_ << offset_ << "class " << class_name << ": protected " <<
        struct_descriptor->name() << DEFAULT_BUFFERS_SUFFIX << "," << std::endl <<
      offset_ << "  public " << struct_descriptor->name() <<
        BASE_SUFFIX << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "public:" << std::endl;

    offset_ += "  ";

    generate_common_funs_decl_(class_name.c_str());

    offset_.resize(offset_.size() - 2);

    out_ << std::endl << offset_ << "private:" << std::endl;

    Utils::fetch_fields_with_fixed_sum(
      *fields, FieldDeclOps<Utils::HolderTypeOfField>(
        out_, (offset_ + "  ").c_str()));

    out_ << offset_ << "};" << std::endl << std::endl;
  }

  void WriterGenerator::generate_protected_impl(
    Declaration::StructDescriptor* struct_descriptor)
    noexcept
  {
    generate_common_funs_impl_((
      std::string(struct_descriptor->name()) +
        PROTECTED_WRITER_SUFFIX).c_str(),
      struct_descriptor);

    generate_swap_impl_((
      std::string(struct_descriptor->name()) +
        PROTECTED_WRITER_SUFFIX).c_str(),
      struct_descriptor);
  }

  void WriterGenerator::generate_decl(
    Declaration::StructWriter* struct_writer)
    noexcept
  {
    out_ << offset_ << "/* " << struct_writer->name() <<
      " writer declaration */" << std::endl;

    Declaration::StructDescriptor_var struct_descriptor =
      struct_writer->descriptor()->as_struct();
    assert(struct_descriptor.in());

    Declaration::StructWriter::FieldWriterList_var write_fields =
      struct_writer->fields();

    Declaration::StructDescriptor::PosedFieldList_var fields =
      struct_descriptor->fields();

    out_ << offset_ << "class " << struct_writer->name() << ": protected " <<
      struct_writer->descriptor()->name() << DEFAULT_BUFFERS_SUFFIX << "," << std::endl <<
      offset_ << "  public " << struct_writer->descriptor()->name() <<
        BASE_SUFFIX << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "public:" << std::endl;

    offset_ += "  ";

    generate_field_types_decl_(struct_writer);

    offset_.resize(offset_.size() - 2);

    out_ << offset_ << "public:" << std::endl;

    offset_ += "  ";

    generate_common_funs_decl_(struct_writer->name());

    generate_accessors_decl_(struct_writer);

    offset_.resize(offset_.size() - 2);

    out_ << std::endl << offset_ << "private:" << std::endl;

    Utils::fetch_fields_with_fixed_sum(
      *fields,
      FieldDeclOps<Utils::HolderTypeOfField>(
        out_, (offset_ + "  ").c_str(), write_fields.in()));

    out_ << offset_ << "};" << std::endl << std::endl;
  }

  void WriterGenerator::generate_impl(
    Declaration::StructWriter* struct_writer)
    noexcept
  {
    Declaration::StructWriter::FieldWriterList_var fields =
      struct_writer->fields();

    Declaration::StructDescriptor_var struct_descriptor =
      struct_writer->descriptor()->as_struct();

    assert(struct_descriptor.in());

    out_ << offset_ << "/* writer " << struct_writer->name() <<
      " */" << std::endl;

    generate_common_funs_impl_(
      struct_writer->name(),
      struct_descriptor);

    generate_swap_impl_(
      struct_writer->name(),
      struct_descriptor);
      
    generate_accessors_impl_(struct_writer);
  }

  void
  WriterGenerator::generate_common_funs_decl_(const char* name)
    noexcept
  {
    out_ << offset_ << name << "(bool init_defaults = false);" << std::endl <<
        std::endl <<
      offset_ << name << "(const void* buf, unsigned long buf_size);" << std::endl <<
        std::endl <<
      offset_ << name << "(const PlainTypes::ConstBuf& buf);" << std::endl <<
        std::endl <<
      offset_ << "void unsafe_init(const void* buf, unsigned long size);" << std::endl <<
        std::endl <<
      offset_ << "void init(const void* buf, unsigned long size);" << std::endl <<
        std::endl <<
      offset_ << "void init_default();" << std::endl <<
        std::endl <<
      offset_ << "unsigned long size() const;" << std::endl <<
        std::endl <<
      offset_ << "void save(void* buf, unsigned long size) const;" << std::endl <<
        std::endl <<
      offset_ << "void swap(" << name << "& right);" << std::endl <<
        std::endl <<
      offset_ << name << "& operator=(const PlainTypes::ConstBuf& buf);" << std::endl <<
        std::endl <<
      offset_ << "unsigned long dyn_size_() const;" << std::endl <<
        std::endl <<
      offset_ << "void save_(void* buf, void* dyn_buf) const;" << std::endl;
  }

  void WriterGenerator::generate_common_funs_impl_(
    const char* class_name,
    const Declaration::StructDescriptor* struct_descriptor)
    noexcept
  {
    Declaration::StructDescriptor::PosedFieldList_var fields =
      struct_descriptor->fields();

    bool has_non_fixed_of_struct_field = false;
    for(Declaration::StructDescriptor::PosedFieldList::
          const_iterator fit = fields->begin();
        fit != fields->end(); ++fit)
    {
      if(!(*fit)->descriptor()->is_fixed() ||
         (*fit)->descriptor()->as_struct())
      {
        has_non_fixed_of_struct_field = true;
        break;
      }
    }
      
    /* c-tor(bool init_defaults = false) */
    out_ << offset_ << "inline" << std::endl <<
      offset_ << class_name << "::" << class_name <<
        "(bool init_defaults)" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  if(init_defaults)" << std::endl <<
      offset_ << "  {" << std::endl <<
      offset_ << "    init_default();" << std::endl <<
      offset_ << "  }" << std::endl <<
      offset_ << "}" << std::endl << std::endl;

    /* c-tor(const void* buf, unsigned long buf_size) */
    out_ << offset_ << "inline" << std::endl <<
      offset_ << class_name << "::" << class_name <<
        "(const void* buf, unsigned long buf_size)" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  init(buf, buf_size);" << std::endl <<
      offset_ << "}" << std::endl << std::endl;

    /* c-tor(const PlainTypes::ConstBuf& buf) */
    out_ << offset_ << "inline" << std::endl <<
      offset_ << class_name << "::" << class_name <<
        "(const PlainTypes::ConstBuf& buf)" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  unsafe_init(buf.get(), buf.size());" << std::endl <<
      offset_ << "}" << std::endl << std::endl;

    // init_default()
    out_ << offset_ << "inline" << std::endl <<
      offset_ << "void" << std::endl <<
      offset_ << class_name << "::init_default()" << std::endl <<
      offset_ << "{" << std::endl;

    Utils::fetch_fields_with_fixed_sum(
      *fields, FieldInitDefaultImplOps(out_, (offset_ + "  ").c_str()));

    out_ << offset_ << "}" << std::endl << std::endl;

    // unsafe_init(const void* buf)
    out_ << offset_ << "inline" << std::endl <<
      offset_ << "void" << std::endl <<
      offset_ << class_name << "::unsafe_init(const void* buf, unsigned long " <<
        (has_non_fixed_of_struct_field ? "size" : "/*size*/") <<
        ")" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  const unsigned char* buf_ptr = static_cast<const unsigned char*>(buf);" <<
        std::endl;
    
    Utils::fetch_fields_with_fixed_sum(
      *fields, FieldInitImplOps(out_, (offset_ + "  ").c_str()));

    out_ << offset_ << "}" << std::endl << std::endl;

    // init(const void* buf, unsigned long size)
    out_ << offset_ << "inline" << std::endl <<
      offset_ << "void" << std::endl <<
      offset_ << class_name << "::init(" << std::endl <<
      offset_ << "  const void* buf, unsigned long " <<
        // (has_non_fixed_of_struct_field ? "size" : "/*size*/") <<
        "size"
        ")" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  if(size < FIXED_SIZE)" << std::endl <<
      offset_ << "  {" << std::endl <<
      offset_ << "    Stream::Error ostr;" << std::endl <<
      offset_ << "    ostr << \"" << class_name <<
        "::init(): buffer size = \" << size << \" is less then fixed size = \" << FIXED_SIZE;" <<
         std::endl <<
      offset_ << "    throw PlainTypes::CorruptedStruct(ostr);" << std::endl <<
      offset_ << "  }" << std::endl << std::endl <<
      offset_ << "  unsafe_init(buf, size);" << std::endl <<
      offset_ << "}" << std::endl << std::endl;

    // unsigned long size() const
    out_ << offset_ << "inline" << std::endl <<
      offset_ << "unsigned long" << std::endl <<
      offset_ << class_name << "::size() const" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  return FIXED_SIZE" << (
        !struct_descriptor->is_fixed() ? " + dyn_size_()" : "") << ";" << std::endl <<
      offset_ << "}" << std::endl << std::endl;

    // operator=(const PlainTypes::ConstBuf&)
    out_ << offset_ << "inline" << std::endl <<
      offset_ << class_name << "&" << std::endl <<
      offset_ << class_name << "::operator=(const PlainTypes::ConstBuf& buf)" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  unsafe_init(buf.get(), buf.size());" << std::endl <<
      offset_ << "  return *this;" << std::endl <<
      offset_ << "}" << std::endl << std::endl;

    // unsigned long dyn_size_() const
    out_ << offset_ << "inline" << std::endl <<
      offset_ << "unsigned long" << std::endl <<
      offset_ << class_name << "::dyn_size_() const" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  return ";
    
    {
      bool first_dyn_field = true;
      for(Declaration::StructDescriptor::PosedFieldList::
            const_iterator fit = fields->begin();
          fit != fields->end(); ++fit)
      {
        if(!(*fit)->descriptor()->is_fixed())
        {
          if(first_dyn_field)
          {
            out_ << (*fit)->name() << "_.dyn_size_()";
          }
          else
          {
            out_ << " +" << std::endl << offset_ <<
              "    " << (*fit)->name() << "_.dyn_size_()";
          }

          first_dyn_field = false;
        }
      }

      if(first_dyn_field)
      {
        out_ << "0";
      }
    }

    out_ << ";" << std::endl <<
      offset_ << "}" << std::endl << std::endl;

    // void save_(void* buf, void* dyn_buf) const
    bool dyn_buf_required = !struct_descriptor->is_fixed();

    out_ << offset_ << "inline" << std::endl <<
      offset_ << "void" << std::endl <<
      offset_ << class_name << "::save_(" << std::endl <<
      offset_ << "  void* buf, void* " <<
        (dyn_buf_required ? "dyn_buf" : "/*dyn_buf*/") <<
        ") const" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  unsigned char* buf_ptr = static_cast<unsigned char*>(buf);" <<
        std::endl;

    if(dyn_buf_required)
    {
      out_ << offset_ <<
        "  unsigned char* dyn_buf_ptr = static_cast<unsigned char*>(dyn_buf);" <<
        std::endl;
    }

    Utils::fetch_fields_with_fixed_sum(
      *fields, FieldSaveImplOps(out_, (offset_ + "  ").c_str()));

    out_ << offset_ << "}" << std::endl << std::endl;

    // void save(void* buf, unsigned long size) const
    out_ << offset_ << "inline" << std::endl <<
      offset_ << "void" << std::endl <<
      offset_ << class_name << "::save(" << std::endl <<
      offset_ << "  void* buf, unsigned long /*size*/) const" << std::endl <<
      offset_ << "{" << std::endl <<
      offset_ << "  save_(buf, static_cast<unsigned char*>(buf) + FIXED_SIZE);" <<
        std::endl <<
      offset_ << "}" << std::endl << std::endl;
  }

  void WriterGenerator::generate_swap_impl_(
    const char* class_name,
    Declaration::StructDescriptor* struct_descriptor)
    noexcept
  {
    Declaration::StructDescriptor::PosedFieldList_var fields =
      struct_descriptor->as_struct()->fields();

    out_ << offset_ << "inline" << std::endl <<
      offset_ << "void" << std::endl <<
      offset_ << class_name << "::swap(" <<
        class_name << "& " << (!fields->empty() ? "right" : "") <<
        ")" << std::endl <<
      offset_ << "{" << std::endl;

    for(Declaration::StructDescriptor::PosedFieldList::
          const_iterator fit = fields->begin();
        fit != fields->end(); ++fit)
    {
      if(!(*fit)->descriptor()->is_fixed())
      {
        out_ << offset_ << "  " << (*fit)->name() << "_.swap(right." <<
          (*fit)->name() << "_);" << std::endl;
      }
    }

    FieldBuffersMaxOps field_buffers_max_ops;

    Utils::fetch_fields_with_fixed_sum(
      *fields, field_buffers_max_ops);

    if(field_buffers_max_ops.value() > 0)
    {
      out_ << offset_ << "  // fixed buffers swap" <<
          std::endl <<
        offset_ << "  char swap_buf_[" <<
          field_buffers_max_ops.value() << "];" <<
          std::endl << std::endl;

      Utils::fetch_fields_with_fixed_sum(
        *fields, FieldBuffersSwapImplOps(
          out_, offset_.c_str()));
    }

    out_ << offset_ << "}" << std::endl << std::endl;
  }

  void WriterGenerator::generate_field_types_decl_(
    Declaration::StructWriter* struct_writer) noexcept
  {
    Declaration::StructWriter::FieldWriterList_var fields =
      struct_writer->fields();

    for(Declaration::StructWriter::FieldWriterList::
          const_iterator fit = fields->begin();
        fit != fields->end(); ++fit)
    {
      std::string write_type;
      std::string field_type_suffix;

      Declaration::BaseWriter_var field_writer = (*fit)->writer();
      Declaration::SimpleWriter_var simple_field_writer =
        field_writer->as_simple_writer();

      if(simple_field_writer.in())
      {
        Declaration::SimpleWriter::CppWriteTraitsGenerator_var
          cpp_write_traits_generator =
            simple_field_writer->cpp_write_traits_generator();

        Declaration::SimpleWriter::CppWriteTraits_var
          cpp_write_traits =
            cpp_write_traits_generator->generate((*fit)->mapping_specifiers());

        write_type = cpp_write_traits->write_type;
        field_type_suffix = cpp_write_traits->field_type_suffix;
      }
      else
      {
        Declaration::StructWriter_var struct_field_writer =
          field_writer->as_struct_writer();
        if(struct_field_writer.in())
        {
          write_type = struct_field_writer->name();
          field_type_suffix = "_Writer";
        }
      }

      if(!field_type_suffix.empty())
      {
        out_ << std::endl << offset_ << "typedef " << write_type << " " <<
          (*fit)->name() << field_type_suffix << ";" << std::endl;
      }
    }
  }

  void WriterGenerator::generate_accessors_decl_(
    Declaration::StructWriter* struct_writer) noexcept
  {
    Declaration::StructWriter::FieldWriterList_var fields =
      struct_writer->fields();

    for(Declaration::StructWriter::FieldWriterList::
          const_iterator fit = fields->begin();
        fit != fields->end(); ++fit)
    {
      std::string ret_type_name;
      std::string ret_type_name_const;

      Declaration::BaseWriter_var field_writer = (*fit)->writer();
      Declaration::SimpleWriter_var simple_field_writer =
        field_writer->as_simple_writer();

      if(simple_field_writer.in())
      {
        Declaration::SimpleWriter::CppWriteTraitsGenerator_var
          cpp_write_traits_generator =
            simple_field_writer->cpp_write_traits_generator();

        Declaration::SimpleWriter::CppWriteTraits_var
          cpp_write_traits =
            cpp_write_traits_generator->generate((*fit)->mapping_specifiers());

        ret_type_name = cpp_write_traits->write_type_name;
        ret_type_name_const = cpp_write_traits->write_type_const_name;
      }
      else
      {
        Declaration::StructWriter_var struct_field_writer =
          field_writer->as_struct_writer();
        if(struct_field_writer.in())
        {
          ret_type_name = std::string(struct_field_writer->name()) + "&";
          ret_type_name_const = std::string("const ") + ret_type_name;
        }
        else
        {
          assert(0);
        }
      }

      // accessor
      out_ << std::endl << offset_ << ret_type_name << " " <<
        (*fit)->name() << "();" << std::endl;

      // const accessor
      out_ << std::endl << offset_ << ret_type_name_const << " " <<
        (*fit)->name() << "() const;" << std::endl;
    }
  }

  void WriterGenerator::generate_accessors_impl_(
    Declaration::StructWriter* struct_writer) noexcept
  {
    Declaration::StructDescriptor::PosedFieldList_var fields =
      struct_writer->descriptor()->as_struct()->fields();

    Declaration::StructWriter::FieldWriterList_var field_writers =
      struct_writer->fields();

    unsigned long fixed_buf_i = 0;
    unsigned long fixed_buf_pos = 0;

    for(Declaration::StructDescriptor::PosedFieldList::
          const_iterator fit = fields->begin();
        fit != fields->end(); ++fit)
    {
      Declaration::StructWriter::FieldWriterList::const_iterator
        field_writer_it = field_writers->begin();

      for(; field_writer_it != field_writers->end();
          ++field_writer_it)
      {
        if(::strcmp((*fit)->name(), (*field_writer_it)->name()) == 0)
        {
          break;
        }
      }

      if(field_writer_it != field_writers->end())
      {
        std::string ret_type_name, ret_cast_call;
        std::string ret_type_const_name, ret_cast_const_call;

        Declaration::BaseWriter_var field_writer = (*field_writer_it)->writer();
        Declaration::SimpleWriter_var simple_field_writer =
          field_writer->as_simple_writer();
        if(simple_field_writer.in())
        {
          Declaration::SimpleWriter::CppWriteTraitsGenerator_var
            cpp_write_traits_generator =
              simple_field_writer->cpp_write_traits_generator();

          Declaration::SimpleWriter::CppWriteTraits_var
            cpp_write_traits =
              cpp_write_traits_generator->generate((*field_writer_it)->mapping_specifiers());

          ret_type_name = cpp_write_traits->write_type_name;
          ret_cast_call = cpp_write_traits->write_type_cast;

          ret_type_const_name = cpp_write_traits->write_type_const_name;
          ret_cast_const_call = cpp_write_traits->write_type_const_cast;
        }
        else
        {
          Declaration::StructWriter_var struct_field_writer =
            field_writer->as_struct_writer();
          if(struct_field_writer.in())
          {
            ret_type_name = std::string(struct_field_writer->name()) + "&";
            ret_type_const_name = std::string("const ") + ret_type_name;
          }
          else
          {
            assert(0);
          }
        }

        // accessor
        out_ << std::endl << offset_ << "inline" << std::endl <<
          offset_ << ret_type_name << std::endl <<
          offset_ << struct_writer->name() << "::" <<
            (*fit)->name() << "()" << std::endl <<
          offset_ << "{" << std::endl <<
          offset_ << "  return ";
        if(!ret_cast_call.empty()) // fixed simple type
        {
          out_ << ret_cast_call << "(static_cast<void*>(" <<
            FIXED_BUFFER_PREFIX << fixed_buf_i << "_" << " + " <<
            fixed_buf_pos << "))";
        }
        else
        {
          out_ << (*fit)->name() << "_";
        }
        out_ << ";" << std::endl <<
          offset_ << "}" << std::endl << std::endl;

        // const accessor
        out_ << offset_ << "inline" << std::endl <<
          offset_ << ret_type_const_name << std::endl <<
          offset_ << struct_writer->name() << "::" <<
            (*fit)->name() << "() const" << std::endl <<
          offset_ << "{" << std::endl <<
          offset_ << "  return ";
        if(!ret_cast_const_call.empty())
        {
          out_ << ret_cast_const_call << "(static_cast<const void*>(" <<
            FIXED_BUFFER_PREFIX << fixed_buf_i << "_ + " <<
            fixed_buf_pos << "))";
        }
        else
        {
          out_ << (*fit)->name() << "_";
        }
        out_ << ";" << std::endl << offset_ << "}" << std::endl;

        ++field_writer_it;
      }

      // if non fixed or complex start new buffer using
      if(!(*fit)->descriptor()->is_fixed() ||
         (*fit)->descriptor()->as_struct().in())
      {
        if(fixed_buf_pos != 0)
        {
          fixed_buf_pos = 0;
          ++fixed_buf_i;
        }
      }
      else
      {
        fixed_buf_pos += (*fit)->descriptor()->fixed_size();
      }
    }
  }
}
