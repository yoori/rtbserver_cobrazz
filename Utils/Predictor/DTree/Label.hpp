#ifndef LABEL_HPP_
#define LABEL_HPP_

#include <String/SubString.hpp>
#include "Predictor.hpp"

namespace Vanga
{
  DECLARE_EXCEPTION(LabelException, eh::DescriptiveException);

  struct BoolLabel
  {
    bool value;

    void
    load(const String::SubString& str);

    void
    save(std::ostream& ostr) const;

    bool
    orig() const;

    double
    to_float() const;

    bool
    operator<(const BoolLabel& right) const;

    void
    print(std::ostream& ostr) const noexcept;
  };

  typedef Generics::SimpleDecimal<uint64_t, 18, 8> PredDecimal;

  struct PredictedBoolLabel: public BoolLabel
  {
    double pred;

    void
    load(const String::SubString& str);

    bool
    operator<(const PredictedBoolLabel& right) const;

    void
    print(std::ostream& ostr) const noexcept;
  };

  struct PredictedBoolLabelAddConverter
  {
  public:
    typedef PredictedBoolLabel ResultType;

  public:
    PredictedBoolLabelAddConverter(Predictor* predictor)
      : predictor_(ReferenceCounting::add_ref(predictor))
    {}

    PredictedBoolLabel
    operator()(const Row* row, const PredictedBoolLabel& label) const
    {
      PredictedBoolLabel converted_label = label;

      if(predictor_)
      {
        converted_label.pred += predictor_->predict(row->features);
      }
      else
      {
        converted_label.pred = 0;
      }

      /*
      std::cout << "converted_label.pred = " << converted_label.pred <<
        ", exp = " << (1.0 / (1.0 + std::exp(-converted_label.pred))) << std::endl;
      */
      return converted_label;
    }

  protected:
    Predictor_var predictor_;
  };

  /*
  struct LabelType: public Generics::SimpleDecimal<uint64_t, 18, 8>
  {
    LabelType() {}

    LabelType(int val)
      : Generics::SimpleDecimal<uint64_t, 18, 8>(val < 0, std::abs(val), 0)
    {}

    LabelType(const Generics::SimpleDecimal<uint64_t, 18, 8>& init)
      : Generics::SimpleDecimal<uint64_t, 18, 8>(init)
    {}

    LabelType(const String::SubString& str)
      : Generics::SimpleDecimal<uint64_t, 18, 8>(str)
    {}

    LabelType
    operator*(int right) const
    {
      return LabelType::mul(
        *this,
        Generics::SimpleDecimal<uint64_t, 18, 8>(right < 0, std::abs(right), 0),
        Generics::DMR_FLOOR);
    }
  };

  inline LabelType
  str_to_label(const String::SubString& str)
  {
    return LabelType(str);
  }

  inline double
  label_to_double(LabelType label)
  {
    double res;
    label.to_floating(res);
    return res;
  }
  */
}

namespace Vanga
{
  // BoolLabel impl
  inline void
  BoolLabel::load(const String::SubString& str)
  {
    if(str.length() != 1)
    {
      Stream::Error ostr;
      ostr << "can't parse label '" << str << "'";
      throw LabelException(ostr);
    }
    else if(str[0] == '1')
    {
      value = true;
    }
    else if(str[0] == '0')
    {
      value = false;
    }
    else
    {
      Stream::Error ostr;
      ostr << "can't parse label '" << str << "'";
      throw LabelException(ostr);
    }
  }

  void
  BoolLabel::save(std::ostream& ostr) const
  {
    ostr << (value ? "1" : "0");
  }

  inline bool
  BoolLabel::orig() const
  {
    return value;
  }

  inline double
  BoolLabel::to_float() const
  {
    return value ? 1.0 : 0.0;
  }

  inline bool
  BoolLabel::operator<(const BoolLabel& right) const
  {
    return !this->value && right.value;
  }

  void
  BoolLabel::print(std::ostream& ostr) const noexcept
  {
    ostr << orig();
  }

  // PredictedBoolLabel impl
  inline void
  PredictedBoolLabel::load(const String::SubString& str)
  {
    BoolLabel::load(str);
    pred = 0.0;
  }

  inline bool
  PredictedBoolLabel::operator<(const PredictedBoolLabel& right) const
  {
    if(!value)
    {
      return right.value || pred < right.pred;
    }

    return right.value && pred < right.pred;
  }

  void
  PredictedBoolLabel::print(std::ostream& ostr) const noexcept
  {
    ostr << "label = " << orig() <<
      ", pred = " << pred;
  }
}

#endif /*LABEL_HPP_*/
