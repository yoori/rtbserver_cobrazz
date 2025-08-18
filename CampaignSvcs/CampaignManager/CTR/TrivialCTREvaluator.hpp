#ifndef CAMPAIGNMANAGER_CTR_TRIVIALCTREVALUATOR_HPP_
#define CAMPAIGNMANAGER_CTR_TRIVIALCTREVALUATOR_HPP_

//#include <PredictorSvcs/BidCostPredictor/CtrPredictor.hpp>
#include <Logger/Logger.hpp>
#include <String/SubString.hpp>
#include "CTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  struct TrivialCTREvaluator: public CTREvaluator
  {
    DECLARE_EXCEPTION(InvalidConfig, CTREvaluator::Exception);

    TrivialCTREvaluator(const String::SubString& model_file);

    virtual ~TrivialCTREvaluator() noexcept;

    RevenueDecimal
    get_ctr(
      const ModelTraits& model,
      const CampaignSelectParams* request_params,
      const Creative* creative,
      const HashArray* request_hashes,
      const HashArray* auction_hashes,
      const HashArray* candidate_hashes,
      const HashArray* opt_hashes) const override;

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
    void load_(std::istream& istr);

  protected:
    CTRMap ctr_map_;
    static const KeyHashAdapter DEFAULT_KEY_;
  };
}

#endif /*CAMPAIGNMANAGER_CTR_TRIVIALCTREVALUATOR_HPP_*/
