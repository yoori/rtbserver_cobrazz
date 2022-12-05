#ifndef DTREE_HPP_
#define DTREE_HPP_

#include <iostream>
#include <map>
#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Generics/Time.hpp>
#include <Commons/Containers.hpp>

#include "SVM.hpp"
#include "Predictor.hpp"

namespace Vanga
{
  struct PredictorLoader
  {
    typedef Predictor::Exception Exception;

    static Predictor_var
    load(std::istream& istr);
  };

  // DTree
  class DTree:
    public Predictor,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DTree_var
    as_dtree() noexcept;

    void
    save(std::ostream& ostr) const;

    static ReferenceCounting::SmartPtr<DTree>
    load(std::istream& istr, bool with_head = true);

    // features should be sorted
    double
    predict(const FeatureArray& features) const noexcept;

    std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = nullptr,
      double base = 0.0)
      const noexcept;

    template<typename LabelType>
    std::string
    to_string_ext(
      const char* prefix,
      const FeatureDictionary* dict,
      double base,
      const SVM<LabelType>* svm)
      const noexcept;

    DTree_var
    copy() noexcept;

    unsigned long
    node_count() const noexcept;

  protected:
    virtual ~DTree() noexcept {}

    void
    save_node_(std::ostream& ostr) const;

    template<typename LabelType>
    std::string
    to_string_ext_(
      const char* prefix,
      const FeatureDictionary* dict,
      double base,
      const SVM<LabelType>* svm,
      unsigned long full_size) const
      noexcept;

  public:
    unsigned long tree_id;
    unsigned long feature_id;
    double delta_prob;
    ReferenceCounting::SmartPtr<DTree> yes_tree;
    ReferenceCounting::SmartPtr<DTree> no_tree;
  };

  // PredictorSet
  class PredictorSet:
    public Predictor,
    public ReferenceCounting::AtomicImpl
  {
  public:
    enum Type
    {
      AVG,
      SUM
    };

    PredictorSet(Type type = AVG);

    template<typename IteratorType>
    PredictorSet(IteratorType begin, IteratorType end, Type type = AVG);

    virtual PredictorSet_var
    as_predictor_set() noexcept;

    void
    add(Predictor* tree);

    double
    predict(const FeatureArray& features) const noexcept;

    void
    save(std::ostream& ostr) const;

    std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = nullptr,
      double base = 0.0)
      const noexcept;

    static ReferenceCounting::SmartPtr<PredictorSet>
    load(std::istream& istr, bool with_head = true, PredictorSet::Type type = AVG);

    const std::vector<Predictor_var>&
    predictors() const;

  protected:
    virtual ~PredictorSet() noexcept {}

  protected:
    Type type_;
    std::vector<Predictor_var> predictors_;
  };

  typedef ReferenceCounting::SmartPtr<PredictorSet>
    PredictorSet_var;
}

namespace Vanga
{
  // PredictorSet impl
  inline
  PredictorSet::PredictorSet(Type type)
    : type_(type)
  {}

  template<typename IteratorType>
  PredictorSet::PredictorSet(IteratorType begin, IteratorType end, Type type)
    : type_(type)
  {
    for(; begin != end; ++begin)
    {
      add(*begin);
    }
  }
}

#include "DTree.tpp"

#endif /*DTREE_HPP_*/
