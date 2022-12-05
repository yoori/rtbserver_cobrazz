/// @file DomainParser.hpp
#ifndef _DOMAINPARSER_HPP_
#define _DOMAINPARSER_HPP_

#include <set>
#include <map>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <String/SubString.hpp>
#include <xsd/CampaignSvcs/DomainConfig.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    class DomainParser: public virtual ReferenceCounting::AtomicImpl
    {
    public:
      typedef xsd::AdServer::Configuration::DomainConfigurationType
        DomainConfig;

      /**
       * @param domain_config Load list of Top Level Domains from XML file
       */
      DomainParser(const DomainConfig& domain_config) noexcept;

      /**
       * @param domain Must be in lower case
       * @param specific_domain The return value
       */
      void
      specific_domain(
        const String::SubString& domain,
        std::string& specific_domain)
        const
        noexcept;

    protected:
      virtual
      ~DomainParser() noexcept;
    private:
      typedef std::set<std::string> ThirdsDomainSet;
      typedef std::map<std::string, ThirdsDomainSet> SecondsDomainMap;
      typedef std::map<std::string, SecondsDomainMap> FirstDomainMap;

      /// Prefix tree - Radix Trie used for data lookup.
      FirstDomainMap domains_trie_;
    };

    typedef ReferenceCounting::SmartPtr<DomainParser> DomainParser_var;
  }
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    inline
    DomainParser::~DomainParser() noexcept
    {
    }
  }
}

#endif // _DOMAINPARSER_HPP_
