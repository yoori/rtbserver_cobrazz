#ifndef PREDICTOR_HPP_
#define PREDICTOR_HPP_

#include <iostream>
#include <map>
#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include "SVM.hpp"

namespace Vanga
{
  typedef std::map<unsigned long, std::string> FeatureDictionary;

  struct PredArrayHolder: public ReferenceCounting::AtomicImpl
  {
    typedef std::deque<double> PredArray;

    PredArray values;

  protected:
    virtual ~PredArrayHolder() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<PredArrayHolder> PredArrayHolder_var;

  class DTree;
  typedef ReferenceCounting::SmartPtr<DTree> DTree_var;

  class PredictorSet;
  typedef ReferenceCounting::SmartPtr<PredictorSet> PredictorSet_var;

  // Predictor
  class Predictor: public virtual ReferenceCounting::Interface
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    virtual double
    predict(const FeatureArray& features) const noexcept = 0;

    PredArrayHolder_var
    predict(const RowArray& rows) const noexcept;

    template<typename LabelType>
    PredArrayHolder_var
    predict(const SVM<LabelType>* svm) const noexcept;

    virtual void
    save(std::ostream& ostr) const = 0;

    virtual std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = nullptr,
      double base = 0.0)
      const noexcept = 0;

    virtual DTree_var
    as_dtree() noexcept;

    virtual PredictorSet_var
    as_predictor_set() noexcept;

  protected:
    virtual ~Predictor() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<Predictor> Predictor_var;

  // LogRegPredictor
  class LogRegPredictor:
    public Predictor,
    public ReferenceCounting::AtomicImpl
  {
  public:
    LogRegPredictor(Predictor* predictor) noexcept;

    virtual double
    predict(const FeatureArray& features) const noexcept;

    virtual void
    save(std::ostream& ostr) const;

    virtual std::string
    to_string(
      const char* prefix,
      const FeatureDictionary* dict = nullptr,
      double base = 0.0)
      const noexcept;

    virtual DTree_var
    as_dtree() noexcept;

    virtual PredictorSet_var
    as_predictor_set() noexcept;

  protected:
    virtual ~LogRegPredictor() noexcept {}

  protected:
    Predictor_var predictor_;
  };
}

#include "Predictor.tpp"

#endif /*PREDICTOR_HPP_*/
