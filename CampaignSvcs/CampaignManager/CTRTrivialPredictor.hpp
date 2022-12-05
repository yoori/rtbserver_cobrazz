#ifndef CTRTRIVIALPREDICTOR_HPP_
#define CTRTRIVIALPREDICTOR_HPP_

#include <memory>
#include <list>

#include <eh/Exception.hpp>
#include <String/SubString.hpp>

#include "CTRFeatureCalculators.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
namespace CTR
{
  struct TrivialPredictor;

  typedef std::shared_ptr<TrivialPredictor> TrivialPredictor_var;

  struct TrivialPredictor
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidConfig, Exception);

  public:
    virtual
    ~TrivialPredictor() noexcept;

    static TrivialPredictor_var
    load(std::istream& istr);

    RevenueDecimal
    predict(unsigned long tag_id, const String::SubString& domain) const;

  protected:
    struct KeyHashAdapter
    {
      KeyHashAdapter(unsigned long tag_id_val, const String::SubString& domain_val);

      bool
      operator==(const KeyHashAdapter& right) const;

      size_t
      hash() const throw ();

      const unsigned long tag_id;
      const std::string domain;

    private:
      size_t hash_;
    };

    typedef Generics::GnuHashTable<KeyHashAdapter, RevenueDecimal>
      CTRMap;

  protected:
    TrivialPredictor(CTRMap ctr_map) noexcept;

  protected:
    CTRMap ctr_map_;
    static const TrivialPredictor::KeyHashAdapter DEFAULT_KEY_;
  };
}
}
}

#endif /*CTRTRIVIALPREDICTOR_HPP_*/
