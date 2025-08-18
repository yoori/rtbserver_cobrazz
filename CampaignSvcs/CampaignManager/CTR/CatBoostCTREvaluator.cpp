#include <mutex>

#include <catboost/wrapped_calcer.h>

#include "CatBoostCTREvaluator.hpp"

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
            (*this)[feature_index % size()] = feature_value;
          }
        }
      }

      void rollback(const HashArray* hashes)
      {
        if (hashes)
        {
          for (const auto& [feature_index, _] : *hashes)
          {
            (*this)[feature_index % size()] = 0.0;
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
        if(!feature_bufs_.empty())
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
    release(std::shared_ptr<Buf> feature_buf)
    {
      std::unique_lock<std::mutex> lock(lock_);
      feature_bufs_.emplace_back(nullptr);
      feature_bufs_.rbegin()->swap(feature_buf);
    }

  protected:
    const unsigned int buf_size_;
    std::mutex lock_;
    std::deque<std::shared_ptr<Buf>> feature_bufs_;
  };

  CatBoostCTREvaluator::CatBoostCTREvaluator(
    const String::SubString& model_file,
    unsigned int features_size)
    : catboost_model_(std::make_unique<ModelCalcerWrapper>(model_file.str().c_str())),
      feature_buf_provider_(std::make_unique<FeatureBufProvider>(features_size))
  {}

  CatBoostCTREvaluator::~CatBoostCTREvaluator()
  {}

  RevenueDecimal
  CatBoostCTREvaluator::get_ctr(
    const ModelTraits& model,
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

    const double result_x = catboost_model_->Calc(*feature_buf, empty_cat_features_);

    feature_buf->rollback(request_hashes);
    feature_buf->rollback(auction_hashes);
    feature_buf->rollback(candidate_hashes);
    feature_buf->rollback(opt_hashes);

    return Generics::convert_float<RevenueDecimal>(1 / (1 + std::exp(-result_x)));
  }
}
