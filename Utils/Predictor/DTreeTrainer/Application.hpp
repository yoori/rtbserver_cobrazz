#ifndef UTILS_DTREETRAINER_APPLICATION_HPP_
#define UTILS_DTREETRAINER_APPLICATION_HPP_

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

#include <Utils/Predictor/DTree/TreeLearner.hpp>
#include <Utils/Predictor/DTree/Label.hpp>

using namespace Vanga;

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef PredictedLogLossGain<DefaultPinaltyStrategy> UseLogLossGain;
  //typedef LogLossGain UseLogLossGain;

  struct Fold
  {
    double portion;
    unsigned long weight;
  };

  typedef std::vector<Fold> FoldArray;

  struct DTreeProp: public ReferenceCounting::AtomicImpl
  {
    typedef std::map<std::string, std::string> PropMap;
    PropMap props;

    std::string
    to_string() const
    {
      std::ostringstream ss;
      for(auto it = props.begin(); it != props.end(); ++it)
      {
        ss << (it != props.begin() ? "," : "") << it->first << "=" << it->second;
      }
      return ss.str();
    }

  protected:
    virtual ~DTreeProp() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<DTreeProp> DTreeProp_var;

  typedef SVM<PredictedBoolLabel> SVMImpl;

  typedef ReferenceCounting::SmartPtr<SVMImpl> SVMImpl_var;

  typedef std::vector<SVMImpl_var> SVMImplArray;

  class TrainTreeTask;

  typedef ReferenceCounting::SmartPtr<TrainTreeTask> TrainTreeTask_var;

public:
  Application_() noexcept;

  virtual
  ~Application_() noexcept;

  void
  main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  double
  train_(
    DTree_var& tree,
    Predictor* prev_predictor,
    SVMImpl* train_svm,
    SVMImpl* test_svm,
    unsigned long max_iterations,
    unsigned long step_depth,
    unsigned long check_depth,
    unsigned long train_bags,
    bool out_of_bag_validate,
    bool print_trace);

  /*
   * Set => Train + Test
   *   Set of DTree:
   *     Train => SubTrain + Noise + SubTest
   *     SubTrain => SubTrainBag's
   */
  void
  train_forest_(
    std::vector<std::pair<DTree_var, DTreeProp_var> >& trees,
    SVMImpl* train_svm,
    const SVMImpl* test_svm,
    unsigned long max_iterations,
    unsigned long max_global_iterations,
    unsigned long max_super_iterations,
    unsigned long step_depth,
    unsigned long check_depth,
    unsigned long num_trees,
    unsigned long threads,
    unsigned long train_bugs_number,
    unsigned long test_bugs_number)
    noexcept;

  void
  train_sub_forest_(
    std::vector<std::pair<DTree_var, DTreeProp_var> >& res_trees,
    SVMImpl* train_svm,
    SVMImpl* test_svm,
    Generics::TaskRunner* task_runner,
    unsigned long max_iterations,
    unsigned long max_global_iterations,
    unsigned long step_depth,
    unsigned long check_depth,
    unsigned long num_trees,
    unsigned long train_bugs_number,
    unsigned long test_bugs_number)
    noexcept;

  void
  train_dtree_set_(
    std::vector<DTree_var>& new_dtrees,
    const std::vector<DTree_var>& prev_dtrees,
    SVMImpl* ext_train_svm,
    SVMImpl* ext_test_svm,
    unsigned long max_global_iterations,
    unsigned long step_depth,
    unsigned long check_depth,
    unsigned long train_bags,
    bool out_of_bag_validate,
    bool add_trees,
    bool only_add_trees,
    double add_gain_coef,
    const char* opt_step_model_out,
    unsigned long threads)
    noexcept;

  double
  train_on_bags_(
    DTree_var& res_tree,
    Predictor* prev_predictor,
    TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context* context,
    SVMImpl* train_svm,
    SVMImpl* test_svm,
    SVMImpl* ext_test_svm,
    unsigned long max_iterations,
    unsigned long step_depth,
    unsigned long check_depth,
    bool print_trace,
    Generics::TaskRunner* task_runner);

  void
  init_bags_(
    SVMImplArray& bags,
    SVMImpl_var& test_svm,
    SVMImpl_var& train_svm,
    SVMImpl* ext_train_svm,
    SVMImpl* ext_test_svm,
    bool train_bags,
    bool out_of_bag_validate);

  void
  prepare_bags_(
    TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context_var& context,
    SVMImplArray& bags,
    Predictor* predictor);

  void
  init_context_and_bags_(
    TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context_var& context,
    SVMImplArray& bags,
    SVMImpl_var& test_svm,
    SVMImpl_var& train_svm,
    Predictor* predictor,
    SVMImpl* ext_train_svm,
    SVMImpl* ext_test_svm,
    bool train_bags,
    bool out_of_bag_validate);

  void
  construct_best_forest_(
    std::vector<std::pair<DTree_var, DTreeProp_var> >& res_trees,
    const std::vector<std::pair<DTree_var, DTreeProp_var> >& check_trees,
    unsigned long num_trees,
    const SVMImpl* svm)
    noexcept;

  void
  select_best_forest_(
    std::vector<unsigned long>& indexes,
    const std::vector<PredArrayHolder_var>& preds,
    const SVMImpl* svm,
    unsigned long max_trees);

  double
  logloss_(
    const std::vector<std::pair<DTree_var, DTreeProp_var> >& trees,
    const SVMImpl* svm)
    noexcept;

  template<int FOLD_SIZE>
  void
  fill_bags_(
    SVMImplArray& bags,
    const Fold (&folds)[FOLD_SIZE],
    unsigned long bag_number,
    SVMImpl* svm);

  static DTreeProp_var
  fill_dtree_prop_(
    unsigned long step_depth,
    unsigned long best_iteration,
    const SVMImplArray& bags,
    unsigned long train_size);

  void
  load_dictionary_(
    Vanga::FeatureDictionary& dict,
    const char* file);

  template<typename IteratorType>
  Predictor_var
  init_reg_predictor(
    IteratorType begin_it,
    IteratorType end_it);

  void
  deep_print_(
    std::ostream& ostr,
    Predictor* predictor,
    const char* prefix,
    const FeatureDictionary* dict,
    double base,
    const SVMImpl* svm)
    const noexcept;
};

typedef Generics::Singleton<Application_> Application;

#endif /*UTILS_DTREETRAINER_APPLICATION_HPP_*/
