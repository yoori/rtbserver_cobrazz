namespace Vanga
{
  template<typename LabelType>
  PredArrayHolder_var
  Predictor::predict(const SVM<LabelType>* svm) const noexcept
  {
    PredArrayHolder_var res = new PredArrayHolder();
    res->values.resize(svm->size());

    unsigned long i = 0;
    for(auto group_it = svm->grouped_rows.begin();
      group_it != svm->grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin();
        row_it != (*group_it)->rows.end(); ++row_it, ++i)
      {
        res->values[i] = predict((*row_it)->features);
      }
    }

    return res;
  }
}
