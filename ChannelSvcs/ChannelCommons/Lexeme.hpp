#ifndef AD_SERVER_CHANNELSVCS_LEXEME_HPP__
#define AD_SERVER_CHANNELSVCS_LEXEME_HPP__

#include<vector>
#include<String/SubString.hpp>
#include<ReferenceCounting/AtomicImpl.hpp>
#include<ReferenceCounting/SmartPtr.hpp>

namespace AdServer
{
namespace ChannelSvcs
{
  struct LexemeData
  {
    typedef std::vector<Generics::SubStringHashAdapter> Forms;
    std::string data;
    Forms forms;
  };

  class Lexeme:
    public LexemeData,
    public ReferenceCounting::AtomicImpl
  {
  public:
    size_t memory_size() const noexcept;

  private:
    virtual ~Lexeme() noexcept {};
  };

  typedef ReferenceCounting::SmartPtr<Lexeme> Lexeme_var;
}
}

namespace AdServer
{
namespace ChannelSvcs
{
  inline
  size_t Lexeme::memory_size() const noexcept
  {
    return sizeof(Lexeme) + data.capacity() +
      forms.capacity() * sizeof(String::SubString);
  }
}
}
#endif //CHANNELSVCS_LEXEME_HPP__

