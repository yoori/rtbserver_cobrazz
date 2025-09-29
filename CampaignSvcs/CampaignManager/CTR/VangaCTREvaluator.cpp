#include <fstream>
#include <deque>
#include <mutex>
#include <cfloat>

#include <DTree/SVM.hpp>
#include <DTree/FastFeatureSet.hpp>

#include <Generics/Singleton.hpp>

#include "VangaCTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  struct FastFeatureSetHolder:
    public Vanga::FastFeatureSet,
    public ReferenceCounting::AtomicImpl
  {
  protected:
    virtual ~FastFeatureSetHolder() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<FastFeatureSetHolder>
    FastFeatureSetHolder_var;

  namespace
  {
    class FeatureSetProvider_: public ReferenceCounting::AtomicImpl
    {
    public:
      FastFeatureSetHolder_var
      get()
      {
        FastFeatureSetHolder_var res;

        {
          SyncPolicy::WriteGuard lock(lock_);
          if(!feature_sets_.empty())
          {
            feature_sets_.rbegin()->swap(res);
            feature_sets_.pop_back();
          }
        }

        return res.in() ? res :
          FastFeatureSetHolder_var(new FastFeatureSetHolder());
      }

      void
      release(FastFeatureSetHolder_var& feature_set)
      {
        // DEBUG - need remove after test
        //assert(feature_set->check(0));

        SyncPolicy::WriteGuard lock(lock_);
        feature_sets_.push_back(FastFeatureSetHolder_var());
        feature_sets_.rbegin()->swap(feature_set);
      }

    protected:
      typedef Sync::Policy::PosixThread SyncPolicy;

    protected:
      virtual ~FeatureSetProvider_() noexcept
      {}

    protected:
      SyncPolicy::Mutex lock_;
      std::deque<FastFeatureSetHolder_var> feature_sets_;
    };

    typedef ReferenceCounting::SmartPtr<FeatureSetProvider_> FeatureSetProvider_var;

    typedef Generics::Singleton<FeatureSetProvider_, FeatureSetProvider_var>
      FeatureSetProvider;
  }

  VangaCTREvaluator::VangaCTREvaluator(
    const String::SubString& model_file)
  {
    try
    {
      std::ifstream model_file_istr(model_file.str().c_str());
      Vanga::DTree_var regression_predictor = Vanga::DTree::load(model_file_istr);
      vanga_predictor_ =
        new Vanga::LogRegPredictor<Vanga::DTree>(regression_predictor);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't init Vanga predictor: " << ex.what();
      throw Exception(ostr);
    }
  }

  RevenueDecimal
  VangaCTREvaluator::get_ctr(
    const ModelTraits& /*model*/,
    const CampaignSelectParams* /*request_params*/,
    const Creative* /*creative*/,
    const HashArray* request_hashes,
    const HashArray* auction_hashes,
    const HashArray* candidate_hashes,
    const HashArray* opt_hashes) const
  {
    const HashArray* HASH_SETS[] = {
      request_hashes,
      auction_hashes,
      candidate_hashes,
      opt_hashes
    };

    FeatureSetProvider_& feature_set_provider = FeatureSetProvider::instance();

    FastFeatureSetHolder_var vanga_feature_set = feature_set_provider.get();

    for (unsigned int i = 0; i < sizeof(HASH_SETS) / sizeof(HASH_SETS[0]); ++i)
    {
      if (HASH_SETS[i])
      {
        vanga_feature_set->set(HASH_SETS[i]->begin(), HASH_SETS[i]->end());
      }
    }

    if (DEBUG_CTR_CALCULATION_)
    {
      std::cout << "CTR DEBUG: vanga hashes: ";
      for (unsigned int i = 0; i < sizeof(HASH_SETS) / sizeof(HASH_SETS[0]); ++i)
      {
        if (HASH_SETS[i])
        {
          for(auto it = HASH_SETS[i]->begin(); it != HASH_SETS[i]->end(); ++it)
          {
            std::cout << (it != HASH_SETS[i]->begin() ? ", " : "") << it->first;
          }

          std::cout << "; ";
        }
      }

      std::cout << std::endl;
    }

    float ctr = vanga_predictor_->fpredict(*vanga_feature_set); // no throw

    for (unsigned int i = 0; i < sizeof(HASH_SETS) / sizeof(HASH_SETS[0]); ++i)
    {
      if (HASH_SETS[i])
      {
        vanga_feature_set->rollback(HASH_SETS[i]->begin(), HASH_SETS[i]->end());
      }
    }

    feature_set_provider.release(vanga_feature_set);

    if(ctr < DBL_MIN) // prevent sub normal states (FP_ZERO, FP_SUBNORMAL)
    {
      return RevenueDecimal::ZERO;
    }

    return Generics::convert_float<RevenueDecimal>(ctr);
  }
}
