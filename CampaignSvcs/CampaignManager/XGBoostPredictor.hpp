#ifndef XGBOOSTPREDICTOR_HPP_
#define XGBOOSTPREDICTOR_HPP_

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
  class XGBoostPredictorPool: public std::enable_shared_from_this<XGBoostPredictorPool>
  {
  protected:
    // pimpl for xgboost library
    class PredictorWrapperDescriptor;
    class PredictorWrapper;
    typedef std::unique_ptr<PredictorWrapper> PredictorWrapperPtr;
    typedef std::list<PredictorWrapperPtr> PredictorWrapperList;

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidConfig, Exception);

    struct Predictor
    {
    public:
      Predictor(
        std::shared_ptr<XGBoostPredictorPool> pool,
        PredictorWrapperPtr&& predictor_impl)
        noexcept;

      virtual
      ~Predictor() noexcept;

      float
      predict(
        const HashArray& hashes,
        const HashArray* add_hashes1 = 0,
        const HashArray* add_hashes2 = 0,
        const HashArray* add_hashes3 = 0);

    protected:
      std::shared_ptr<XGBoostPredictorPool> pool_;
      PredictorWrapperPtr predictor_impl_;
    };

    typedef std::shared_ptr<Predictor> Predictor_var;

  public:
    XGBoostPredictorPool(const String::SubString& model_file);

    virtual ~XGBoostPredictorPool() noexcept;

    Predictor_var
    get_predictor() noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

  protected:
    void release_(PredictorWrapperPtr&& predictor_impl) noexcept;

  protected:
    // data required for initialize new PredictorWrapper
    std::unique_ptr<PredictorWrapperDescriptor> predictor_descriptor_;

    // PredictorWrapper pool
    SyncPolicy::Mutex lock_;
    PredictorWrapperList predictors_;
  };

  typedef std::shared_ptr<XGBoostPredictorPool>
    XGBoostPredictorPool_var;
}
}
}

#endif /*XGBOOSTPREDICTOR_HPP_*/
