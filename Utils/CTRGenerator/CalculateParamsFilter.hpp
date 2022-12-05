#pragma once

#include "CTRGenerator.hpp"
#include <Generics/BitAlgs.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    class CalculateParamsFiller
    {
    public:
      static const unsigned long BF_TIMESTAMP;
      static const unsigned long BF_LINK;
    
      CalculateParamsFiller();
      
      void
      set_value(
        CTRGenerator::CalculateParams& calc_params,
        unsigned long basic_feature, // CTR::BasicFeature + syntetic features
        const String::SubString& str);
      
    protected:
      class CalcParamFiller: public ReferenceCounting::AtomicImpl
      {
      public:
        virtual void
        set_value(
          CTRGenerator::CalculateParams& calc_params,
          const String::SubString& str) = 0;
        
      protected:
        virtual
        ~CalcParamFiller() noexcept = default;
      };
      
      typedef ReferenceCounting::SmartPtr<CalcParamFiller>
      CalcParamFiller_var;
      
      typedef std::vector<CalcParamFiller_var> CalcParamFillerArray;
    
      template<typename IntType>
      class CalcParamFillerIntImpl: public CalcParamFiller
      {
      public:
        CalcParamFillerIntImpl(
          IntType CTRGenerator::CalculateParams::* field);
        
        virtual void
        set_value(
          CTRGenerator::CalculateParams& calc_params,
          const String::SubString& str);
        
      protected:
        virtual
        ~CalcParamFillerIntImpl() noexcept = default;
        
        IntType CTRGenerator::CalculateParams::* field_;
      };
      
      class CampaignFreqFiller: public CalcParamFiller
      {
      public:
        CampaignFreqFiller();
      
        virtual void
        set_value(
          CTRGenerator::CalculateParams& calc_params,
          const String::SubString& str);
      
      protected:
        virtual
        ~CampaignFreqFiller() noexcept = default;
      };
      
      class CalcParamFillerStringImpl: public CalcParamFiller
      {
      public:
        CalcParamFillerStringImpl(
          std::string CTRGenerator::CalculateParams::* field);
      
        virtual void
        set_value(
          CTRGenerator::CalculateParams& calc_params,
          const String::SubString& str);
      
      protected:
        virtual
        ~CalcParamFillerStringImpl() noexcept = default;
      
        std::string CTRGenerator::CalculateParams::* field_;
      };
    
      class CalcParamFillerIntListImpl: public CalcParamFiller
      {
      public:
        CalcParamFillerIntListImpl(
          CTRGenerator::IdSet CTRGenerator::CalculateParams::* field);
      
        virtual void
        set_value(
          CTRGenerator::CalculateParams& calc_params,
          const String::SubString& str);

      protected:
        virtual
        ~CalcParamFillerIntListImpl() noexcept = default;
      
        CTRGenerator::IdSet CTRGenerator::CalculateParams::* field_;
      };
    
      class CalcParamFillerTimestampImpl: public CalcParamFiller
      {
      public:
        CalcParamFillerTimestampImpl();
      
        virtual void
        set_value(
          CTRGenerator::CalculateParams& calc_params,
          const String::SubString& str);
      
      protected:
        virtual
        ~CalcParamFillerTimestampImpl() noexcept = default;
      };

      struct LinkFillerImpl: public CalcParamFiller
      {
      public:
        LinkFillerImpl();

        virtual void
        set_value(
          CTRGenerator::CalculateParams& calc_params,
          const String::SubString& str);
      };
    
    protected:
      CalcParamFillerArray processors_;
    };
  }
}

#include "CalculateParamsFilter.tpp"
