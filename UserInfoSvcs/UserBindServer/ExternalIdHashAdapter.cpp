#include <vector>
#include <iostream>

#include <Generics/Singleton.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>

#include "SimpleFixedAllocator.hpp"
#include "ExternalIdHashAdapter.hpp"

namespace AdServer
{
namespace UserInfoSvcs
{
  namespace
  {
    const String::AsciiStringManip::CharCategory DECIMAL_NUMBER("0-9");
    const String::AsciiStringManip::CharCategory HEX_LOWALPHA_NUMBER("0-9a-f");
    const String::AsciiStringManip::CharCategory HEX_UPALPHA_NUMBER("0-9A-F");
    const String::AsciiStringManip::CharCategory BASE64MOD_CHARS("0-9a-zA-Z_-");

    const char HEX_LOW_DIGITS[17] = "0123456789abcdef";
    const char HEX_UP_DIGITS[17] = "0123456789ABCDEF";

    std::string
    hex_low_encode(
      const unsigned char* data,
      size_t size,
      bool skip_leading_zeroes) /*throw(eh::Exception)*/
    {
      if(!size)
      {
        return std::string();
      }

      if(skip_leading_zeroes)
      {
        while(!*data)
        {
          data++;
          if(!--size)
          {
            return std::string(1, '0');
          }
        }
      }

      std::string result;
      result.reserve(size * 2);

      if(skip_leading_zeroes && !((*data) & 0xF0))
      {
        result.push_back(HEX_LOW_DIGITS[*data]);
        data++;
        size--;
      }

      for(; size--; data++)
      {
        char buf[2] =
          {
            HEX_LOW_DIGITS[(*data) >> 4],
            HEX_LOW_DIGITS[(*data) & 0xF]
          };
        result.append(buf, 2);
      }

      return result;
    }

    std::string
    hex_up_encode(
      const unsigned char* data,
      size_t size,
      bool skip_leading_zeroes) /*throw(eh::Exception)*/
    {
      if(!size)
      {
        return std::string();
      }

      if(skip_leading_zeroes)
      {
        while(!*data)
        {
          data++;
          if(!--size)
          {
            return std::string(1, '0');
          }
        }
      }

      std::string result;
      result.reserve(size * 2);

      if(skip_leading_zeroes && !((*data) & 0xF0))
      {
        result.push_back(HEX_UP_DIGITS[*data]);
        data++;
        size--;
      }

      for(; size--; data++)
      {
        char buf[2] =
          {
            HEX_UP_DIGITS[(*data) >> 4],
            HEX_UP_DIGITS[(*data) & 0xF]
          };
        result.append(buf, 2);
      }

      return result;
    }

    /*
    void
    base64_decode(
      std::string& dst,
      const String::SubString& src,
      bool padding = true)
    {
      if(!size)
      {
        return std::string();
      }

      if(skip_leading_zeroes)
      {
        while(!*data)
        {
          data++;
          if(!--size)
          {
            return std::string(1, '0');
          }
        }
      }

      std::string result;
      result.reserve(size * 2);

      if(skip_leading_zeroes && !((*data) & 0xF0))
      {
        result.push_back(HEX_LOW_DIGITS[*data]);
        data++;
        size--;
      }

      for(; size--; data++)
      {
        char buf[2] =
          {
            HEX_LOW_DIGITS[(*data) >> 4],
            HEX_LOW_DIGITS[(*data) & 0xF]
          };
        result.append(buf, 2);
      }

      return result;
    }
    */

    typedef Generics::Singleton<
      SimpleDistribAllocator,
      Generics::Helper::AutoPtr<SimpleDistribAllocator>,
      Generics::AtExitDestroying::DP_LOUD_COUNTER + 1>
      ExternalIdKeyAllocator;
  };

  class ExternalIdHashAdapter::EncodingSelector:
    public ReferenceCounting::AtomicImpl
  {
  public:
    enum EncoderId
    {
      EI_DEFAULT = 0,
      EI_SIZE_LESS_256 = 1,
      EI_8H_4H_4H_4H_12H = 2 // ifa
    };

    struct Encoder: public ReferenceCounting::AtomicImpl
    {
      Encoder(unsigned char id_val)
        : id_(id_val)
      {}

      unsigned char
      id() const
      {
        return id_;
      }

      virtual unsigned long
      encode_size(const String::SubString& text) const = 0;

      virtual void
      encode(void* buf, const String::SubString& text) const = 0;

      virtual unsigned long
      encoded_size_by_buf(const void* buf) const = 0;

      virtual std::string
      decode(const void* buf) const = 0;

    protected:
      unsigned char id_;
    };

    typedef ReferenceCounting::ConstPtr<Encoder>
      CEncoder_var;

  public:
    EncodingSelector() noexcept;

    const Encoder*
    select_encoder(const String::SubString& text);

    const Encoder*
    get_encoder(unsigned char index);

  protected:
    virtual
    ~EncodingSelector() noexcept
    {}

  protected:
    CEncoder_var default_encoder_;
    CEncoder_var size_less_256_encoder_;

    CEncoder_var uint_encoder_;

    CEncoder_var h24_encoder_;
    CEncoder_var h32_encoder_;
    CEncoder_var h40_encoder_;
    CEncoder_var h64_encoder_;
    CEncoder_var aid_h32_encoder_;
    CEncoder_var H32_encoder_;

    CEncoder_var h8_h4_h4_h4_h12_encoder_;
    CEncoder_var H8_H4_H4_H4_H12_encoder_;

    CEncoder_var caese_b22_encoder_;
    CEncoder_var b8_encoder_;
    CEncoder_var b12_encoder_;
    CEncoder_var b20_encoder_;
    CEncoder_var h96_encoder_;
    CEncoder_var h20_encoder_;

    std::vector<CEncoder_var> encoders_;
  };

  typedef ReferenceCounting::SmartPtr<ExternalIdHashAdapter::EncodingSelector>
    EncodingSelector_var;

  typedef Generics::Singleton<
    ExternalIdHashAdapter::EncodingSelector,
    EncodingSelector_var,
    Generics::AtExitDestroying::DP_LOUD_COUNTER>
    EncodingSelectorSingleton;

  // ExternalIdHashAdapter
  const unsigned int EXT_HASH_BUF_PRESPACE = 4;

  ExternalIdHashAdapter::
  ExternalIdHashAdapter(const String::SubString& text, size_t hash)
    noexcept
  {
    const EncodingSelector::Encoder* encoder =
      EncodingSelectorSingleton::instance().select_encoder(text);

    unsigned long encode_size = encoder->encode_size(text);
    data_ = ExternalIdKeyAllocator::instance().alloc(EXT_HASH_BUF_PRESPACE + 1 + encode_size);
    *static_cast<uint32_t*>(data_) = hash;
    *(static_cast<unsigned char*>(data_) + EXT_HASH_BUF_PRESPACE) = encoder->id();

    encoder->encode(
      static_cast<unsigned char*>(data_) + EXT_HASH_BUF_PRESPACE + 1,
      text);
  }

  ExternalIdHashAdapter::
  ExternalIdHashAdapter(const ExternalIdHashAdapter& init)
    noexcept
  {
    if(init.data_)
    {
      const EncodingSelector::Encoder* encoder =
        EncodingSelectorSingleton::instance().get_encoder(
          *(static_cast<unsigned char*>(init.data_) + EXT_HASH_BUF_PRESPACE));

      int ssize = encoder->encoded_size_by_buf(
        static_cast<char*>(init.data_) + EXT_HASH_BUF_PRESPACE + 1);
      data_ = ExternalIdKeyAllocator::instance().alloc(EXT_HASH_BUF_PRESPACE + 1 + ssize);
      ::memcpy(data_, init.data_, EXT_HASH_BUF_PRESPACE + 1 + ssize);
    }
    else
    {
      data_ = 0;
    }
  }

  ExternalIdHashAdapter::
  ~ExternalIdHashAdapter() noexcept
  {
    free_buf_();
  }

  ExternalIdHashAdapter&
  ExternalIdHashAdapter::
  operator=(ExternalIdHashAdapter&& init)
    noexcept
  {
    free_buf_();

    data_ = init.data_;
    init.data_ = 0;

    return *this;
  }

  void
  ExternalIdHashAdapter::free_buf_() noexcept
  {
    if(data_)
    {
      const EncodingSelector::Encoder* encoder =
        EncodingSelectorSingleton::instance().get_encoder(
          *(static_cast<unsigned char*>(data_) + EXT_HASH_BUF_PRESPACE));

      int ssize = encoder->encoded_size_by_buf(
        static_cast<char*>(data_) + EXT_HASH_BUF_PRESPACE + 1);

      ExternalIdKeyAllocator::instance().dealloc(
        data_,
        EXT_HASH_BUF_PRESPACE + 1 + ssize);

      data_ = 0;
    }
  }

  bool
  ExternalIdHashAdapter::
  operator==(const ExternalIdHashAdapter& right) const
    noexcept
  {
    if(::memcmp(data_, right.data_, EXT_HASH_BUF_PRESPACE + 1) != 0)
    {
      return false;
    }

    const EncodingSelector::Encoder* encoder =
      EncodingSelectorSingleton::instance().get_encoder(
        *(static_cast<unsigned char*>(data_) + EXT_HASH_BUF_PRESPACE));

    unsigned long left_data_size = encoder->encoded_size_by_buf(
      static_cast<char*>(data_) + EXT_HASH_BUF_PRESPACE + 1);
    unsigned long right_data_size = encoder->encoded_size_by_buf(
      static_cast<char*>(right.data_) + EXT_HASH_BUF_PRESPACE + 1);
    return left_data_size == right_data_size &&
      ::memcmp(
        static_cast<char*>(data_) + EXT_HASH_BUF_PRESPACE + 1,
        static_cast<char*>(right.data_) + EXT_HASH_BUF_PRESPACE + 1,
        left_data_size) == 0;
  }

  std::string
  ExternalIdHashAdapter::text() const
    noexcept
  {
    if(data_)
    {
      const EncodingSelector::Encoder* encoder =
        EncodingSelectorSingleton::instance().get_encoder(
          *(static_cast<unsigned char*>(data_) + EXT_HASH_BUF_PRESPACE));

      return encoder->decode(
        static_cast<char*>(data_) + EXT_HASH_BUF_PRESPACE + 1);
    }
    else
    {
      return std::string();
    }
  }

  unsigned char
  ExternalIdHashAdapter::encoder_id() const
    noexcept
  {
    return *(static_cast<unsigned char*>(data_) + EXT_HASH_BUF_PRESPACE);
  }

  // Encoder's
  namespace Encoders
  {
    class DefaultEncoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      DefaultEncoder(unsigned char id)
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id)
      {}

      virtual unsigned long
      encode_size(const String::SubString& text) const
      {
        return text.size() + 4;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        *static_cast<uint32_t*>(buf) = text.size();
        ::memcpy(static_cast<unsigned char*>(buf) + 4,
          text.data(),
          text.size());
      }

      virtual unsigned long
      encoded_size_by_buf(const void* buf) const
      {
        return *static_cast<const uint32_t*>(buf) + 4;
      }

      virtual std::string
      decode(const void* buf) const
      {
        return std::string(
          static_cast<const char*>(buf) + 4,
          *static_cast<const uint32_t*>(buf));
      }

    protected:
      virtual ~DefaultEncoder() noexcept
      {}
    };

    // SizeLess256Encoder
    class SizeLess256Encoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      SizeLess256Encoder(unsigned char id)
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id)
      {}

      virtual unsigned long
      encode_size(const String::SubString& text) const
      {
        assert(text.size() < 256);
        return text.size() + 1;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        assert(text.size() < 256);
        *static_cast<unsigned char*>(buf) = text.size();
        ::memcpy(static_cast<unsigned char*>(buf) + 1,
          text.data(),
          text.size());
      }

      virtual unsigned long
      encoded_size_by_buf(const void* buf) const
      {
        return *static_cast<const unsigned char*>(buf) + 1;
      }

      virtual std::string
      decode(const void* buf) const
      {
        return std::string(
          static_cast<const char*>(buf) + 1,
          *static_cast<const unsigned char*>(buf));
      }

    protected:
      virtual ~SizeLess256Encoder() noexcept
      {}
    };

    // h8_h4_h4_h4_h12_Encoder
    class h8_h4_h4_h4_h12_Encoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      h8_h4_h4_h4_h12_Encoder(unsigned char id)
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id),
          minus_("-")
      {}

      virtual unsigned long
      encode_size(const String::SubString&) const
      {
        return 16;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        assert(text.size() == 36);
        char* cbuf = static_cast<char*>(buf);
        String::AsciiStringManip::hex_to_buf(text.substr(0, 8), cbuf + 0);
        String::AsciiStringManip::hex_to_buf(text.substr(9, 4), cbuf + 4);
        String::AsciiStringManip::hex_to_buf(text.substr(14, 4), cbuf + 6);
        String::AsciiStringManip::hex_to_buf(text.substr(19, 4), cbuf + 8);
        String::AsciiStringManip::hex_to_buf(text.substr(24, 12), cbuf + 10);
      }

      virtual unsigned long
      encoded_size_by_buf(const void*) const
      {
        return 16;
      }

      virtual std::string
      decode(const void* buf) const
      {
        const unsigned char* cbuf = static_cast<const unsigned char*>(buf);
        return hex_low_encode(cbuf, 4, false) + minus_ +
          hex_low_encode(cbuf + 4, 2, false) + minus_ +
          hex_low_encode(cbuf + 6, 2, false) + minus_ +
          hex_low_encode(cbuf + 8, 2, false) + minus_ +
          hex_low_encode(cbuf + 10, 6, false);
      }

    protected:
      virtual ~h8_h4_h4_h4_h12_Encoder() noexcept
      {}

    protected:
      const std::string minus_;
    };

    // H8_H4_H4_H4_H12_Encoder
    class H8_H4_H4_H4_H12_Encoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      H8_H4_H4_H4_H12_Encoder(unsigned char id)
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id),
          minus_("-")
      {}

      virtual unsigned long
      encode_size(const String::SubString&) const
      {
        return 16;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        assert(text.size() == 36);
        char* cbuf = static_cast<char*>(buf);
        String::AsciiStringManip::hex_to_buf(text.substr(0, 8), cbuf + 0);
        String::AsciiStringManip::hex_to_buf(text.substr(9, 4), cbuf + 4);
        String::AsciiStringManip::hex_to_buf(text.substr(14, 4), cbuf + 6);
        String::AsciiStringManip::hex_to_buf(text.substr(19, 4), cbuf + 8);
        String::AsciiStringManip::hex_to_buf(text.substr(24, 12), cbuf + 10);
      }

      virtual unsigned long
      encoded_size_by_buf(const void*) const
      {
        return 16;
      }

      virtual std::string
      decode(const void* buf) const
      {
        const unsigned char* cbuf = static_cast<const unsigned char*>(buf);
        return hex_up_encode(cbuf, 4, false) + minus_ +
          hex_up_encode(cbuf + 4, 2, false) + minus_ +
          hex_up_encode(cbuf + 6, 2, false) + minus_ +
          hex_up_encode(cbuf + 8, 2, false) + minus_ +
          hex_up_encode(cbuf + 10, 6, false);
      }

    protected:
      virtual ~H8_H4_H4_H4_H12_Encoder() noexcept
      {}

    protected:
      const std::string minus_;
    };

    // hX_Encoder
    class hX_Encoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      hX_Encoder(
        unsigned char id,
        unsigned long x,
        const String::SubString& prefix = String::SubString())
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id),
          len_(x),
          prefix_(prefix.str())
      {}

      virtual unsigned long
      encode_size(const String::SubString&) const
      {
        return (len_ + 1) / 2;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        assert(text.size() == len_ + prefix_.size());
        char* cbuf = static_cast<char*>(buf);
        String::AsciiStringManip::hex_to_buf(
          text.substr(prefix_.size()), cbuf);
      }

      virtual unsigned long
      encoded_size_by_buf(const void*) const
      {
        return (len_ + 1) / 2;
      }

      virtual std::string
      decode(const void* buf) const
      {
        const unsigned char* cbuf = static_cast<const unsigned char*>(buf);
        return prefix_ + hex_low_encode(cbuf, (len_ + 1) / 2, false);
      }

    protected:
      virtual ~hX_Encoder() noexcept
      {}

    protected:
      const unsigned long len_;
      const std::string prefix_;
    };

    // HX_Encoder
    class HX_Encoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      HX_Encoder(
        unsigned char id,
        unsigned long x,
        const String::SubString& prefix = String::SubString())
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id),
          len_(x),
          prefix_(prefix.str())
      {}

      virtual unsigned long
      encode_size(const String::SubString&) const
      {
        return (len_ + 1) / 2;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        assert(text.size() == len_ + prefix_.size());
        char* cbuf = static_cast<char*>(buf);
        String::AsciiStringManip::hex_to_buf(
          text.substr(prefix_.size()), cbuf);
      }

      virtual unsigned long
      encoded_size_by_buf(const void*) const
      {
        return (len_ + 1) / 2;
      }

      virtual std::string
      decode(const void* buf) const
      {
        const unsigned char* cbuf = static_cast<const unsigned char*>(buf);
        return prefix_ + String::StringManip::hex_encode(
          cbuf, (len_ + 1) / 2, false);
      }

    protected:
      virtual ~HX_Encoder() noexcept
      {}

    protected:
      const unsigned long len_;
      const std::string prefix_;
    };

    // CAESE_B22_Encoder
    class CAESE_B22_Encoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      CAESE_B22_Encoder(unsigned char id)
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id),
          prefix_("CAESE")
      {}

      virtual unsigned long
      encode_size(const String::SubString&) const
      {
        return 17;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        assert(text.size() == 27);
        std::string d;
        String::StringManip::base64mod_decode(d, text.substr(5, 20), false);
        assert(d.size() == 15);
        ::memcpy(buf, d.data(), d.size());
        char* cbuf = static_cast<char*>(buf);
        cbuf[15] = text[25];
        cbuf[16] = text[26];        
      }

      virtual unsigned long
      encoded_size_by_buf(const void*) const
      {
        return 17;
      }

      virtual std::string
      decode(const void* buf) const
      {
        std::string d;
        d.reserve(17);
        String::StringManip::base64mod_encode(d, buf, 15, false);
        const char* cbuf = static_cast<const char*>(buf);
        d.push_back(cbuf[15]);
        d.push_back(cbuf[16]);
        return prefix_ + d;
      }

    protected:
      virtual ~CAESE_B22_Encoder() noexcept
      {}

    protected:
      const std::string prefix_;
    };

    // BMX_Encoder
    class BMX_Encoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      BMX_Encoder(unsigned char id, unsigned long len, bool padding)
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id),
          len_(len),
          padding_(padding)
      {}

      virtual unsigned long
      encode_size(const String::SubString&) const
      {
        return len_ * 3 / 4;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        assert(text.size() == len_);
        std::string d;
        String::StringManip::base64mod_decode(d, text, padding_);
        assert(d.size() == len_ * 3 / 4);
        ::memcpy(buf, d.data(), d.size());
      }

      virtual unsigned long
      encoded_size_by_buf(const void*) const
      {
        return len_ * 3 / 4;
      }

      virtual std::string
      decode(const void* buf) const
      {
        std::string d;
        String::StringManip::base64mod_encode(d, buf, len_ * 3 / 4, padding_);
        return d;
      }

    protected:
      virtual ~BMX_Encoder() noexcept
      {}

    protected:
      const unsigned long len_;
      const bool padding_;
    };

    // UIntEncoder
    class UIntEncoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      UIntEncoder(unsigned char id)
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id)
      {}

      virtual unsigned long
      encode_size(const String::SubString&) const
      {
        return 4;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        uint32_t val;
        if(!String::StringManip::str_to_int(text, val))
        {
          assert(0);
        }

        *static_cast<uint32_t*>(buf) = val;
      }

      virtual unsigned long
      encoded_size_by_buf(const void*) const
      {
        return 4;
      }

      virtual std::string
      decode(const void* buf) const
      {
        char res_str[std::numeric_limits<uint32_t>::digits10 + 3];
        if(!String::StringManip::int_to_str(*static_cast<const uint32_t*>(buf), res_str, sizeof(res_str)))
        {
          assert(0);
        }

        return std::string(res_str);
      }

    protected:
      virtual ~UIntEncoder() noexcept
      {}
    };

    /*
    // BOX_Encoder
    class BOX_Encoder:
      public ExternalIdHashAdapter::EncodingSelector::Encoder
    {
    public:
      BOX_Encoder(unsigned char id, unsigned long len)
        : ExternalIdHashAdapter::EncodingSelector::Encoder(id),
          len_(len)
      {}

      virtual unsigned long
      encode_size(const String::SubString&) const
      {
        return len_ * 3 / 4;
      }

      virtual void
      encode(void* buf, const String::SubString& text) const
      {
        assert(text.size() == len_);
        std::string d;
        String::StringManip::base64_decode(d, text, false);
        assert(d.size() == len_ * 3 / 4);
        ::memcpy(buf, d.data(), d.size());
      }

      virtual unsigned long
      encoded_size_by_buf(const void*) const
      {
        return len_ * 3 / 4;
      }

      virtual std::string
      decode(const void* buf) const
      {
        std::string d;
        String::StringManip::base64_encode(d, buf, len_ * 3 / 4, false);
        return d;
      }

    protected:
      virtual ~BOX_Encoder() noexcept
      {}

    protected:
      unsigned long len_;
    };
    */
  };

  ExternalIdHashAdapter::EncodingSelector::
  EncodingSelector() noexcept
  {
    default_encoder_ = new Encoders::DefaultEncoder(1);
    size_less_256_encoder_ = new Encoders::SizeLess256Encoder(2);
    h8_h4_h4_h4_h12_encoder_ = new Encoders::h8_h4_h4_h4_h12_Encoder(3);
      // ifa, advark, btw, buz, rambler, rpb, alfasense, umedia
    h64_encoder_ = new Encoders::hX_Encoder(4, 64); // bln, decenterads
    caese_b22_encoder_ = new Encoders::CAESE_B22_Encoder(5); // google
    h40_encoder_ = new Encoders::hX_Encoder(6, 40); // videonow
    b8_encoder_ = new Encoders::BMX_Encoder(7, 8, false); // relap, otm
    b12_encoder_ = new Encoders::BMX_Encoder(8, 12, false); // otm, mgid
    h32_encoder_ = new Encoders::hX_Encoder(9, 32); // gpm, admixer, hyper, admedia, adlook, otm
    aid_h32_encoder_ = new Encoders::hX_Encoder(10, 32, String::SubString("aid")); // otm(aid..)
    H32_encoder_ = new Encoders::HX_Encoder(11, 32); // sape
    H8_H4_H4_H4_H12_encoder_ = new Encoders::H8_H4_H4_H4_H12_Encoder(12); // mobfox
    h24_encoder_ = new Encoders::hX_Encoder(13, 24); // rtw(subset)
    b20_encoder_ = new Encoders::BMX_Encoder(14, 20, false); // rtw(subset)
    uint_encoder_ = new Encoders::UIntEncoder(15); // pprofit
    h96_encoder_ = new Encoders::hX_Encoder(16, 96); // moevideo
    h20_encoder_ = new Encoders::hX_Encoder(17, 20); // moevideo
    // TODO c/, yandex/

    encoders_.resize(256);
    encoders_[default_encoder_->id()] = default_encoder_;
    encoders_[size_less_256_encoder_->id()] = size_less_256_encoder_;

    encoders_[uint_encoder_->id()] = uint_encoder_;

    encoders_[h24_encoder_->id()] = h24_encoder_;
    encoders_[h32_encoder_->id()] = h32_encoder_;
    encoders_[h40_encoder_->id()] = h40_encoder_;
    encoders_[h64_encoder_->id()] = h64_encoder_;
    encoders_[aid_h32_encoder_->id()] = aid_h32_encoder_;
    encoders_[H32_encoder_->id()] = H32_encoder_;

    encoders_[h8_h4_h4_h4_h12_encoder_->id()] = h8_h4_h4_h4_h12_encoder_;
    encoders_[H8_H4_H4_H4_H12_encoder_->id()] = H8_H4_H4_H4_H12_encoder_;

    encoders_[caese_b22_encoder_->id()] = caese_b22_encoder_;
    encoders_[b8_encoder_->id()] = b8_encoder_;
    encoders_[b12_encoder_->id()] = b12_encoder_;
    encoders_[b20_encoder_->id()] = b20_encoder_;
    encoders_[h96_encoder_->id()] = h96_encoder_;
    encoders_[h20_encoder_->id()] = h20_encoder_;
  };

  const ExternalIdHashAdapter::EncodingSelector::Encoder*
  ExternalIdHashAdapter::EncodingSelector::
  select_encoder(const String::SubString& text)
  {
    if(!text.empty() &&
      text.size() <= 9 &&
      *text.begin() != '0' && (
        DECIMAL_NUMBER.find_nonowned(text.begin(), text.end()) == text.end()))
    {
      return uint_encoder_;
    }
    
    if(text.size() == 36 && text[8] == '-' && text[13] == '-' &&
       text[18] == '-' && text[23] == '-')
    {
      if((HEX_LOWALPHA_NUMBER.find_nonowned(text.begin(), text.begin() + 8) ==
          text.begin() + 8) &&
        (HEX_LOWALPHA_NUMBER.find_nonowned(text.begin() + 9, text.begin() + 13) ==
          text.begin() + 13) &&
        (HEX_LOWALPHA_NUMBER.find_nonowned(text.begin() + 14, text.begin() + 18) ==
          text.begin() + 18) &&
        (HEX_LOWALPHA_NUMBER.find_nonowned(text.begin() + 19, text.begin() + 23) ==
          text.begin() + 23) &&
        (HEX_LOWALPHA_NUMBER.find_nonowned(text.begin() + 24, text.begin() + 36) ==
          text.begin() + 36))
      {
        return h8_h4_h4_h4_h12_encoder_;
      }
    }

    if(text.size() == 24 && (
         HEX_LOWALPHA_NUMBER.find_nonowned(text.begin(), text.end()) == text.end()))
    {
      return h24_encoder_;
    }

    if(text.size() == 32 && (
         HEX_LOWALPHA_NUMBER.find_nonowned(text.begin(), text.end()) == text.end()))
    {
      return h32_encoder_;
    }

    if(text.size() == 40 && (
         HEX_LOWALPHA_NUMBER.find_nonowned(text.begin(), text.end()) == text.end()))
    {
      return h40_encoder_;
    }

    if(text.size() == 64 && (
         HEX_LOWALPHA_NUMBER.find_nonowned(text.begin(), text.end()) == text.end()))
    {
      return h64_encoder_;
    }

    if(text.size() == 32 && (
         HEX_UPALPHA_NUMBER.find_nonowned(text.begin(), text.end()) == text.end()))
    {
      return H32_encoder_;
    }

    if(text.size() == 96 && (
         HEX_LOWALPHA_NUMBER.find_nonowned(text.begin(), text.end()) == text.end()))
    {
      return h96_encoder_;
    }

    if(text.size() == 35 &&
      text.compare(0, 3, "aid", 3) == 0 &&
      HEX_LOWALPHA_NUMBER.find_nonowned(text.begin() + 3, text.end()) == text.end())
    {
      return aid_h32_encoder_;
    }

    if(text.size() == 36 && text[8] == '-' && text[13] == '-' &&
       text[18] == '-' && text[23] == '-')
    {
      if((HEX_UPALPHA_NUMBER.find_nonowned(text.begin(), text.begin() + 8) ==
          text.begin() + 8) &&
        (HEX_UPALPHA_NUMBER.find_nonowned(text.begin() + 9, text.begin() + 13) ==
          text.begin() + 13) &&
        (HEX_UPALPHA_NUMBER.find_nonowned(text.begin() + 14, text.begin() + 18) ==
          text.begin() + 18) &&
        (HEX_UPALPHA_NUMBER.find_nonowned(text.begin() + 19, text.begin() + 23) ==
          text.begin() + 23) &&
        (HEX_UPALPHA_NUMBER.find_nonowned(text.begin() + 24, text.begin() + 36) ==
          text.begin() + 36))
      {
        return H8_H4_H4_H4_H12_encoder_;
      }
    }

    // base64 encoders (placed after hex, because hex is subset of base64)
    if(text.size() == 27 &&
      text.compare(0, 5, "CAESE", 5) == 0 &&
      BASE64MOD_CHARS.find_nonowned(text.begin() + 5, text.end()) == text.end())
    {
      return caese_b22_encoder_;
    }

    if(text.size() == 8 && (
         BASE64MOD_CHARS.find_nonowned(text.begin(), text.end()) == text.end()))
    {
      return b8_encoder_;
    }

    if(text.size() == 12 && (
         BASE64MOD_CHARS.find_nonowned(text.begin(), text.end()) == text.end()))
    {
      return b12_encoder_;
    }

    if(text.size() < 256)
    {
      return size_less_256_encoder_;
    }

    return default_encoder_;
  };

  const ExternalIdHashAdapter::EncodingSelector::Encoder*
  ExternalIdHashAdapter::EncodingSelector::
  get_encoder(unsigned char index)
  {
    const ExternalIdHashAdapter::EncodingSelector::Encoder* ret =
      encoders_[index];
    assert(ret);
    return ret;
  };  
};
};
