#include <list>
#include <vector>
#include <iterator>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <unordered_set>

#include <Generics/Rand.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/BitAlgs.hpp>
#include <Generics/TaskRunner.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/CsvReader.hpp>
#include <Commons/Algs.hpp>
#include <Commons/FileManip.hpp>

#include <Utils/Predictor/DTree/Utils.hpp>
#include <Utils/Predictor/DTree/Gain.hpp>

#include "Application.hpp"

//#define TRACE_OUTPUT

namespace
{
  const char USAGE[] =
    "\nUsage: \n"
    "DTreeTrainer [train|train-add|train-forest|print|predict]\n";

  class Callback:
    public Generics::ActiveObjectCallback,
    public ReferenceCounting::AtomicImpl
  {
  public:
    void report_error(
      Generics::ActiveObjectCallback::Severity severity,
      const String::SubString& description, const char* error_code = "" ) noexcept
    {
      try
      {
        std::cerr << severity << "(" << error_code << "): " <<
          description << std::endl;
      }
      catch (...) {}
    }
  };
}

class Application_::TrainTreeTask:
  public Generics::Task,
  public ReferenceCounting::AtomicImpl
{
public:
  TrainTreeTask(
    TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context* context,
    unsigned long max_iterations,
    unsigned long step_depth,
    unsigned long check_depth,
    const SVMImplArray& bags,
    const SVMImplArray& test_bags,
    unsigned long train_size)
    noexcept;

  virtual void
  execute() noexcept;

  void
  wait() noexcept;

  std::pair<DTree_var, DTreeProp_var>
  result() const noexcept;

protected:
  virtual
  ~TrainTreeTask() noexcept = default;

private:
  const TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context_var context_;
  const unsigned long max_iterations_;
  const unsigned long step_depth_;
  const unsigned long check_depth_;
  SVMImplArray bags_;
  SVMImplArray test_bags_;
  unsigned long train_size_;

  mutable Sync::Condition cond_;
  bool finished_;

  std::pair<DTree_var, DTreeProp_var> result_;
};

// Application_::TrainTreeTask
Application_::TrainTreeTask::TrainTreeTask(
  TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context* context,
  unsigned long max_iterations,
  unsigned long step_depth,
  unsigned long check_depth,
  const SVMImplArray& bags,
  const SVMImplArray& test_bags,
  unsigned long train_size)
  noexcept
  : context_(ReferenceCounting::add_ref(context)),
    max_iterations_(max_iterations),
    step_depth_(step_depth),
    check_depth_(check_depth),
    bags_(bags),
    test_bags_(test_bags),
    train_size_(train_size),
    finished_(false)
{}

void
Application_::TrainTreeTask::execute() noexcept
{
  try
  {
    TreeLearner<PredictedBoolLabel, UseLogLossGain>::LearnContext_var learn_context =
      context_->create_learner(nullptr);

    DTree_var cur_dtree;
    const unsigned long cur_step_depth = Generics::safe_rand(step_depth_) + 1;

    unsigned long best_iter_i = 0;
    double best_test_logloss = 1000000.0;
    DTree_var best_tree;
    DTreeProp_var best_tree_props;

    for(unsigned long iter_i = 0; iter_i < max_iterations_; ++iter_i)
    {
      DTree_var cur_dtree = learn_context->train(
        cur_step_depth,
        check_depth_,
        TreeLearner<PredictedBoolLabel, UseLogLossGain>::LearnContext::FSS_BEST);

      double test_logloss = 0.0;
      for(auto test_bag_it = test_bags_.begin(); test_bag_it != test_bags_.end(); ++test_bag_it)
      {
        test_logloss += Utils::logloss(cur_dtree, test_bag_it->in());
      }

      if(test_logloss < best_test_logloss)
      {
        best_test_logloss = test_logloss;
        best_iter_i = iter_i;
        best_tree = cur_dtree;
        best_tree_props = Application_::fill_dtree_prop_(
          cur_step_depth,
          iter_i,
          bags_,
          train_size_);
      }
    }

    result_ = std::make_pair(best_tree, best_tree_props);

    {
      Sync::ConditionalGuard guard(cond_);
      finished_ = true;
      cond_.signal();
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "TrainTreeTask::execute(): caught eh::Exception: " << ex.what() << std::endl;
    abort();
  }
}

void
Application_::TrainTreeTask::wait() noexcept
{
  Sync::ConditionalGuard guard(cond_);
  while(!finished_)
  {
    guard.wait();
  }
}

std::pair<DTree_var, Application_::DTreeProp_var>
Application_::TrainTreeTask::result() const noexcept
{
  return result_;
}

// Application
template<typename IteratorType>
Predictor_var
Application_::init_reg_predictor(
  IteratorType begin_it,
  IteratorType end_it)
{
  Predictor_var sub_predictor_set = new PredictorSet(
    begin_it,
    end_it,
    PredictorSet::SUM);
  return new LogRegPredictor(sub_predictor_set);
}

Application_::Application_()
  noexcept
{}

Application_::~Application_() noexcept
{}

void
Application_::deep_print_(
  std::ostream& ostr,
  Predictor* predictor,
  const char* prefix,
  const FeatureDictionary* dict,
  double base,
  const SVMImpl* svm)
  const noexcept
{
  PredictorSet_var predictor_set = predictor->as_predictor_set();
  if(predictor_set)
  {
    unsigned long predictor_i = 0;
    for(auto pr_it = predictor_set->predictors().begin();
      pr_it != predictor_set->predictors().end();
      ++pr_it, ++predictor_i)
    {
      ostr << prefix << "Predictor #" << predictor_i << ":" << std::endl;
      deep_print_(
        ostr,
        *pr_it,
        (std::string(prefix) + "  ").c_str(),
        dict,
        base,
        svm);
      ostr << std::endl;
    }
  }
  else // dtree
  {
    DTree_var dtree = predictor->as_dtree();
    assert(dtree.in());
    ostr << dtree->to_string_ext(
      (std::string(prefix) + "  ").c_str(),
      dict,
      base,
      svm);
    ostr << std::endl;
  }
}

void
Application_::main(int& argc, char** argv)
  /*throw(eh::Exception)*/
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::Option<unsigned long> opt_max_features(10);
  Generics::AppUtils::Option<unsigned long> opt_max_top_element(10);
  Generics::AppUtils::Option<unsigned long> opt_depth(5);
  Generics::AppUtils::Option<unsigned long> opt_step_depth(1);
  Generics::AppUtils::Option<unsigned long> opt_check_depth(1);
  Generics::AppUtils::Option<unsigned long> opt_iterations(10);
  Generics::AppUtils::Option<unsigned long> opt_global_iterations(10);
  Generics::AppUtils::Option<unsigned long> opt_super_iterations(10);
  Generics::AppUtils::Option<unsigned long> opt_num_trees(10);
  Generics::AppUtils::Option<unsigned long> opt_threads(1);
  Generics::AppUtils::Option<unsigned long> opt_train_bags_number(0);
  Generics::AppUtils::Option<unsigned long> opt_test_bags_number(3);
  Generics::AppUtils::Option<double> opt_add_tree_coef(1.0);
  Generics::AppUtils::StringOption opt_feature_dictionary;
  Generics::AppUtils::StringOption opt_step_model_out;
  Generics::AppUtils::CheckOption opt_out_of_bag_validate;
  Generics::AppUtils::CheckOption opt_not_add;
  Generics::AppUtils::CheckOption opt_only_add;

  // RAM ~ O(opt_max_top_element ^ opt_depth)
  // RAM ~ 100000 by default
  //
  Generics::AppUtils::Args args(-1);

  args.add(
    Generics::AppUtils::equal_name("help") ||
    Generics::AppUtils::short_name("h"),
    opt_help);
  args.add(
    Generics::AppUtils::equal_name("max-top-element") ||
    Generics::AppUtils::short_name("me"),
    opt_max_top_element);
  args.add(
    Generics::AppUtils::equal_name("depth") ||
    Generics::AppUtils::short_name("d"),
    opt_depth);
  args.add(
    Generics::AppUtils::equal_name("step-depth") ||
    Generics::AppUtils::short_name("sd"),
    opt_step_depth);
  args.add(
    Generics::AppUtils::equal_name("check-depth") ||
    Generics::AppUtils::short_name("cd"),
    opt_check_depth);
  args.add(
    Generics::AppUtils::equal_name("steps") ||
    Generics::AppUtils::short_name("n"),
    opt_iterations);
  args.add(
    Generics::AppUtils::equal_name("gsteps") ||
    Generics::AppUtils::short_name("gn"),
    opt_global_iterations);
  args.add(
    Generics::AppUtils::equal_name("ssteps") ||
    Generics::AppUtils::short_name("sn"),
    opt_super_iterations);
  args.add(
    Generics::AppUtils::equal_name("trees") ||
    Generics::AppUtils::short_name("nt"),
    opt_num_trees);
  args.add(
    Generics::AppUtils::equal_name("dict"),
    opt_feature_dictionary);
  args.add(
    Generics::AppUtils::equal_name("threads") ||
    Generics::AppUtils::short_name("t"),
    opt_threads);
  args.add(
    Generics::AppUtils::equal_name("train-bags"),
    opt_train_bags_number);
  args.add(
    Generics::AppUtils::equal_name("test-bags"),
    opt_test_bags_number);
  args.add(
    Generics::AppUtils::equal_name("out-of-bag"),
    opt_out_of_bag_validate);
  args.add(
    Generics::AppUtils::equal_name("step-model-out"),
    opt_step_model_out);
  args.add(
    Generics::AppUtils::equal_name("not-add"),
    opt_not_add);
  args.add(
    Generics::AppUtils::equal_name("only-add"),
    opt_only_add);
  args.add(
    Generics::AppUtils::equal_name("add-coef"),
    opt_add_tree_coef);

  args.parse(argc - 1, argv + 1);

  const Generics::AppUtils::Args::CommandList& commands = args.commands();

  if(commands.empty() || opt_help.enabled() ||
     *commands.begin() == "help")
  {
    std::cout << USAGE << std::endl;
    return;
  }

  auto command_it = commands.begin();
  std::string command = *command_it;
  ++command_it;

  if(command == "train" || command == "train-add" || command == "train-forest" ||
    command == "train-trees")
  {
    if(command_it == commands.end())
    {
      std::cerr << "result file not defined" << std::endl;
      return;
    }

    const std::string result_file_path = *command_it;

    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "train file not defined" << std::endl;
      return;
    }

    const std::string train_file_path = *command_it;

    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "train file not defined" << std::endl;
      return;
    }

    const std::string test_file_path = *command_it;

    std::ifstream train_file(train_file_path.c_str());
    SVMImpl_var train_svm = SVMImpl::load(train_file);

    std::ifstream test_file(test_file_path.c_str());
    SVMImpl_var test_svm = SVMImpl::load(test_file);

    // div to folds
    

    if(command == "train-trees")
    {
      std::vector<DTree_var> trees;

      if(AdServer::FileManip::file_exists(result_file_path))
      {
        std::ifstream result_file(result_file_path.c_str());
        if(result_file.fail())
        {
          Stream::Error ostr;
          ostr << "can't open model file '" << result_file_path << "'" << std::endl;
          throw Exception(ostr);
        }

        PredictorSet_var predictor_set = PredictorSet::load(result_file);

        for(auto it = predictor_set->predictors().begin(); it != predictor_set->predictors().end(); ++it)
        {
          DTree_var dtree = (*it)->as_dtree();

          if(!dtree)
          {
            Stream::Error ostr;
            ostr << "incorrect file, it contains non dtree model";
            throw Exception(ostr);
          }

          trees.push_back(dtree);
        }

        Predictor_var log_reg_predictor = new LogRegPredictor(predictor_set);

        const double train_logloss = Utils::logloss(log_reg_predictor, train_svm.in());
        const double abs_train_logloss = Utils::absloss(log_reg_predictor, train_svm.in());
        const double ext_test_logloss = Utils::logloss(log_reg_predictor, test_svm.in());
        const double abs_ext_test_logloss = Utils::absloss(log_reg_predictor, test_svm.in());
        std::cout << "loaded model: "
          "train-ll(" << train_svm->size() << ") = " << train_logloss <<
          ", abs-train-ll = " << abs_train_logloss <<
          ", etest-ll(" << test_svm->size() << ") = " << ext_test_logloss <<
          ", abs-etest-ll = " << abs_ext_test_logloss <<
          std::endl;
      }

      std::vector<DTree_var> new_trees;

      train_dtree_set_(
        new_trees,
        trees,
        train_svm,
        test_svm,
        *opt_iterations,
        *opt_step_depth,
        *opt_check_depth,
        *opt_train_bags_number,
        opt_out_of_bag_validate.enabled(),
        !opt_not_add.enabled(),
        opt_only_add.enabled(),
        *opt_add_tree_coef,
        opt_step_model_out->c_str(),
        *opt_threads
        );

      PredictorSet_var predictor_set = new PredictorSet(
        new_trees.begin(),
        new_trees.end(),
        PredictorSet::SUM);
      std::ofstream result_file(result_file_path.c_str());
      predictor_set->save(result_file);
    }
    else if(command == "train-forest")
    {
      std::vector<std::pair<DTree_var, DTreeProp_var> > trees;

      if(AdServer::FileManip::file_exists(result_file_path))
      {
        std::ifstream result_file(result_file_path.c_str());
        if(result_file.fail())
        {
          Stream::Error ostr;
          ostr << "can't open model file '" << result_file_path << "'" << std::endl;
          throw Exception(ostr);
        }

        PredictorSet_var predictor_set = PredictorSet::load(result_file);

        for(auto it = predictor_set->predictors().begin(); it != predictor_set->predictors().end(); ++it)
        {
          DTree_var dtree = (*it)->as_dtree();

          if(!dtree)
          {
            Stream::Error ostr;
            ostr << "incorrect file, it contains non dtree model";
            throw Exception(ostr);
          }

          trees.push_back(std::make_pair(dtree, nullptr));
        }
      }

      train_forest_(
        trees,
        train_svm,
        test_svm,
        *opt_iterations,
        *opt_global_iterations,
        *opt_super_iterations,
        *opt_step_depth,
        *opt_check_depth,
        *opt_num_trees,
        *opt_threads,
        *opt_train_bags_number,
        *opt_test_bags_number);

      {
        std::vector<DTree_var> ltrees;
        for(auto tree_it = trees.begin(); tree_it != trees.end(); ++tree_it)
        {
          ltrees.push_back(tree_it->first);
        }

        PredictorSet_var predictor_set = new PredictorSet(ltrees.begin(), ltrees.end());
        std::ofstream result_file(result_file_path.c_str());
        predictor_set->save(result_file);
      }
    }
    else
    {
      assert(0);
    }
  }
  else if(command == "print")
  {
    if(command_it == commands.end())
    {
      std::cerr << "result file not defined" << std::endl;
      return;
    }

    const std::string result_file_path = *command_it;

    ++command_it;

    SVMImpl_var cover_svm;

    if(command_it != commands.end())
    {
      // cover file
      std::ifstream cover_file(command_it->c_str());
      cover_svm = SVMImpl::load(cover_file);
    }

    FeatureDictionary feature_dictionary;
    if(!opt_feature_dictionary->empty())
    {
      // load feature dictionary
      load_dictionary_(feature_dictionary, opt_feature_dictionary->c_str());
    }

    PredictorSet_var predictor_set;
    std::ifstream result_file(result_file_path.c_str());
    predictor_set = PredictorSet::load(result_file);

    deep_print_(
      std::cout,
      predictor_set,
      "",
      &feature_dictionary,
      0.0,
      cover_svm);

    /*
    std::cout << predictor_set->to_string_ext(
      "",
      &feature_dictionary,
      0.0,
      cover_svm);
    */
  }
  else if(command == "predict")
  {
    if(command_it == commands.end())
    {
      std::cerr << "result file not defined" << std::endl;
      return;
    }

    const std::string result_file_path = *command_it;

    ++command_it;
    if(command_it == commands.end())
    {
      std::cerr << "svm file not defined" << std::endl;
      return;
    }

    const std::string svm_file_path = *command_it;

    std::ifstream result_file(result_file_path.c_str());
    Predictor_var predictor = PredictorLoader::load(result_file);
    predictor = new LogRegPredictor(predictor);

    std::ifstream svm_file(svm_file_path.c_str());

    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(7);

    while(!svm_file.eof())
    {
      PredictedBoolLabel label_value;
      Row_var row = SVMImpl::load_line(svm_file, label_value);
      if(!row)
      {
        break;
      }
      double predicted_value = predictor->predict(row->features);
      std::cout << predicted_value << std::endl;
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

void
Application_::load_dictionary_(
  Vanga::FeatureDictionary& dict,
  const char* file)
{
  std::ifstream in(file);
  if(!in.is_open())
  {
    Stream::Error ostr;
    ostr << "can't open '" << file << "'";
    throw Exception(ostr);
  }

  std::vector<std::string> values;

  while(!in.eof())
  {
    // parse line
    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      continue;
    }

    values.resize(0);
    Commons::CsvReader::parse_line(values, line);

    if(values.size() != 2)
    {
      Stream::Error ostr;
      ostr << "invalid dictionary line '" << line << "'";
      throw Exception(ostr);
    }

    unsigned long feature_id;
    if(!String::StringManip::str_to_int(values[0], feature_id))
    {
      Stream::Error ostr;
      ostr << "invalid feature value '" << values[0] << "'";
      throw Exception(ostr);
    }
    dict[feature_id] = values[1];
  }
}

void
Application_::init_bags_(
  SVMImplArray& bags,
  SVMImpl_var& test_svm,
  SVMImpl_var& train_svm,
  SVMImpl* ext_train_svm,
  SVMImpl* ext_test_svm,
  bool train_bags,
  bool out_of_bag_validate)
{
  static const Fold FOLDS[] = {
    { 1.0, 10 },
    { 0.9, 10 },
    // annealing
    { 0.8, 1 },
    { 0.7, 1 }
  };
  //static const Fold FOLDS[] = { { 0.0, 3 }, { 1.0, 3 }, { 0.9, 10 }, { 0.5, 10 }, { 0.3, 10 }, { 0.1, 10 }, { 0.05, 10 } };

  if(out_of_bag_validate)
  {
    std::pair<SVMImpl_var, SVMImpl_var> sets = ext_train_svm->div(ext_train_svm->size() * 9 / 10);
    train_svm = sets.first;
    test_svm = sets.second;
  }
  else
  {
    train_svm = ReferenceCounting::add_ref(ext_train_svm);
    test_svm = ReferenceCounting::add_ref(ext_test_svm);
  }

  bags.clear();

  if(train_bags > 0)
  {
    fill_bags_(bags, FOLDS, train_bags, train_svm);
  }
  else
  {
    bags.push_back(train_svm->copy());
  }
}

void
Application_::prepare_bags_(
  TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context_var& context,
  SVMImplArray& bags,
  Predictor* predictor)
{
  std::cout << "to prepare bags" << std::endl;

  for(auto bag_it = bags.begin(); bag_it != bags.end(); ++bag_it)
  {
    *bag_it = (*bag_it)->copy(PredictedBoolLabelAddConverter(predictor));
  }
  
  context = TreeLearner<PredictedBoolLabel, UseLogLossGain>::create_context(bags);

  std::cout << "bags prepared" << std::endl;
}

void
Application_::train_dtree_set_(
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
  noexcept
{
  Generics::ActiveObjectCallback_var callback(new Callback());

  Generics::TaskRunner_var task_runner = threads > 1 ?
    new Generics::TaskRunner(
      callback,
      threads,
      10*1024*1024 // stack size
      ) :
    nullptr;

  if(task_runner)
  {
    task_runner->activate_object();
  }

  // prepare bags
  TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context_var context;
  SVMImplArray bags;
  SVMImpl_var test_svm;
  SVMImpl_var train_svm;

  // train
  double prev_logloss = 1000000.0;
  std::vector<DTree_var> cur_dtrees;
  std::vector<DTree_var> prev_step_dtrees;
  std::copy(prev_dtrees.begin(), prev_dtrees.end(), std::back_inserter(cur_dtrees));
  prev_step_dtrees = cur_dtrees;

  for(unsigned long gi = 0; gi < max_global_iterations; ++gi)
  {
    bags.clear();

    init_bags_(
      bags,
      test_svm,
      train_svm,
      ext_train_svm,
      ext_test_svm,
      train_bags,
      out_of_bag_validate);
      
    // try extend existing trees
    // TODO: SVM for node
    int best_index = -1;
    double best_choose_logloss = 1000000.0;
    DTree_var best_dtree;

    if(!only_add_trees)
    {
      int dtree_index = 0;
      for(auto dtree_it = cur_dtrees.begin(); dtree_it != cur_dtrees.end();
        ++dtree_it, ++dtree_index)
      {
        assert(dtree_it->in());

        Predictor_var predictor_set;

        if(!cur_dtrees.empty())
        {
          std::vector<DTree_var> sub_dtrees;
          std::copy(cur_dtrees.begin(), dtree_it, std::back_inserter(sub_dtrees));
          auto next_dtree_it = dtree_it;
          std::copy(++next_dtree_it, cur_dtrees.end(), std::back_inserter(sub_dtrees));

          predictor_set = new PredictorSet(
            sub_dtrees.begin(),
            sub_dtrees.end(),
            PredictorSet::SUM);
        }

        prepare_bags_(
          context,
          bags,
          predictor_set);

        DTree_var modified_dtree = (*dtree_it)->copy();

        const double cur_logloss = train_on_bags_(
          modified_dtree,
          predictor_set,
          context,
          train_svm,
          test_svm,
          nullptr, // ext_test_svm
          1, // max_iterations
          step_depth,
          check_depth,
          false,
          task_runner);

        std::cout << "Modify #" << dtree_index << ": ll = " << cur_logloss << ": " << std::endl <<
          (modified_dtree ? modified_dtree->to_string("  ", nullptr) : std::string("null")) << std::endl;

        if(cur_logloss < best_choose_logloss && modified_dtree.in())
        {
          best_index = dtree_index;
          best_dtree = modified_dtree;
          best_choose_logloss = cur_logloss;
        }
      }
    }

    // try add new tree
    double add_logloss = 10000000.0;
    DTree_var new_dtree;

    if(add_trees || cur_dtrees.empty())
    {
      Predictor_var predictor_set;

      if(!cur_dtrees.empty())
      {
        predictor_set = new PredictorSet(
          cur_dtrees.begin(),
          cur_dtrees.end(),
          PredictorSet::SUM);
      }

      prepare_bags_(context, bags, predictor_set);

      /*
      const double pre_add_train_logloss = Utils::logloss_by_pred(train_svm.in());

      for(auto row_it = train_svm->grouped_rows.begin(); row_it != train_svm->grouped_rows.end(); ++row_it)
      {
        std::cout << ">>> label = " << (*row_it)->label.orig() <<
          ", pred = " << (*row_it)->label.pred <<
          ": " << (*row_it)->rows.size() << std::endl;
      }
      */

      //std::cout << "TO ADD TREE with train-logloss = " << pre_add_train_logloss << std::endl;

      add_logloss = train_on_bags_(
        new_dtree,
        predictor_set,
        context,
        train_svm,
        test_svm,
        nullptr, // ext_test_svm
        1, // max_iterations
        step_depth,
        check_depth,
        false,
        task_runner
        );

      add_logloss *= add_gain_coef;

      /*
      std::cout << "FROM ADD TREE with add-logloss = " << add_logloss << ":";
      if(new_dtree)
      {
        std::cout << new_dtree->to_string("", nullptr);
      }
      else
      {
        std::cout << "null";
      }
      std::cout << std::endl;
      */
    }

    std::cout << "Add: ll = " << add_logloss << std::endl;

    if(add_logloss < best_choose_logloss && new_dtree.in())
    {
      best_index = -1;
      best_dtree = new_dtree;
      best_choose_logloss = add_logloss;
    }

    if(best_index < 0)
    {
      cur_dtrees.push_back(new_dtree);
    }
    else
    {
      cur_dtrees[best_index].swap(best_dtree);
    }

    double test_logloss;

    {
      // eval step result
      Predictor_var predictor_set = new PredictorSet(
        cur_dtrees.begin(),
        cur_dtrees.end(),
        PredictorSet::SUM);

      if(opt_step_model_out[0])
      {
        std::ostringstream step_file_name_ostr;
        step_file_name_ostr << opt_step_model_out << gi << ".dtf";
        std::ofstream step_file(step_file_name_ostr.str());
        predictor_set->save(step_file);
      }

      Predictor_var log_reg_predictor = new LogRegPredictor(predictor_set);

      const double train_logloss = Utils::logloss(log_reg_predictor, train_svm.in());
      test_logloss = Utils::logloss(log_reg_predictor, test_svm.in());
      const double ext_test_logloss = Utils::logloss(log_reg_predictor, ext_test_svm);

      const double abs_test_logloss = Utils::absloss(log_reg_predictor, test_svm.in());
      const double abs_ext_test_logloss = Utils::absloss(log_reg_predictor, ext_test_svm);

      std::ostringstream test_losses_ostr;
      std::ostringstream ext_test_losses_ostr;

      for(auto tree_it = cur_dtrees.begin(); tree_it != cur_dtrees.end(); ++tree_it)
      {
        auto next_tree_it = tree_it;
        ++next_tree_it;
        Predictor_var sub_log_reg_predictor = init_reg_predictor(
          cur_dtrees.begin(),
          next_tree_it);
        const double tree_logloss = Utils::logloss(sub_log_reg_predictor, test_svm.in());
        const double tree_ext_test_logloss = Utils::logloss(sub_log_reg_predictor, ext_test_svm);

        unsigned long node_count = (*tree_it)->node_count();
        test_losses_ostr << (tree_it != cur_dtrees.begin() ? "," : "") << tree_logloss << " (" <<
          node_count << ")";
        ext_test_losses_ostr << (tree_it != cur_dtrees.begin() ? "," : "") << tree_ext_test_logloss;
      }

      std::cout << "[" << gi << "]: "
        "train-ll(" << train_svm->size() << ") = " << train_logloss <<
        ", test-ll(" << test_svm->size() << ") = " << test_logloss <<
        ", abs-test-ll = " << abs_test_logloss <<
        ", etest-ll(" << ext_test_svm->size() << ") = " << ext_test_logloss <<
        ", abs-etest-ll = " << abs_ext_test_logloss;

      if(best_index == -1)
      {
        std::cout << " // added tree #" << (cur_dtrees.size() - 1) << ": " << best_choose_logloss;
      }
      else
      {
        std::cout << " // modified tree #" << best_index << "(" << cur_dtrees.size() << ")";
      }

      std::cout << std::endl <<
        "  tree-test-lls = [" << test_losses_ostr.str() << "]" << std::endl <<
        "  tree-ext-test-lls = [" << ext_test_losses_ostr.str() << "]" << std::endl;
    }

    prev_logloss = test_logloss;
    prev_step_dtrees = cur_dtrees;
  }

  if(task_runner)
  {
    task_runner->deactivate_object();
    task_runner->wait_object();
  }

  new_dtrees = cur_dtrees;
}

double
Application_::train_on_bags_(
  DTree_var& res_tree,
  Predictor* prev_predictor,
  TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context* ext_context,
  SVMImpl* train_svm,
  SVMImpl* test_svm,
  SVMImpl* ext_test_svm,
  unsigned long max_iterations,
  unsigned long step_depth,
  unsigned long check_depth,
  bool print_trace,
  Generics::TaskRunner* task_runner)
{
  TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context_var context =
    ReferenceCounting::add_ref(ext_context);

  TreeLearner<PredictedBoolLabel, UseLogLossGain>::LearnContext_var learn_context =
    context->create_learner(res_tree, task_runner);

  DTree_var cur_dtree; // = res_tree;

  std::cout.setf(std::ios::fixed, std::ios::floatfield);
  std::cout.precision(7);

  unsigned long best_iter_i = 0;
  double best_test_logloss = 1000000.0;
  DTree_var best_dtree;

  //train_svm->dump();
  //std::cout << "==============" << std::endl;

  for(unsigned long iter_i = 0; iter_i < max_iterations; ++iter_i)
  {
    cur_dtree = learn_context->train(
      step_depth,
      check_depth,
      TreeLearner<PredictedBoolLabel, UseLogLossGain>::LearnContext::FSS_BEST);

    Predictor_var log_reg_predictor;

    if(prev_predictor)
    {
      std::vector<Predictor_var> predictors;
      predictors.push_back(ReferenceCounting::add_ref(prev_predictor));
      if(cur_dtree)
      {
        predictors.push_back(cur_dtree);
      }
      Predictor_var predictor_set = new PredictorSet(
        predictors.begin(),
        predictors.end(),
        PredictorSet::SUM);
      log_reg_predictor = new LogRegPredictor(predictor_set);
    }
    else
    {
      log_reg_predictor = new LogRegPredictor(cur_dtree);
    }

    const double train_logloss = Utils::logloss(log_reg_predictor, train_svm);
    const double test_logloss = Utils::logloss(log_reg_predictor, test_svm);
    const double ext_test_logloss = ext_test_svm ?
      Utils::logloss(log_reg_predictor, ext_test_svm) :
      0.0;

    if(test_logloss < best_test_logloss)
    {
      best_test_logloss = test_logloss;
      best_iter_i = iter_i;
      best_dtree = cur_dtree;
    }

    if(print_trace)
    {
      std::cout << "[" << iter_i << "]: "
        "train-loss = " << train_logloss <<
        ", test-loss = " << test_logloss <<
        ", ext-test-loss = " << ext_test_logloss <<
        std::endl;
    }

    //std::cout << cur_dtree->to_string("", nullptr) << std::endl;
  }

  if(print_trace)
  {
    std::cout << best_dtree->to_string("", nullptr) << std::endl;
  }

  res_tree = best_dtree;

  return best_test_logloss;
}

double
Application_::train_(
  DTree_var& res_tree,
  Predictor* prev_predictor,
  SVMImpl* ext_train_svm,
  SVMImpl* ext_test_svm,
  unsigned long max_iterations,
  unsigned long step_depth,
  unsigned long check_depth,
  unsigned long train_bags,
  bool out_of_bag_validate,
  bool print_trace)
{
  static const Fold FOLDS[] = { { 0.0, 3 }, { 1.0, 3 }, { 0.9, 10 }, { 0.5, 10 }, { 0.3, 10 }, { 0.1, 10 }, { 0.05, 10 } };

  // div train to bags
  SVMImplArray bags;
  SVMImpl_var test_svm;
  SVMImpl_var train_svm;

  if(out_of_bag_validate)
  {
    std::pair<SVMImpl_var, SVMImpl_var> sets = ext_train_svm->div(ext_train_svm->size() * 9 / 10);
    train_svm = sets.first;
    test_svm = sets.second;
  }
  else
  {
    train_svm = ReferenceCounting::add_ref(ext_train_svm);
    test_svm = ReferenceCounting::add_ref(ext_test_svm);
  }

  if(train_bags > 0)
  {
    fill_bags_(bags, FOLDS, train_bags, train_svm);
  }
  else
  {
    bags.push_back(train_svm->copy());
  }

  TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context_var context =
    TreeLearner<PredictedBoolLabel, UseLogLossGain>::create_context(bags);

  return train_on_bags_(
    res_tree,
    prev_predictor,
    context,
    train_svm,
    test_svm,
    ext_test_svm,
    max_iterations,
    step_depth,
    check_depth,
    print_trace,
    nullptr // task_runner
    );
}

Application_::DTreeProp_var
Application_::fill_dtree_prop_(
  unsigned long step_depth,
  unsigned long best_iteration,
  const SVMImplArray& bags,
  unsigned long train_size)
{
  DTreeProp_var res = new DTreeProp();

  {
    std::ostringstream ss;
    ss << step_depth;
    res->props["sd"] = ss.str();
  }

  {
    std::ostringstream ss;
    ss << best_iteration;
    res->props["iter"] = ss.str();
  }

  {
    std::ostringstream ss;
    for(auto bag_it = bags.begin(); bag_it != bags.end(); ++bag_it)
    {
      if(bag_it != bags.begin())
      {
        ss << ",";
      }
      ss << (static_cast<double>((*bag_it)->size()) / train_size);
    }
    res->props["bags"] = ss.str();
  }
  
  return res;
}

template<int FOLD_SIZE>
void
Application_::fill_bags_(
  SVMImplArray& bags,
  const Fold (&folds)[FOLD_SIZE],
  unsigned long bag_number,
  SVMImpl* svm)
{
  unsigned long fold_sum = 0;
  for(unsigned long i = 0; i < FOLD_SIZE; ++i)
  {
    fold_sum += folds[i].weight;
  }

  std::cout << "to fill " << bag_number << " bags" << std::endl;

  for(unsigned long bug_i = 0; bug_i < bag_number; ++bug_i)
  {
    // select folds randomly
    unsigned long fold_index = 0;
    unsigned long c_fold = Generics::safe_rand(fold_sum);
    unsigned long cur_fold_sum = 0;

    for(unsigned long i = 0; i < FOLD_SIZE; ++i)
    {
      cur_fold_sum += folds[i].weight;
      if(c_fold < cur_fold_sum)
      {
        fold_index = i;
        break;
      }
    }

    unsigned long part_size = static_cast<unsigned long>(folds[fold_index].portion * svm->size());

    SVMImpl_var part_svm;

    /*
    for(auto row_it = svm->grouped_rows.begin(); row_it != svm->grouped_rows.end(); ++row_it)
    {
      std::cout << "X>> label = " << (*row_it)->label.orig() <<
        ", pred = " << (*row_it)->label.pred <<
        ": " << (*row_it)->rows.size() << std::endl;
    }
    */

    if(part_size > 0)
    {
      part_svm = svm->part(part_size);
    }
    else
    {
      part_svm = ReferenceCounting::add_ref(svm);
    }

    /*
    for(auto row_it = part_svm->grouped_rows.begin(); row_it != part_svm->grouped_rows.end(); ++row_it)
    {
      std::cout << "P>> label = " << (*row_it)->label.orig() <<
        ", pred = " << (*row_it)->label.pred <<
        ": " << (*row_it)->rows.size() << std::endl;
    }

    std::cout << std::endl;
    */

    bags.push_back(part_svm);
  }

  std::cout << "bags filled" << std::endl;
}

/*
void
Application_::fill_bags_(
  SVMImplArray& bags,
  const FoldArray& folds,
  unsigned long bag_number,
  SVMImpl* svm)
{
  unsigned long fold_sum = 0;
  for(auto fold_it = folds.begin(); fold_it != folds.end(); ++fold_it)
  {
    fold_sum += fold_it->weight;
  }

  for(unsigned long bug_i = 0; bug_i < bag_number; ++bug_i)
  {
    // select folds randomly
    unsigned long fold_index = 0;
    unsigned long c_fold = Generics::safe_rand(fold_sum);
    unsigned long cur_fold_sum = 0;

    for(auto fold_it = folds.begin(); fold_it != folds.end(); ++fold_it)
    {
      cur_fold_sum += fold_it->weight;
      if(c_fold < cur_fold_sum)
      {
        fold_index = fold_it - folds.begin();
        break;
      }
    }

    unsigned long part_size = static_cast<unsigned long>(folds[fold_index].portion * svm->size());

    SVMImpl_var part_svm;

    if(part_size > 0)
    {
      part_svm = svm->part(part_size);
    }
    else
    {
      part_svm = ReferenceCounting::add_ref(svm);
    }

    bags.push_back(part_svm);
  }
}
*/

void
Application_::train_sub_forest_(
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
  noexcept
{
  static const Fold FOLDS[] = { { 0.0, 3 }, { 1.0, 3 }, { 0.9, 10 }, { 0.5, 10 }, { 0.3, 10 }, { 0.1, 10 }, { 0.05, 10 } };
  static const Fold TEST_FOLDS[] = { { 0.1, 1 } };

  /*
  const FoldArray folds(FOLDS, FOLDS + sizeof(FOLDS) / sizeof(FOLDS[0]));
  const FoldArray test_folds(TEST_FOLDS, TEST_FOLDS + sizeof(TEST_FOLDS) / sizeof(TEST_FOLDS[0]));
  */
  std::cout.setf(std::ios::fixed, std::ios::floatfield);
  std::cout.precision(7);

  std::vector<std::pair<DTree_var, DTreeProp_var> > best_trees;
  std::vector<std::pair<DTree_var, DTreeProp_var> > trees;

  unsigned long best_iter_i = 0;
  double best_train_logloss = 1000000.0;
  double best_test_logloss = 1000000.0;
  //const bool FILL_BUGS_ONCE = true;
  const unsigned long LOSS_ITERATIONS_CHECK = 9;
  SVMImplArray bags;
  SVMImplArray test_bags;
  SVMImpl_var train_full_bag;
  SVMImpl_var combine_bag;
  std::vector<double> log_losses;

  TreeLearner<PredictedBoolLabel, UseLogLossGain>::Context_var context;

  // fill bugs
  std::pair<SVMImpl_var, SVMImpl_var> sets = train_svm->div(train_svm->size() * 9 / 10);
  train_full_bag = sets.first;
  combine_bag = sets.second; // test_svm
  fill_bags_(bags, FOLDS, train_bugs_number, train_full_bag);
  fill_bags_(test_bags, TEST_FOLDS, test_bugs_number, train_full_bag);

  context = TreeLearner<PredictedBoolLabel, UseLogLossGain>::create_context(bags);

  //std::cout << "to train sub forest (" << max_global_iterations << " iterations)" << std::endl;

  for(unsigned long gi = 0; gi < max_global_iterations; ++gi)
  {
    /*
    if(log_losses.size() >= LOSS_ITERATIONS_CHECK)
    {
      std::cout << "T: l=" << log_losses[log_losses.size() - LOSS_ITERATIONS_CHECK] <<
        ", r=" << log_losses[log_losses.size() - 1] << std::endl;
    }
    */

    if(log_losses.size() >= LOSS_ITERATIONS_CHECK &&
       log_losses[log_losses.size() - LOSS_ITERATIONS_CHECK] -
         log_losses[log_losses.size() - 1] <= 0.000001)
    {
      break;
    }

    unsigned long add_trees = trees.size() < num_trees ?
      num_trees - trees.size() :
      1;

    //std::cout << "to train " << add_trees << " trees" << std::endl;

    std::list<TrainTreeTask_var> tasks;

    for(unsigned long i = 0; i < add_trees; ++i)
    {
      TrainTreeTask_var new_task = new TrainTreeTask(
        context,
        max_iterations,
        step_depth,
        check_depth,
        bags,
        test_bags,
        train_full_bag->size());

      task_runner->enqueue_task(new_task);
      tasks.push_back(new_task);
    }

    //std::cout << "to wait trees" << std::endl;

    for(auto task_it = tasks.begin(); task_it != tasks.end(); ++task_it)
    {
      (*task_it)->wait();
      trees.push_back((*task_it)->result());
    }

    tasks.clear();

    //std::cout << "trained " << trees.size() << " trees" << std::endl;

    /*
    {
      //PredictorSet_var forest = new PredictorSet();
      unsigned long tree_i = 0;
      for(auto it = trees.begin(); it != trees.end(); ++it, ++tree_i)
      {
        const double train_logloss = Utils::logloss(it->first, train_svm);
        const double test_logloss = Utils::logloss(it->first, test_svm);
        std::cout << "tree #" << tree_i << ": "
          "train-loss = " << train_logloss <<
          ", test-loss = " << test_logloss <<
          ", props = (" << (it->second ? it->second->to_string() : std::string()) << ")" <<
          std::endl;

        //forest->add(*it);
      }
    }
    */

    //if(trees.size() > num_trees)
    {
      // drop worse trees
      std::vector<PredArrayHolder_var> preds;

      for(auto it = trees.begin(); it != trees.end(); ++it)
      {
        preds.push_back(it->first->Predictor::predict(combine_bag.in()));
      }

      std::vector<unsigned long> result_indexes;
      select_best_forest_(
        result_indexes,
        preds,
        combine_bag,
        num_trees > 3 ? num_trees - 3 : num_trees - 1);

      std::vector<std::pair<DTree_var, DTreeProp_var> > new_trees;
      for(auto tree_index_it = result_indexes.begin();
        tree_index_it != result_indexes.end();
        ++tree_index_it)
      {
        new_trees.push_back(trees[*tree_index_it]);
      }

      /*
      std::cout << "Best forest: ";
      Algs::print(std::cout, result_indexes.begin(), result_indexes.end());
      std::cout << std::endl;
      */

      trees.swap(new_trees);
    }

    {
      std::vector<DTree_var> ltrees;
      for(auto tree_it = trees.begin(); tree_it != trees.end(); ++tree_it)
      {
        ltrees.push_back(tree_it->first);
      }
      PredictorSet_var forest = new PredictorSet(ltrees.begin(), ltrees.end());
      const double train_logloss = Utils::logloss(forest, train_svm);
      const double train_absloss = Utils::absloss(forest, train_svm);
      const double test_logloss = Utils::logloss(forest, test_svm);
      const double test_absloss = Utils::absloss(forest, test_svm);

      std::cout << "forest #" << gi << ": "
        "train-loss = " << train_logloss <<
        ", test-loss = " << test_logloss <<
        ", train-absloss = " << train_absloss <<
        ", test-absloss = " << test_absloss <<
        std::endl;

      if(test_logloss < best_test_logloss)
      {
        best_train_logloss = train_logloss;
        best_test_logloss = test_logloss;
        best_iter_i = gi;
        best_trees = trees;
        //predictor_set = forest;
      }
    }

    log_losses.push_back(best_test_logloss);
  }

  res_trees = best_trees;
}

void
Application_::train_forest_(
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
  noexcept
{
  Generics::ActiveObjectCallback_var callback(new Callback());

  Generics::TaskRunner_var task_runner = new Generics::TaskRunner(
    callback,
    threads,
    10*1024*1024 // stack size
    );

  task_runner->activate_object();

  double min_logloss = 1000000.0;
  std::vector<std::pair<DTree_var, DTreeProp_var> > best_trees = trees;

  if(!best_trees.empty())
  {
    double base_train_logloss = logloss_(best_trees, train_svm);
    min_logloss = logloss_(best_trees, test_svm);

    std::cout << ">>>>>>>>>>> Base Super forest: "
      "train-loss = " << base_train_logloss <<
      ", test-loss = " << min_logloss << "(" << min_logloss << ")" <<
      std::endl;
  }
  else
  {
    std::cout << ">>>>>>>>>>> No Base Super forest" << std::endl;
  }

  std::vector<std::pair<DTree_var, DTreeProp_var> > cur_trees;

  for(unsigned long super_i = 0; super_i < max_super_iterations; ++super_i)
  {
    std::pair<SVMImpl_var, SVMImpl_var> sets = train_svm->div(train_svm->size() * 9 / 10);
    SVMImpl_var sub_train_svm = sets.first;
    SVMImpl_var sub_test_svm = sets.second;

    std::vector<std::pair<DTree_var, DTreeProp_var> > sub_trees;

    train_sub_forest_(
      sub_trees,
      sub_train_svm,
      sub_test_svm,
      task_runner,
      max_iterations,
      max_global_iterations,
      step_depth,
      check_depth,
      6, // num_trees
      train_bugs_number,
      test_bugs_number);

    std::vector<std::pair<DTree_var, DTreeProp_var> > res_trees;
    std::vector<std::pair<DTree_var, DTreeProp_var> > union_trees;
    union_trees = cur_trees;
    union_trees.insert(union_trees.end(), sub_trees.begin(), sub_trees.end());

    construct_best_forest_(
      res_trees,
      union_trees,
      num_trees,
      sub_test_svm);

    double cur_train_logloss = logloss_(res_trees, train_svm);
    double cur_logloss = logloss_(res_trees, test_svm);
    bool best_iteration = false;

    if(min_logloss > cur_logloss)
    {
      best_trees = res_trees;
      min_logloss = cur_logloss;
      best_iteration = true;
    }

    std::cout << (best_iteration ? "* " : ">>") << ">>>>>>>>> Super forest #" << super_i << ": "
      "train-loss = " << cur_train_logloss <<
      ", test-loss = " << cur_logloss << "(" << min_logloss << ")" <<
      std::endl;

    cur_trees = res_trees;
  }

  task_runner->deactivate_object();
  task_runner->wait_object();

  trees = best_trees;

  // print result
  std::cout << "Best forest(size=" << trees.size() <<
    "): test-loss = " << min_logloss <<
    std::endl;
}

double
Application_::logloss_(
  const std::vector<std::pair<DTree_var, DTreeProp_var> >& trees,
  const SVMImpl* svm)
  noexcept
{
  std::vector<DTree_var> ltrees;
  for(auto tree_it = trees.begin(); tree_it != trees.end(); ++tree_it)
  {
    ltrees.push_back(tree_it->first);
  }

  PredictorSet_var forest = new PredictorSet(ltrees.begin(), ltrees.end());
  const double res_logloss = Utils::logloss(forest, svm);

  return res_logloss;
}

void
Application_::construct_best_forest_(
  std::vector<std::pair<DTree_var, DTreeProp_var> >& res_trees,
  const std::vector<std::pair<DTree_var, DTreeProp_var> >& check_trees,
  unsigned long max_trees,
  const SVMImpl* svm)
  noexcept
{
  std::vector<PredArrayHolder_var> preds;

  for(auto it = check_trees.begin(); it != check_trees.end(); ++it)
  {
    preds.push_back(it->first->Predictor::predict(svm));
  }

  std::vector<unsigned long> result_indexes;
  select_best_forest_(
    result_indexes,
    preds,
    svm,
    max_trees);

  std::vector<std::pair<DTree_var, DTreeProp_var> > new_trees;
  for(auto tree_index_it = result_indexes.begin();
    tree_index_it != result_indexes.end();
    ++tree_index_it)
  {
    new_trees.push_back(check_trees[*tree_index_it]);
  }

  res_trees.swap(new_trees);
}

void
Application_::select_best_forest_(
  std::vector<unsigned long>& indexes,
  const std::vector<PredArrayHolder_var>& preds,
  const SVMImpl* svm,
  unsigned long max_trees)
{
  //std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> > loss_preds;
  //TreeLearner::fill_logloss(loss_preds, preds);

  //uint64_t m = 0xFFFFFFFFFFFFFFFF;
  indexes.clear();

  uint64_t maskmax = 0xFFFFFFFFFFFFFFFF >> (64 - preds.size());
  std::vector<PredArrayHolder_var> variant;
  variant.reserve(preds.size());

  uint64_t min_mask = 0;
  double min_logloss = 1000000.0;
  unsigned long min_trees_num = 100;

  PredArrayHolder_var labels = Utils::labels(svm);

  //std::cout << "maxmask=" << maskmax << std::endl;

  for(uint64_t mask = 1; mask <= maskmax; ++mask)
  {
    // fill variant
    variant.clear();

    auto pred_it = preds.begin();
    unsigned long trees_num = 0;
    for(unsigned long i = 0; pred_it != preds.end(); ++i, ++pred_it)
    {
      if((i > 0 ? mask >> i : mask) & 0x1)
      {
        ++trees_num;
        variant.push_back(*pred_it);
      }
    }

    /*
    std::cout << "mask=" << mask <<
      ", trees_num = " << trees_num <<
      ", max_trees = " << max_trees << std::endl;
    */

    if(trees_num >= max_trees)
    {
      continue;
    }

    double avg_logloss = Utils::avg_logloss(variant, labels);
    //std::cout << "mask=" << mask << ", avg_logloss = " << avg_logloss << std::endl;

    if(avg_logloss < min_logloss ||
       (std::abs(avg_logloss - min_logloss) < 0.000001 && trees_num < min_trees_num))
    {
      min_logloss = avg_logloss;
      min_mask = mask;
      min_trees_num = trees_num;
    }
  }

  auto pred_it = preds.begin();
  for(unsigned long i = 0; pred_it != preds.end(); ++i, ++pred_it)
  {
    if((i > 0 ? min_mask >> i : min_mask) & 0x1)
    {
      indexes.push_back(i);
    }
  }
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


