#ifndef AD_CHANNEL_SVCS_CHANNEL_SESSION_FACTORY_HPP_
#define AD_CHANNEL_SVCS_CHANNEL_SESSION_FACTORY_HPP_

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Sync/SyncPolicy.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/ObjectPool.hpp>

#include <ChannelSvcs/ChannelManagerController/ChannelManagerController.hpp>

namespace AdServer
{
namespace ChannelSvcs
{

  namespace Protected
  {
    class Task :public Generics::TaskImpl
    {
    };
    typedef ReferenceCounting::SmartPtr<Task> Task_var;
  }

  /**
   * NullStat
   * implementation of empty statistic class
   */

  class NullStat
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    NullStat(){};
    virtual ~NullStat(){};

    virtual void prepare(Generics::Timer& /*timer*/){};

    virtual void consider(
      size_t /*index*/, size_t /*num*/, Generics::Timer& /*timer*/)
      noexcept{};

    virtual void check() noexcept {};

    virtual void dump() noexcept {};

  };

  typedef std::unique_ptr<NullStat> Stat_var;
  /**
   * TimedQueryStat
   * implementation of statistic for matching
   */

  class TimedQueryStat: public NullStat
  {
  public:

    DECLARE_EXCEPTION(InvalidArgument, eh::DescriptiveException);

    TimedQueryStat(
      Logging::Logger* logger,
      Generics::TaskRunner* task_runner,
      unsigned long dump_margin,
      const std::vector<size_t>& lengths)
      /*throw(InvalidArgument, eh::Exception)*/;

    virtual ~TimedQueryStat()noexcept{};

    virtual void prepare(Generics::Timer& timer);

    virtual void consider(size_t index, size_t num, Generics::Timer& timer)
      noexcept;

    virtual void check() noexcept;

    virtual void dump() noexcept;


    static NullStat* CreateStatistic(
      Logging::Logger* logger,
      Generics::TaskRunner* task_runner,
      unsigned long dump_margin,
      const std::vector<size_t>& lengths)
      /*throw(eh::Exception)*/;

    static const char* generate_name(size_t index, size_t num, char name[64])
      noexcept;

  protected:

    bool need_dump_() noexcept;

  private:
    typedef Sync::PosixRWLock Mutex_;
    typedef Sync::PosixRGuard ReadGuard_;
    typedef Sync::PosixWGuard WriteGuard_;

    class StatisticTask : public Protected::Task
    {
    public:
      StatisticTask(TimedQueryStat* callback) noexcept;
      ~StatisticTask() noexcept{};
      virtual void execute() noexcept;
    private:
      TimedQueryStat* callback_;
    };

  private:
    Logging::Logger_var logger_;
    Generics::Statistics::Collection_var collection_;
    Generics::Statistics::DumpPolicy_var policy_;
    Generics::TaskRunner* task_runner_;
    unsigned long counter_;
    mutable Mutex_ lock_;
    unsigned long dump_margin_;
    static const char* ASPECT;
  };

}
}

  /**
   * ChannelServerSessionFactoryImpl
   * implementation of corba valuetype factory
   */
class ChannelServerSessionFactoryImpl :
  public virtual Generics::CompositeActiveObject,
  public virtual CORBACommons::ReferenceCounting::CorbaRefCountImpl<
    CORBA::ValueFactoryBase>
{
public:
  ChannelServerSessionFactoryImpl(
    Logging::Logger* stat_logger,
    unsigned long count_threads,
    Generics::ActiveObjectCallback* callback,
    unsigned long check_period)
    noexcept;

  virtual ~ChannelServerSessionFactoryImpl() noexcept;

  virtual CORBA::ValueBase* create_for_unmarshal();

  virtual void report_error(
    Generics::ActiveObjectCallback::Severity severity,
    const char* description,
    const char* error_code = 0) noexcept;

private:
  typedef Sync::PosixMutex Mutex_;
  typedef Sync::PosixGuard ReadGuard_;
  typedef Sync::PosixGuard WriteGuard_;

private:
  Logging::Logger_var stat_logger_;
  Generics::ActiveObjectCallback_var callback_;
  Generics::TaskRunner_var task_runner_;
  unsigned long check_period_;
  static const char* ASPECT;
};

typedef ReferenceCounting::SmartPtr<ChannelServerSessionFactoryImpl>
  ChannelServerSessionFactoryImpl_var;

namespace AdServer
{
namespace ChannelSvcs
{
  /**
   * ChannelServerSessionImpl
   * implementation of ChannelServerSession valuetype
   */
  class ChannelServerSessionImpl:
    public virtual OBV_AdServer::ChannelSvcs::ChannelServerSession
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(BadParameter, eh::DescriptiveException);

    ChannelServerSessionImpl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* stat_logger,
      Generics::TaskRunner* runner,
      unsigned long check_period)
      /*throw(eh::Exception)*/;

    ChannelServerSessionImpl(
      ChannelServerSessionImpl& init)
      /*throw(Exception)*/;

    ChannelServerSessionImpl(
      const AdServer::ChannelSvcs::Protected::GroupDescriptionSeq&
        servers)
      /*throw(Exception)*/;

    virtual ~ChannelServerSessionImpl() noexcept;


    virtual CORBA::ValueBase* _copy_value();

    virtual void report_error(
      Generics::ActiveObjectCallback::Severity severity,
      const char* description,
      const char* error_code = 0) noexcept;

    //
    // IDL:AdServer/ChannelSvcs/ChannelServer/match:1.0
    //
    virtual void match(
      const ::AdServer::ChannelSvcs::ChannelServerBase::MatchQuery& query,
      ::AdServer::ChannelSvcs::ChannelServerBase::MatchResult_out result)
      /*throw(AdServer::ChannelSvcs::ImplementationException,
        AdServer::ChannelSvcs::NotConfigured)*/;

    //
    // IDL:AdServer/ChannelSvcs/ChannelServer/match:1.0
    //
    virtual ChannelServerBase::CCGKeywordSeq* get_ccg_traits(
     const ::AdServer::ChannelSvcs::ChannelIdSeq& query)
     /*throw(AdServer::ChannelSvcs::ImplementationException,
       AdServer::ChannelSvcs::NotConfigured)*/;

  private:
    typedef std::vector<ChannelSvcs::ChannelServer::MatchResult_var>
      ResultsVector;
    typedef Sync::PosixMutex Mutex_;
    typedef Sync::PosixGuard WriteGuard_;

    class GroupPoolConfiguration :
      public CORBACommons::ObjectPoolConfiguration<CORBA::ULong, CORBA::ULong>
    {
    public:

      GroupPoolConfiguration() noexcept {};

      GroupPoolConfiguration(
        const Protected::GroupDescriptionSeq& groups,
        const Generics::Time& period)
        noexcept;

      struct Resolver
      {
        template <typename T>
        T resolve(CORBA::ULong ref) noexcept;
      };

      Resolver resolver;
    };

    typedef class CORBACommons::ObjectPool<CORBA::ULong,
            GroupPoolConfiguration,
             CORBACommons::ObjectPlainVar<CORBA::ULong>,
            false
            > GroupPool;

    typedef GroupPool::ObjectHandlerType ObjectHandlerType;

    void init_stat_() noexcept;

    GroupPool::ObjectHandlerType get_item_() /*throw(GroupPool::Exception)*/;

    typedef std::list<size_t> ListIndex;
    typedef std::vector<size_t> VectorOffset;

    template<class T, class Value>
    static size_t merge_channels_(const T& value, T& result)
      /*throw(eh::Exception)*/;

    static void compose_result_(
      ResultsVector& results,
      ::AdServer::ChannelSvcs::ChannelServerBase::MatchResult* out)
      /*throw(eh::Exception)*/;

    /*
    static void move_result_value(
      AdServer::ChannelSvcs::ChannelServerBase::TriggerMatchResult& from,
      AdServer::ChannelSvcs::ChannelServerBase::TriggerMatchResult& to)
      noexcept;*/

    static void merge_content_(
      ResultsVector& results,
      ::AdServer::ChannelSvcs::ChannelServerBase::MatchResult* out)
      /*throw(eh::Exception)*/;

    template<class ADAPTER>
    void merge_unmatched_(
      const ADAPTER& adapter,
      ResultsVector& results,
      ::AdServer::ChannelSvcs::ChannelServerBase::MatchResult* out)
      /*throw(eh::Exception)*/;

    class ContentIndex
    {
    public:
      size_t size(const ChannelSvcs::ChannelServer::MatchResult& result) const
        noexcept;
      unsigned long id(
        const ChannelSvcs::ChannelServer::MatchResult& result,
        size_t index = 0) const
        noexcept;

      const ChannelSvcs::ChannelServerBase::ContentChannelAtom& elem(
        ChannelSvcs::ChannelServer::MatchResult& result,
        size_t index = 0) const
        noexcept;
    };


    template<class INDEX, typename ELEM>
    class ResultsAdapter
    {
    public:

      ResultsAdapter(ResultsVector& results)
        /*throw(eh::Exception)*/;

      bool empty() const noexcept { return sort_index_.empty(); }

      const ELEM& elem() const /*throw(BadParameter)*/;

      void next() noexcept;

      size_t size() const noexcept;

    private:
      void create_index_() /*throw(eh::Exception)*/;

    private:
      INDEX index_;
      ResultsVector& results_;
      VectorOffset sort_off_;//current offsets in buffers
      ListIndex sort_index_;//indexes of buffers sorted by ids
      size_t all_length_;
    };


    typedef std::unique_ptr<GroupPool> GroupPool_var;

  private:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var stat_logger_;
    Generics::TaskRunner_var task_runner_;
    unsigned long check_period_;
    Stat_var stat_;
    GroupPool_var pool_;
    Mutex_ init_lock_;
    static const char* ASPECT;
    static const unsigned long stat_dump_margin;
  };

} /* ChannelSvcs */
} /* AdServer */

namespace AdServer
{
namespace ChannelSvcs
{
  inline
  bool operator==(
    const ChannelServerBase::ChannelAtom& v1,
    const ChannelServerBase::ChannelAtom& v2) 
  {
    return (v1.id == v2.id && v1.trigger_channel_id == v2.trigger_channel_id);
  }

  inline
  bool operator<(
    const ChannelServerBase::ChannelAtom& v1,
    const ChannelServerBase::ChannelAtom& v2) 
  {
    if(v1.id == v2.id)
    {
      return (v1.trigger_channel_id < v2.trigger_channel_id);
    }
    return (v1.id < v2.id);
  }

  template<class T, class Value>
  size_t ChannelServerSessionImpl::merge_channels_(const T& value, T& result)
    /*throw(eh::Exception)*/
  {
    unsigned long value_length = value.length();
    if(value_length)
    {
      unsigned long old_length, new_length;
      old_length = result.length();
      new_length = old_length + value_length;
      result.length(new_length);
      std::copy(
          value.get_buffer(),
          value.get_buffer() + value_length,
          result.get_buffer() + old_length);
      if(old_length)
      {
        std::inplace_merge(
            result.get_buffer(),
            result.get_buffer() + old_length,
            result.get_buffer() + new_length);
        Value* end = std::unique(
            result.get_buffer(),
            result.get_buffer() + new_length);
        result.length(end - result.get_buffer());
      }
    }
    return result.length();
  }

  inline
  size_t ChannelServerSessionImpl::ContentIndex::size(
    const ChannelSvcs::ChannelServer::MatchResult& result) const
    noexcept
  {
    return result.content_channels.length();
  }

  inline
  unsigned long ChannelServerSessionImpl::ContentIndex::id(
    const ChannelSvcs::ChannelServer::MatchResult& result,
    size_t index) const
    noexcept
  {
    return result.content_channels[index].id;
  }

  inline
  const ChannelSvcs::ChannelServerBase::ContentChannelAtom&
  ChannelServerSessionImpl::ContentIndex::elem(
    ChannelSvcs::ChannelServer::MatchResult& result,
    size_t index) const
    noexcept
  {
    return result.content_channels[index];
  }

  template <typename T>
  T
  ChannelServerSessionImpl::GroupPoolConfiguration::Resolver::resolve(CORBA::ULong ref)
    noexcept
  {
    return ref;
  }

} /* ChannelSvcs */
} /* AdServer */


#endif /*AD_CHANNEL_SVCS_CHANNEL_SESSION_FACTORY_HPP_*/
