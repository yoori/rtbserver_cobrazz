#ifndef EXTERNAL_ID_HASH_ADAPTER_HPP_
#define EXTERNAL_ID_HASH_ADAPTER_HPP_

// STD
#include <sstream>

// THIS
#include <UserInfoSvcs/UserBindServer/Utils.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  class ExternalIdHashAdapter final
  {
  public:
    class EncodingSelector;

  public:
    explicit ExternalIdHashAdapter() noexcept;

    ExternalIdHashAdapter(
      const String::SubString& text,
      size_t hash) noexcept;

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

    std::string text() const noexcept;

    std::string str() const noexcept;

    unsigned char
    encoder_id() const noexcept;

    friend std::ostream& operator<<(
      std::ostream& stream,
      const ExternalIdHashAdapter& adapter)
    {
      static const std::string empty("###");
      const auto& str = adapter.str();
      stream << (str.empty() ? empty : str);
      return stream;
    }

  private:
    void free_buf_() noexcept;

  private:
    void* data_;
  };  
};
};

namespace AdServer
{
namespace UserInfoSvcs
{
  inline std::istream& operator>>(
    std::istream& stream,
    ExternalIdHashAdapter& adapter)
  {
    static std::string empty("###");

    std::string text;
    stream >> text;
    if (text.size() == 3 && text == empty)
    {
      text = std::string();
    }

    const auto hash = Utils::hash(text);
    ExternalIdHashAdapter other(text, hash);
    adapter = std::move(other);

    return stream;
  }

  // StringDefHashAdapter
  inline
  ExternalIdHashAdapter::ExternalIdHashAdapter() noexcept
    : data_(nullptr)
  {
  }

  inline
  ExternalIdHashAdapter::ExternalIdHashAdapter(
    ExternalIdHashAdapter&& init) noexcept
  {
    data_ = init.data_;
    init.data_ = nullptr;
  }

  inline
  size_t ExternalIdHashAdapter::hash() const noexcept
  {
    return data_ ? *static_cast<uint32_t*>(data_) : 0;
  }
};
};

#endif /*EXTERNAL_ID_HASH_ADAPTER_HPP_*/
