#pragma once

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Hash.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/TaskRunner.hpp>
#include <Sync/SyncPolicy.hpp>

//#include <PredictorSvcs/BidCostPredictor/CtrPredictor.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignManager/CTR/Algorithm.hpp>

#include "CTR/CTRFeatureCalculators.hpp"
#include "CampaignSelectParams.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  using namespace AdInstances;

  /*
    Publisher ID
    Site ID
    Tag ID
    External Tag ID (provided by publisher)
    Domain of the referrer
    Full referrer
    User Channels (i.e. behavioural channels in user profile)Yes
    Hours since midnight - [0,23], UTC
    Week day - days since Sunday, [0, 6]
    ISP ID
    Colocation ID
    Device Channel ID (most specific)
    Geo Channel ID for City
    Geo Channel ID for Region

    Advertiser ID
    Campaign ID
    Campaign Creative Group ID
    Creative Protocol Size (728x90)

    Campaign Creative ID 3.4+
    Creative ID 3.4+
    Name of Size Type of Creative (Banner, Overlay, Video etc) 3.4+
  */

  class FeatureNameResolver
  {
  public:
    FeatureNameResolver() noexcept;

    bool
    basic_feature_by_name(
      BasicFeature& basic_feature,
      const String::SubString& name)
      const
      noexcept;

  protected:
    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, BasicFeature>
      FeatureNameMap;

  protected:
    FeatureNameMap feature_names_;
  };

  class CTRProvider: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidConfig, Exception);
    DECLARE_EXCEPTION(Overflow, Exception);

    class Calculation;
    class CalculationContext;

    using CTRProvider_var = ReferenceCounting::SmartPtr<CTRProvider>;
    using ConstCTRProvider_var = ReferenceCounting::SmartPtr<const CTRProvider>;
    using Calculation_var = ReferenceCounting::SmartPtr<Calculation>;
    using CalculationContext_var = ReferenceCounting::SmartPtr<CalculationContext>;

    class CalculationContext: public ReferenceCounting::AtomicImpl
    {
    public:
      using CTRList = std::list<RevenueDecimal>;

      virtual RevenueDecimal
      get_ctr(const Creative* creative) const = 0;

      virtual bool
      check_rate(
        const Creative* creative,
        RevenueDecimal* rate = 0,
        bool* creative_dependent = 0) const = 0;

      virtual void
      get_ctr_details(
        CTRList& ctrs,
        const Creative* creative) const = 0;

    protected:
      virtual ~CalculationContext() noexcept;
    };

    class Calculation: public ReferenceCounting::AtomicImpl
    {
    public:
      virtual CalculationContext_var
      create_context(const Tag::Size* tag_size) const noexcept = 0;

      virtual std::string
      algorithm_id(const Creative* creative) const noexcept = 0;

    protected:
      virtual ~Calculation() noexcept;
    };

  public:
    virtual Calculation_var
    create_calculation(
      const CampaignSelectParams* request_params) const noexcept = 0;

  protected:
    virtual ~CTRProvider() noexcept;
  };

  using CTRProvider_var = CTRProvider::CTRProvider_var;
  using ConstCTRProvider_var = CTRProvider::ConstCTRProvider_var;
  using CTRProviderCalculation_var = CTRProvider::Calculation_var;
  using CTRProviderCalculationContext_var = CTRProvider::CalculationContext_var;
} // namespace AdServer::CampaignSvcs::CTR
