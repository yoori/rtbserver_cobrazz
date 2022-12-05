#include <sstream>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/LogCommons.ipp>

#include "DTree.hpp"
#include "SVM.hpp"

namespace Vanga
{
  namespace
  {
    const char DTREE_MODEL_HEAD[] = "dtree";
    const char UNION_MODEL_AVG_HEAD[] = "union-avg";
    const char UNION_MODEL_SUM_HEAD[] = "union-sum";
    const char UNION_MODEL_SUM_HEAD_2[] = "union";
  }

  struct FeatureLess
  {
    bool
    operator()(unsigned long left, const std::pair<unsigned long, unsigned long>& right)
      const
    {
      return left < right.first;
    }

    bool
    operator()(const std::pair<unsigned long, unsigned long>& left, unsigned long right)
      const
    {
      return left.first < right;
    }

    bool
    operator()(
      const std::pair<unsigned long, unsigned long>& left,
      const std::pair<unsigned long, unsigned long>& right)
      const
    {
      return left.first < right.first;
    }
  };

  struct DTreeLoadHelper
  {
    unsigned long feature_id;
    double delta_prob;
    unsigned long yes_tree_id;
    unsigned long no_tree_id;
    DTree_var resolved_tree;
  };

  // DTree
  DTree_var
  DTree::as_dtree() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  void
  DTree::save(std::ostream& ostr) const
  {
    ostr << DTREE_MODEL_HEAD << std::endl;

    ostr.setf(std::ios::fixed, std::ios::floatfield);
    ostr.precision(7);

    this->save_node_(ostr);
  }

  void
  DTree::save_node_(std::ostream& ostr) const
  {
    ostr << tree_id << '\t' <<
      feature_id << '\t' <<
      delta_prob << '\t' <<
      (yes_tree ? yes_tree->tree_id : 0) << '\t' <<
      (no_tree ? no_tree->tree_id : 0) << std::endl;

    if(yes_tree)
    {
      yes_tree->save_node_(ostr);
    }

    if(no_tree)
    {
      no_tree->save_node_(ostr);
    }
  }

  ReferenceCounting::SmartPtr<DTree>
  DTree::load(std::istream& istr, bool with_head)
  {
    unsigned long root_tree_id = 0;
    std::map<unsigned long, DTreeLoadHelper> trees;
    unsigned long line_i = 0;

    if(with_head)
    {
      std::string line;
      AdServer::LogProcessing::read_until_eol(istr, line, false);
      if(line != DTREE_MODEL_HEAD)
      {
        Stream::Error ostr;
        ostr << "DTree::load(): invalid model type: '" << line << "'(" << line.size() << ")";
        throw Exception(ostr);
      }
    }

    while(!istr.eof())
    {
      std::string line;

      try
      {
        AdServer::LogProcessing::read_until_eol(istr, line, false);

        if(line.empty())
        {
          break;
        }

        if(istr.fail())
        {
          throw Exception("read failed or empty line");
        }

        String::StringManip::SplitTab tokenizer(line);
        String::SubString tree_id_str;
        if(!tokenizer.get_token(tree_id_str))
        {
          throw Exception("no id");
        }

        String::SubString feature_id_str;
        if(!tokenizer.get_token(feature_id_str))
        {
          throw Exception("no feature id");
        }

        String::SubString delta_prob_str;
        if(!tokenizer.get_token(delta_prob_str))
        {
          throw Exception("no 'delta prob'");
        }

        String::SubString yes_tree_id_str;
        if(!tokenizer.get_token(yes_tree_id_str))
        {
          throw Exception("no 'yes tree id'");
        }

        String::SubString no_tree_id_str;
        if(!tokenizer.get_token(no_tree_id_str))
        {
          throw Exception("no 'no tree id'");
        }

        unsigned long tree_id;
        DTreeLoadHelper dtree_load_helper;

        if(!String::StringManip::str_to_int(tree_id_str, tree_id))
        {
          Stream::Error ostr;
          ostr << "invalid tree id value: '" << tree_id_str << "'";
          throw Exception(ostr);
        }

        if(!String::StringManip::str_to_int(feature_id_str, dtree_load_helper.feature_id))
        {
          Stream::Error ostr;
          ostr << "invalid feature id value: '" << feature_id_str << "'";
          throw Exception(ostr);
        }

        {
          std::istringstream istr(delta_prob_str.str().c_str());
          istr >> dtree_load_helper.delta_prob;

          if(!istr.eof() || istr.fail())
          {
            Stream::Error ostr;
            ostr << "invalid delta prob value: '" << delta_prob_str << "'";
            throw Exception(ostr);
          }
        }

        if(!String::StringManip::str_to_int(yes_tree_id_str, dtree_load_helper.yes_tree_id))
        {
          Stream::Error ostr;
          ostr << "invalid 'yes tree id' value: '" << yes_tree_id_str << "'";
          throw Exception(ostr);
        }

        if(!String::StringManip::str_to_int(no_tree_id_str, dtree_load_helper.no_tree_id))
        {
          Stream::Error ostr;
          ostr << "invalid 'no tree id' value: '" << no_tree_id_str << "'";
          throw Exception(ostr);
        }

        if(root_tree_id == 0)
        {
          root_tree_id = tree_id;
        }

        trees.insert(std::make_pair(tree_id, dtree_load_helper));
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Can't load tree (line #" << line_i << "): " << ex.what() <<
          ", line='" << line << "'";
        throw Exception(ostr);
      }

      ++line_i;
    }

    // create stub trees
    for(auto tree_it = trees.begin(); tree_it != trees.end(); ++tree_it)
    {
      DTree_var dtree = new DTree();
      dtree->tree_id = tree_it->first;
      dtree->feature_id = tree_it->second.feature_id;
      dtree->delta_prob = tree_it->second.delta_prob;
      tree_it->second.resolved_tree = dtree;
    }

    // link trees
    for(auto tree_it = trees.begin(); tree_it != trees.end(); ++tree_it)
    {
      if(tree_it->second.yes_tree_id != 0)
      {
        auto res_tree_it = trees.find(tree_it->second.yes_tree_id);
        if(res_tree_it == trees.end())
        {
          Stream::Error ostr;
          ostr << "can't resolve tree id: " << tree_it->second.yes_tree_id;
          throw Exception(ostr);
        }

        tree_it->second.resolved_tree->yes_tree = res_tree_it->second.resolved_tree;
      }
      
      if(tree_it->second.no_tree_id != 0)
      {
        auto res_tree_it = trees.find(tree_it->second.no_tree_id);
        if(res_tree_it == trees.end())
        {
          Stream::Error ostr;
          ostr << "can't resolve tree id: " << tree_it->second.no_tree_id;
          throw Exception(ostr);
        }

        tree_it->second.resolved_tree->no_tree = res_tree_it->second.resolved_tree;
      }
    }

    auto root_tree_it = trees.find(root_tree_id);
    assert(root_tree_it != trees.end());

    return root_tree_it->second.resolved_tree;
  }

  double
  DTree::predict(const FeatureArray& features) const noexcept
  {
    double res = delta_prob;

    if(feature_id)
    {
      if(std::binary_search(
           features.begin(), features.end(), feature_id, FeatureLess()))
      {
        // yes tree
        if(yes_tree)
        {
          res += yes_tree->predict(features);
        }
      }
      else
      {
        // no tree
        if(no_tree)
        {
          res += no_tree->predict(features);
        }
      }
    }

    return res;
  }

  std::string
  DTree::to_string(
    const char* prefix,
    const FeatureDictionary* dict,
    double base) const
    noexcept
  {
    std::ostringstream ostr;
    ostr << prefix << feature_id << "{" << tree_id << "}";
    if(dict)
    {
      auto dict_it = dict->find(feature_id);
      if(dict_it != dict->end())
      {
        ostr << " [" << dict_it->second << "]";
      }
    }

    ostr << ": " << (delta_prob > 0 ? "+" : "") << delta_prob <<
      " = " << (base + delta_prob) <<
      "(p = " << (1.0 / (1.0 + std::exp(-(base + delta_prob)))) << ")" <<
      std::endl;

    if(yes_tree)
    {
      ostr << prefix << "  yes =>" << std::endl <<
        yes_tree->to_string(
          (std::string(prefix) + ">   ").c_str(),
          dict,
          base + delta_prob);
    }

    if(no_tree)
    {
      ostr << prefix << "  no =>" << std::endl <<
        no_tree->to_string(
          (std::string(prefix) + ">   ").c_str(),
          dict,
          base + delta_prob);
    }

    return ostr.str();
  }

  DTree_var
  DTree::copy() noexcept
  {
    DTree_var res = new DTree();
    res->tree_id = tree_id;
    res->feature_id = feature_id;
    res->delta_prob = delta_prob;

    if(yes_tree)
    {
      res->yes_tree = yes_tree->copy();
    }

    if(no_tree)
    {
      res->no_tree = no_tree->copy();
    }

    return res;
  }


  unsigned long
  DTree::node_count() const noexcept
  {
    return 1 +
      (yes_tree ? yes_tree->node_count() : 0) +
      (no_tree ? no_tree->node_count() : 0);
  }

  // PredictorSet
  PredictorSet_var
  PredictorSet::as_predictor_set() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  void
  PredictorSet::add(Predictor* predictor)
  {
    predictors_.push_back(ReferenceCounting::add_ref(predictor));
  }

  double
  PredictorSet::predict(const FeatureArray& features) const noexcept
  {
    double sum = 0.0;
    for(auto it = predictors_.begin(); it != predictors_.end(); ++it)
    {
      sum += (*it)->predict(features);
    }
    return type_ == AVG ? sum / predictors_.size() : sum;
  }

  void
  PredictorSet::save(std::ostream& ostr) const
  {
    ostr << (type_ == AVG ? UNION_MODEL_AVG_HEAD : UNION_MODEL_SUM_HEAD) << std::endl;

    for(auto it = predictors_.begin(); it != predictors_.end(); ++it)
    {
      //ostr << ">>>>>>>> TO PREDICTOR SAVE" << std::endl;
      (*it)->save(ostr);
      //ostr << ">>>>>>>> FROM PREDICTOR SAVE" << std::endl;
      ostr << std::endl;
    }
  }

  ReferenceCounting::SmartPtr<PredictorSet>
  PredictorSet::load(std::istream& istr, bool with_head, Type type)
  {
    PredictorSet_var res = new PredictorSet(type);

    if(with_head)
    {
      std::string line;
      AdServer::LogProcessing::read_until_eol(istr, line, false);
      if(line == UNION_MODEL_SUM_HEAD || line == UNION_MODEL_SUM_HEAD_2)
      {
        res->type_ = SUM;
      }
      else if(line == UNION_MODEL_AVG_HEAD)
      {
        res->type_ = AVG;
      }
      else
      {
        Stream::Error ostr;
        ostr << "PredictorSet::load(): invalid model type: '" << line << "'";
        throw Exception(ostr);
      }
    }

    while(!istr.eof())
    {
      Predictor_var predictor = PredictorLoader::load(istr);
      if(predictor)
      {
        res->add(predictor);
      }
    }

    return res;
  }

  const std::vector<Predictor_var>&
  PredictorSet::predictors() const
  {
    return predictors_;
  }

  std::string
  PredictorSet::to_string(
    const char* prefix,
    const FeatureDictionary* dict,
    double base) const
    noexcept
  {
    std::ostringstream ostr;

    unsigned long predictor_i = 0;
    for(auto predictor_it = predictors_.begin();
      predictor_it != predictors_.end(); ++predictor_it, ++predictor_i)
    {
      ostr << prefix << "Predictor #" << predictor_i << ":" << std::endl <<
        (*predictor_it)->to_string(
          (std::string(prefix) + "  ").c_str(),
          dict,
          base) << std::endl;
    }

    return ostr.str();
  }

  // PredictorLoader
  Predictor_var
  PredictorLoader::load(std::istream& istr)
  {
    std::string line;
    AdServer::LogProcessing::read_until_eol(istr, line, false);
    if(line.empty())
    {
      return Predictor_var();
    }
    if(line == DTREE_MODEL_HEAD)
    {
      return DTree::load(istr, false);
    }
    else if(line == UNION_MODEL_AVG_HEAD)
    {
      return PredictorSet::load(istr, false, PredictorSet::AVG);
    }
    else if(line == UNION_MODEL_SUM_HEAD || line == UNION_MODEL_SUM_HEAD_2)
    {
      return PredictorSet::load(istr, false, PredictorSet::SUM);
    }
    else
    {
      Stream::Error ostr;
      ostr << "PredictorLoader::load(): invalid model type: '" << line << "'";
      throw Exception(ostr);
    }
  }
}
