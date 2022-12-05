#include <sstream>

namespace Vanga
{
  template<typename LabelType>
  std::string
  DTree::to_string_ext_(
    const char* prefix,
    const FeatureDictionary* dict,
    double base,
    const SVM<LabelType>* svm,
    unsigned long full_size) const
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
      "(p = " << (1.0 / (1.0 + std::exp(-(base + delta_prob))));

    if(svm)
    {
      ostr << ", cover = " << (static_cast<double>(svm->size()) * 100.0 / full_size) << "%";
    }

    ostr << ")" << std::endl;

    if(yes_tree)
    {
      ReferenceCounting::SmartPtr<SVM<LabelType> > yes_svm =
        svm ?
        svm->by_feature(feature_id, true) :
        ReferenceCounting::SmartPtr<SVM<LabelType> >();

      ostr << prefix << "  yes =>" << std::endl <<
        yes_tree->to_string_ext_(
          (std::string(prefix) + ">   ").c_str(),
          dict,
          base + delta_prob,
          yes_svm.in(),
          full_size);
    }

    if(no_tree)
    {
      ReferenceCounting::SmartPtr<SVM<LabelType> > no_svm =
        svm ?
        svm->by_feature(feature_id, false) :
        ReferenceCounting::SmartPtr<SVM<LabelType> >();

      ostr << prefix << "  no =>" << std::endl <<
        no_tree->to_string_ext_(
          (std::string(prefix) + ">   ").c_str(),
          dict,
          base + delta_prob,
          no_svm.in(),
          full_size);
    }

    return ostr.str();
  }

  template<typename LabelType>
  std::string
  DTree::to_string_ext(
    const char* prefix,
    const FeatureDictionary* dict,
    double base,
    const SVM<LabelType>* svm)
    const noexcept
  {
    return to_string_ext_(
      prefix,
      dict,
      base,
      svm,
      svm ? svm->size() : 0);
  }
}
