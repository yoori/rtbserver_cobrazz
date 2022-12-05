#ifndef EXTERNAL_ID_HASH_ADAPTER_HPP_
#define EXTERNAL_ID_HASH_ADAPTER_HPP_

namespace AdServer
{
namespace UserInfoSvcs
{
  class ExternalIdHashAdapter
  {
  public:
    class EncodingSelector;

  public:
    ExternalIdHashAdapter() noexcept;

    ExternalIdHashAdapter(const String::SubString& text, size_t hash)
      noexcept;

    ExternalIdHashAdapter(ExternalIdHashAdapter&& init) noexcept;

    ExternalIdHashAdapter(const ExternalIdHashAdapter& init) noexcept;

    ExternalIdHashAdapter&
    operator=(const ExternalIdHashAdapter& init) noexcept;

    ExternalIdHashAdapter&
    operator=(ExternalIdHashAdapter&& init) noexcept;

    ~ExternalIdHashAdapter() noexcept;

    bool
    operator==(const ExternalIdHashAdapter& right) const noexcept;

    size_t
    hash() const noexcept;

    std::string
    text() const noexcept;

    unsigned char
    encoder_id() const noexcept;

  protected:
    void
    free_buf_() noexcept;

  protected:
    void* data_;
  };  
};
};

namespace AdServer
{
namespace UserInfoSvcs
{
  // StringDefHashAdapter
  inline
  ExternalIdHashAdapter::
  ExternalIdHashAdapter() noexcept
    : data_(0)
  {}

  inline
  ExternalIdHashAdapter::
  ExternalIdHashAdapter(ExternalIdHashAdapter&& init)
    noexcept
  {
    data_ = init.data_;
    init.data_ = 0;
  }

  inline
  size_t
  ExternalIdHashAdapter::
  hash() const
    noexcept
  {
    return data_ ? *static_cast<uint32_t*>(data_) : 0;
  }
};
};

#endif /*EXTERNAL_ID_HASH_ADAPTER_HPP_*/
