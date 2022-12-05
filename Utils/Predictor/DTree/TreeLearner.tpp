namespace Vanga
{
  namespace
  {
    const double EPS = 0.0000001;
    const double LABEL_DOUBLE_MIN = EPS;
    const double LABEL_DOUBLE_MAX = 1 - EPS;

    const double LABEL_MIN = EPS;
    const double LABEL_MAX = 1.0 - EPS;

    const double GAIN_SHARE_PENALTY = 0.0001;
    const double GAIN_ABS_PENALTY = 0.0001;
  }

  struct GetBestFeatureResult: public ReferenceCounting::AtomicImpl
  {
    struct BestChoose
    {
      BestChoose()
        : feature_id(0),
          no_delta(0.0),
          yes_delta(0.0),
          gain(0.0)
      {}

      BestChoose(
        unsigned long feature_id_val,
        double no_delta_val,
        double yes_delta_val,
        double gain_val)
        : feature_id(feature_id_val),
          no_delta(no_delta_val),
          yes_delta(yes_delta_val),
          gain(gain_val)
      {}

      unsigned long feature_id;
      double no_delta;
      double yes_delta;
      double gain;
    };

    typedef std::deque<BestChoose> BestChooseArray;

    GetBestFeatureResult()
      : best_gain(-EPS),
        tasks_in_progress(0)
    {}

    void
    inc()
    {
      Sync::ConditionalGuard guard(cond_);
      ++tasks_in_progress;
    }

    void
    set(unsigned long feature_id, double gain, double no_delta, double yes_delta)
    {
      Sync::ConditionalGuard guard(cond_);

      if(gain < best_gain + EPS)
      {
        if(gain <= best_gain - EPS)
        {
          best_features.clear();
        }

        best_features.push_back(
          BestChoose(feature_id, no_delta, yes_delta, gain));
        best_gain = gain;
      }

      assert(tasks_in_progress > 0);

      if(--tasks_in_progress == 0 || tasks_in_progress % 100 == 0)
      {
        cond_.signal();
      }
    }

    void
    wait(unsigned long all_tasks)
    {
      unsigned long prev_tasks_in_progress = 0;
      Sync::ConditionalGuard guard(cond_);
      while(tasks_in_progress > 0)
      {
        guard.wait();

        if(tasks_in_progress % 100 == 0 && tasks_in_progress != tasks_in_progress)
        {
          prev_tasks_in_progress = tasks_in_progress;
          std::cout << "processed " << (all_tasks - tasks_in_progress) << "/" << tasks_in_progress << " features" << std::endl;
        }
      }
    }

    bool
    get_result(BestChoose& res) const
    {
      if(!best_features.empty())
      {
        /*
        std::cout << "select between features: ";
        for(auto it = best_features.begin(); it != best_features.end(); ++it)
        {
          std::cout << "[#" << it->feature_id << ":" << it->gain << "]";
        }
        std::cout << std::endl;
        */

        unsigned long i = Generics::safe_rand(best_features.size());
        res = best_features[i];
        return true;
      }

      return false;
    }

    Sync::Condition cond_;
    BestChooseArray best_features;
    double best_gain;
    unsigned long tasks_in_progress;

  protected:
    virtual ~GetBestFeatureResult() noexcept = default;
  };

  typedef ReferenceCounting::SmartPtr<GetBestFeatureResult>
    GetBestFeatureResult_var;

  template<typename LearnerType>
  struct GetBestFeatureParams: public ReferenceCounting::AtomicImpl
  {
    double top_pred;
    const typename LearnerType::FeatureRowsMap* feature_rows;
    const FeatureSet* skip_features;
    typename LearnerType::ConstSVM_var node_svm;
    unsigned long check_depth;
    bool top_eval;
  };

  template<typename LearnerType>
  class GetBestFeatureTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    typedef ReferenceCounting::SmartPtr<GetBestFeatureParams<LearnerType> >
      GetBestFeatureParams_var;

    GetBestFeatureTask(
      GetBestFeatureResult* result,
      GetBestFeatureParams<LearnerType>* params,
      unsigned long feature_id,
      typename LearnerType::SVMT* feature_svm)
      noexcept;

    virtual void
    execute() noexcept;

  protected:
    virtual
    ~GetBestFeatureTask() noexcept = default;

  private:
    const GetBestFeatureResult_var result_;
    const GetBestFeatureParams_var params_;
    const unsigned long feature_id_;
    typename LearnerType::SVMT* feature_svm_;
  };

  // GetBestFeatureTask impl
  template<typename LearnerType>
  GetBestFeatureTask<LearnerType>::GetBestFeatureTask(
    GetBestFeatureResult* result,
    GetBestFeatureParams<LearnerType>* params,
    unsigned long feature_id,
    typename LearnerType::SVMT* feature_svm)
    noexcept
    : result_(ReferenceCounting::add_ref(result)),
      params_(ReferenceCounting::add_ref(params)),
      feature_id_(feature_id),
      feature_svm_(feature_svm)
  {
    result->inc();
  }

  template<typename LearnerType>
  void
  GetBestFeatureTask<LearnerType>::execute() noexcept
  {
    double gain;
    double no_delta;
    double yes_delta;
    typename LearnerType::GainT gain_calc;

    /*
    {
      Stream::Error ostr;
      ostr << "============================" << std::endl;
      std::cout << ostr.str() << std::endl;
    }
    */

    LearnerType::check_feature_(
      gain,
      no_delta,
      yes_delta,
      gain_calc,
      params_->top_pred,
      feature_id_,
      *(params_->feature_rows),
      *(params_->skip_features),
      feature_svm_,
      params_->node_svm,
      params_->check_depth);

    /*
    {
      Stream::Error ostr;
      ostr << "GAIN FOR #" << feature_id_ << ": " << gain << std::endl;
      std::cout << ostr.str() << std::endl;
    }
    */

    result_->set(feature_id_, gain, no_delta, yes_delta);
  }

  template<typename LabelType, typename GainType>
  struct TreeLearner<LabelType, GainType>::LearnTreeHolder:
    public ReferenceCounting::AtomicImpl
  {
  public:

  public:
    unsigned long tree_id;
    unsigned long feature_id;
    double delta_gain;
    double delta_prob;

    ReferenceCounting::SmartPtr<LearnTreeHolder> yes_tree;
    ReferenceCounting::SmartPtr<LearnTreeHolder> no_tree;

    BagPartArray bags;

  protected:
    virtual ~LearnTreeHolder() noexcept
    {}
  };

  const double NULL_GAIN = 0.00003;

  namespace
  {
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

    template<typename ContainerType>
    unsigned long
    binsearch_count_cross_rows(const ContainerType& left, const ContainerType& right)
    {
      // fetch left with lookup to right
      unsigned long count = 0;
      auto right_begin_it = right.begin();
      for(auto it = left.begin(); it != left.end(); ++it)
      {
        right_begin_it = std::lower_bound(right_begin_it, right.end(), *it);

        if(right_begin_it == right.end())
        {
          break;
        }
        else if(*right_begin_it == *it)
        {
          ++count;
        }
      }

      return count;
    }

    template<typename ContainerType>
    unsigned long
    count_cross_rows(const ContainerType& left, const ContainerType& right)
    {
      // left and right sorted
      size_t left_size = left.size();
      size_t right_size = right.size();

      if(right_size > left_size * 12)
      {
        // fetch left with lookup to right
        return binsearch_count_cross_rows(left, right);
      }
      else if(left_size > right_size * 12)
      {
        // fetch right with lookup to left
        return binsearch_count_cross_rows(right, left);
      }

      // fetch intersect
      CountIterator counter = std::set_intersection(
        left.begin(),
        left.end(),
        right.begin(),
        right.end(),
        CountIterator());
      return counter.count();
    }
  }

  // GetBestLearnTreeHolderProcessor
  template<typename LabelType, typename GainType>
  struct GetBestLearnTreeHolderProcessor
  {
    struct ContextType
    {
      ContextType(
        double base_prob_val,
        const FeatureSet& skip_features_val,
        double depth_pinalty_val,
        unsigned long top_element_limit_val,
        unsigned long depth_limit_val)
        noexcept
        : base_prob(base_prob_val),
          skip_features(skip_features_val),
          depth_pinalty(depth_pinalty_val),
          top_element_limit(top_element_limit_val),
          depth_limit(depth_limit_val)
      {}

      const double base_prob;
      const FeatureSet& skip_features;

      const double depth_pinalty;
      const unsigned long top_element_limit;
      const unsigned long depth_limit;
    };

    typedef typename TreeLearner<LabelType, GainType>::LearnTreeHolder_var ResultType;

    static typename TreeLearner<LabelType, GainType>::LearnTreeHolder_var
    aggregate(
      unsigned long feature_id,
      double gain,
      double delta,
      const typename TreeLearner<LabelType, GainType>::BagPartArray&, //bags,
      typename TreeLearner<LabelType, GainType>::LearnTreeHolder* yes_arg,
      typename TreeLearner<LabelType, GainType>::LearnTreeHolder* no_arg)
    {
      typename TreeLearner<LabelType, GainType>::LearnTreeHolder_var union_tree =
        new typename TreeLearner<LabelType, GainType>::LearnTreeHolder();
      union_tree->feature_id = feature_id;
      union_tree->delta_gain = gain;
      union_tree->delta_prob = delta; //TreeLearner<LabelType, GainType>::delta_prob(bags);
      union_tree->yes_tree = ReferenceCounting::add_ref(yes_arg);
      union_tree->no_tree = ReferenceCounting::add_ref(no_arg);
      return union_tree;
    }

    static typename TreeLearner<LabelType, GainType>::LearnTreeHolder_var
    null_result(
      double delta,
      const typename TreeLearner<LabelType, GainType>::BagPartArray& //bags
      )
    {
      // stop node
      typename TreeLearner<LabelType, GainType>::LearnTreeHolder_var stop_tree =
        new typename TreeLearner<LabelType, GainType>::LearnTreeHolder();
      stop_tree->feature_id = 0;
      stop_tree->delta_gain = 0.0;
      stop_tree->delta_prob = delta; //TreeLearner<LabelType, GainType>::delta_prob(bags);
      //std::cerr << "stop_tree->delta_prob = " << stop_tree->delta_prob << std::endl;
      return stop_tree;
    }
  };

  // TreeLearner::TreeNodeDescr
  /*
  void
  TreeNodeDescr::collect_avg_covers_(
    AvgCoverMap& avg_covers_map,
    unsigned long all_rows) const
  {
    if(!no_tree.in() && !yes_tree.in())
    {
      // leaf
      double avg_val = avg();
      auto it = avg_covers_map.find(avg_val);
      if(it != avg_covers_map.end())
      {
        it->second += accurate_cover(all_rows);
      }
      else
      {
        avg_covers_map.insert(std::make_pair(avg_val, accurate_cover(all_rows)));
      }
    }
    else if(!no_tree.in()) // !no_tree.in() && yes_tree.in()
    {
      // pseudo no leaf
      double no_avg = labeled + unlabeled - (yes_tree->labeled + yes_tree->unlabeled) > 0 ?
        static_cast<double>(labeled - yes_tree->labeled) /
          (labeled + unlabeled - (yes_tree->labeled + yes_tree->unlabeled)) :
        0.0;
      double no_cover = accurate_cover(all_rows) - yes_tree->accurate_cover(all_rows);

      auto it = avg_covers_map.find(no_avg);
      if(it != avg_covers_map.end())
      {
        it->second += no_cover;
      }
      else
      {
        avg_covers_map.insert(std::make_pair(no_avg, no_cover));
      }
    }
    else if(!yes_tree.in()) // no_tree.in() && !yes_tree.in()
    {
      // pseudo yes leaf
      double yes_avg = labeled + unlabeled - (no_tree->labeled + no_tree->unlabeled) > 0 ?
        static_cast<double>(labeled - no_tree->labeled) /
          (labeled + unlabeled - (no_tree->labeled + no_tree->unlabeled)) :
        0.0;
      double yes_cover = accurate_cover(all_rows) - no_tree->accurate_cover(all_rows);

      auto it = avg_covers_map.find(yes_avg);
      if(it != avg_covers_map.end())
      {
        it->second += yes_cover;
      }
      else
      {
        avg_covers_map.insert(std::make_pair(yes_avg, yes_cover));
      }
    }

    if(no_tree.in())
    {
      no_tree->collect_avg_covers_(avg_covers_map, all_rows);
    }

    if(yes_tree.in())
    {
      yes_tree->collect_avg_covers_(avg_covers_map, all_rows);
    }
  }

  void
  TreeNodeDescr::avg_covers(
    AvgCoverMap& avg_covers_map,
    unsigned long all_rows) const
    noexcept
  {
    collect_avg_covers_(avg_covers_map, all_rows);

    double prev_sum = 0.0;
    for(auto it = avg_covers_map.rbegin(); it != avg_covers_map.rend(); ++it)
    {
      double cover = it->second;
      it->second += prev_sum;
      prev_sum += cover;
    }
  }
  */

  void
  TreeNodeDescr::collect_feature_gains_(
    FeatureToGainMap& feature_to_gains)
    const
  {
    if(feature_id)
    {
      auto feature_it = feature_to_gains.find(feature_id);
      if(feature_it != feature_to_gains.end())
      {
        feature_it->second += delta_gain;
      }
      else
      {
        feature_to_gains.insert(std::make_pair(feature_id, delta_gain));
      }
    }

    if(yes_tree)
    {
      yes_tree->collect_feature_gains_(feature_to_gains);
    }

    if(no_tree)
    {
      no_tree->collect_feature_gains_(feature_to_gains);
    }
  }

  void
  TreeNodeDescr::gain_features(
    GainToFeatureMap& gain_to_features)
    const noexcept
  {
    FeatureToGainMap feature_to_gains;
    collect_feature_gains_(feature_to_gains);

    for(auto it = feature_to_gains.begin(); it != feature_to_gains.end(); ++it)
    {
      gain_to_features.insert(std::make_pair(it->second, it->first));
    }
  }

  /*
  TreeNodeDescr_var
  TreeNodeDescr::sub_tree(
    double min_avg,
    unsigned long min_node_cover) const noexcept
  {
    if(this->labeled + this->unlabeled < min_node_cover)
    {
      return nullptr;
    }

    TreeNodeDescr_var res_yes_tree;
    TreeNodeDescr_var res_no_tree;

    if(yes_tree)
    {
      res_yes_tree = yes_tree->sub_tree(min_avg, min_node_cover);
      if(res_yes_tree && res_yes_tree->labeled + res_yes_tree->unlabeled < min_node_cover)
      {
        res_yes_tree = nullptr;
      }
    }

    if(no_tree)
    {
      res_no_tree = no_tree->sub_tree(min_avg, min_node_cover);
      if(res_no_tree && res_no_tree->labeled + res_no_tree->unlabeled < min_node_cover)
      {
        res_no_tree = nullptr;
      }
    }

    if(res_yes_tree.in() || res_no_tree.in() || this->avg() >= min_avg - 0.000001)
    {
      TreeNodeDescr_var res_tree = new TreeNodeDescr();
      res_tree->feature_id = feature_id;
      res_tree->delta_gain = delta_gain;
      res_tree->labeled = labeled;
      res_tree->unlabeled = unlabeled;
      if(res_yes_tree)
      {
        res_tree->yes_tree = res_yes_tree;
      }

      if(res_no_tree)
      {
        res_tree->no_tree = res_no_tree;
      }

      return res_tree;
    }

    return nullptr;
  }
  */

  /*
  std::string
  TreeNodeDescr::to_string(
    const char* prefix,
    const FeatureDictionary* dict,
    const unsigned long* all_rows) const
    noexcept
  {
    std::ostringstream ostr;
    ostr << prefix << feature_id;
    if(dict)
    {
      auto dict_it = dict->find(feature_id);
      if(dict_it != dict->end())
      {
        ostr << " [" << dict_it->second << "]";
      }
    }

    ostr << " (delta gain=" << delta_gain;
      //", avg=" << avg();

    if(all_rows)
    {
      ostr << ", cover=" << cover(*all_rows);
    }

    ostr << ", l=" << this->labeled << ", t=" << (this->labeled + this->unlabeled) << ")" << std::endl;

    if(yes_tree)
    {
      ostr << prefix << "  yes (avg=" << yes_tree->avg();
      if(all_rows)
      {
        ostr << ", cover=" << yes_tree->cover(*all_rows);
      }
      ostr << ", l=" << yes_tree->labeled << ", t=" << (yes_tree->labeled + yes_tree->unlabeled) <<
        ") =>" << std::endl <<
        yes_tree->to_string((std::string(prefix) + ">   ").c_str(), dict, all_rows);
    }

    if(no_tree)
    {
      ostr << prefix << "  no (avg=" << no_tree->avg();
      if(all_rows)
      {
        ostr << ", cover=" << no_tree->cover(*all_rows);
      }
      ostr << ", l=" << no_tree->labeled << ", t=" << (no_tree->labeled + no_tree->unlabeled) <<
        ") =>" << std::endl <<
        no_tree->to_string((std::string(prefix) + ">   ").c_str(), dict, all_rows);
    }

    return ostr.str();
  }

  std::string
  TreeNodeDescr::to_xml(
    const char* prefix,
    const FeatureDictionary* dict,
    const unsigned long* all_rows) const
    noexcept
  {
    std::ostringstream ostr;
    ostr << prefix << "<node hash=\"" << feature_id << "\"";
    if(dict)
    {
      auto dict_it = dict->find(feature_id);
      if(dict_it != dict->end())
      {
        ostr << " id=\"" << dict_it->second << "\"";
      }
    }

    ostr << " delta_gain=\"" << delta_gain << "\" avg=\"" << avg() << "\"";

    if(all_rows)
    {
      ostr << " cover=\"" << cover(*all_rows) << "\"";
    }

    ostr << " labels=\"" << this->labeled << "\" rows=\"" << (this->labeled + this->unlabeled) << "\">" << std::endl;

    if(yes_tree)
    {
      ostr << prefix << "  <yes avg=\"" << yes_tree->avg() << "\"";
      if(all_rows)
      {
        ostr << " cover=\"" << yes_tree->cover(*all_rows) << "\"";
      }
      ostr << " labels=\"" << yes_tree->labeled << "\" rows=\"" << (yes_tree->labeled + yes_tree->unlabeled) <<
        "\">" << std::endl <<
        yes_tree->to_xml((std::string(prefix) + "   ").c_str(), dict, all_rows);
      ostr << prefix << "  </yes>" << std::endl;
    }

    if(no_tree)
    {
      ostr << prefix << "  <no avg=\"" << no_tree->avg() << "\"";
      if(all_rows)
      {
        ostr << " cover=\"" << no_tree->cover(*all_rows) << "\"";
      }
      ostr << " labels=\"" << no_tree->labeled << "\" rows=\"" << (no_tree->labeled + no_tree->unlabeled) <<
        "\">" << std::endl <<
        no_tree->to_xml((std::string(prefix) + "  ").c_str(), dict, all_rows);
      ostr << prefix << "  </no>" << std::endl;
    }

    ostr << prefix << "</node>" << std::endl;

    return ostr.str();
  }
  */

  void
  TreeNodeDescr::features(FeatureSet& features) const
  {
    features.insert(feature_id);

    if(yes_tree)
    {
      yes_tree->features(features);
    }

    if(no_tree)
    {
      no_tree->features(features);
    }
  }

  // TreeLearner::Context
  template<typename LabelType, typename GainType>
  TreeLearner<LabelType, GainType>::Context::Context(const BagPartArray& bag_parts)
    : bag_parts_(bag_parts)
  {}

  template<typename LabelType, typename GainType>
  TreeLearner<LabelType, GainType>::Context::~Context() noexcept
  {}

  template<typename LabelType, typename GainType>
  typename TreeLearner<LabelType, GainType>::LearnContext_var
  TreeLearner<LabelType, GainType>::Context::create_learner(
    const DTree* base_tree,
    Generics::TaskRunner* task_runner)
  {
    return new LearnContext(
      task_runner,
      bag_parts_,
      base_tree);
  }

  // TreeLearner::LearnContext
  template<typename LabelType, typename GainType>
  TreeLearner<LabelType, GainType>::LearnContext::LearnContext(
    Generics::TaskRunner* task_runner,
    const BagPartArray& bags,
    const DTree* base_tree)
    : task_runner_(ReferenceCounting::add_ref(task_runner)),
      max_tree_id_(0)
  {
    cur_tree_ = fill_learn_tree_(
      max_tree_id_,
      bags,
      base_tree);
  }

  template<typename LabelType, typename GainType>
  TreeLearner<LabelType, GainType>::LearnContext::~LearnContext() noexcept
  {}

  template<typename LabelType, typename GainType>
  DTree_var
  TreeLearner<LabelType, GainType>::LearnContext::train(
    unsigned long max_add_depth,
    unsigned long check_depth,
    FeatureSelectionStrategy feature_selection_strategy)
  {
    // collect stop nodes &
    std::multimap<double, TreeReplace> nodes;

    fetch_nodes_(
      nodes,
      cur_tree_,
      0.0,
      max_add_depth,
      check_depth,
      feature_selection_strategy);

    // apply node change
    if(!nodes.empty())
    {
      const TreeReplace& tree_replace = nodes.begin()->second;
      LearnTreeHolder_var old_node = tree_replace.old_tree;
      LearnTreeHolder_var new_node = tree_replace.new_tree;
      //new_node->delta_prob = 0;

      /*
      std::cout << "tree_replace: tree_id = " << tree_replace.old_tree->tree_id <<
        ", no = " << new_node->no_tree <<
        ", yes = " << new_node->yes_tree << std::endl;
      */
      convert_abs_prob_to_delta_(
        new_node,
        tree_replace.old_tree_prob_base,
        old_node->bags);

      // correct abs prob to delta prob
      old_node->tree_id = new_node->tree_id;
      old_node->feature_id = new_node->feature_id;
      old_node->delta_gain = new_node->delta_gain;
      //old_node->delta_prob = tree_replace.old_tree_prob_base;
      old_node->yes_tree = new_node->yes_tree;
      old_node->no_tree = new_node->no_tree;
      old_node->bags.swap(new_node->bags);
    }

    return fill_dtree_(cur_tree_);
  }

  template<typename LabelType, typename GainType>
  void
  TreeLearner<LabelType, GainType>::LearnContext::convert_abs_prob_to_delta_(
    LearnTreeHolder* tree,
    double base_prob,
    const BagPartArray& bags)
  {
    tree->tree_id = ++max_tree_id_;
    //tree->delta_prob = tree->delta_prob - base_prob;

    if(tree->yes_tree || tree->no_tree)
    {
      assert(tree->yes_tree && tree->no_tree);

      BagPartArray yes_bag_parts;
      BagPartArray no_bag_parts;

      for(auto bag_it = bags.begin(); bag_it != bags.end(); ++bag_it)
      {
        SVM_var yes_svm;
        SVM_var no_svm;

        div_rows_(
          yes_svm,
          no_svm,
          tree->feature_id,
          (*bag_it)->svm,
          (*bag_it)->bag_holder->feature_rows);

        BagPart_var yes_bag_part = new BagPart();
        yes_bag_part->bag_holder = (*bag_it)->bag_holder;
        yes_bag_part->svm = yes_svm;
        yes_bag_parts.push_back(yes_bag_part);

        BagPart_var no_bag_part = new BagPart();
        no_bag_part->bag_holder = (*bag_it)->bag_holder;
        no_bag_part->svm = no_svm;
        no_bag_parts.push_back(no_bag_part);
      }

      convert_abs_prob_to_delta_(
        tree->yes_tree,
        base_prob + tree->delta_prob,
        yes_bag_parts);

      convert_abs_prob_to_delta_(
        tree->no_tree,
        base_prob + tree->delta_prob,
        no_bag_parts);
    }
    else
    {
      tree->bags = bags;
    }
  }

  template<typename LabelType, typename GainType>
  DTree_var
  TreeLearner<LabelType, GainType>::LearnContext::fill_dtree_(LearnTreeHolder* learn_tree_holder)
  {
    if(learn_tree_holder)
    {
      DTree_var res = new DTree();
      res->tree_id = learn_tree_holder->tree_id;
      res->feature_id = learn_tree_holder->feature_id;
      res->delta_prob = learn_tree_holder->delta_prob;

      if(learn_tree_holder->yes_tree)
      {
        res->yes_tree = fill_dtree_(learn_tree_holder->yes_tree);
      }

      if(learn_tree_holder->no_tree)
      {
        res->no_tree = fill_dtree_(learn_tree_holder->no_tree);
      }

      return res;
    }

    return nullptr;
  }

  template<typename LabelType, typename GainType>
  void
  TreeLearner<LabelType, GainType>::LearnContext::fetch_nodes_(
    std::multimap<double, TreeReplace>& nodes,
    LearnTreeHolder* tree,
    double prob,
    unsigned long max_add_depth,
    unsigned long check_depth,
    FeatureSelectionStrategy feature_selection_strategy
    )
  {
    if(tree->yes_tree || tree->no_tree)
    {
      assert(tree->no_tree && tree->yes_tree);

      fetch_nodes_(
        nodes,
        tree->yes_tree,
        prob + tree->delta_prob,
        max_add_depth,
        check_depth,
        feature_selection_strategy);
      fetch_nodes_(
        nodes,
        tree->no_tree,
        prob + tree->delta_prob,
        max_add_depth,
        check_depth,
        feature_selection_strategy);
    }
    else
    {
      // stop node - best dig
      typename GetBestLearnTreeHolderProcessor<LabelType, GainType>::ContextType
        gain_search_context(
          prob,
          FeatureSet(), // skip_features
          0.0, // pinalty
          10, // top_element_limit
          max_add_depth
          );

      /*
      std::cout << "to best_dig_: tree_id = " << tree->tree_id <<
        ", delta_prob = " << tree->delta_prob <<
        ", max_add_depth = " << max_add_depth <<
        std::endl;
      */

      LearnTreeHolder_var delta_tree = best_dig_(
        task_runner_,
        gain_search_context,
        GetBestLearnTreeHolderProcessor<LabelType, GainType>(),
        prob,
        tree->delta_prob,
        tree->bags,
        max_add_depth,
        check_depth);

      if(delta_tree && delta_tree->delta_gain < -EPS)
      {
        //std::cout << "from best_dig_: delta_tree->delta_prob = " <<
        //  delta_tree->delta_prob << std::endl;

        TreeReplace tree_replace;
        tree_replace.old_tree_prob_base = prob;
        tree_replace.old_tree = ReferenceCounting::add_ref(tree);
        tree_replace.new_tree = delta_tree;
        nodes.insert(std::make_pair(delta_tree->delta_gain, tree_replace));
      }
    }
  }

  template<typename LabelType, typename GainType>
  typename TreeLearner<LabelType, GainType>::LearnTreeHolder_var
  TreeLearner<LabelType, GainType>::LearnContext::fill_learn_tree_(
    unsigned long& max_tree_id,
    const BagPartArray& bags,
    const DTree* tree)
  {
    LearnTreeHolder_var res = new LearnTreeHolder();

    if(tree)
    {
      res->tree_id = tree->tree_id;
      res->feature_id = tree->feature_id;
      res->delta_prob = tree->delta_prob;

      if(tree->yes_tree || tree->no_tree)
      {
        assert(tree->yes_tree && tree->no_tree);

        BagPartArray yes_bag_parts;
        BagPartArray no_bag_parts;

        TreeLearner::div_bags_(yes_bag_parts, no_bag_parts, bags, tree->feature_id);

        res->yes_tree = fill_learn_tree_(
          max_tree_id,
          yes_bag_parts,
          tree->yes_tree);

        res->no_tree = fill_learn_tree_(
          max_tree_id,
          no_bag_parts,
          tree->no_tree);
      }
      else
      {
        res->bags = bags;
      }
    }
    else
    {
      // init root node
      unsigned long bag_i = Generics::safe_rand(bags.size());
      const BagPart& bag_part = *bags[bag_i];

      res->tree_id = 1;
      res->feature_id = 0;

      /*
      std::cout << "---- eval_init_delta_ ----" << std::endl;
      bag_part.svm->print_labels(std::cout);
      std::cout << std::endl;
      */
      GainType gain_calc;
      eval_init_delta_(res->delta_prob, gain_calc, 0.0, bag_part.svm);
      res->bags = bags;
    }

    max_tree_id = std::max(max_tree_id, res->tree_id);

    //std::cerr << "delta_prob(" << res->tree_id << "): " << res->delta_prob << std::endl;

    return res;
  }

  template<typename LabelType, typename GainType>
  double
  TreeLearner<LabelType, GainType>::eval_init_delta_(
    double& delta,
    GainType& gain_calc,
    double top_pred,
    const SVM<LabelType>* node_svm)
    noexcept
  {
    gain_calc.start_count(top_pred);

    for(auto node_group_it = node_svm->grouped_rows.begin();
      node_group_it != node_svm->grouped_rows.end(); ++node_group_it)
    {
      gain_calc.add_count((*node_group_it)->label, false, (*node_group_it)->rows.size());
    }

    gain_calc.start_eval();

    for(auto node_group_it = node_svm->grouped_rows.begin();
      node_group_it != node_svm->grouped_rows.end(); ++node_group_it)
    {
      gain_calc.add_eval((*node_group_it)->label, false, (*node_group_it)->rows.size());
    }

    delta = gain_calc.unfeatured_delta();

    return gain_calc.result();
  }

  // TreeLearner
  template<typename LabelType, typename GainType>
  template<typename ProcessorType>
  typename ProcessorType::ResultType
  TreeLearner<LabelType, GainType>::best_dig_(
    Generics::TaskRunner* task_runner,
    const typename ProcessorType::ContextType& context,
    const ProcessorType& processor,
    double base_pred,
    double cur_delta,
    const BagPartArray& bags,
    unsigned long max_depth,
    unsigned long check_depth)
    noexcept
  {
    // select bag randomly
    unsigned long bag_i = Generics::safe_rand(bags.size());
    const BagPart& bag_part = *bags[bag_i];

    unsigned long best_feature_id = 0;
    double best_gain = 0.0;
    double best_no_delta = 0.0;
    double best_yes_delta = 0.0;

    if(max_depth > 0 &&
      get_best_feature_(
        best_feature_id,
        best_gain,
        best_no_delta,
        best_yes_delta,
        task_runner,
        base_pred + cur_delta,
        bag_part.bag_holder->feature_rows,
        FeatureSet(),
        bag_part.svm,
        check_depth,
        true
        ))
    {
      /*
      std::cout << "get_best_feature_: best_feature_id = " << best_feature_id <<
        ", best_no_delta = " << best_no_delta <<
        ", best_yes_delta = " << best_yes_delta <<
        std::endl;
      */
      // divide rows to yes/no arrays
      BagPartArray yes_bag_parts;
      BagPartArray no_bag_parts;

      TreeLearner::div_bags_(yes_bag_parts, no_bag_parts, bags, best_feature_id);

      /*
      std::cout << "feature (" << best_feature_id <<
        "): base_pred = " << base_pred <<
        ", cur_delta = " << cur_delta <<
        ", best_yes_delta = " << best_yes_delta <<
        ", best_no_delta = " << best_no_delta << std::endl;
      */
      auto yes_res = max_depth > 1 ?
        best_dig_(
          task_runner,
          context,
          processor,
          base_pred + cur_delta,
          best_yes_delta,
          yes_bag_parts,
          max_depth - 1,
          check_depth) :
        processor.null_result(best_yes_delta, yes_bag_parts);

      auto no_res = max_depth > 1 ?
        best_dig_(
          task_runner,
          context,
          processor,
          base_pred + cur_delta,
          best_no_delta,
          no_bag_parts,
          max_depth - 1,
          check_depth) :
        processor.null_result(best_no_delta, no_bag_parts);

      return processor.aggregate(
        best_feature_id,
        best_gain,
        cur_delta,
        bags,
        yes_res,
        no_res);
    }

    return processor.null_result(cur_delta, bags);
  }

  template<typename LabelType, typename GainType>
  bool
  TreeLearner<LabelType, GainType>::get_best_feature_(
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
    noexcept
  {
    GainType gain_calc;

    typedef TreeLearner<LabelType, GainType> ThisType;

    best_feature_id = 0;
    best_gain = eval_init_delta_(best_no_delta, gain_calc, top_pred, node_svm);
    best_yes_delta = best_no_delta;

    if(check_depth == 0)
    {
      // stop tracing
      return false;
    }

    // find best features (by gain)
    unsigned long feature_i = 1;

    ReferenceCounting::SmartPtr<GetBestFeatureResult> result =
      new GetBestFeatureResult();

    result->inc();
    result->set(0, best_gain, best_no_delta, best_yes_delta);

    ReferenceCounting::SmartPtr<GetBestFeatureParams<ThisType> >
      params = new GetBestFeatureParams<ThisType>();
    params->top_pred = top_pred;
    params->feature_rows = &feature_rows;
    params->skip_features = &skip_features;
    params->node_svm = ReferenceCounting::add_ref(node_svm);
    params->check_depth = check_depth;
    params->top_eval = top_eval;

    /*
    if(top_eval)
    {
      std::cout << "=============" << std::endl;
    }
    */

    for(auto feature_it = feature_rows.begin();
      feature_it != feature_rows.end();
      ++feature_it, ++feature_i)
    {
      if(top_eval && !task_runner && feature_i % 100 == 0)
      {
        std::cout << "processed " << feature_i << "/" << feature_rows.size() << " features" << std::endl;
      }

      if(skip_features.find(feature_it->first) == skip_features.end())
      {
        if(task_runner)
        {
          Generics::Task_var task = new GetBestFeatureTask<ThisType>(
            result,
            params,
            feature_it->first,
            feature_it->second);

          task_runner->enqueue_task(task);
        }
        else
        {
          double gain;
          double no_delta;
          double yes_delta;

          result->inc();

          check_feature_(
            gain,
            no_delta,
            yes_delta,
            gain_calc,
            top_pred,
            feature_it->first,
            feature_rows,
            skip_features,
            feature_it->second,
            node_svm,
            check_depth);

          gain += (-gain * GAIN_SHARE_PENALTY) + GAIN_ABS_PENALTY;

          /*
          if(top_eval)
          {
            Stream::Error ostr;
            ostr << "GAIN FOR #" << feature_it->first << ": " << gain << std::endl;
            std::cout << ostr.str() << std::endl;
          }
          else
          {
            Stream::Error ostr;
            ostr << "SUB GAIN FOR #" << feature_it->first << ": " << gain << std::endl;
            std::cout << ostr.str() << std::endl;
          }
          */

          result->set(feature_it->first, gain, no_delta, yes_delta);
        }
      }
    }

    result->wait(feature_rows.size());

    GetBestFeatureResult::BestChoose best_choose;
    if(result->get_result(best_choose))
    {
      best_feature_id = best_choose.feature_id;
      best_gain = best_choose.gain;
      best_no_delta = best_choose.no_delta;
      best_yes_delta = best_choose.yes_delta;
    }

    /*
    if(top_eval)
    {
      Stream::Error ostr;
      ostr << "BGAIN FOR #" << best_feature_id << ": " << best_gain << std::endl;
      std::cout << ostr.str() << std::endl;
    }
    else
    {
      Stream::Error ostr;
      ostr << "SUB BGAIN FOR #" << best_feature_id << ": " << best_gain << std::endl;
      std::cout << ostr.str() << std::endl;
    }
    */

    if(top_eval)
    {
      if(best_feature_id)
      {
        auto best_feature_it = feature_rows.find(best_feature_id);
        assert(best_feature_it != feature_rows.end());

        eval_feature_gain_(
          best_no_delta,
          best_yes_delta,
          gain_calc,
          top_pred,
          best_feature_id,
          best_feature_it->second,
          node_svm);
      }
    }

    if(best_gain < NULL_GAIN && best_gain > -NULL_GAIN) // ~ 0
    {
      best_feature_id = 0;
      best_gain = 0.0;
      best_no_delta = 0.0;
      best_yes_delta = 0.0;
    }

    return best_feature_id;
  }

  template<typename LabelType, typename GainType>
  void
  TreeLearner<LabelType, GainType>::check_feature_(
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
    noexcept
  {
    double sub_gain = 0.0;

    // precheck feature
    if(check_depth - 1 > 0)
    {
      SVM_var yes_svm;
      SVM_var no_svm;

      div_rows_(
        yes_svm,
        no_svm,
        feature_id,
        node_svm,
        feature_rows);

      unsigned long next_yes_best_feature_id = 0;
      double next_div_yes_best_gain = 0.0;
      double unused;

      get_best_feature_(
        next_yes_best_feature_id,
        next_div_yes_best_gain,
        unused, // best_no_delta
        unused, // best_yes_delta
        nullptr, // task_runner
        top_pred,
        feature_rows,
        skip_features,
        yes_svm,
        check_depth - 1,
        false);

      /*
      double next_yes_best_gain = eval_init_delta_(
        unused, // delta
        top_pred,
        yes_svm);

      if(next_yes_best_gain < next_div_yes_best_gain + GAIN_ABS_PENALTY)
      {
        next_yes_best_feature_id = 0;
        sub_gain += next_yes_best_gain;
      }
      else
      {
        sub_gain += next_div_yes_best_gain + GAIN_ABS_PENALTY;
      }
      */

      unsigned long next_no_best_feature_id = 0;
      double next_div_no_best_gain = 0.0;
      //double next_no_best_no_delta = 0.0;
      //double next_no_best_yes_delta = 0.0;

      get_best_feature_(
        next_no_best_feature_id,
        next_div_no_best_gain,
        unused, // best_no_delta
        unused, // best_yes_delta
        nullptr, // task_runner
        top_pred,
        feature_rows,
        skip_features,
        no_svm,
        check_depth - 1,
        false);

      /*
      double next_no_best_gain = eval_init_delta_(
        unused, // delta
        top_pred,
        no_svm);

      if(next_no_best_gain < next_div_no_best_gain + GAIN_ABS_PENALTY)
      {
        next_no_best_feature_id = 0;
        sub_gain += next_no_best_gain;
      }
      else
      {
        sub_gain += next_div_no_best_gain + GAIN_ABS_PENALTY;
      }
      */

      sub_gain += next_div_yes_best_gain + next_div_no_best_gain;

      /*
      std::cout << "SUB GAIN: next_div_yes_best_gain = " << next_div_yes_best_gain <<
        //", next_yes_best_gain = " << next_yes_best_gain <<
        ", next_div_no_best_gain = " << next_div_no_best_gain <<
        //", next_no_best_gain = " << next_no_best_gain <<
        std::endl;
      */
    }
    else
    {
      sub_gain = eval_feature_gain_(
        res_no_delta,
        res_yes_delta,
        gain_calc,
        top_pred,
        feature_id,
        feature_svm,
        node_svm
        );
    }

    res_gain = sub_gain; //std::min(sub_gain + GAIN_ABS_PENALTY, 0.0);
  }

  template<typename LabelType, typename GainType>
  double
  TreeLearner<LabelType, GainType>::eval_feature_gain_(
    double& no_delta,
    double& yes_delta,
    GainType& gain_calc,
    double add_delta,
    unsigned long, // feature_id,
    const SVM<LabelType>* feature_svm,
    const SVM<LabelType>* node_svm)
    noexcept
  {
    // p = SUM(label) / <rows number>

    // eval p
    // fetch feature rows & fetch node rows
    gain_calc.start_count(add_delta);

    //std::cout << "> eval_feature_gain_(" << feature_id << ")" << std::endl;

    auto feature_group_it = feature_svm->grouped_rows.begin();
    auto node_group_it = node_svm->grouped_rows.begin();

    //std::cout << "> eval_feature_gain_" << std::endl;

    while(feature_group_it != feature_svm->grouped_rows.end() &&
      node_group_it != node_svm->grouped_rows.end())
    {
      if((*feature_group_it)->label < (*node_group_it)->label)
      {
        // non cross rows
        ++feature_group_it;
      }
      else if((*node_group_it)->label < (*feature_group_it)->label)
      {
        // non cross rows
        //std::cout << "rows.size = " << (*node_group_it)->rows.size() << std::endl;

        gain_calc.add_count((*node_group_it)->label, false, (*node_group_it)->rows.size());
        //unfeatured_sum += (*node_group_it)->label * (*node_group_it)->rows.size();
        //unfeatured_count += (*node_group_it)->rows.size();

        ++node_group_it;
      }
      else // (*node_group_it)->label == (*feature_group_it)->label
      {
        // count cross & non cross rows
        unsigned long cross_size = count_cross_rows(
          (*node_group_it)->rows,
          (*feature_group_it)->rows);

        /*
        std::cout << "rows.size = " << (*node_group_it)->rows.size() <<
          ", cross_size = " << cross_size <<
          std::endl;
        */
        if(cross_size != (*node_group_it)->rows.size())
        {
          gain_calc.add_count((*node_group_it)->label, false, (*node_group_it)->rows.size() - cross_size);
        }

        if(cross_size > 0)
        {
          gain_calc.add_count((*node_group_it)->label, true, cross_size);
        }

        //unfeatured_sum += (*node_group_it)->label * (
        //  (*node_group_it)->rows.size() - cross_size);
        //unfeatured_count += (*node_group_it)->rows.size() - cross_size;
        //featured_sum += (*node_group_it)->label * cross_size;
        //featured_count += cross_size;

        ++feature_group_it;
        ++node_group_it;
      }
    }

    while(node_group_it != node_svm->grouped_rows.end())
    {
      gain_calc.add_count((*node_group_it)->label, false, (*node_group_it)->rows.size());
      ++node_group_it;
    }

    // ignore tail's - non cross rows
    gain_calc.start_eval();

    feature_group_it = feature_svm->grouped_rows.begin();
    node_group_it = node_svm->grouped_rows.begin();

    while(feature_group_it != feature_svm->grouped_rows.end() &&
      node_group_it != node_svm->grouped_rows.end())
    {
      if((*feature_group_it)->label < (*node_group_it)->label)
      {
        // non cross rows
        ++feature_group_it;
      }
      else if((*node_group_it)->label < (*feature_group_it)->label)
      {
        // non cross rows
        gain_calc.add_eval((*node_group_it)->label, false, (*node_group_it)->rows.size());

        ++node_group_it;
      }
      else // (*node_group_it)->label == (*feature_group_it)->label
      {
        // count cross & non cross rows
        unsigned long cross_size = count_cross_rows(
          (*node_group_it)->rows, (*feature_group_it)->rows);

        if(cross_size != (*node_group_it)->rows.size())
        {
          gain_calc.add_eval((*node_group_it)->label, false, (*node_group_it)->rows.size() - cross_size);
        }

        if(cross_size > 0)
        {
          gain_calc.add_eval((*node_group_it)->label, true, cross_size);
        }

        ++feature_group_it;
        ++node_group_it;
      }
    }

    while(node_group_it != node_svm->grouped_rows.end())
    {
      gain_calc.add_eval((*node_group_it)->label, false, (*node_group_it)->rows.size());
      ++node_group_it;
    }

    yes_delta = gain_calc.featured_delta();
    no_delta = gain_calc.unfeatured_delta();
    double result_gain = gain_calc.result();

    //std::cout << "< eval_feature_gain_(" << feature_id << "): gain = " << result_gain << std::endl;

    return result_gain;
  }

  /*
  double
  TreeLearner::eval_gain(
    unsigned long yes_value_labeled,
    unsigned long yes_value_unlabeled,
    unsigned long labeled,
    unsigned long unlabeled)
  {
    assert(yes_value_unlabeled <= unlabeled);
    assert(yes_value_labeled <= labeled);

    const unsigned long total = labeled + unlabeled;

    // eval old logloss
    const double p_old = total > 0 ?
      std::min(std::max(static_cast<double>(labeled) / total, EPS), 1.0 - EPS) :
      0.0;

    const double old_logloss = - (
      labeled > 0 ? unlabeled * std::log(1.0 - p_old) + labeled * std::log(p_old) :
      0.0);

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

#   ifdef TRACE_OUTPUT
    std::cerr << "yes_value_labeled = " << yes_value_labeled <<
      ", yes_value_unlabeled = " << yes_value_unlabeled <<
      ", no_value_labeled = " << no_value_labeled <<
      ", no_value_unlabeled = " << no_value_unlabeled <<
      ", labeled = " << labeled <<
      ", unlabeled = " << unlabeled << std::endl;
#   endif

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

#   ifdef TRACE_OUTPUT
    std::cerr << "no_part = " << no_part << ", p1 = " << p1 <<
      ", yes_part = " << yes_part << ", p2 = " << p2 <<
      ", old_logloss = " << old_logloss <<
      ", new_logloss = " << new_logloss <<
      std::endl;
#   endif

    // new logloss += gain, new logloss = 
    return new_logloss - old_logloss; // negative, new logloss = old logloss + gain 
  }
  */

  template<typename LabelType, typename GainType>
  void
  TreeLearner<LabelType, GainType>::fill_feature_rows_(
    FeatureIdArray& features,
    FeatureRowsMap& feature_rows,
    const SVM<LabelType>& svm)
    noexcept
  {
    std::deque<unsigned long> features_queue;

    unsigned long row_i = 0;
    for(auto group_it = svm.grouped_rows.begin(); group_it != svm.grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin();
        row_it != (*group_it)->rows.end(); ++row_it, ++row_i)
      {
        for(auto feature_it = (*row_it)->features.begin();
          feature_it != (*row_it)->features.end(); ++feature_it)
        {
          SVM_var& fr = feature_rows[feature_it->first];
          if(!fr.in())
          {
            fr = new SVM<LabelType>();
            features_queue.push_back(feature_it->first);
          }

          fr->add_row(*row_it, (*group_it)->label);
        }

        if(row_i > 0 && row_i % 100000 == 0)
        {
          std::cerr << row_i << " rows processed" << std::endl;
        }
      }
    }

    features.reserve(features_queue.size());
    std::copy(features_queue.begin(), features_queue.end(), std::back_inserter(features));
  }

  template<typename LabelType, typename GainType>
  typename TreeLearner<LabelType, GainType>::Context_var
  TreeLearner<LabelType, GainType>::create_context(const SVMArray& svm_array)
  {
    BagPartArray bag_parts;
    for(auto svm_it = svm_array.begin(); svm_it != svm_array.end(); ++svm_it)
    {
      BagHolder_var new_bag_holder = new BagHolder();
      new_bag_holder->bag = *svm_it;
      TreeLearner::fill_feature_rows_(
        new_bag_holder->features,
        new_bag_holder->feature_rows,
        **svm_it);

      BagPart_var new_bag_part = new BagPart();
      new_bag_part->bag_holder = new_bag_holder;
      new_bag_part->svm = (*svm_it)->copy();
      bag_parts.push_back(new_bag_part);
    }

    return new Context(bag_parts);
  }

  template<typename LabelType, typename GainType>
  void
  TreeLearner<LabelType, GainType>::div_bags_(
    BagPartArray& yes_bag_parts,
    BagPartArray& no_bag_parts,
    const BagPartArray& bag_parts,
    unsigned long feature_id)
  {
    for(auto bag_it = bag_parts.begin(); bag_it != bag_parts.end(); ++bag_it)
    {
      SVM_var yes_svm;
      SVM_var no_svm;
      
      div_rows_(
        yes_svm,
        no_svm,
        feature_id,
        (*bag_it)->svm,
        (*bag_it)->bag_holder->feature_rows);

      BagPart_var yes_bag_part = new BagPart();
      yes_bag_part->bag_holder = (*bag_it)->bag_holder;
      yes_bag_part->svm = yes_svm;
      yes_bag_parts.push_back(yes_bag_part);

      BagPart_var no_bag_part = new BagPart();
      no_bag_part->bag_holder = (*bag_it)->bag_holder;
      no_bag_part->svm = no_svm;
      no_bag_parts.push_back(no_bag_part);
    }
  }

  template<typename LabelType, typename GainType>
  void
  TreeLearner<LabelType, GainType>::div_rows_(
    SVM_var& yes_svm,
    SVM_var& no_svm,
    unsigned long feature_id,
    const SVM<LabelType>* div_svm,
    const FeatureRowsMap& feature_rows)
  {
    auto feature_it = feature_rows.find(feature_id);
    if(feature_it != feature_rows.end())
    {
      const SVM<LabelType>* feature_svm = feature_it->second;
      SVM<LabelType>::cross(yes_svm, no_svm, div_svm, feature_svm);
      assert(yes_svm->size() + no_svm->size() == div_svm->size());
    }
    else
    {
      yes_svm = new SVM<LabelType>();
      no_svm = div_svm->copy();
    }
  }
}
