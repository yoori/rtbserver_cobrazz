#include <Generics/Rand.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/SubString.hpp>
#include <Stream/MemoryStream.hpp>

namespace Vanga
{
  struct FirstLess
  {
    template<typename ArgType>
    bool
    operator()(const ArgType& left, const ArgType& right)
      const
    {
      return left.first < right.first;
    }
  };

  struct FirstEqual
  {
    template<typename ArgType>
    bool
    operator()(const ArgType& left, const ArgType& right)
      const
    {
      return left.first == right.first;
    }
  };

  struct PredictGroupLess
  {
    template<typename LabelType>
    bool
    operator()(const PredictGroup<LabelType>* left, const PredictGroup<LabelType>* right)
      const noexcept
    {
      return left->label < right->label;
    }

    template<typename LabelType>
    bool
    operator()(const PredictGroup<LabelType>* left, const LabelType& right)
      const noexcept
    {
      return left->label < right;
    }

    template<typename LabelType>
    bool
    operator()(const LabelType& left, const PredictGroup<LabelType>* right)
      const noexcept
    {
      return left < right->label;
    }

    template<typename LabelType>
    bool
    operator()(
      const ReferenceCounting::SmartPtr<PredictGroup<LabelType>, ReferenceCounting::PolicyAssert>& left,
      const LabelType& right)
      const noexcept
    {
      return left->label < right;
    }

    template<typename LabelType>
    bool
    operator()(const LabelType& left, const ReferenceCounting::SmartPtr<PredictGroup<LabelType> >& right)
      const noexcept
    {
      return left < right->label;
    }
  };

  // SVM impl
  template<typename LabelType>
  void
  SVM<LabelType>::dump() const
  {
    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        std::cout << "ROW(" << (*group_it)->label.to_float() << ")" << std::endl;
      }
    }
  }

  template<typename LabelType>
  void
  SVM<LabelType>::save_line(
    std::ostream& out,
    const Row* row,
    const LabelType& label_value)
  {
    label_value.save(out);
    for(auto feature_it = row->features.begin(); feature_it != row->features.end(); ++feature_it)
    {
      out << " " << feature_it->first << ":" << feature_it->second;
    }
    out << std::endl;
  }

  template<typename LabelType>
  Row_var
  SVM<LabelType>::load_line(
    std::istream& in,
    LabelType& label_value)
  {
    FeatureArray features;
    return load_line_(in, label_value, features);
  }

  template<typename LabelType>
  ReferenceCounting::SmartPtr<SVM<LabelType> >
  SVM<LabelType>::copy() const noexcept
  {
    ReferenceCounting::SmartPtr<SVM<LabelType> > res = new SVM<LabelType>();

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      res->grouped_rows.push_back(new PredictGroup<LabelType>(**group_it));
    }

    return res;
  }

  template<typename LabelType>
  template<typename LabelAdapterType>
  ReferenceCounting::SmartPtr<SVM<typename LabelAdapterType::ResultType> >
  SVM<LabelType>::copy(const LabelAdapterType& label_adapter) const noexcept
  {
    ReferenceCounting::SmartPtr<SVM<typename LabelAdapterType::ResultType> > res =
      new SVM<typename LabelAdapterType::ResultType>();

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        res->add_row(*row_it, label_adapter(*row_it, (*group_it)->label));
      }
    }

    res->sort_();

    return res;
  }

  template<typename LabelType>
  void
  SVM<LabelType>::cross(
    ReferenceCounting::SmartPtr<SVM<LabelType> >& cross_svm,
    ReferenceCounting::SmartPtr<SVM<LabelType> >& diff_svm,
    const SVM<LabelType>* left_svm,
    const SVM<LabelType>* right_svm)
    noexcept
  {
    cross_svm = new SVM<LabelType>();
    diff_svm = new SVM<LabelType>();

    auto left_group_it = left_svm->grouped_rows.begin();
    auto right_group_it = right_svm->grouped_rows.begin();

    while(left_group_it != left_svm->grouped_rows.end() &&
      right_group_it != right_svm->grouped_rows.end())
    {
      if((*right_group_it)->label < (*left_group_it)->label)
      {
        // non cross rows
        ++right_group_it;
      }
      else if((*left_group_it)->label < (*right_group_it)->label)
      {
        diff_svm->grouped_rows.push_back(new PredictGroup<LabelType>(**left_group_it));

        ++left_group_it;
      }
      else // (*left_group_it)->label == (*right_group_it)->label
      {
        PredictGroup_var cross_group = new PredictGroup<LabelType>();
        cross_group->label = (*left_group_it)->label;

        std::set_intersection(
          (*left_group_it)->rows.begin(),
          (*left_group_it)->rows.end(),
          (*right_group_it)->rows.begin(),
          (*right_group_it)->rows.end(),
          std::back_inserter(cross_group->rows));

        PredictGroup_var diff_group = new PredictGroup<LabelType>();
        diff_group->label = (*left_group_it)->label;

        std::set_difference(
          (*left_group_it)->rows.begin(),
          (*left_group_it)->rows.end(),
          (*right_group_it)->rows.begin(),
          (*right_group_it)->rows.end(),
          std::back_inserter(diff_group->rows));

        if(!cross_group->rows.empty())
        {
          cross_svm->grouped_rows.push_back(cross_group);
        }

        if(!diff_group->rows.empty())
        {
          diff_svm->grouped_rows.push_back(diff_group);
        }

        ++left_group_it;
        ++right_group_it;
      }
    }

    // process tail
    while(left_group_it != left_svm->grouped_rows.end())
    {
      diff_svm->grouped_rows.push_back(new PredictGroup<LabelType>(**left_group_it));
      ++left_group_it;
    }
  }

  template<typename LabelType>
  ReferenceCounting::SmartPtr<SVM<LabelType> >
  SVM<LabelType>::part(unsigned long res_size)
    const noexcept
  {
    ReferenceCounting::SmartPtr<SVM<LabelType> > res = new SVM<LabelType>();
    const unsigned long cur_size = size();

    if(cur_size > 0)
    {
      for(unsigned long i = 0; i < res_size; ++i)
      {
        unsigned long pos = Generics::safe_rand(cur_size);
        LabelType label_value;
        Row_var row = get_row_(label_value, pos);
        res->add_row(row, label_value);
      }
    }

    res->sort_();

    return res;
  }

  template<typename LabelType>
  std::pair<ReferenceCounting::SmartPtr<SVM<LabelType> >, ReferenceCounting::SmartPtr<SVM<LabelType> > >
  SVM<LabelType>::div(unsigned long res_size)
    const noexcept
  {
    ReferenceCounting::SmartPtr<SVM<LabelType> > res_first = new SVM<LabelType>();
    ReferenceCounting::SmartPtr<SVM<LabelType> > res_second = new SVM<LabelType>();

    unsigned long cur_size = this->size();

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        unsigned long pos = Generics::safe_rand(cur_size);
        if(pos < res_size)
        {
          res_first->add_row(*row_it, (*group_it)->label);
        }
        else
        {
          res_second->add_row(*row_it, (*group_it)->label);
        }
      }
    }

    res_first->sort_();
    res_second->sort_();

    return std::make_pair(res_first, res_second);
  }

  template<typename LabelType>
  ReferenceCounting::SmartPtr<SVM<LabelType> >
  SVM<LabelType>::by_feature(unsigned long feature_id, bool yes)
    const
  {
    ReferenceCounting::SmartPtr<SVM<LabelType> > res = new SVM<LabelType>();

    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin(); row_it != (*group_it)->rows.end(); ++row_it)
      {
        bool found = std::binary_search(
          (*row_it)->features.begin(),
          (*row_it)->features.end(),
          std::pair<uint32_t, uint32_t>(feature_id, 0),
          FirstLess());

        if(yes && found)
        {
          res->add_row(*row_it, (*group_it)->label);
        }
        else if(!yes && !found)
        {
          res->add_row(*row_it, (*group_it)->label);
        }
      }
    }

    res->sort_();

    return res;    
  }

  template<typename LabelType>
  double
  SVM<LabelType>::label_float_sum() const noexcept
  {
    double res = 0.0;
    
    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      res += (*group_it)->label.to_float() * (*group_it)->rows.size();
    }

    return res;
  }

  template<typename LabelType>
  unsigned long
  SVM<LabelType>::size() const noexcept
  {
    unsigned long res_size = 0;
    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      res_size += (*group_it)->rows.size();
    }
    return res_size;
  }

  template<typename LabelType>
  ReferenceCounting::SmartPtr<SVM<LabelType> >
  SVM<LabelType>::load(std::istream& in)
    /*throw(Exception)*/
  {
    unsigned long line_i = 0;
    ReferenceCounting::SmartPtr<SVM<LabelType> > svm(new SVM<LabelType>());
    FeatureArray features;
    features.reserve(10240);

    while(!in.eof())
    {
      LabelType label;
      Row_var new_row = load_line_(in, label, features);

      if(new_row)
      {
        svm->add_row(new_row, label);
      }

      ++line_i;

      if(line_i % 100000 == 0)
      {
        std::cerr << "loaded " << line_i << " lines" << std::endl;
      }
    }

    std::cerr << "loading finished (" << line_i << " lines)" << std::endl;

    svm->sort_();

    std::cerr << "rows sorted" << std::endl;

    //std::cout << "GROUPS: " << svm->grouped_rows.size() << std::endl;
    //svm->dump();
    //std::cout << "^^^^^^^^^^^^^^^^^^" << std::endl;

    return svm;
  }

  template<typename LabelType>
  void
  SVM<LabelType>::add_row(Row* row, const LabelType& label)
    noexcept
  {
    // find group
    auto found_it = std::lower_bound(
      grouped_rows.begin(),
      grouped_rows.end(),
      label,
      PredictGroupLess());

    if(found_it == grouped_rows.end() || label < (*found_it)->label)
    {
      PredictGroup_var new_group = new PredictGroup<LabelType>();
      new_group->label = label;
      found_it = grouped_rows.insert(found_it, new_group);
    }

    (*found_it)->rows.push_back(ReferenceCounting::add_ref(row));
  }

  template<typename LabelType>
  void
  SVM<LabelType>::print_labels(std::ostream& ostr)
    noexcept
  {
    for(auto row_it = grouped_rows.begin(); row_it != grouped_rows.end(); ++row_it)
    {
      ostr << "label = ";
      (*row_it)->label.print(ostr);
      ostr << ": " << (*row_it)->rows.size() << std::endl;
    }
  }

  template<typename LabelType>
  Row_var
  SVM<LabelType>::load_line_(
    std::istream& in,
    LabelType& label_value,
    FeatureArray& features)
  {
    features.clear();

    std::string line;
    std::getline(in, line);
    if(line.empty())
    {
      return nullptr;
    }

    String::StringManip::Splitter<
      String::AsciiStringManip::SepSpace> tokenizer(line);
    String::SubString token;
    bool label = true;

    while(tokenizer.get_token(token))
    {
      if(label)
      {
        label = false;
        label_value.load(token.str());
        //std::cerr << "label_value = " << label_value << std::endl;

        /*
        if(!String::StringManip::str_to_int(token, label_value))
        {
          Stream::Error ostr;
          ostr << "can't parse label '" << token << "'";
          throw Exception(ostr);
        }
        */
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

        unsigned long value = 1;
        if(pos != String::SubString::NPOS)
        {
          String::SubString value_str = token.substr(pos + 1);
          if(!String::StringManip::str_to_int(value_str, value))
          {
            Stream::Error ostr;
            ostr << "can't parse feature value '" << value_str << "'";
            throw Exception(ostr);
          }
        }

        features.push_back(std::make_pair(feature_value, value));
      }
    }

    std::sort(features.begin(), features.end(), FirstLess());
    auto erase_begin_it = std::unique(features.begin(), features.end(), FirstEqual());
    features.erase(erase_begin_it, features.end());

    // create Row
    Row_var new_row(new Row());
    new_row->features.swap(features);
    return new_row;
  }

  template<typename LabelType>
  Row_var
  SVM<LabelType>::get_row_(LabelType& label, unsigned long pos)
    const noexcept
  {
    for(auto group_it = grouped_rows.begin();
      group_it != grouped_rows.end(); ++group_it)
    {
      if(pos < (*group_it)->rows.size())
      {
        label = (*group_it)->label;
        return (*group_it)->rows[pos];
      }

      pos -= (*group_it)->rows.size();
    }

    return Row_var();
  }

  template<typename LabelType>
  void
  SVM<LabelType>::sort_() noexcept
  {
    for(auto group_it = grouped_rows.begin(); group_it != grouped_rows.end(); ++group_it)
    {
      std::sort((*group_it)->rows.begin(), (*group_it)->rows.end());
    }
  }
}
