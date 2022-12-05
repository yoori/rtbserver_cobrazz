#ifndef TREELEARNER_HPP_
#define TREELEARNER_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Generics/Time.hpp>
#include <Generics/TaskRunner.hpp>
#include <Commons/Containers.hpp>

#include "SVM.hpp"
#include "DTree.hpp"

namespace Vanga
{
  typedef std::set<unsigned long> FeatureSet;

  // TreeNodeDescr
  struct TreeNodeDescr: public ReferenceCounting::AtomicImpl
  {
  public:
    typedef std::map<double, double> AvgCoverMap;
    typedef std::multimap<double, unsigned long> GainToFeatureMap;

  public:
    TreeNodeDescr()
      : delta_prob(0.0)
    {}

    unsigned long feature_id;
    double delta_prob;
    double delta_gain;
    ReferenceCounting::SmartPtr<TreeNodeDescr> yes_tree;
    ReferenceCounting::SmartPtr<TreeNodeDescr> no_tree;

  public:
    // return avg => cover mapping
    void
    avg_covers(AvgCoverMap& avg_covers, unsigned long all_rows) const noexcept;

    void
    gain_features(GainToFeatureMap& gain_to_features) const noexcept;

    ReferenceCounting::SmartPtr<TreeNodeDescr>
    sub_tree(
      double min_avg,
      unsigned long min_node_cover = 0)
      const noexcept;

    /*
    std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = nullptr,
      const unsigned long* all_rows = nullptr)
      const noexcept;

    std::string
    to_xml(
      const char* prefix,
      const FeatureDictionary* dict = nullptr,
      const unsigned long* all_rows = nullptr)
      const noexcept;
    */

    void
    features(FeatureSet& features) const;

  protected:
    typedef std::map<unsigned long, double> FeatureToGainMap;

  protected:
    void
    collect_avg_covers_(
      AvgCoverMap& avg_covers,
      unsigned long all_rows) const;

    void
    collect_feature_gains_(
      FeatureToGainMap& feature_to_gains)
      const;
  };

  typedef ReferenceCounting::SmartPtr<TreeNodeDescr> TreeNodeDescr_var;

  // TreeLearner
  template<typename LabelType, typename GainType>
  class TreeLearner
  {
  public:
    typedef SVM<LabelType> SVMT;
    typedef GainType GainT;

    typedef ReferenceCounting::SmartPtr<const SVM<LabelType> > ConstSVM_var;

    typedef ReferenceCounting::SmartPtr<SVM<LabelType> > SVM_var;

    typedef std::vector<SVM_var> SVMArray;

    typedef std::deque<Row*> RowPtrArray;

    typedef std::vector<unsigned long> FeatureIdArray;

    typedef std::unordered_map<unsigned long, SVM_var> FeatureRowsMap;

    class LearnTreeHolder;
    typedef ReferenceCounting::SmartPtr<LearnTreeHolder> LearnTreeHolder_var;

    struct GainTreeNodeDescrKey
    {
      GainTreeNodeDescrKey(double gain_val, TreeNodeDescr* tree_node_val);

      bool
      operator<(const GainTreeNodeDescrKey& right) const;

      double gain;
      TreeNodeDescr_var tree_node;
    };

    typedef std::multiset<GainTreeNodeDescrKey> GainToTreeNodeDescrMap;

    // BagHolder
    struct BagHolder: public ReferenceCounting::AtomicImpl
    {
      FeatureIdArray features;
      FeatureRowsMap feature_rows;
      SVM_var bag;

    protected:
      virtual ~BagHolder() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<BagHolder> BagHolder_var;
    typedef std::vector<BagHolder_var> BagHolderArray;

    // BagPart
    struct BagPart: public ReferenceCounting::AtomicImpl
    {
      BagHolder_var bag_holder;
      SVM_var svm;

    protected:
      virtual ~BagPart() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<BagPart> BagPart_var;
    typedef std::vector<BagPart_var> BagPartArray;

    // LearnContext
    class LearnContext: public ReferenceCounting::AtomicImpl
    {
      friend class TreeLearner;

    public:
      enum FeatureSelectionStrategy
      {
        FSS_BEST,
        FSS_TOP3_RANDOM
      };

      DTree_var
      train(
        unsigned long max_add_depth,
        unsigned long check_depth,
        FeatureSelectionStrategy feature_selection_strategy);

    protected:
      struct TreeReplace
      {
        double old_tree_prob_base;
        LearnTreeHolder_var old_tree;
        LearnTreeHolder_var new_tree;
      };

      struct DigCacheKey
      {
        DigCacheKey(unsigned long tree_id_val, unsigned long bag_id_val)
          : tree_id(tree_id_val),
            bag_id(bag_id_val)
        {}

        unsigned long tree_id;
        unsigned long bag_id;
      };

      struct DigCache
      {
        unsigned long best_feature_id;
        double best_gain;
      };

      typedef std::map<DigCacheKey, DigCache> DigCacheMap;

    protected:
      LearnContext(
        Generics::TaskRunner* task_runner,
        const BagPartArray& bags,
        const DTree* tree);

      virtual ~LearnContext() noexcept;

      LearnTreeHolder_var
      fill_learn_tree_(
        unsigned long& max_tree_id,
        const BagPartArray& bags,
        const DTree* tree);

      double
      init_delta_prob_(const BagPartArray& bags);

      void
      fetch_nodes_(
        std::multimap<double, TreeReplace>& nodes,
        LearnTreeHolder* tree,
        double prob,
        unsigned long max_add_depth,
        unsigned long check_depth,
        FeatureSelectionStrategy feature_selection_strategy);

      DTree_var
      fill_dtree_(LearnTreeHolder* learn_tree_holder);

      void
      convert_abs_prob_to_delta_(
        LearnTreeHolder* tree,
        double base_prob,
        const BagPartArray& bags);

    protected:
      Generics::TaskRunner_var task_runner_;

      BagHolderArray bags;

      LearnTreeHolder_var cur_tree_;
      unsigned long max_tree_id_;

      DigCacheMap dig_cache_;
    };

    typedef ReferenceCounting::SmartPtr<LearnContext>
      LearnContext_var;

    // Context
    class Context: public ReferenceCounting::AtomicImpl
    {
      friend class TreeLearner;

    public:
      LearnContext_var
      create_learner(
        const DTree* base_tree,
        Generics::TaskRunner* task_runner = nullptr);

    protected:
      Context(const BagPartArray& bag_parts);

      virtual ~Context() noexcept;

    protected:
      BagPartArray bag_parts_;
    };

    typedef ReferenceCounting::SmartPtr<Context> Context_var;

  public:
    static Context_var
    create_context(const SVMArray& svm_array);

    static double
    eval_gain(
      unsigned long yes_value_labeled,
      unsigned long yes_value_unlabeled,
      unsigned long labeled,
      unsigned long unlabeled);

    /*
    static double
    delta_prob(const BagPartArray& bags);
    */

    static void
    div_bags_(
      BagPartArray& yes_bag_parts,
      BagPartArray& no_bag_parts,
      const BagPartArray& bag_parts,
      unsigned long feature_id);

    static void
    div_rows_(
      SVM_var& yes_svm,
      SVM_var& no_svm,
      unsigned long feature_id,
      const SVM<LabelType>* svm,
      const FeatureRowsMap& feature_rows);

    static void
    fill_feature_rows_(
      FeatureIdArray& features,
      FeatureRowsMap& feature_rows,
      const SVM<LabelType>& svm)
      noexcept;

    static void
    check_feature_(
      double& res_gain,
      double& res_no_delta,
      double& res_yes_delta,
      GainType& gain_calc,
      double top_pred,
      unsigned long feature_id,
      const FeatureRowsMap& feature_rows,
      const FeatureSet& skip_features,
      const SVM<LabelType>* feature_svm,
      const SVM<LabelType>* node_svm,
      unsigned long check_depth)
      noexcept;

  protected:
    // ProcessorType
    //   ContextType
    //   ResultType
    //   aggregate(const ResultType& yes_res, const ResultType& no_res)
    //   null_result()
    //
    template<typename ProcessorType>
    static typename ProcessorType::ResultType
    best_dig_(
      Generics::TaskRunner* task_runner,
      const typename ProcessorType::ContextType& context,
      const ProcessorType& processor,
      double base_pred,
      double cur_delta,
      const BagPartArray& bags,
      unsigned long max_depth,
      unsigned long check_depth)
      noexcept;

    static bool
    get_best_feature_(
      unsigned long& best_feature_id,
      double& best_gain,
      double& best_no_delta,
      double& best_yes_delta,
      Generics::TaskRunner* task_runner,
      double top_pred,
      const FeatureRowsMap& feature_rows,
      const FeatureSet& skip_features,
      const SVM<LabelType>* node_svm,
      unsigned long check_depth,
      bool top_eval)
      noexcept;

    static double
    eval_feature_gain_(
      double& no_delta,
      double& yes_delta,
      GainType& gain_calc,
      double add_delta,
      unsigned long feature_id,
      const SVM<LabelType>* feature_svm,
      const SVM<LabelType>* node_svm)
      noexcept;

    static double
    eval_init_delta_(
      double& delta,
      GainType& gain_calc,
      double top_pred,
      const SVM<LabelType>* node_svm)
      noexcept;
  };
}

namespace Vanga
{
  template<typename LabelType, typename GainType>
  TreeLearner<LabelType, GainType>::GainTreeNodeDescrKey::GainTreeNodeDescrKey(
    double gain_val,
    TreeNodeDescr* tree_node_val)
    : gain(gain_val),
      tree_node(ReferenceCounting::add_ref(tree_node_val))
  {}

  template<typename LabelType, typename GainType>
  bool
  TreeLearner<LabelType, GainType>::GainTreeNodeDescrKey::operator<(
    const GainTreeNodeDescrKey& right) const
  {
    return gain < right.gain ||
      (gain == right.gain && tree_node < right.tree_node);
  }
}

#include "TreeLearner.tpp"

#endif /*TREELEARNER_HPP_*/
