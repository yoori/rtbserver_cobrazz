#include <mutex>

#include <catboost/model_interface/wrapped_calcer.h>

#include <Commons/ScopeGuard.hpp>

#include "CatBoostCTREvaluator.hpp"
#include "FeatureHash.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  class CatBoostCTREvaluator::FeatureBufProvider: public ReferenceCounting::AtomicImpl
  {
  public:
    struct Buf: public std::vector<float>
    {
      void set(const HashArray* hashes)
      {
        if (hashes)
        {
          for (const auto& [feature_index, feature_value] : *hashes)
          {
            (*this)[feature_hash_index(feature_index, size())] = feature_value;
          }
        }
      }

      void rollback(const HashArray* hashes)
      {
        if (hashes)
        {
          for (const auto& [feature_index, _] : *hashes)
          {
            (*this)[feature_hash_index(feature_index, size())] = 0.0;
          }
        }
      }
    };

    FeatureBufProvider(unsigned int buf_size)
      : buf_size_(buf_size)
    {}

    virtual ~FeatureBufProvider() noexcept
    {}

    std::shared_ptr<Buf> get()
    {
      std::shared_ptr<Buf> res;

      {
        std::unique_lock<std::mutex> lock(lock_);
        if (!feature_bufs_.empty())
        {
          feature_bufs_.rbegin()->swap(res);
          feature_bufs_.pop_back();
        }
      }

      if (!res)
      {
        res = std::make_shared<Buf>();
        res->resize(buf_size_, 0.0);
      }

      return res;
    }

    void
    release(std::shared_ptr<Buf> feature_buf) noexcept
    {
      try
      {
        std::unique_lock<std::mutex> lock(lock_);
        feature_bufs_.emplace_back(std::move(feature_buf));
      }
      catch (...)
      {
      }
    }

  protected:
    const unsigned int buf_size_;
    std::mutex lock_;
    std::deque<std::shared_ptr<Buf>> feature_bufs_;
  };

  class CatBoostCTREvaluator::PredictionContextImpl final:
    public CTREvaluator::PredictionContext
  {
  public:
    PredictionContextImpl(
      const CatBoostCTREvaluator& evaluator,
      const HashArray* request_hashes,
      const HashArray* auction_hashes,
      Generics::MonoAllocatorArena* arena)
      : evaluator_(evaluator),
        feature_buf_(evaluator.feature_buf_provider_->get()),
        saved_values_(decltype(saved_values_)::allocator_type(*arena)),
        base_feature_indexes_(decltype(base_feature_indexes_)::allocator_type(*arena))
    {
      set_base_(request_hashes);
      set_base_(auction_hashes);
    }

    ~PredictionContextImpl() noexcept override
    {
      for (const std::size_t feature_index : base_feature_indexes_)
      {
        (*feature_buf_)[feature_index] = 0.0;
      }
      evaluator_.feature_buf_provider_->release(std::move(feature_buf_));
    }

    double
    predict(
      const HashArray* candidate_hashes,
      const HashArray* opt_hashes) override
    {
      saved_values_.clear();
      saved_values_.reserve(
        (candidate_hashes ? candidate_hashes->size() : 0) +
        (opt_hashes ? opt_hashes->size() : 0));
      set_(candidate_hashes);
      set_(opt_hashes);

      auto rollback_guard = AdServer::Commons::make_scope_guard([this]() noexcept {
        for (auto it = saved_values_.rbegin(); it != saved_values_.rend(); ++it)
        {
          (*feature_buf_)[it->first] = it->second;
        }
      });

      return evaluator_.catboost_model_->CalcFlat(*feature_buf_);
    }

  private:
    void
    set_base_(const HashArray* hashes)
    {
      if (hashes)
      {
        base_feature_indexes_.reserve(base_feature_indexes_.size() + hashes->size());
        for (const auto& [feature_index, feature_value] : *hashes)
        {
          const std::size_t index = feature_hash_index(feature_index, feature_buf_->size());
          (*feature_buf_)[index] = feature_value;
          base_feature_indexes_.emplace_back(index);
        }
      }
    }

    void
    set_(const HashArray* hashes)
    {
      if (hashes)
      {
        for (const auto& [feature_index, feature_value] : *hashes)
        {
          const std::size_t index = feature_hash_index(feature_index, feature_buf_->size());
          saved_values_.emplace_back(index, (*feature_buf_)[index]);
          (*feature_buf_)[index] = feature_value;
        }
      }
    }

  private:
    const CatBoostCTREvaluator& evaluator_;
    std::shared_ptr<FeatureBufProvider::Buf> feature_buf_;
    Generics::MonoVector<std::pair<std::size_t, float>> saved_values_;
    Generics::MonoVector<std::size_t> base_feature_indexes_;
  };

  CatBoostCTREvaluator::CatBoostCTREvaluator(
    const String::SubString& model_file,
    unsigned int features_size)
    : catboost_model_(std::make_unique<ModelCalcerWrapper>(model_file.str().c_str())),
      feature_buf_provider_(std::make_unique<FeatureBufProvider>(features_size))
  {}

  CatBoostCTREvaluator::~CatBoostCTREvaluator()
  {}

  std::unique_ptr<CTREvaluator::PredictionContext>
  CatBoostCTREvaluator::create_prediction_context(
    const HashArray* request_hashes,
    const HashArray* auction_hashes,
    Generics::MonoAllocatorArena* arena) const
  {
    return std::make_unique<PredictionContextImpl>(
      *this,
      request_hashes,
      auction_hashes,
      arena);
  }

  double
  CatBoostCTREvaluator::predict(
    const ModelTraits& /*model*/,
    const CampaignSelectParams* /*request_params*/,
    const Creative* /*creative*/,
    const HashArray* request_hashes,
    const HashArray* auction_hashes,
    const HashArray* candidate_hashes,
    const HashArray* opt_hashes) const
  {
    auto feature_buf = feature_buf_provider_->get();
    feature_buf->set(request_hashes);
    feature_buf->set(auction_hashes);
    feature_buf->set(candidate_hashes);
    feature_buf->set(opt_hashes);

    auto feature_buf_guard = AdServer::Commons::make_scope_guard([&]() noexcept {
      feature_buf->rollback(request_hashes);
      feature_buf->rollback(auction_hashes);
      feature_buf->rollback(candidate_hashes);
      feature_buf->rollback(opt_hashes);
      feature_buf_provider_->release(std::move(feature_buf));
    });

    return catboost_model_->CalcFlat(*feature_buf);
  }
}
