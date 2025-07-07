#ifndef UTILS_HPP_
#define UTILS_HPP_

#include "Predictor.hpp"

namespace Vanga
{
  const double LOGLOSS_EPS = 0.0000001;

  namespace Utils
  {
    template<typename LabelType>
    double
    logloss_by_pred(const SVM<LabelType>* svm);

    template<typename LabelType>
    double
    logloss(const Predictor* tree, const SVM<LabelType>* svm);

    template<typename LabelType>
    double
    absloss(const Predictor* tree, const SVM<LabelType>* svm);

    template<typename LabelType>
    PredArrayHolder_var
    labels(const SVM<LabelType>* svm);

    double
    avg_logloss(
      const std::vector<PredArrayHolder_var>& preds,
      const PredArrayHolder* labels);

    void
    fill_logloss(
      std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> >& res_preds,
      const std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> >& preds);

    // find min of L = SUM(ln(1 + exp(-(xi + x))) + (xi + x)*(1 - yi))
    // exp_array array of exp(xi)
    // unlabeled_sum = SUM(1 - yi)
    template<typename ContainerType>
    double
    solve_grouped_logloss_min(
      const ContainerType& exp_array,
      unsigned long unlabeled_sum);
  }
}

namespace Vanga
{
namespace Utils
{
  template<typename LabelType>
  double
  logloss(const Predictor* predictor, const SVM<LabelType>* svm)
  {
    double loss = 0;
    unsigned long rows = 0;

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      for(auto row_it = (*it)->rows.begin(); row_it != (*it)->rows.end(); ++row_it)
      {
        double pred = predictor->predict((*row_it)->features);

        if((*it)->label.orig())
        {
          loss -= ::log(std::max(pred, LOGLOSS_EPS));
        }
        else
        {
          loss -= ::log(1 - std::min(pred, 1 - LOGLOSS_EPS));
        }

        ++rows;
      }
    }

    return rows > 0 ? loss / rows : 0.0;
  }

  template<typename LabelType>
  double
  logloss_by_pred(const SVM<LabelType>* svm)
  {
    const double DOUBLE_ONE = 1.0;

    double loss = 0;
    unsigned long rows = 0;

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      const double pred = DOUBLE_ONE / (DOUBLE_ONE + std::exp(-(*it)->label.pred));

      if((*it)->label.orig())
      {
        loss -= ::log(std::max(pred, LOGLOSS_EPS)) * (*it)->rows.size();
      }
      else
      {
        loss -= ::log(1 - std::min(pred, 1 - LOGLOSS_EPS)) * (*it)->rows.size();
      }

      rows += (*it)->rows.size();
    }

    return rows > 0 ? loss / rows : 0.0;
  }

  template<typename LabelType>
  double
  absloss(const Predictor* predictor, const SVM<LabelType>* svm)
  {
    double loss = 0;
    unsigned long rows = 0;

    for(auto it = svm->grouped_rows.begin(); it != svm->grouped_rows.end(); ++it)
    {
      for(auto row_it = (*it)->rows.begin(); row_it != (*it)->rows.end(); ++row_it)
      {
        double pred = predictor->predict((*row_it)->features);
        loss += std::abs((*it)->label.to_float() - pred);
        ++rows;
      }
    }

    return rows > 0 ? loss / rows : 0.0;
  }

  template<typename LabelType>
  PredArrayHolder_var
  labels(const SVM<LabelType>* svm)
  {
    PredArrayHolder_var res = new PredArrayHolder();
    res->values.resize(svm->size());

    unsigned long row_i = 0;
    for(auto group_it = svm->grouped_rows.begin();
      group_it != svm->grouped_rows.end(); ++group_it)
    {
      for(auto row_it = (*group_it)->rows.begin();
        row_it != (*group_it)->rows.end(); ++row_it, ++row_i)
      {
        res->values[row_i] = (*group_it)->label.orig() ? 1.0 : 0.0;
      }
    }

    return res;
  }

  void
  fill_logloss(
    std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> >& res_preds,
    const std::vector<std::pair<PredArrayHolder_var, PredArrayHolder_var> >& preds)
  {
    for(auto pred_it = preds.begin(); pred_it != preds.end(); ++pred_it)
    {
      PredArrayHolder_var res_first = new PredArrayHolder();
      res_first->values.resize(pred_it->first->values.size());

      auto res_it = res_first->values.begin();

      for(auto labeled_it = pred_it->first->values.begin();
        labeled_it != pred_it->first->values.end();
        ++labeled_it, ++res_it)
      {
        *res_it = - ::log(std::max(*labeled_it, LOGLOSS_EPS));
      }

      PredArrayHolder_var res_second = new PredArrayHolder();
      res_second->values.resize(pred_it->second->values.size());

      res_it = res_second->values.begin();

      for(auto unlabeled_it = pred_it->second->values.begin();
        unlabeled_it != pred_it->second->values.end();
        ++unlabeled_it, ++res_it)
      {
        *res_it = - ::log(1 - std::min(*unlabeled_it, 1 - LOGLOSS_EPS));
      }

      res_preds.push_back(std::make_pair(res_first, res_second));
    }
  }

  inline double
  avg_logloss(
    const std::vector<PredArrayHolder_var>& preds,
    const PredArrayHolder* labels)
  {
    double loss = 0;

    for(size_t i = 0; i < labels->values.size(); ++i)
    {
      double pred = 0.0;
      for(auto pred_it = preds.begin(); pred_it != preds.end(); ++pred_it)
      {
        pred += (*pred_it)->values[i];
      }
      pred /= preds.size();

      loss -= (1.0 - labels->values[i]) * ::log(1 - std::min(pred, 1 - LOGLOSS_EPS)) +
        labels->values[i] * ::log(std::max(pred, LOGLOSS_EPS));
    }

    return loss;
  }

  template<typename ContainerType>
  double
  eval_r(
    const ContainerType& exp_array,
    unsigned long unlabeled_sum,
    double point)
  {
    double f_val = -static_cast<double>(unlabeled_sum);

    for(auto exp_it = exp_array.begin(); exp_it != exp_array.end(); ++exp_it)
    {
      double r = exp_it->first * std::exp(point);
      double divider = 1.0 + r;
      f_val -= static_cast<double>(exp_it->second) * r / (divider * divider);
    }

    return f_val;
  }

  template<typename ContainerType>
  double
  eval_r_derivative(
    const ContainerType& exp_array,
    unsigned long unlabeled_sum,
    double point)
  {
    double f_val = static_cast<double>(unlabeled_sum);

    for(auto exp_it = exp_array.begin(); exp_it != exp_array.end(); ++exp_it)
    {
      double r = std::exp(exp_it->first + point);
      double divider = 1.0 + r;
      f_val -= static_cast<double>(exp_it->second) / divider;
    }

    return f_val;
  }

#if 0
  double
  solve_grouped_logloss_min(
    const std::deque<std::pair<double, unsigned long> >& exp_array,
    unsigned long unlabeled_sum)
  {
    const double Y_PRECISION = 0.0001;
    const double Y_MAX = 1000000;
    const double PRECISION = 0.001;
    const double X_MIN = -100;
    const double X_MAX = 100;
    [[maybe_unused]] const unsigned long MAX_ITERATIONS = 1000;

    if(unlabeled_sum == 0)
    {
      return X_MAX; // ?
    }

    if(exp_array.empty())
    {
      return 0.0;
    }

    double cur_y;
    double next_y = 0.01;
    unsigned long i = 0;

    do
    {
      cur_y = next_y;

      double f_val = -static_cast<double>(unlabeled_sum);
      double f_derivative = 0;
      for(auto exp_it = exp_array.begin(); exp_it != exp_array.end(); ++exp_it)
      {
        double divider = 1.0 + exp_it->first * cur_y * cur_y;
        f_val += static_cast<double>(exp_it->second) / divider;
        f_derivative += - 2.0 * static_cast<double>(exp_it->second) *
          exp_it->first * cur_y / (divider * divider);
      }

      next_y = cur_y - f_val / f_derivative;

      while(next_y <= 0.00001)
      {
        next_y = (cur_y + next_y) / 2;
      }

      ++i;

      assert(i < MAX_ITERATIONS);
    }
    while(std::abs(next_y - cur_y) > Y_PRECISION && next_y < Y_MAX);

    const double base_x = 2.0 * std::log(std::min(std::max(next_y, 0.00001), Y_MAX));

    double cur_x;
    double next_x = base_x;

    i = 0;

    do
    {
      cur_x = next_x;

      double f_val = -static_cast<double>(unlabeled_sum);
      double f_derivative = 0;
      for(auto exp_it = exp_array.begin(); exp_it != exp_array.end(); ++exp_it)
      {
        double r = exp_it->first * std::exp(cur_x);
        double divider = 1.0 + r;
        f_val += static_cast<double>(exp_it->second) / divider;
        f_derivative -= static_cast<double>(exp_it->second) * r / (divider * divider);
      }

      next_x = cur_x - f_val / f_derivative;
      //std::cout << "cur_x = " << cur_x << ", next_x = " << next_x << std::endl;

      ++i;

      assert(i < MAX_ITERATIONS);
    }
    while(std::abs(next_x - cur_x) > PRECISION && next_x > X_MIN && next_x < X_MAX);

    double res = std::min(std::max(next_x, X_MIN), X_MAX);

    /*
    std::cout << "solve_grouped_logloss_min(): res = " << res <<
      ", next_x = " << next_x <<
      ", exp_array.size = " << exp_array.size() <<
      ", exp_array: [";
    for(auto it = exp_array.begin(); it != exp_array.end(); ++it)
    {
      std::cout << "{" << it->first << "," << it->second << "}";
    }
    std::cout << "]" << std::endl;
    */
    return res;
  }
#endif

  template<typename ContainerType>
  double
  solve_grouped_logloss_min(
    const ContainerType& exp_array,
    unsigned long unlabeled_sum)
  {
    const double PRECISION = 0.00001;
    const double X_MIN = -100;
    const double X_MAX = 100;
    const unsigned long MAX_ITERATIONS = 1000;

    if(exp_array.empty())
    {
      return 0.0;
    }

    if(unlabeled_sum == 0)
    {
      return X_MAX; // ?
    }

    unsigned long count_sum = 0;
    for(auto exp_it = exp_array.begin(); exp_it != exp_array.end(); ++exp_it)
    {
      count_sum += exp_it->second;
    }

    if(unlabeled_sum == count_sum)
    {
      return X_MIN;
    }

    double left_x = X_MIN;
    double right_x = X_MAX;
    double left_f = eval_r_derivative(exp_array, unlabeled_sum, left_x);  // unlabeled - 1/(1+k*exp(-100)) < 0
    while(left_f > 0.0)
    {
      left_x *= 10.0;
      left_f = eval_r_derivative(exp_array, unlabeled_sum, left_x);
    }

    double right_f = eval_r_derivative(exp_array, unlabeled_sum, right_x); // unlabeled - 1/(1+k*exp(+100)) > 0
    while(right_f < 0.0)
    {
      right_x *= 10.0;
      right_f = eval_r_derivative(exp_array, unlabeled_sum, right_x);
    }

    unsigned long i = 0;

    if(!(left_f <= 0.0 && right_f >= 0.0))
    {
      std::cerr << "unlabeled_sum = " << unlabeled_sum <<
        ", exp_array.size = " << exp_array.size() <<
        ", left_x = " << left_x <<
        ", left_f = " << left_f <<
        ", right_x = " << right_x <<
        ", right_f = " << right_f << std::endl;
      assert(0);
    }

    i = 0;

    do
    {
      double div_x = (left_x + right_x) / 2;
      double div_f = eval_r_derivative(exp_array, unlabeled_sum, div_x);

      if(div_f <= 0)
      {
        left_x = div_x;
      }
      else // div_f > 0
      {
        right_x = div_x;
      }

      ++i;

      assert(i < MAX_ITERATIONS);
    }
    while(right_x - left_x > PRECISION);

    double res = (right_x + left_x) / 2;

    /*
    std::cout << "solve_grouped_logloss_min(): res = " << res <<
      ", res = " << res <<
      ", exp_array.size = " << exp_array.size() <<
      ", exp_array: [";
    for(auto it = exp_array.begin(); it != exp_array.end(); ++it)
    {
      std::cout << "{" << it->first << "," << it->second << "}";
    }
    std::cout << "]" << std::endl;
    */
    //std::cout << "res = " << res << std::endl;
    return res;
  }
}
}

#endif /*UTILS_HPP_*/
