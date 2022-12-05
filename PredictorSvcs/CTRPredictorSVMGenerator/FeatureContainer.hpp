/* $Id: FeatureContainer.hpp 185978 2020-07-04 00:12:49Z jurij_kuznecov $
* @file FeatureContainer.hpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Feature container
*/
#pragma once

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Singleton.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <xsd/Predictor/CTRPredictorSVMGeneratorConfig.hpp>
#include <Utils/CTRGenerator/CTRGenerator.hpp>

namespace AdServer
{
  namespace Predictor
  {

    typedef std::map<unsigned long, unsigned long> FeatureColumns;
    typedef std::vector<std::string> FeatureNames;

    /**
     * Features container 
     */
    class FeatureContainer_:
          public ReferenceCounting::AtomicImpl
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      
    public:
      typedef xsd::AdServer::Configuration::SVMGeneratorConfigurationType SVMGeneratorConfig;
      typedef SVMGeneratorConfig::Model_type::Feature_sequence FeatureSeq;
      typedef SVMGeneratorConfig::Model_type::Feature_type FeatureType;
      
    public:
      
      /**
       * @brief Initialize contaner.
       *
       * @param configured feature sequence
       */
      void init(
        const FeatureSeq& feature_seq) /*throw(eh::Exception)*/;
      
      /**
       * @brief Resolve features.
       *
       * @param feature names
       * @param resolved feature columns
       */
      unsigned long resolve_features(
        const FeatureNames& feature_names,
        FeatureColumns& feature_columns) /*throw(eh::Exception)*/;

      /**
       * @brief Get feature list.
       *
       * @return feature list
       */
      const AdServer::CampaignSvcs::CTRGenerator::FeatureList&
      features() const;

    protected:

      /**
       * @brief Destructor.
       */
      virtual
      ~FeatureContainer_() noexcept = default;

    private:
      AdServer::CampaignSvcs::CTRGenerator::FeatureList features_;
      AdServer::CampaignSvcs::CTR::FeatureNameResolver feature_name_resolver_;
    };

    typedef ReferenceCounting::QualPtr<FeatureContainer_> FeatureContainer_var;  
    typedef Generics::Singleton<FeatureContainer_, FeatureContainer_var>
      FeatureContainer;
  }
}

