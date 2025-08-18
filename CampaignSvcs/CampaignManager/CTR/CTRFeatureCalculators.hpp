#ifndef CTRFEATURECALCULATORS_HPP_
#define CTRFEATURECALCULATORS_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/Hash.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignManager/CampaignSelectParams.hpp>

namespace AdServer::CampaignSvcs
{
  const int DEBUG_CTR_CALCULATION_ = 0; // 1;

  namespace CTR
  {
    typedef std::vector<std::pair<uint32_t, uint32_t> > HashArray;

    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<uint32_t>, uint32_t>
      HashMap;

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
      FeatureCalculatorFinalImplHelper()
        noexcept
      {}      

      bool
      hash_index_(
        uint32_t& index,
        const HashMap* hash_mapping,
        uint32_t hash)
        const
      {
        if(hash_mapping)
        {
          auto it = hash_mapping->find(hash);
          if(it != hash_mapping->end())
          {
            index = it->second;
            return true;
          }
        }

        index = hash;
        return true;
      }
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
      FeatureCalculatorFinalImpl()
        noexcept
        : FeatureCalculatorFinalImplHelper()
      {}

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
      FeatureCalculatorIntArrayFinalHelper()
        noexcept
        : FeatureCalculatorFinalImplHelper()
      {}

    protected:
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
        ContainerType CampaignSelectParams::* field)
        noexcept
        : FeatureCalculatorIntArrayFinalHelper<ContainerType>(),
          field_(field)
      {}

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
        ContainerType Creative::* field)
        noexcept
        : FeatureCalculatorIntArrayFinalHelper<ContainerType>(),
          field_(field)
      {}

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
      create_final() = 0;

      virtual FeatureCalculator_var
      create_delegate(FeatureCalculator* next_calculator) = 0;
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
      create_final()
      {
        return new FeatureCalculatorFinalImpl<add_hash_fun>();
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
      create_final()
      {
        return new FeatureCalculatorIntArrayParamFinalImpl<ContainerType>(field_);
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
      create_final()
      {
        return new FeatureCalculatorIntArrayCreativeFinalImpl<ContainerType>(field_);
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

#endif /*CTRFEATURECALCULATORS_HPP_*/
