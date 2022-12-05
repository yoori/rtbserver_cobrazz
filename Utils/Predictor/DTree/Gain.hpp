#ifndef GAIN_HPP_
#define GAIN_HPP_

#include <Generics/Singleton.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/SyncPolicy.hpp>

#include "Label.hpp"
#include "Utils.hpp"

namespace Vanga
{
  // LogLossGain
  class LogLossGain
  {
  public:
    LogLossGain();

    void
    start_count(double add_delta);

    void
    add_count(const BoolLabel& label, bool featured, unsigned long count);

    void
    start_eval();

    void
    add_eval(const BoolLabel& label, bool featured, unsigned long count);

    double
    featured_delta() const;

    double
    unfeatured_delta() const;

    double
    result() const;

  protected:
    double add_delta_;

    double unfeatured_sum_;
    unsigned long unfeatured_count_;
    double featured_sum_;
    unsigned long featured_count_;

    double featured_p_;
    double unfeatured_p_;
    double log_full_p_;
    double log_featured_p_;
    double log_unfeatured_p_;
    double inv_log_full_p_;
    double inv_log_featured_p_;
    double inv_log_unfeatured_p_;

    double old_logloss_;
    double new_logloss_;
    unsigned long loss_count_;
  };

  class PredBuffer;

  class PredBufferPtr;

  typedef ReferenceCounting::SmartPtr<PredBufferPtr> PredBufferPtr_var;

  struct DefaultPinaltyStrategy
  {
    double
    operator()(double gain) const
    {
      return gain;
    }
  };

  // PredictedLogLossGain
  template<typename PinaltyStrategyType = DefaultPinaltyStrategy>
  class PredictedLogLossGain
  {
  public:
    PredictedLogLossGain();

    void
    start_count(double add_delta);

    void
    add_count(const PredictedBoolLabel& label, bool featured, unsigned long count);

    void
    start_eval();

    void
    add_eval(const PredictedBoolLabel& label, bool featured, unsigned long count);

    double
    featured_delta() const;

    double
    unfeatured_delta() const;

    double
    result() const;

  protected:
    void
    add_pred_(
      PredBuffer& preds,
      double pred,
      unsigned long count);

  protected:
    PinaltyStrategyType pinalty_strategy_;

    double add_delta_;

    PredBufferPtr_var featured_preds_;
    PredBufferPtr_var unfeatured_preds_;
    unsigned long featured_unlabeled_;
    unsigned long unfeatured_unlabeled_;

    double featured_delta_x_;
    double unfeatured_delta_x_;
    double old_logloss_;
    double new_logloss_;
    unsigned long loss_count_;
  };
}

namespace Vanga
{
  // helpers for force buffers allocation
  struct PredBuffer:
    public std::vector<std::pair<double, unsigned long> >,
    public ReferenceCounting::AtomicImpl
  {
  protected:
    virtual ~PredBuffer() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<PredBuffer> PredBuffer_var;

  class PredBufferProviderImpl;

  typedef ReferenceCounting::SmartPtr<PredBufferProviderImpl>
    PredBufferProviderImpl_var;

  struct PredBufferPtr:
    public ReferenceCounting::AtomicImpl
  {
  public:
    PredBufferPtr(PredBufferProviderImpl* provider, PredBuffer_var& buffer)
      : provider_(ReferenceCounting::add_ref(provider))
    {
      buffer_.swap(buffer);
    };

    PredBuffer&
    buf()
    {
      return *buffer_;
    }

  protected:
    virtual ~PredBufferPtr() noexcept;

  protected:
    PredBufferProviderImpl_var provider_;
    PredBuffer_var buffer_;
  };

  class PredBufferProviderImpl: public ReferenceCounting::AtomicImpl
  {
    friend class PredBufferPtr;

  public:
    PredBufferProviderImpl() noexcept
    {
      pred_buffers_.reserve(128);
    }

    PredBufferPtr_var
    get()
    {
      PredBuffer_var res;

      {
        SyncPolicy::WriteGuard lock(lock_);
        if(!pred_buffers_.empty())
        {
          res.swap(pred_buffers_.back());
          pred_buffers_.pop_back();
        }
      }

      if(!res)
      {
        res = new PredBuffer();
        res->reserve(1024*1024);
      }

      return new PredBufferPtr(this, res);
    }

  protected:
    typedef Sync::Policy::PosixSpinThread SyncPolicy;

  protected:
    virtual ~PredBufferProviderImpl() noexcept
    {};

    void
    release_(PredBuffer_var& buffer)
    {
      buffer->clear();

      SyncPolicy::WriteGuard lock(lock_);
      pred_buffers_.push_back(PredBuffer_var());
      pred_buffers_.back().swap(buffer);
    }

  protected:
    SyncPolicy::Mutex lock_;
    std::vector<PredBuffer_var> pred_buffers_;
  };

  typedef Generics::Singleton<PredBufferProviderImpl, PredBufferProviderImpl_var>
    PredBufferProvider;

  inline
  PredBufferPtr::~PredBufferPtr() noexcept
  {
    provider_->release_(buffer_);
  }

  // LogLossGain impl
  inline
  LogLossGain::LogLossGain()
  {}

  inline void
  LogLossGain::start_count(double add_delta)
  {
    add_delta_ = add_delta;
    unfeatured_sum_ = 0.0;
    unfeatured_count_ = 0;
    featured_sum_ = 0.0;
    featured_count_ = 0;
    featured_p_ = 0.0;
    unfeatured_p_ = 0.0;
    log_full_p_ = 0.0;
    log_featured_p_ = 0.0;
    log_unfeatured_p_ = 0.0;
    inv_log_full_p_ = 0.0;
    inv_log_featured_p_ = 0.0;
    inv_log_unfeatured_p_ = 0.0;
    // losses
    old_logloss_ = 0.0;
    new_logloss_ = 0.0;
    loss_count_ = 0;
  }

  inline void
  LogLossGain::add_count(const BoolLabel& label, bool featured, unsigned long count)
  {
    if(!featured)
    {
      if(label.value)
      {
        unfeatured_sum_ += count;
      }

      unfeatured_count_ += count;
    }
    else
    {
      if(label.value)
      {
        featured_sum_ += count;
      }

      featured_count_ += count;
    }
  }

  inline void
  LogLossGain::start_eval()
  {
    const double EPS = 0.0000001;
    const double LABEL_DOUBLE_MIN = EPS;
    const double LABEL_DOUBLE_MAX = 1 - EPS;

    double full_p = featured_count_ > 0 || unfeatured_count_ > 0 ?
      std::min(
        std::max(
          (featured_sum_ + unfeatured_sum_) / (featured_count_ + unfeatured_count_),
          LABEL_DOUBLE_MIN),
        LABEL_DOUBLE_MAX) :
      0.0;

    featured_p_ = featured_count_ > 0 ?
      std::min(std::max(featured_sum_ / featured_count_, LABEL_DOUBLE_MIN), LABEL_DOUBLE_MAX) :
      0.0;

    unfeatured_p_ = unfeatured_count_ > 0 ?
      std::min(std::max(unfeatured_sum_ / unfeatured_count_, LABEL_DOUBLE_MIN), LABEL_DOUBLE_MAX) :
      0.0;

    log_full_p_ = std::log(full_p);
    log_featured_p_ = std::log(featured_p_);
    log_unfeatured_p_ = std::log(unfeatured_p_);
    inv_log_full_p_ = std::log(1.0 - full_p);
    inv_log_featured_p_ = std::log(1.0 - featured_p_);
    inv_log_unfeatured_p_ = std::log(1.0 - unfeatured_p_);
  }

  inline void
  LogLossGain::add_eval(const BoolLabel& label, bool featured, unsigned long count)
  {
    if(label.value)
    {
      old_logloss_ -= log_full_p_ * count;
      new_logloss_ -= (featured ? log_featured_p_ : log_unfeatured_p_) * count;
    }
    else
    {
      old_logloss_ -= inv_log_full_p_ * count;
      new_logloss_ -= (featured ? inv_log_featured_p_ : inv_log_unfeatured_p_) * count;
    }

    loss_count_ += count;
  }

  inline double
  LogLossGain::featured_delta() const
  {
    return featured_p_ - add_delta_;
  }

  double
  LogLossGain::unfeatured_delta() const
  {
    return unfeatured_p_ - add_delta_;
  }

  inline double
  LogLossGain::result() const
  {
    return new_logloss_ - old_logloss_;
    //return loss_count_ > 0 ? (new_logloss_ - old_logloss_) / loss_count_ : 0.0;
  }

  // PredictedLogLossGain impl
  template<typename PinaltyStrategyType>
  PredictedLogLossGain<PinaltyStrategyType>::PredictedLogLossGain()
    : featured_preds_(PredBufferProvider::instance().get()),
      unfeatured_preds_(PredBufferProvider::instance().get())
  {}

  template<typename PinaltyStrategyType>
  void
  PredictedLogLossGain<PinaltyStrategyType>::start_count(double add_delta)
  {
    add_delta_ = add_delta;
    featured_preds_->buf().clear();
    unfeatured_preds_->buf().clear();
    featured_unlabeled_ = 0;
    unfeatured_unlabeled_ = 0;
    featured_delta_x_ = 0.0;
    unfeatured_delta_x_ = 0.0;
    old_logloss_ = 0.0;
    new_logloss_ = 0.0;
    loss_count_ = 0;
  }

  template<typename PinaltyStrategyType>
  void
  PredictedLogLossGain<PinaltyStrategyType>::add_count(
    const PredictedBoolLabel& label,
    bool featured,
    unsigned long count)
  {
    assert(count != 0);

    if(!label.orig())
    {
      if(featured)
      {
        featured_unlabeled_ += count;
      }
      else
      {
        unfeatured_unlabeled_ += count;
      }
    }

    if(featured)
    {
      /*
      std::cout << "add_pred_(featured): label = " << label.orig() <<
        ", val = " << (label.pred + add_delta_) <<
        ", count = " << count <<
        ", add_delta_ = " << add_delta_ << std::endl;
      */
      add_pred_(featured_preds_->buf(), label.pred, count);
    }
    else
    {
      /*
      std::cout << "add_pred_(unfeatured): label = " << label.orig() <<
        ", val = " << (label.pred + add_delta_) <<
        ", count = " << count <<
        ", add_delta_ = " << add_delta_ << std::endl;
      */
      add_pred_(unfeatured_preds_->buf(), label.pred, count);
    }
  }

  template<typename PinaltyStrategyType>
  void
  PredictedLogLossGain<PinaltyStrategyType>::start_eval()
  {
    // eval delta x
    featured_delta_x_ = Utils::solve_grouped_logloss_min(
      featured_preds_->buf(),
      featured_unlabeled_);

    unfeatured_delta_x_ = Utils::solve_grouped_logloss_min(
      unfeatured_preds_->buf(),
      unfeatured_unlabeled_);

    /*
    std::cout << "start_eval: featured_delta_x_ = " << featured_delta_x_ <<
      ", featured_unlabeled_ = " << featured_unlabeled_ <<
      ", unfeatured_delta_x_ = " << unfeatured_delta_x_ <<
      ", unfeatured_unlabeled_ = " << unfeatured_unlabeled_ <<
      ", featured_preds_ = [";
    for(auto it = featured_preds_.begin(); it != featured_preds_.end(); ++it)
    {
      std::cout << "{" << it->first << "," << it->second << "}";
    }
    std::cout << "], unfeatured_preds_ = [";
    for(auto it = unfeatured_preds_.begin(); it != unfeatured_preds_.end(); ++it)
    {
      std::cout << "{" << it->first << "," << it->second << "}";
    }
    std::cout << "]" << std::endl;
    */
  }

  template<typename PinaltyStrategyType>
  void
  PredictedLogLossGain<PinaltyStrategyType>::add_eval(
    const PredictedBoolLabel& label,
    bool featured,
    unsigned long count)
  {
    const double DOUBLE_ONE = 1.0;
    //const double EPS = 0.0000001;

    // reconstruct x by label.pred
    /*
    double pred = std::min(std::max(label.pred, EPS), 1 - EPS);
    double x = std::log(pred / (DOUBLE_ONE - pred));
    */
    double x = label.pred + add_delta_;
    double pred = DOUBLE_ONE / (DOUBLE_ONE + std::exp(-(x + add_delta_)));
    double new_p;

    if(featured)
    {
      new_p = DOUBLE_ONE / (
        DOUBLE_ONE + std::exp(-(x + featured_delta_x_)));
    }
    else
    {
      new_p = DOUBLE_ONE / (
        DOUBLE_ONE + std::exp(-(x + unfeatured_delta_x_)));
    }

    if(label.orig())
    {
      old_logloss_ -= std::log(pred) * count;
      new_logloss_ -= std::log(new_p) * count;
    }
    else
    {
      old_logloss_ -= std::log(DOUBLE_ONE - pred) * count;
      new_logloss_ -= std::log(DOUBLE_ONE - new_p) * count;
    }

    loss_count_ += count;
  }

  template<typename PinaltyStrategyType>
  double
  PredictedLogLossGain<PinaltyStrategyType>::featured_delta() const
  {
    return featured_delta_x_;
  }

  template<typename PinaltyStrategyType>
  double
  PredictedLogLossGain<PinaltyStrategyType>::unfeatured_delta() const
  {
    return unfeatured_delta_x_;
  }

  template<typename PinaltyStrategyType>
  double
  PredictedLogLossGain<PinaltyStrategyType>::result() const
  {
    /*
    std::cerr << "featured_delta_x_ = " << featured_delta_x_ <<
      ", unfeatured_delta_x_ = " << unfeatured_delta_x_ <<
      ", old_logloss_ = " << old_logloss_ <<
      ", new_logloss_ = " << new_logloss_ <<
      ", featured_p = " << (1.0 / (1.0 + std::exp(-featured_delta_x_))) <<
      ", unfeatured_p = " << (1.0 / (1.0 + std::exp(-unfeatured_delta_x_))) <<
      std::endl;
    */
    //return loss_count_ > 0 ? (new_logloss_ - old_logloss_) / loss_count_ : 0.0;
    return pinalty_strategy_(new_logloss_ - old_logloss_);
  }

  template<typename PinaltyStrategyType>
  void
  PredictedLogLossGain<PinaltyStrategyType>::add_pred_(
    PredBuffer& preds,
    double pred,
    unsigned long count)
  {
    const double val = pred + add_delta_;
    preds.push_back(std::make_pair(val, count));
  }
}

#endif /*GAIN_HPP_*/
