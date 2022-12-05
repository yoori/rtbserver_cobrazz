#include <list>
#include <vector>
#include <iterator>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

#include <Generics/AppUtils.hpp>
#include <Generics/BitAlgs.hpp>
#include <xsd/Utils/CTRGeneratorConfig.hpp>
#include <xsd/Utils/CTRGeneratorDataConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/CsvReader.hpp>
#include <Commons/Algs.hpp>
#include <ProfilingCommons/PlainStorage3/FileWriter.hpp>

//#include "CTRGenerator.hpp"
#include "Application.hpp"

//using namespace AdServer::CampaignSvcs;

//#define TRACE_OUTPUT

const double EPS = 0.00001;
const double DEPTH_PINALTY = 0.0001;

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "PrFeatureFilter\n";

  typedef const String::AsciiStringManip::Char2Category<',', '|'>
    ListSepType;

  class CountIterator:
    public std::iterator<std::output_iterator_tag, void, void, void, void>
  {
  public:
    CountIterator()
      : count_(0)
    {}

    template<typename ValueType>
    CountIterator&
    operator=(const ValueType& /*val*/)
    {
      return *this;
    }

    CountIterator& operator*()
    {
      return *this;
    }

    CountIterator& operator++()
    {
      ++count_;
      return *this;
    }

    CountIterator operator++(int)
    {
      ++count_;
      return *this;
    }

    unsigned long
    count() const
    {
      return count_;
    }

  private:
    unsigned long count_;
  };

  unsigned long
  count_cross_rows(const Application_::RowArray& left, const Application_::RowArray& right)
  {
    CountIterator counter = std::set_symmetric_difference(
      left.begin(),
      left.end(),
      right.begin(),
      right.end(),
      CountIterator());
    return counter.count();
  }

  void
  cross_rows(
    Application_::RowArray& res,
    const Application_::RowArray& left,
    const Application_::RowArray& right)
  {
    Application_::RowArray local_res;
    //local_res.reserve(std::min(left.size(), right.size()));
    std::set_intersection(
      left.begin(),
      left.end(),
      right.begin(),
      right.end(),
      std::back_inserter(local_res));
    //Application_::RowArray in_end(local_res);
    res.swap(local_res);
  }
}

// Application
Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::Option<unsigned long> opt_max_features(10);
  Generics::AppUtils::Option<unsigned long> opt_max_top_element(10);
  Generics::AppUtils::Option<unsigned long> opt_depth(5);

  // RAM ~ O(opt_max_top_element ^ opt_depth)
  // RAM ~ 100000 by default
  //
  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);
  args.add(
    Generics::AppUtils::equal_name("max-features") ||
    Generics::AppUtils::short_name("mf"),
    opt_max_features);
  args.add(
    Generics::AppUtils::equal_name("max-top-element") ||
    Generics::AppUtils::short_name("me"),
    opt_max_top_element);
  args.add(
    Generics::AppUtils::equal_name("depth") ||
    Generics::AppUtils::short_name("d"),
    opt_depth);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  std::string command = *commands.begin();

  if(command == "eval-gains")
  {
    //const size_t DEPTH = 4;

    SVM_var svm = load_svm_(std::cin);
    FeatureRowsMap feature_rows;
    fill_feature_rows_(feature_rows, *svm);

    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(17);

    FeatureSet selected_features;
    unsigned long iteration = 0;

    while(iteration < *opt_max_features && selected_features.size() < *opt_max_features)
    {
      std::cerr << "iteration #" << iteration << std::endl;

      //
      TreeNode_var root_node = new TreeNode();
      root_node->svm = new SVM();
      root_node->svm->labeled_rows = svm->labeled_rows;
      root_node->svm->unlabeled_rows = svm->unlabeled_rows;

      std::cerr << "to fill branching" << std::endl;

      fill_tree_leafs_branching_(
        root_node,
        feature_rows,
        0.0,
        *opt_depth,
        selected_features,
        *opt_max_top_element // top_element_limit
        );

      std::cerr << "branching filled" << std::endl;

      // trace pathes
      //std::cout << std::endl;
      //trace_tree_branching_(std::cout, root_node, 0.0, "");

      /*
      std::set<double> best_gains;
      trace_tree_branching_(best_gains, root_node, 0.0, 10);

      if(!best_gains.empty())
      {
        std::cout << "Best gains in [" << *best_gains.begin() << "," <<
          *best_gains.rbegin() << "]" << std::endl;
      }
      */

      GainToTreeNodeDescrMap best_trees;
      get_tree_branching_(
        best_trees,
        root_node,
        1, // max number of trees
        0.0,
        0 // cur depth
        );

      //std::cout << "best_trees.size() = " << best_trees.size() << std::endl;

      if(!best_trees.empty())
      {
        // collect selected features
        FeatureSet local_selected_features;

        const unsigned long MAX_TREES = 1;
        unsigned long tree_i = 0;
        for(auto tree_it = best_trees.begin();
          tree_it != best_trees.end() && tree_i < MAX_TREES;
          ++tree_it, ++tree_i)
        {
          if(tree_it->gain < -EPS)
          {
            tree_features_(local_selected_features, tree_it->tree_node);
          }

          /*
          std::cout << "Tree #" << (tree_i + 1) << "(" << tree_it->gain << "): " << std::endl <<
            tree_to_string_(tree_it->tree_node, " ") << std::endl;

          trace_gain_(std::cout, root_node, tree_it->tree_node, 0.0, "");
          std::cout << std::endl;
          */
        }

        std::cout << "Iteration #" << iteration << ":" << std::endl;
        for(auto feature_it = local_selected_features.begin();
          feature_it != local_selected_features.end(); ++feature_it)
        {
          std::cout << *feature_it << std::endl;
        }

        std::copy(
          local_selected_features.begin(),
          local_selected_features.end(),
          std::inserter(selected_features, selected_features.begin()));
      }
      else
      {
        break;
      }

      ++iteration;
    }
  }
  else
  {
    Stream::Error ostr;
    ostr << "unknown command '" << command << "', "
      "see help for more info" << std::endl;
    throw Exception(ostr);
  }
}

Application_::SVM_var
Application_::load_svm_(std::istream& in)
{
  using namespace xsd::AdServer;

  unsigned long line_i = 0;
  SVM_var svm(new SVM());
  Row::FeatureArray features;
  features.reserve(10240);

  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    String::StringManip::Splitter<
      String::AsciiStringManip::SepSpace> tokenizer(line);
    String::SubString token;
    bool label = true;
    unsigned long label_value = 0;

    while(tokenizer.get_token(token))
    {
      if(label)
      {
        label = false;
        if(!String::StringManip::str_to_int(token, label_value))
        {
          Stream::Error ostr;
          ostr << "can't parse label '" << token << "'";
          throw Exception(ostr);
        }
      }
      else
      {
        String::SubString::SizeType pos = token.find(':');
        String::SubString feature_value_str = token.substr(0, pos);
        unsigned long feature_value = 0;
        if(!String::StringManip::str_to_int(feature_value_str, feature_value))
        {
          Stream::Error ostr;
          ostr << "can't parse feature '" << feature_value_str << "'";
          throw Exception(ostr);
        }

        features.push_back(feature_value);
      }
    }

    std::sort(features.begin(), features.end());

    // create Row
    Row_var new_row(new Row());
    new_row->features.swap(features);
    if(label_value > 0)
    {
      svm->labeled_rows.push_back(new_row);
    }
    else
    {
      svm->unlabeled_rows.push_back(new_row);
    }

    ++line_i;

    if(line_i % 100000 == 0)
    {
      std::cerr << "loaded " << line_i << " lines" << std::endl;
    }
  }

  std::cerr << "loading finished (" << line_i << " lines)" << std::endl;

  std::sort(svm->labeled_rows.begin(), svm->labeled_rows.end());
  std::sort(svm->unlabeled_rows.begin(), svm->unlabeled_rows.end());

  std::cerr << "rows sorted" << std::endl;

  return svm;
}

void
Application_::fill_feature_rows_(
  FeatureRowsMap& feature_rows,
  const SVM& svm)
{
  std::cerr << "to fill labeled feature rows" << std::endl;

  unsigned long row_i = 0;
  for(RowArray::const_iterator row_it = svm.labeled_rows.begin();
      row_it != svm.labeled_rows.end();
      ++row_it, ++row_i)
  {
    for(auto feature_it = (*row_it)->features.begin();
      feature_it != (*row_it)->features.end(); ++feature_it)
    {
      FeatureRows_var& fr = feature_rows[*feature_it];
      if(!fr.in())
      {
        fr = new FeatureRows();
      }
      fr->labeled_rows.push_back(*row_it);
    }

    if(row_i % 100000 == 0)
    {
      std::cerr << row_i << " rows processed" << std::endl;
    }
  }

  std::cerr << "labeled feature rows filled (" << feature_rows.size() << ")" << std::endl;
  std::cerr << "to fill unlabeled feature rows" << std::endl;

  row_i = 0;
  for(RowArray::const_iterator row_it = svm.unlabeled_rows.begin();
      row_it != svm.unlabeled_rows.end();
      ++row_it, ++row_i)
  {
    for(auto feature_it = (*row_it)->features.begin();
      feature_it != (*row_it)->features.end(); ++feature_it)
    {
      FeatureRows_var& fr = feature_rows[*feature_it];
      if(!fr.in())
      {
        fr = new FeatureRows();
      }
      fr->unlabeled_rows.push_back(*row_it);
    }

    if(row_i % 100000 == 0)
    {
      std::cerr << row_i << " rows processed" << std::endl;
    }
  }

  std::cerr << "unlabeled feature rows filled (" << feature_rows.size() << ")" << std::endl;
}

void
Application_::tree_features_(
  FeatureSet& features,
  const TreeNodeDescr* node)
{
  if(node)
  {
    features.insert(node->feature_id);

    if(node->yes_tree)
    {
      tree_features_(features, node->yes_tree);
    }

    if(node->no_tree)
    {
      tree_features_(features, node->no_tree);
    }
  }
}

std::string
Application_::tree_to_string_(const TreeNodeDescr* node, const char* prefix)
{
  if(!node)
  {
    return "";
  }

  std::ostringstream ostr;
  ostr << prefix << node->feature_id << " (delta gain=" << node->delta_gain << ")" << std::endl;

  if(node->yes_tree)
  {
    ostr << prefix << "  yes =>" << std::endl <<
      tree_to_string_(node->yes_tree, (std::string(prefix) + "    ").c_str());
  }

  if(node->no_tree)
  {
    ostr << prefix << "  no =>" << std::endl <<
      tree_to_string_(node->no_tree, (std::string(prefix) + "    ").c_str());
  }

  return ostr.str();
}

/*
TreeNode_var
Application_::copy_tree_(const TreeNode* node)
{
  TreeNode_var new_node = new TreeNode(*node);
  if(node->yes_tree)
  {
    new_node->yes_tree = copy_tree_(node->yes_tree);
  }

  if(node->no_tree)
  {
    new_node->no_tree = copy_tree_(node->no_tree);
  }

  return new_node;
}
*/
 
double
Application_::eval_gain_(
  unsigned long yes_value_labeled,
  unsigned long yes_value_unlabeled,
  unsigned long labeled,
  unsigned long unlabeled)
{
  const unsigned long total = labeled + unlabeled;

  ///*
  // eval old logloss
  const double p_old = total > 0 ?
    std::min(std::max(static_cast<double>(labeled) / total, EPS), 1.0 - EPS) :
    0.0;

  const double old_logloss = - (
    labeled > 0 ? unlabeled * std::log(1.0 - p_old) + labeled * std::log(p_old) :
    0.0);
  //*/

  // eval new logloss
  unsigned long yes_value_total = yes_value_labeled + yes_value_unlabeled;
  // no_value_unlabeled : unlabeled - yes_value_unlabeled
  unsigned long no_value_unlabeled = total - yes_value_total - (labeled - yes_value_labeled);
  unsigned long no_value_labeled = labeled - yes_value_labeled;

  assert(yes_value_labeled + yes_value_unlabeled + no_value_labeled + no_value_unlabeled == labeled + unlabeled);

  const double p1 = no_value_unlabeled + no_value_labeled > 0 ?
    std::min(std::max(static_cast<double>(no_value_labeled) / (no_value_unlabeled + no_value_labeled), EPS), 1.0 - EPS) :
    0.0;
  const double p2 = yes_value_unlabeled + yes_value_labeled > 0 ?
    std::min(std::max(static_cast<double>(yes_value_labeled) / (yes_value_unlabeled + yes_value_labeled), EPS), 1.0 - EPS) :
    0.0;

# ifdef TRACE_OUTPUT
  std::cerr << "yes_value_labeled = " << yes_value_labeled <<
    ", yes_value_unlabeled = " << yes_value_unlabeled <<
    ", no_value_labeled = " << no_value_labeled <<
    ", no_value_unlabeled = " << no_value_unlabeled <<
    ", labeled = " << labeled <<
    ", unlabeled = " << unlabeled << std::endl;
# endif

  const double no_part = (
    no_value_unlabeled > 0 ?
      static_cast<double>(no_value_unlabeled) * std::log(1.0 - p1) +
      static_cast<double>(no_value_labeled) * std::log(p1) :
    0.0);
  const double yes_part = (
    yes_value_labeled > 0 ?
      static_cast<double>(yes_value_unlabeled) * std::log(1.0 - p2) +
      static_cast<double>(yes_value_labeled) * std::log(p2) :
    0.0);
  const double new_logloss = - (no_part + yes_part);

# ifdef TRACE_OUTPUT
  std::cerr << "no_part = " << no_part << ", p1 = " << p1 <<
    ", yes_part = " << yes_part << ", p2 = " << p2 <<
    ", old_logloss = " << old_logloss <<
    ", new_logloss = " << new_logloss <<
    std::endl;
# endif

  // new logloss += gain, new logloss = 
  return new_logloss - old_logloss; // negative, new logloss = old logloss + gain 
}

void
Application_::trace_gain_(
  std::ostream& out,
  TreeNode* root_node,
  TreeNodeDescr* gain_node,
  double base_gain,
  const char* prefix)
  noexcept
{
  if(gain_node)
  {
    const unsigned long feature_id = gain_node->feature_id;

    unsigned long pos = 0;
    for(auto gain_it = root_node->branching->begin();
        gain_it != root_node->branching->end(); ++gain_it, ++pos)
    {
      if(gain_it->second.feature_id == feature_id)
      {
        out << prefix << "feature #" << feature_id <<
          ": sum gain = " << (base_gain + gain_it->first) <<
          ", branching delta gain = " << gain_it->first <<
          ", pos = " << pos << std::endl;

        if(gain_node->yes_tree)
        {
          out << prefix << "  yes:" << std::endl;
          trace_gain_(
            out,
            gain_it->second.yes_tree,
            gain_node->yes_tree,
            base_gain + gain_it->first,
            (std::string(prefix) + "    ").c_str());
        }

        if(gain_node->no_tree)
        {
          out << prefix << "  no:" << std::endl;
          trace_gain_(
            out,
            gain_it->second.no_tree,
            gain_node->no_tree,
            base_gain + gain_it->first,
            (std::string(prefix) + "    ").c_str());
        }

        break;
      }
    }
  }
}

void
Application_::trace_tree_branching_(
  std::ostream& out,
  const TreeNode* node,
  double base_gain,
  const char* prefix)
  noexcept
{
  if(node && node->branching.present())
  {
    for(auto branch_it = node->branching->begin();
      branch_it != node->branching->end(); ++branch_it)
    {
      out << prefix << (base_gain + branch_it->first) << ": " << branch_it->second.feature_id << std::endl;
      out << prefix << "  yes:" << std::endl;
      trace_tree_branching_(
        out,
        branch_it->second.yes_tree,
        base_gain + branch_it->first,
        (std::string(prefix) + "    ").c_str());
      out << prefix << "  no:" << std::endl;
      trace_tree_branching_(
        out,
        branch_it->second.no_tree,
        base_gain + branch_it->first,
        (std::string(prefix) + "    ").c_str());
    }
  }
}

void
Application_::get_tree_branching_(
  GainToTreeNodeDescrMap& result_trees,
  const TreeNode* node,
  size_t limit,
  double base_gain,
  unsigned long cur_depth)
  noexcept
{
  //std::cerr << "node: " << node->branching.present() << std::endl;

  if(node && node->branching.present())
  {
    //std::cout << "D0(IN), node->branching->size() = " << node->branching->size() << std::endl;

    for(auto branch_it = node->branching->begin();
      branch_it != node->branching->end(); ++branch_it)
    {
      GainToTreeNodeDescrMap yes_best_trees;

      // get best trees in left
      get_tree_branching_(
        yes_best_trees,
        branch_it->second.yes_tree,
        limit,
        0.0, ///*base_gain + */branch_it->first // base gain + delta gain
        cur_depth + 1
        );

      GainToTreeNodeDescrMap no_best_trees;

      // get best trees in right
      get_tree_branching_(
        no_best_trees,
        branch_it->second.no_tree,
        limit,
        0.0, ///*base_gain + */branch_it->first // base gain + delta gain
        cur_depth + 1
        );

      auto null_yes_it = yes_best_trees.insert(GainTreeNodeDescrKey(0.0, nullptr));
      auto null_no_it = no_best_trees.insert(GainTreeNodeDescrKey(0.0, nullptr));

      /*
      if(cur_depth == 0)
      {
        std::cout << "yes_best_trees.size() = " << yes_best_trees.size() <<
          ", no_best_trees.size() = " << no_best_trees.size() << std::endl;
      }
      */

      for(auto yes_it = yes_best_trees.begin(); yes_it != yes_best_trees.end(); ++yes_it)
      {
        for(auto no_it = no_best_trees.begin(); no_it != no_best_trees.end(); ++no_it)
        {
          double union_gain = base_gain + branch_it->first + yes_it->gain + no_it->gain;

          if((result_trees.empty() ||
              result_trees.size() < limit ||
              union_gain < result_trees.rbegin()->gain) &&
            (yes_it != null_yes_it || no_it != null_no_it))
          {
            TreeNodeDescr_var union_tree = new TreeNodeDescr();
            union_tree->feature_id = branch_it->second.feature_id;
            union_tree->delta_gain = /*base_gain + */branch_it->first; // union_gain;
            union_tree->yes_tree = yes_it->tree_node;
            union_tree->no_tree = no_it->tree_node;
            //std::cout << "D1(IN), result_trees.size() = " << result_trees.size() << std::endl;
            result_trees.insert(GainTreeNodeDescrKey(union_gain, union_tree));
            //std::cout << "D1(MID), result_trees.size() = " << result_trees.size() << std::endl;
            if(result_trees.size() > limit)
            {
              result_trees.erase(--result_trees.end());
            }
            //std::cout << "D1(OUT), result_trees.size() = " << result_trees.size() << std::endl;
          }
        }
      }
    }

    //std::cout << "D0(OUT), result_trees.size() = " << result_trees.size() <<
    //  ", node->branching->size() = " << node->branching->size() << std::endl;
  }

  // try add empty tree
  if(result_trees.empty() ||
    result_trees.size() < limit ||
    base_gain < result_trees.rbegin()->gain)
  {
    // push stop node
    //std::cout << "D2(IN), result_trees.size() = " << result_trees.size() << std::endl;
    result_trees.insert(GainTreeNodeDescrKey(base_gain, nullptr));
    //std::cout << "D2(MID), result_trees.size() = " << result_trees.size() << std::endl;
    if(result_trees.size() > limit)
    {
      result_trees.erase(--result_trees.end());
    }
    //std::cout << "D2(OUT), result_trees.size() = " << result_trees.size() << std::endl;
  }
}

double
Application_::fill_tree_branching_by_feature_(
  TreeNode_var& yes_node,
  TreeNode_var& no_node,
  const RowArray& feature_labeled_rows,
  const RowArray& node_labeled_rows,
  const RowArray& feature_unlabeled_rows,
  const RowArray& node_unlabeled_rows)
  noexcept
{
  // yes
  // labeled: feature_labeled_rows ^ node_labeled_rows
  // unlabeled: feature_unlabeled_rows ^ node_unlabeled_rows
  RowArray yes_feature_labeled_cross_rows;
  cross_rows(yes_feature_labeled_cross_rows, feature_labeled_rows, node_labeled_rows);
  RowArray yes_feature_unlabeled_cross_rows;
  cross_rows(yes_feature_unlabeled_cross_rows, feature_unlabeled_rows, node_unlabeled_rows);
  yes_node = new TreeNode();
  yes_node->svm = new SVM();
  yes_node->svm->labeled_rows.swap(yes_feature_labeled_cross_rows);
  yes_node->svm->unlabeled_rows.swap(yes_feature_unlabeled_cross_rows);

  // no
  // labeled: node_labeled_rows / feature_labeled_rows
  // unlabeled: node_unlabeled_rows / feature_unlabeled_rows
  RowArray no_feature_labeled_cross_rows;
  std::set_difference(
    node_labeled_rows.begin(),
    node_labeled_rows.end(),
    feature_labeled_rows.begin(),
    feature_labeled_rows.end(),
    std::back_inserter(no_feature_labeled_cross_rows));
  RowArray no_feature_unlabeled_cross_rows;
  std::set_difference(
    node_unlabeled_rows.begin(),
    node_unlabeled_rows.end(),
    feature_unlabeled_rows.begin(),
    feature_unlabeled_rows.end(),
    std::back_inserter(no_feature_unlabeled_cross_rows));
  no_node = new TreeNode();
  no_node->svm = new SVM();
  no_node->svm->labeled_rows.swap(no_feature_labeled_cross_rows);
  no_node->svm->unlabeled_rows.swap(no_feature_unlabeled_cross_rows);

# ifdef TRACE_OUTPUT
  std::cerr << "node_labeled_rows: ";
  Algs::print(std::cerr, node_labeled_rows.begin(), node_labeled_rows.end());
  std::cerr << std::endl << "node_unlabeled_rows: ";
  Algs::print(std::cerr, node_unlabeled_rows.begin(), node_unlabeled_rows.end());
  std::cerr << std::endl;
# endif

  return eval_gain_(
    yes_node->svm->labeled_rows.size(), // value_labeled
    yes_node->svm->unlabeled_rows.size(), // value_unlabeled
    node_labeled_rows.size(), // labeled
    node_unlabeled_rows.size() // unlabeled
    );
}

double
Application_::find_best_gain_by_feature_(
  const RowArray& feature_labeled_rows,
  const RowArray& node_labeled_rows,
  const RowArray& feature_unlabeled_rows,
  const RowArray& node_unlabeled_rows)
  noexcept
{
  // yes
  // labeled: feature_labeled_rows ^ node_labeled_rows
  // unlabeled: feature_unlabeled_rows ^ node_unlabeled_rows
  RowArray yes_feature_labeled_cross_rows;
  cross_rows(yes_feature_labeled_cross_rows, feature_labeled_rows, node_labeled_rows);
  RowArray yes_feature_unlabeled_cross_rows;
  cross_rows(yes_feature_unlabeled_cross_rows, feature_unlabeled_rows, node_unlabeled_rows);
  yes_node = new TreeNode();
  yes_node->svm = new SVM();
  yes_node->svm->labeled_rows.swap(yes_feature_labeled_cross_rows);
  yes_node->svm->unlabeled_rows.swap(yes_feature_unlabeled_cross_rows);

  // no
  // labeled: node_labeled_rows / feature_labeled_rows
  // unlabeled: node_unlabeled_rows / feature_unlabeled_rows
  RowArray no_feature_labeled_cross_rows;
  std::set_difference(
    node_labeled_rows.begin(),
    node_labeled_rows.end(),
    feature_labeled_rows.begin(),
    feature_labeled_rows.end(),
    std::back_inserter(no_feature_labeled_cross_rows));
  RowArray no_feature_unlabeled_cross_rows;
  std::set_difference(
    node_unlabeled_rows.begin(),
    node_unlabeled_rows.end(),
    feature_unlabeled_rows.begin(),
    feature_unlabeled_rows.end(),
    std::back_inserter(no_feature_unlabeled_cross_rows));
  no_node = new TreeNode();
  no_node->svm = new SVM();
  no_node->svm->labeled_rows.swap(no_feature_labeled_cross_rows);
  no_node->svm->unlabeled_rows.swap(no_feature_unlabeled_cross_rows);

# ifdef TRACE_OUTPUT
  std::cerr << "node_labeled_rows: ";
  Algs::print(std::cerr, node_labeled_rows.begin(), node_labeled_rows.end());
  std::cerr << std::endl << "node_unlabeled_rows: ";
  Algs::print(std::cerr, node_unlabeled_rows.begin(), node_unlabeled_rows.end());
  std::cerr << std::endl;
# endif

  return eval_gain_(
    yes_node->svm->labeled_rows.size(), // value_labeled
    yes_node->svm->unlabeled_rows.size(), // value_unlabeled
    node_labeled_rows.size(), // labeled
    node_unlabeled_rows.size() // unlabeled
    );
}

double
Application_::find_best_gain_(
  TreeNode* node,
  const FeatureRowsMap& feature_rows,
  double pinalty,
  int depth,
  const FeatureSet& skip_features,
  unsigned long top_element_limit)
  noexcept
{
  for(auto feature_it = feature_rows.begin();
    feature_it != feature_rows.end(); ++feature_it)
  {
    if(skip_features.find(feature_it->first) == skip_features.end())
    {
#     ifdef TRACE_OUTPUT
      std::cerr << "find_best_gain_(" << depth << "): feature #" << feature_it->first << std::endl;
#     endif

      double gain = fill_tree_branching_by_feature_(
        yes_node,
        no_node,
        feature_it->second->labeled_rows, // feature labeled rows
        svm.labeled_rows, // node labeled rows
        feature_it->second->unlabeled_rows, // feature unlabeled rows
        svm.unlabeled_rows // node unlabeled rows
        );

      TreeNodeBranching tree_node_branching;
      tree_node_branching.feature_id = feature_it->first;
      tree_node_branching.yes_tree = yes_node;
      tree_node_branching.no_tree = no_node;

      tree_branching.insert(std::make_pair(gain + gain_add, tree_node_branching));

      if(tree_branching.size() > top_element_limit)
      {
        tree_branching.erase(--tree_branching.end());
      }
    }
  }
}

void
Application_::fill_tree_leafs_branching_(
  TreeNode* node,
  const FeatureRowsMap& feature_rows,
  double pinalty,
  int depth,
  const FeatureSet& skip_features,
  unsigned long top_element_limit)
  noexcept
{
  GainToTreeNodeBranchingMap node_branching;
  fill_tree_branching_(
    node_branching,
    *(node->svm),
    feature_rows,
    pinalty,
    skip_features,
    top_element_limit);

  if(depth - 1 > 0)
  {
    unsigned long node_i = 0;
    for(auto node_it = node_branching.begin(); node_it != node_branching.end();
        ++node_it, ++node_i)
    {
      FeatureSet local_skip_features = skip_features;
      local_skip_features.insert(node_it->second.feature_id);

      fill_tree_leafs_branching_(
        node_it->second.yes_tree,
        feature_rows,
        pinalty + DEPTH_PINALTY,
        depth - 1,
        local_skip_features,
        top_element_limit);
      fill_tree_leafs_branching_(
        node_it->second.no_tree,
        feature_rows,
        pinalty + DEPTH_PINALTY,
        depth - 1,
        local_skip_features,
        top_element_limit);

      if(depth == 0)
      {
        std::cerr << "filled " << node_i << "/" << node_branching.size() << " root nodes" << std::endl;
      }
    }
  }

  node->branching.fill().swap(node_branching);
}

void
Application_::fill_tree_branching_(
  GainToTreeNodeBranchingMap& tree_branching,
  const SVM& svm,
  const FeatureRowsMap& feature_rows,
  double gain_add,
  const FeatureSet& skip_features,
  unsigned long top_element_limit)
  noexcept
{
  for(auto feature_it = feature_rows.begin();
    feature_it != feature_rows.end(); ++feature_it)
  {
    if(skip_features.find(feature_it->first) == skip_features.end())
    {
#     ifdef TRACE_OUTPUT
      std::cerr << "feature #" << feature_it->first << std::endl;
#     endif

      TreeNode_var yes_node;
      TreeNode_var no_node;

      double gain = fill_tree_branching_by_feature_(
        yes_node,
        no_node,
        feature_it->second->labeled_rows, // feature labeled rows
        svm.labeled_rows, // node labeled rows
        feature_it->second->unlabeled_rows, // feature unlabeled rows
        svm.unlabeled_rows // node unlabeled rows
        );

      TreeNodeBranching tree_node_branching;
      tree_node_branching.feature_id = feature_it->first;
      tree_node_branching.yes_tree = yes_node;
      tree_node_branching.no_tree = no_node;

      tree_branching.insert(std::make_pair(gain + gain_add, tree_node_branching));

      if(tree_branching.size() > top_element_limit)
      {
        tree_branching.erase(--tree_branching.end());
      }
    }
  }
}

double
Application_::get_gain_(
  const RowArray& node_feature_labeled_rows,
  const RowArray& node_feature_unlabeled_rows,
  const RowArray& all_feature_labeled_rows,
  const RowArray& all_feature_unlabeled_rows)
{
  return eval_gain_(
    count_cross_rows(node_feature_labeled_rows, all_feature_labeled_rows), // value_labeled
    count_cross_rows(node_feature_unlabeled_rows, all_feature_unlabeled_rows), // value_unlabeled
    all_feature_labeled_rows.size(), // labeled
    all_feature_unlabeled_rows.size() // unlabeled
    );
}

// main
int
main(int argc, char** argv)
{
  Application_* app = 0;

  try
  {
    app = &Application::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  assert(app);

  try
  {
    app->main(argc, argv);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}


