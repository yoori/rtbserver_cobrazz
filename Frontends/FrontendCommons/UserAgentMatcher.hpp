#ifndef _FRONTENDCOMMONS_USERAGENTMATCHER_HPP_
#define _FRONTENDCOMMONS_USERAGENTMATCHER_HPP_

#include <string>
#include <eh/Exception.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>

namespace FrontendCommons
{
  class UserAgentMatcher
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    UserAgentMatcher(): hash_set_() {}

    void init(const char *filename) /*throw(Exception, eh::Exception)*/;

    bool match(const char *user_agent) const
    {
      return hash_set_.find(user_agent) == hash_set_.end() ? false : true;
    }

  private:
    typedef Generics::GnuHashSet<Generics::StringHashAdapter> HashSetT;

    HashSetT hash_set_;
  };
} // namespace FrontendCommons

#endif /* _FRONTENDCOMMONS_USERAGENTMATCHER_HPP_ */

