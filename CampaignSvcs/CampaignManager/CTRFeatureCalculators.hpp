#ifndef CTRFEATURECALCULATORS_HPP_
#define CTRFEATURECALCULATORS_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/Hash.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "CampaignSelectParams.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  const int DEBUG_CTR_CALCULATION_ = 0; // 1;

  namespace CTR
  {
    typedef std::vector<std::pair<uint32_t, uint32_t> > HashArray;

    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<uint32_t>, uint32_t>
      HashMap;

    struct FeatureWeightTable: public std::vector<float>
    {
      unsigned long shifter;
    };

    struct Murmur32v3Adapter: public Generics::Murmur32v3Hasher
    {
      Murmur32v3Adapter(std::size_t hash_seed)
        : Generics::Murmur32v3Hasher(hash_seed)
      {}

      template <typename Value>
      void
      add(const Value& value) noexcept
      {
        Generics::Murmur32v3Hasher::add(&value, sizeof(value));
      }

      void
      add(const std::string& value) noexcept
      {
        Generics::Murmur32v3Hasher::add(value.data(), value.size());
      }
    };

    struct FeatureCalculator: public ReferenceCounting::AtomicImpl
    {
      virtual float
      eval(
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params, // request level
        const Tag::Size* tag_size, // auction level
        const Creative* creative // candidate level
        )
        noexcept = 0;

      virtual void
      eval_hashes(
        CTR::HashArray& result_hashes,
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params, // request level
        const Tag::Size* tag_size, // auction level
        const Creative* creative // candidate level
        )
        noexcept = 0;
    };

    typedef ReferenceCounting::SmartPtr<FeatureCalculator>
      FeatureCalculator_var;

    // FeatureCalculator implementations

    // FeatureCalculatorFinalImplHelper
    // weight by hash resolve helper
    class FeatureCalculatorFinalImplHelper
    {
    public:
      FeatureCalculatorFinalImplHelper(
        const FeatureWeightTable& weight_table)
        noexcept
        : weight_table_(weight_table)
      {}      

      bool
      hash_index_(
        uint32_t& index,
        const HashMap* hash_mapping,
        uint32_t hash)
        const
      {
        // shifter > 0 => index > 1
        index = (hash >> weight_table_.shifter) + 1;

        /*
        std::cerr << "weight_table_.shifter = " << weight_table_.shifter <<
          ", index = " << index <<
          ", hash_mapping = " << (hash_mapping ? "not null" : "null") << std::endl;
        */

        if(hash_mapping)
        {
          auto it = hash_mapping->find(index);
          if(it != hash_mapping->end())
          {
            index = it->second;
            return true;
          }
        }
        else
        {
          return true;
        }

        return false;
      }

      float
      weight_(const HashMap* hash_mapping, uint32_t hash) const
      {
        uint32_t index;

        if(hash_index_(index, hash_mapping, hash))
        {
          float weight = weight_table_[index - 1];

          if(DEBUG_CTR_CALCULATION_)
          {
            std::cout << "CTR DEBUG: hash = " << hash <<
              ", index = " << index <<
              ", weight = " << weight <<
              std::endl;
          }

          return weight;
        }

        return 0.0;
      }

    protected:
      const FeatureWeightTable& weight_table_;
    };

    typedef void (*AddHashFun)(
      Murmur32v3Adapter& hash,
      const CampaignSelectParams& request_params,
      const Tag::Size* tag_size,
      const Campaign* campaign);

    // FeatureCalculatorFinalImpl
    // eval hash and return weight
    template<void (*add_hash_fun)(
      Murmur32v3Adapter& hash,
      const CampaignSelectParams& request_params,
      const Tag::Size* tag_size,
      const Creative* creative)>
    class FeatureCalculatorFinalImpl:
      public FeatureCalculator,
      public FeatureCalculatorFinalImplHelper
    {
    public:
      FeatureCalculatorFinalImpl(
        const FeatureWeightTable& weight_table)
        noexcept
        : FeatureCalculatorFinalImplHelper(weight_table)
      {}

      virtual float
      eval(
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        add_hash_fun(hash_adapter, request_params, tag_size, creative);
        return weight_(hash_mapping, hash_adapter.finalize());
      }

      virtual void
      eval_hashes(
        HashArray& result_hashes,
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        add_hash_fun(hash_adapter, request_params, tag_size, creative);
        uint32_t index;
        if(hash_index_(index, hash_mapping, hash_adapter.finalize()))
        {
          result_hashes.push_back(std::make_pair(index, 1));
        }
      }
    };

    // FeatureCalculatorDelegateImpl
    template<void (*add_hash_fun)(
      Murmur32v3Adapter& hash,
      const CampaignSelectParams& request_params,
      const Tag::Size* tag_size,
      const Creative* creative)>
    class FeatureCalculatorDelegateImpl: public FeatureCalculator
    {
    public:
      FeatureCalculatorDelegateImpl(
        FeatureCalculator* next_calculator)
        noexcept
        : next_calculator_(ReferenceCounting::add_ref(next_calculator))
      {}

      virtual float
      eval(
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        add_hash_fun(hash_adapter, request_params, tag_size, creative);
        return next_calculator_->eval(
          hash_adapter,
          hash_mapping,
          request_params,
          tag_size,
          creative);
      }

      virtual void
      eval_hashes(
        HashArray& result_hashes,
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        add_hash_fun(hash_adapter, request_params, tag_size, creative);
        next_calculator_->eval_hashes(
          result_hashes,
          hash_adapter,
          hash_mapping,
          request_params,
          tag_size,
          creative);
      }

    protected:
      FeatureCalculator_var next_calculator_;
    };

    // FeatureCalculatorIntArrayFinalHelper
    // basis for array based feature final calculators
    //   keep weight table
    template<typename ContainerType>
    class FeatureCalculatorIntArrayFinalHelper:
      public FeatureCalculator,
      public FeatureCalculatorFinalImplHelper
    {
    public:
      FeatureCalculatorIntArrayFinalHelper(
        const FeatureWeightTable& weight_table)
        noexcept
        : FeatureCalculatorFinalImplHelper(weight_table)
      {}

    protected:
      float
      eval_final_(
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const ContainerType& elements)
        noexcept
      {
        float result_weight = 0;
        if(!elements.empty())
        {
          for(auto it = elements.begin(); it != elements.end(); ++it)
          {
            // need local hasher
            Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(static_cast<uint32_t>(*it));
            result_weight += weight_(hash_mapping, hash_adapter_copy.finalize());
          }
        }
        else
        {
          hash_adapter.add(static_cast<uint32_t>(0));
          result_weight += weight_(hash_mapping, hash_adapter.finalize());
        }

        return result_weight;
      }

      void
      eval_hashes_final_(
        CTR::HashArray& result_hashes,
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const ContainerType& elements)
        const
        noexcept
      {
        if(!elements.empty())
        {
          for(auto it = elements.begin(); it != elements.end(); ++it)
          {
            // need local hasher
            Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(static_cast<uint32_t>(*it));
            uint32_t index;
            if(hash_index_(index, hash_mapping, hash_adapter_copy.finalize()))
            {
              result_hashes.push_back(std::make_pair(index, 1));
            }
          }
        }
        else
        {
          hash_adapter.add(static_cast<uint32_t>(0));
          uint32_t index;
          if(hash_index_(index, hash_mapping, hash_adapter.finalize()))
          {
            result_hashes.push_back(std::make_pair(index, 1));
          }
        }
      }
    };

    // FeatureCalculatorIntArrayDelegateHelper
    // basis for array based feature delegate calculators
    //   keep next calculator ref
    template<typename ContainerType>
    class FeatureCalculatorIntArrayDelegateHelper:
      public FeatureCalculator
    {
    public:
      FeatureCalculatorIntArrayDelegateHelper(
        FeatureCalculator* next_calculator)
        noexcept
        : next_calculator_(ReferenceCounting::add_ref(next_calculator))
      {}

    protected:
      float
      eval_delegate_(
        Murmur32v3Adapter& hash_adapter,
        const ContainerType& elements,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        float result_weight = 0;

        if(!elements.empty())
        {
          for(auto ch_it = elements.begin();
              ch_it != elements.end(); ++ch_it)
          {
            Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(static_cast<uint32_t>(*ch_it));
            result_weight += next_calculator_->eval(
              hash_adapter_copy,
              hash_mapping,
              request_params,
              tag_size,
              creative);
          }
        }
        else
        {
          hash_adapter.add(static_cast<uint32_t>(0));
          result_weight += next_calculator_->eval(
            hash_adapter,
            hash_mapping,
            request_params,
            tag_size,
            creative);
        }

        return result_weight;
      }

      void
      eval_hashes_delegate_(
        CTR::HashArray& result_hashes,
        Murmur32v3Adapter& hash_adapter,
        const ContainerType& elements,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        if(!elements.empty())
        {
          for(auto ch_it = elements.begin();
              ch_it != elements.end(); ++ch_it)
          {
            Murmur32v3Adapter hash_adapter_copy(hash_adapter);
            hash_adapter_copy.add(static_cast<uint32_t>(*ch_it));
            next_calculator_->eval_hashes(
              result_hashes,
              hash_adapter_copy,
              hash_mapping,
              request_params,
              tag_size,
              creative);
          }
        }
        else
        {
          hash_adapter.add(static_cast<uint32_t>(0));
          next_calculator_->eval_hashes(
            result_hashes,
            hash_adapter,
            hash_mapping,
            request_params,
            tag_size,
            creative);
        }
      }

    protected:
      FeatureCalculator_var next_calculator_;
    };

    // FeatureCalculatorArrayParamFinalImpl
    // implementation of final calculator for CampaignSelectParams field
    //
    template<typename ContainerType>
    class FeatureCalculatorIntArrayParamFinalImpl:
      public FeatureCalculatorIntArrayFinalHelper<ContainerType>
    {
    public:
      FeatureCalculatorIntArrayParamFinalImpl(
        const FeatureWeightTable& weight_table,
        ContainerType CampaignSelectParams::* field)
        noexcept
        : FeatureCalculatorIntArrayFinalHelper<ContainerType>(weight_table),
          field_(field)
      {}

      virtual float
      eval(
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size*,
        const Creative*)
        noexcept
      {
        return this->eval_final_(
          hash_adapter,
          hash_mapping,
          request_params.*field_);
      }

      virtual void
      eval_hashes(
        CTR::HashArray& result_hashes,
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size*,
        const Creative*)
        noexcept
      {
        this->eval_hashes_final_(
          result_hashes,
          hash_adapter,
          hash_mapping,
          request_params.*field_);
      }

    protected:
      ContainerType CampaignSelectParams::* field_;
    };

    // FeatureCalculatorIntArrayParamDelegateImpl
    // implementation of delegate calculator for CampaignSelectParams field
    //
    template<typename ContainerType>
    class FeatureCalculatorIntArrayParamDelegateImpl:
      public FeatureCalculatorIntArrayDelegateHelper<ContainerType>
    {
    public:
      FeatureCalculatorIntArrayParamDelegateImpl(
        FeatureCalculator* next_calculator,
        ContainerType CampaignSelectParams::* field)
        noexcept
        : FeatureCalculatorIntArrayDelegateHelper<ContainerType>(
            next_calculator),
          field_(field)
      {}

      virtual float
      eval(
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        return this->eval_delegate_(
          hash_adapter,
          request_params.*field_,
          hash_mapping,
          request_params,
          tag_size,
          creative);
      }

      virtual void
      eval_hashes(
        CTR::HashArray& result_hashes,
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        this->eval_hashes_delegate_(
          result_hashes,
          hash_adapter,
          request_params.*field_,
          hash_mapping,
          request_params,
          tag_size,
          creative);
      }

    protected:
      ContainerType CampaignSelectParams::* field_;
    };

    // FeatureCalculatorArrayCreativeFinalImpl
    // implementation of final calculator for Creative array field
    //
    template<typename ContainerType>
    class FeatureCalculatorIntArrayCreativeFinalImpl:
      public FeatureCalculatorIntArrayFinalHelper<ContainerType>
    {
    public:
      FeatureCalculatorIntArrayCreativeFinalImpl(
        const FeatureWeightTable& weight_table,
        ContainerType Creative::* field)
        noexcept
        : FeatureCalculatorIntArrayFinalHelper<ContainerType>(weight_table),
          field_(field)
      {}

      virtual float
      eval(
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams&,
        const Tag::Size*,
        const Creative* creative)
        noexcept
      {
        return this->eval_final_(hash_adapter, hash_mapping, creative->*field_);
      }

      virtual void
      eval_hashes(
        CTR::HashArray& result_hashes,
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams&,
        const Tag::Size*,
        const Creative* creative)
        noexcept
      {
        this->eval_hashes_final_(result_hashes, hash_adapter, hash_mapping, creative->*field_);
      }

    protected:
      ContainerType Creative::* field_;
    };

    // FeatureCalculatorIntArrayCreativeDelegateImpl
    // implementation of delegate calculator for Creative field
    //
    template<typename ContainerType>
    class FeatureCalculatorIntArrayCreativeDelegateImpl:
      public FeatureCalculatorIntArrayDelegateHelper<ContainerType>
    {
    public:
      FeatureCalculatorIntArrayCreativeDelegateImpl(
        FeatureCalculator* next_calculator,
        ContainerType Creative::* field)
        noexcept
        : FeatureCalculatorIntArrayDelegateHelper<ContainerType>(
            next_calculator),
          field_(field)
      {}

      virtual float
      eval(
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        return this->eval_delegate_(
          hash_adapter,
          creative->*field_,
          hash_mapping,
          request_params,
          tag_size,
          creative);
      }

      virtual void
      eval_hashes(
        CTR::HashArray& result_hashes,
        Murmur32v3Adapter& hash_adapter,
        const HashMap* hash_mapping,
        const CampaignSelectParams& request_params,
        const Tag::Size* tag_size,
        const Creative* creative)
        noexcept
      {
        this->eval_hashes_delegate_(
          result_hashes,
          hash_adapter,
          creative->*field_,
          hash_mapping,
          request_params,
          tag_size,
          creative);
      }

    protected:
      ContainerType Creative::* field_;
    };

    // FeatureCalculatorCreator factory for create calculators
    // used for delegate weight_table to Calculator c-tor
    //
    struct FeatureCalculatorCreator: public ReferenceCounting::AtomicImpl
    {
      virtual FeatureCalculator_var
      create_final(const FeatureWeightTable& weight_table) = 0;

      virtual FeatureCalculator_var
      create_delegate(
        FeatureCalculator* next_calculator) = 0;
    };

    typedef ReferenceCounting::SmartPtr<FeatureCalculatorCreator>
      FeatureCalculatorCreator_var;

    // add_hash_fun passed in all places only as template argument for "garantee"
    // its inlining into FeatureCalculator::eval (not used as runtime function pointer arguments)
    template<void (*add_hash_fun)(
      Murmur32v3Adapter& hash,
      const CampaignSelectParams& request_params,
      const Tag::Size*,
      const Creative* creative)>
    struct TrivialFeatureCalculatorCreator: public FeatureCalculatorCreator
    {
      FeatureCalculator_var
      create_final(const FeatureWeightTable& weight_table)
      {
        return new FeatureCalculatorFinalImpl<add_hash_fun>(
          weight_table);
      }

      FeatureCalculator_var
      create_delegate(
        FeatureCalculator* next_calculator)
      {
        return new FeatureCalculatorDelegateImpl<add_hash_fun>(
          next_calculator);
      }
    };

    template<typename ContainerType>
    struct ArrayParamFeatureCalculatorCreator:
      public FeatureCalculatorCreator
    {
      ArrayParamFeatureCalculatorCreator(
        ContainerType CampaignSelectParams::* field)
        noexcept
        : field_(field)
      {}

      FeatureCalculator_var
      create_final(const FeatureWeightTable& weight_table)
      {
        return new FeatureCalculatorIntArrayParamFinalImpl<ContainerType>(
          weight_table, field_);
      }

      FeatureCalculator_var
      create_delegate(
        FeatureCalculator* next_calculator)
      {
        return new FeatureCalculatorIntArrayParamDelegateImpl<ContainerType>(
          next_calculator, field_);
      }

    private:
      ContainerType CampaignSelectParams::* field_;
    };

    template<typename ContainerType>
    struct ArrayCreativeFeatureCalculatorCreator:
      public FeatureCalculatorCreator
    {
      ArrayCreativeFeatureCalculatorCreator(
        ContainerType Creative::* field)
        noexcept
        : field_(field)
      {}

      FeatureCalculator_var
      create_final(const FeatureWeightTable& weight_table)
      {
        return new FeatureCalculatorIntArrayCreativeFinalImpl<ContainerType>(
          weight_table, field_);
      }

      FeatureCalculator_var
      create_delegate(
        FeatureCalculator* next_calculator)
      {
        return new FeatureCalculatorIntArrayCreativeDelegateImpl<ContainerType>(
          next_calculator, field_);
      }

    private:
      ContainerType Creative::* field_;
    };
  }
}
}

#endif /*CTRFEATURECALCULATORS_HPP_*/
