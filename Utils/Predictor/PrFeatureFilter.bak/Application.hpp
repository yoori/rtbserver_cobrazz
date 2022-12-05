#ifndef UTILS_CTRGENERATOR_APPLICATION_HPP_
#define UTILS_CTRGENERATOR_APPLICATION_HPP_

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

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  struct Row: public ReferenceCounting::AtomicImpl
  {
    typedef std::vector<unsigned long> FeatureArray;

    FeatureArray features;

  protected:
    virtual ~Row() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<Row> Row_var;

  typedef std::deque<Row_var> RowArray;

  typedef std::set<unsigned long> FeatureSet;

  struct SVM: public ReferenceCounting::AtomicImpl
  {
    RowArray labeled_rows;
    RowArray unlabeled_rows;

  protected:
    virtual ~SVM() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<SVM> SVM_var;

  struct TreeNode;

  struct TreeNodeBranching
  {
    TreeNodeBranching(): feature_id(0)
    {}

    unsigned long feature_id;
    ReferenceCounting::SmartPtr<TreeNode> yes_tree;
    ReferenceCounting::SmartPtr<TreeNode> no_tree;
  };

  typedef std::multimap<double, TreeNodeBranching> GainToTreeNodeBranchingMap;

  struct TreeNode: public ReferenceCounting::AtomicCopyImpl
  {
    SVM_var svm;
    AdServer::Commons::Optional<GainToTreeNodeBranchingMap> branching;

  protected:
    virtual ~TreeNode() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<TreeNode> TreeNode_var;

  typedef std::vector<TreeNode_var> TreeNodeArray;

  // feature -> rows
  struct FeatureRows: public ReferenceCounting::AtomicImpl
  {
    RowArray labeled_rows;
    RowArray unlabeled_rows;

  protected:
    virtual ~FeatureRows() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<FeatureRows> FeatureRows_var;

  typedef std::map<unsigned long, FeatureRows_var> FeatureRowsMap;

  //
  struct TreeNodeDescr: public ReferenceCounting::AtomicImpl
  {
    unsigned long feature_id;
    double delta_gain;
    ReferenceCounting::SmartPtr<TreeNodeDescr> yes_tree;
    ReferenceCounting::SmartPtr<TreeNodeDescr> no_tree;
  };

  typedef ReferenceCounting::SmartPtr<TreeNodeDescr> TreeNodeDescr_var;

  struct GainTreeNodeDescrKey
  {
    GainTreeNodeDescrKey(double gain_val, TreeNodeDescr* tree_node_val)
      : gain(gain_val),
        tree_node(ReferenceCounting::add_ref(tree_node_val))
    {}

    bool
    operator<(const GainTreeNodeDescrKey& right) const
    {
      return gain < right.gain ||
        (gain == right.gain && tree_node < right.tree_node);
    }

    double gain;
    TreeNodeDescr_var tree_node;
  };

  typedef std::multiset<GainTreeNodeDescrKey> GainToTreeNodeDescrMap;
  //typedef std::multimap<double, TreeNodeDescr_var> GainToTreeNodeDescrMap;

public:
  Application_() noexcept;

  virtual
  ~Application_() noexcept;

  void
  main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  // for each node we can:
  // eval_gains_(gains, svm, node)
  //   foreach feature not in skip features:
  //     cross node rows and feature rows for eval value.labels, value.total
  //     labels = node.labeled_rows.size, total = labels + node.unlabeled_rows.size
  // split_node_(node, feature_index)
  //

  // eval_gains_
  //   construct gain => TreeNode mapping for SVM (feature_rows should be synced with this svm)
  //
  void
  fill_tree_leafs_branching_(
    TreeNode* node,
    const FeatureRowsMap& feature_rows,
    double pinalty,
    int depth,
    const FeatureSet& skip_features,
    unsigned long top_element_limit)
    noexcept;

  static double
  fill_tree_branching_by_feature_(
    TreeNode_var& yes_node,
    TreeNode_var& no_node,
    const RowArray& feature_labeled_rows,
    const RowArray& node_labeled_rows,
    const RowArray& feature_unlabeled_rows,
    const RowArray& node_unlabeled_rows)
    noexcept;

  void
  fill_tree_branching_(
    GainToTreeNodeBranchingMap& tree_branching,
    const SVM& svm,
    const FeatureRowsMap& feature_rows,
    double gain_add,
    const FeatureSet& skip_features,
    unsigned long top_element_limit)
    noexcept;

  void
  get_tree_branching_(
    GainToTreeNodeDescrMap& result_trees,
    const TreeNode* node,
    size_t limit,
    double base_gain,
    unsigned long cur_depth)
    noexcept;

  void
  trace_tree_branching_(
    std::ostream& out,
    const TreeNode* node,
    double base_gain,
    const char* prefix)
    noexcept;

  static void
  tree_features_(
    FeatureSet& features,
    const TreeNodeDescr* node);

  /*
  void
  trace_tree_branching_(
    std::set<double>& best_gains,
    const TreeNode* node,
    double gain,
    unsigned long limit)
    noexcept;
  */

  void
  trace_gain_(
    std::ostream& out,
    TreeNode* root_node,
    TreeNodeDescr* gain_node,
    double base_gain,
    const char* prefix)
    noexcept;

  // utils
  double
  get_gain_(
    const RowArray& node_feature_labeled_rows,
    const RowArray& node_feature_unlabeled_rows,
    const RowArray& all_feature_labeled_rows,
    const RowArray& all_feature_unlabeled_rows);

  void
  fill_feature_rows_(
    FeatureRowsMap& feature_rows,
    const SVM& svm);

  SVM_var
  load_svm_(std::istream& in);

  static double
  eval_gain_(
    unsigned long value_labeled,
    unsigned long value_unlabeled,
    unsigned long labeled,
    unsigned long unlabeled);

  static std::string
  tree_to_string_(const TreeNodeDescr* node, const char* prefix);

  TreeNode_var
  copy_tree_(const TreeNode* node);
};

typedef Generics::Singleton<Application_> Application;

#endif /*UTILS_CTRGENERATOR_APPLICATION_HPP_*/
