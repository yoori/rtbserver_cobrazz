#ifndef PREDICTOR_SVMUTIL_APPLICATION_HPP_
#define PREDICTOR_SVMUTIL_APPLICATION_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>
#include <Generics/Time.hpp>
#include <Commons/Containers.hpp>

#include <DTree/TreeLearner.hpp>
#include <DTree/Label.hpp>

using namespace Vanga;

class Application_
{
public:
  Application_() noexcept;

  virtual
  ~Application_() noexcept;

  void
  main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef SVM<BoolLabel> SVMImpl;

  typedef ReferenceCounting::SmartPtr<SVMImpl> SVMImpl_var;

  typedef std::unordered_set<unsigned long> FeatureIdSet;

  struct CountFeatureKey
  {
    CountFeatureKey()
      : count(0),
        feature_id(0)
    {}

    CountFeatureKey(unsigned long count_val, unsigned long feature_id_val)
      : count(count_val),
        feature_id(feature_id_val)
    {}

    bool
    operator<(const CountFeatureKey& right) const
    {
      return count < right.count || (
        count == right.count && feature_id < right.feature_id);
    }

    const unsigned long count;
    const unsigned long feature_id;
  };

  typedef std::set<CountFeatureKey> CountFeatureSet;

protected:
  // load only counters
  void
  load_feature_counters_(
    CountFeatureSet& feature_counters,
    const char* file_path);

  void
  correlate_(
    FeatureIdSet& checked_features,
    FeatureIdSet& skip_features,
    bool& some_feature_unloaded,
    const char* file_path,
    const FeatureIdSet& untouch_features,
    const FeatureIdSet& ignore_features,
    unsigned long max_features,
    double max_corr_coef,
    unsigned long min_occur_coef);
};

typedef Generics::Singleton<Application_> Application;

#endif /*PREDICTOR_SVMUTIL_APPLICATION_HPP_*/
