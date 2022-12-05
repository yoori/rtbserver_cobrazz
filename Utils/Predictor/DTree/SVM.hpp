#ifndef SVM_HPP_
#define SVM_HPP_

#include <vector>
#include <deque>
#include <iostream>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Generics/SimpleDecimal.hpp>

namespace Vanga
{
  typedef std::vector<std::pair<uint32_t, uint32_t> > FeatureArray;

  struct Row: public ReferenceCounting::AtomicImpl
  {
    FeatureArray features;

  protected:
    virtual ~Row() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<Row, ReferenceCounting::PolicyAssert>
    Row_var;

  //typedef std::deque<Row_var> RowArray;
  typedef std::vector<Row_var> RowArray;

  template<typename LabelType>
  struct PredictGroup: public ReferenceCounting::AtomicCopyImpl
  {
    LabelType label;
    RowArray rows;

  protected:
    virtual ~PredictGroup() noexcept = default;
  };

  // SVM
  template<typename LabelType>
  struct SVM: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef ReferenceCounting::SmartPtr<
      PredictGroup<LabelType>, ReferenceCounting::PolicyAssert>
      PredictGroup_var;

    typedef std::vector<PredictGroup_var> PredictGroupArray;

  public:
    PredictGroupArray grouped_rows;

  public:
    void
    dump() const;

    ReferenceCounting::SmartPtr<SVM<LabelType> >
    copy() const noexcept;

    template<typename LabelAdapterType>
    ReferenceCounting::SmartPtr<SVM<typename LabelAdapterType::ResultType> >
    copy(const LabelAdapterType& label_adapter) const noexcept;

    static void
    cross(
      ReferenceCounting::SmartPtr<SVM>& cross_svm,
      ReferenceCounting::SmartPtr<SVM>& diff_svm,
      const SVM* left_svm,
      const SVM* right_svm)
      noexcept;

    ReferenceCounting::SmartPtr<SVM<LabelType> >
    part(unsigned long res_size) const noexcept;

    std::pair<ReferenceCounting::SmartPtr<SVM<LabelType> >, ReferenceCounting::SmartPtr<SVM<LabelType> > >
    div(unsigned long res_size) const noexcept;

    void
    add_row(Row* row, const LabelType& label)
      noexcept;

    double
    label_float_sum() const noexcept;

    unsigned long
    size() const noexcept;

    void
    print_labels(std::ostream& ostr)
      noexcept;

    static ReferenceCounting::SmartPtr<SVM<LabelType> >
    load(std::istream& in)
      /*throw(Exception)*/;

    static Row_var
    load_line(
      std::istream& in,
      LabelType& label_value);

    static void
    save_line(
      std::ostream& out,
      const Row* row,
      const LabelType& label_value);

    ReferenceCounting::SmartPtr<SVM<LabelType> >
    by_feature(unsigned long feature_id, bool yes)
      const;

  protected:
    Row_var
    get_row_(LabelType& label_value, unsigned long pos)
      const noexcept;

    void
    sort_() noexcept;

    static Row_var
    load_line_(
      std::istream& in,
      LabelType& label_value,
      FeatureArray& features);

  protected:
    virtual ~SVM() noexcept {}
  };

  /*
  typedef ReferenceCounting::ConstPtr<SVM> CSVM_var;
  typedef ReferenceCounting::SmartPtr<SVM> SVM_var;

  typedef std::vector<SVM_var> SVMArray;
  */
}

#include "SVM.tpp"

#endif /*SVM_HPP_*/
