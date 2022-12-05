/**
 * @file RequestInfoSvcs/RequestInfoContainerTest.cpp
 */

#include <cmath>
#include <map>

#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>

#include <TestCommons/MTTester.hpp>

#include <RequestInfoSvcs/RequestInfoManager/RequestInfoContainer.hpp>
#include <RequestInfoSvcs/RequestInfoManager/CompositeRequestOperationProcessor.hpp>

#include "RequestInfoContainerMTTest.hpp"

using namespace AdServer::RequestInfoSvcs;

namespace
{
  /**
   * Error level of test should be equal to the number of errors
   * occured. Below two stream manipulators are using for errors
   * counting. Application is too complex, for another ways for
   * errors calculation.
   */
  /// Get index for std::cerr manipulator - errors counter,
  const int ERRORS_AMOUNT_INDEX = std::ios_base::xalloc();
  const RevenueDecimal PUB_CURRENCY_RATE("3");
  const RevenueDecimal BASE_PUB_IMP_SYS_REVENUE("0.3"); // bid cost
  const RevenueDecimal PUB_IMP_SYS_REVENUE("0.1"); // cost after truncating by publisher

  const RevenueDecimal BASE_PUB_IMP_REVENUE = RevenueDecimal::mul(
    BASE_PUB_IMP_SYS_REVENUE, PUB_CURRENCY_RATE, Generics::DMR_CEIL); // 0.9

  // cost after impression for RT_ABSOLUTE case
  const RevenueDecimal PUB_IMP_REVENUE = RevenueDecimal::mul(
    PUB_IMP_SYS_REVENUE, PUB_CURRENCY_RATE, Generics::DMR_CEIL); // 0.3

  const RevenueDecimal PUB_SHARE("0.2");
  // cost after impression for RT_SHARE case
  const RevenueDecimal PUB_SHARE_REVENUE(
    RevenueDecimal::mul(BASE_PUB_IMP_REVENUE, PUB_SHARE, Generics::DMR_CEIL));
  const RevenueDecimal PUB_SHARE_SYS_REVENUE(
    RevenueDecimal::mul(BASE_PUB_IMP_SYS_REVENUE, PUB_SHARE, Generics::DMR_CEIL));

  /**
   * Put this manipulator to std::cerr and they increment errors counter
   */
  std::ios_base&
  add_error(std::ios_base& iosbase) noexcept
  {
    ++iosbase.iword(ERRORS_AMOUNT_INDEX);
    return iosbase;
  }

  /**
   * This manipulator get the number of errors occured (counter value)
   */
  int
  get_errors(std::ios_base& iosbase) noexcept
  {
    return iosbase.iword(ERRORS_AMOUNT_INDEX);
  }


  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char TEST_FOLDER[] = "/RequestInfoContainerTestDir/";

  const char USAGE[] =
    "RequestInfoContainerTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";

  const long REQUEST_OP = -1;
  //const long CUSTOM_ACT_OP = -2;
  const long IMPRESSION_OP = -3;
  const long NOTICE_OP = -4;

  bool
  double_compare(double left, double right)
  {
    return ::fabs(left - right) < 0.00001;
  }

  Generics::Uuid
  generate_user_id(bool impression = false)
  {
    Generics::Uuid r = Generics::Uuid::create_random_based();
    *r.begin() = static_cast<unsigned char>(impression ? 'I' : 'R');
    return r;
  }

  struct CustomActionKey
  {
    CustomActionKey(
      unsigned long action_id_val,
      const AdServer::Commons::UserId& action_request_id_val,
      const char* referer_val)
      : action_id(action_id_val),
        action_request_id(action_request_id_val),
        referer(referer_val)
    {}

    bool
    operator <(const CustomActionKey& right) const
    {
      return
        (action_id < right.action_id ||
          (action_id == right.action_id &&
          (action_request_id < right.action_request_id ||
           (action_request_id == right.action_request_id &&
             referer < right.referer))));
    }

    bool
    operator ==(const CustomActionKey& right) const
    {
      return
        action_id == right.action_id &&
        action_request_id == right.action_request_id &&
        referer == right.referer;
    }

    bool
    operator !=(const CustomActionKey& right) const
    {
      return !(*this == right);
    }

    unsigned long action_id;
    AdServer::Commons::UserId action_request_id;
    std::string referer;
  };

  typedef std::set<CustomActionKey> CustomActionSet;

  struct Counter
  {
    Counter()
      : requests(0),
        impressions(0),
        clicks(0),
        actions(0),
        prime_requests(0), // requests without rollback's and resave
        double_impressions(0),
        double_clicks(0)
    {}

    Counter(
      int requests_val,
      int impressions_val,
      int clicks_val,
      int actions_val,
      int prime_requests_val,
      int double_impressions_val,
      int double_clicks_val,
      const CustomActionSet& ca = CustomActionSet())
      : requests(requests_val),
        impressions(impressions_val),
        clicks(clicks_val),
        actions(actions_val),
        prime_requests(prime_requests_val),
        double_impressions(double_impressions_val),
        double_clicks(double_clicks_val),
        custom_actions(ca)
    {}

    Counter&
    operator+=(const Counter& right)
    {
      requests += right.requests;
      impressions += right.impressions;
      clicks += right.clicks;
      actions += right.actions;
      prime_requests += right.prime_requests;
      double_impressions += right.double_impressions;
      double_clicks += right.double_clicks;
      return *this;
    }

    bool
    operator ==(const Counter& right) const
    {
      if (custom_actions.size() != right.custom_actions.size())
      {
        return false;
      }

      CustomActionSet::const_iterator right_ca_it =
        right.custom_actions.begin();

      for (CustomActionSet::const_iterator left_ca_it =
            custom_actions.begin();
          left_ca_it != custom_actions.end();
          ++left_ca_it, ++right_ca_it)
      {
        if (*left_ca_it != *right_ca_it)
        {
          return false;
        }
      }

      return
        requests == right.requests &&
        impressions == right.impressions &&
        clicks == right.clicks &&
        actions == right.actions &&
        prime_requests == right.prime_requests &&
        double_impressions == right.double_impressions &&
        double_clicks == right.double_clicks;
    }

    bool
    operator !=(const Counter& right) const
    {
      return !(*this == right);
    }

    int requests;
    int impressions;
    int clicks;
    int actions;
    int prime_requests;
    int double_impressions;
    int double_clicks;

    CustomActionSet custom_actions;
  };

  std::ostream&
  operator <<(std::ostream& ostr, const Counter& counter)
  {
    ostr << "{ requests = " << counter.requests <<
      ", impressions = " << counter.impressions <<
      ", clicks = " << counter.clicks <<
      ", actions = " << counter.actions <<
      ", prime_requests = " << counter.prime_requests <<
      ", double_impressions = " << counter.double_impressions <<
      ", double_clicks = " << counter.double_clicks <<
      ", custom_actions = ";

    for (CustomActionSet::const_iterator ca_it =
      counter.custom_actions.begin();
        ca_it != counter.custom_actions.end(); ++ca_it)
    {
      ostr << "[" << ca_it->action_id << ", " <<
        ca_it->action_request_id << ", " <<
        ca_it->referer << "]";
    }

    ostr << " }";

    return ostr;
  }

  struct TestRequestActionProcessorImpl :
    public virtual RequestActionProcessor,
    public virtual ReferenceCounting::AtomicImpl,
    public std::map<AdServer::Commons::RequestId, Counter>
  {
    TestRequestActionProcessorImpl(
      RequestActionProcessor* aggregate_processor = nullptr,
      const char* name = "")
      : assert_on_non_request_(false),
        aggregate_processor_(ReferenceCounting::add_ref(aggregate_processor)),
        name_(name),
        current_test_("none")
    {}

    void
    set_current_test(const char* cur_test)
    {
      current_test_ = cur_test;
    }

    std::string
    get_current_test() const
    {
      return current_test_;
    }

    void
    check_expression(const RequestInfo& ri)
    {
      if (ri.expression != "TEST-SHARE" && ri.expression != "TEST-ABSOLUTE")
      {
        Stream::Error ostr;
        ostr << add_error << "request - lost expression: " <<
          ri.expression << "'";
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    void
    check_user_id(const Generics::Uuid& user_id, bool impression = false)
    {
      if(impression)
      {
        if(*user_id.begin() != 'I')
        {
          Stream::Error ostr;
          ostr << "request user_id isn't rewritten: '" <<
            static_cast<char>(*user_id.begin()) << "'";
          throw RequestActionProcessor::Exception(ostr);
        }
      }
      else
      {
        if(*user_id.begin() != 'R')
        {
          Stream::Error ostr;
          ostr << "impression user_id unexpected";
          throw RequestActionProcessor::Exception(ostr);
        }
      }
    }

    void
    check_pub_revenue(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
    {
      const RevenueDecimal& expected_pub_imp_revenue =
        (ri.expression == "TEST-ABSOLUTE" ? PUB_IMP_REVENUE : PUB_SHARE_REVENUE); // 0.3, 
      const RevenueDecimal& expected_pub_imp_sys_revenue =
        (ri.expression == "TEST-ABSOLUTE" ? PUB_IMP_SYS_REVENUE : PUB_SHARE_SYS_REVENUE); // 0.1,

      RevenueDecimal pub_sys_impression = ri.pub_revenue.sys_impression();
      if (ri.pub_revenue.impression != expected_pub_imp_revenue ||
        pub_sys_impression != expected_pub_imp_sys_revenue)
      {
        std::cerr << get_current_test() << ": impression - incorrect pub revenue (" <<
          RequestInfo::request_state_string(processing_state.state) << "):" <<
          std::endl;
        if(ri.pub_revenue.impression != expected_pub_imp_revenue)
        {
          std::cerr << "  pub revenue = " << ri.pub_revenue.impression <<
            " instead " << expected_pub_imp_revenue << std::endl;
        }

        if(pub_sys_impression != expected_pub_imp_sys_revenue)
        {
          std::cerr << "  pub sys revenue = " << pub_sys_impression <<
            " instead " << expected_pub_imp_sys_revenue << std::endl;
        }

        ri.print(std::cerr, "  ");
        std::cerr << add_error << std::endl;
      }
    }

    virtual void
    process_request(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      check_expression(ri);

      const RevenueDecimal etal("0.1");

      if (ri.adv_revenue.impression != etal ||
        ri.adv_revenue.click != etal ||
        ri.adv_revenue.action != etal)
      {
        std::cerr << "request - lost adv revenue :" << std::endl;
        ri.print(std::cerr, "  ");
        std::cerr << add_error << std::endl;
      }

      if (!ri.channels.empty())
      {
        Stream::Error ostr;

        ostr << "request - non empty channels: ";
        for (RequestInfo::ChannelIdList::const_iterator ch_it =
         ri.channels.begin();
            ch_it != ri.channels.end();
            ++ch_it)
        {
          ostr << *ch_it << std::endl;
        }
        throw RequestActionProcessor::Exception(ostr);
      }

      if(aggregate_processor_)
      {
        aggregate_processor_->process_request(ri, processing_state);
      }

      SyncPolicy::WriteGuard lock(lock_);
      Counter& counter = (*this)[ri.request_id];

      if(processing_state.state == RequestInfo::RS_NORMAL)
      {
        ++counter.prime_requests;
      }

      //int old_requests = counter.requests;

      if(processing_state.state == RequestInfo::RS_NORMAL ||
        processing_state.state == RequestInfo::RS_RESAVE)
      {
        ++counter.requests;
      }
      else
      {
        --counter.requests;
      }

      /*
      std::cerr << "process_request(" << name_ << "): ";
      RequestInfo::print_request_state(std::cerr, processing_state.state);
      std::cerr << ", " << old_requests << " => " << counter.requests <<
        ", rid = " << ri.request_id << std::endl;
      */
    }

    virtual void
    process_impression(
      const RequestInfo& ri,
      const ImpressionInfo& imp_info,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      /*
      std::cerr << "TestRequestActionProcessorImpl::process_impression(): "
        "this = " << this <<
        ", state = " << RequestInfo::request_state_string(processing_state.state) << std::endl;
      */

      assert(!assert_on_non_request_ || processing_state.state == RequestInfo::RS_DUPLICATE);

      check_expression(ri);
      check_user_id(ri.user_id, ri.device_channel_id);

      if(ri.request_id.is_null())
      {
        std::cerr << add_error << "impression - null request id" <<
          std::endl;
      }

      const RevenueDecimal etal("0.1");

      if (ri.adv_revenue.impression != etal ||
        ri.adv_revenue.click != etal ||
        ri.adv_revenue.action != etal)
      {
        std::cerr << "impression - lost adv revenue :" << std::endl;
        ri.print(std::cerr, "  ");
        std::cerr << add_error << std::endl;
      }

      check_pub_revenue(ri, processing_state);

      if (!ri.channels.empty())
      {
        Stream::Error ostr;

        ostr << "impression - non empty channels: ";
        for (RequestInfo::ChannelIdList::const_iterator ch_it =
          ri.channels.begin();
            ch_it != ri.channels.end();
            ++ch_it)
        {
          ostr << *ch_it << std::endl;
        }
        throw RequestActionProcessor::Exception(ostr);
      }

      if(aggregate_processor_)
      {
        aggregate_processor_->process_impression(ri, imp_info, processing_state);
      }

      //std::cerr << "imp: " << RequestInfo::request_state_string(processing_state.state) << std::endl;

      SyncPolicy::WriteGuard lock(lock_);
      if(processing_state.state != RequestInfo::RS_DUPLICATE &&
        processing_state.state != RequestInfo::RS_ROLLBACK)
      {
        ++(*this)[ri.request_id].impressions;
      }
      else
      {
        ++(*this)[ri.request_id].double_impressions;
      }
    }

    virtual void
    process_click(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      try
      {
        assert(!assert_on_non_request_ || processing_state.state == RequestInfo::RS_DUPLICATE);

        check_expression(ri);
        check_user_id(ri.user_id, ri.device_channel_id);

        RevenueDecimal etal("0.1");

        if (ri.adv_revenue.impression != etal ||
          ri.adv_revenue.click != etal ||
          ri.adv_revenue.action != etal)
        {
          std::cerr << "click - lost revenue, instead (0.1, 0.1, 0.1) :" << std::endl;
          ri.print(std::cerr, "  ");
          std::cerr << add_error << std::endl;
        }

        check_pub_revenue(ri, processing_state);

        if(aggregate_processor_)
        {
          aggregate_processor_->process_click(ri, processing_state);
        }

        SyncPolicy::WriteGuard lock(lock_);
        if(processing_state.state != RequestInfo::RS_DUPLICATE &&
          processing_state.state != RequestInfo::RS_ROLLBACK)
        {
          ++(*this)[ri.request_id].clicks;
        }
        else
        {
          ++(*this)[ri.request_id].double_clicks;
        }
      }
      catch (const RequestActionProcessor::Exception&)
      {
        throw;
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FNS << ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    virtual void
    process_action(const RequestInfo& ri)
      /*throw(RequestActionProcessor::Exception)*/
    {
      try
      {
        assert(!assert_on_non_request_);

        check_expression(ri);
        check_user_id(ri.user_id, ri.device_channel_id);

        const RevenueDecimal etal("0.1");

        if (ri.adv_revenue.impression != etal ||
          ri.adv_revenue.click != etal ||
          ri.adv_revenue.action != etal)
        {
          std::cerr << "action - lost revenue :" << std::endl;
          ri.print(std::cerr, "  ");
          std::cerr << add_error << std::endl;
        }

        check_pub_revenue(ri, ProcessingState());

        if(aggregate_processor_)
        {
          aggregate_processor_->process_action(ri);
        }

        SyncPolicy::WriteGuard lock(lock_);
        ++(*this)[ri.request_id].actions;
      }
      catch (const RequestActionProcessor::Exception&)
      {
        throw;
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FNS << ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    virtual void
    process_custom_action(
      const RequestInfo& ri,
      const AdvCustomActionInfo& adv_custom_action_info) noexcept
    {
      SyncPolicy::WriteGuard lock(lock_);

      (*this)[ri.request_id].custom_actions.insert(
        CustomActionKey(
          adv_custom_action_info.action_id,
          adv_custom_action_info.action_request_id,
          adv_custom_action_info.referer.c_str()));
    }

    Counter
    sum() const
    {
      Counter result;
      for(std::map<AdServer::Commons::RequestId, Counter>::
            const_iterator it = begin();
          it != end(); ++it)
      {
        result += it->second;
      }
      return result;
    }

    void
    assert_on_non_request(bool assert_on_non_request_val)
    {
      assert_on_non_request_ = assert_on_non_request_val;
    }

  protected:
    virtual ~TestRequestActionProcessorImpl() noexcept
    {}

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

  protected:
    bool assert_on_non_request_;
    RequestActionProcessor_var aggregate_processor_;
    const std::string name_;
    std::string current_test_;
    SyncPolicy::Mutex lock_;
  };

  typedef
    ReferenceCounting::SmartPtr<TestRequestActionProcessorImpl>
    TestRequestActionProcessorImpl_var;

  class CheckRequestOperationProcessor:
    public virtual RequestOperationProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, RequestOperationProcessor::Exception);

    CheckRequestOperationProcessor(RequestOperationProcessor* delegate_processor)
      noexcept
      : delegate_processor_(ReferenceCounting::add_ref(delegate_processor))
    {}

    virtual void
    process_impression(
      const ImpressionInfo& impression_info)
      /*throw(Exception)*/
    {
      /*
      std::cout << "CheckRequestOperationProcessor: move " <<
        (impression_info.verify_impression ? "impression" : "notice") <<
        " operation for '" << impression_info.user_id << "'" << std::endl;
      */
      assert(!impression_info.user_id.is_null());
      if(*impression_info.user_id.begin() != 'I')
      {
        Stream::Error ostr;
        ostr << "CheckRequestOperationProcessor::process_impression(): "
          "non impression user_id: '" <<
          static_cast<char>(*impression_info.user_id.begin()) << "'";
        throw RequestActionProcessor::Exception(ostr);
      }
      delegate_processor_->process_impression(impression_info);
    }

    virtual void
    process_action(
      const AdServer::Commons::UserId& new_user_id,
      RequestContainerProcessor::ActionType action_type,
      const Generics::Time& time,
      const AdServer::Commons::RequestId& request_id)
      /*throw(Exception)*/
    {
      if(*new_user_id.begin() != 'I')
      {
        Stream::Error ostr;
        ostr << "CheckRequestOperationProcessor::process_action(): "
          "non impression user_id: '" <<
          static_cast<char>(*new_user_id.begin()) << "'";
        throw RequestActionProcessor::Exception(ostr);
      }
      delegate_processor_->process_action(
        new_user_id, action_type, time, request_id);
    }

    virtual void
    process_request_post_action(
      const AdServer::Commons::UserId&,
      const AdServer::Commons::RequestId&,
      const RequestPostActionInfo&)
      /*throw(Exception)*/
    {}

    virtual void
    process_impression_post_action(
      const AdServer::Commons::UserId&,
      const AdServer::Commons::RequestId&,
      const AdServer::RequestInfoSvcs::RequestPostActionInfo&)
      /*throw(Exception)*/
    {}

    virtual void
    change_request_user_id(
      const AdServer::Commons::UserId& new_user_id,
      const AdServer::Commons::RequestId& request_id,
      const Generics::ConstSmartMemBuf* request_profile)
      /*throw(Exception)*/
    {
      /*
      std::cout << "CheckRequestOperationProcessor: change request user id '" <<
        new_user_id << "'" << std::endl;
      */
      delegate_processor_->change_request_user_id(
        new_user_id, request_id, request_profile);
    }

  protected:
    virtual ~CheckRequestOperationProcessor() noexcept {}

  private:
    RequestOperationProcessor_var delegate_processor_;
  };
}

/**
 * Perform calls to RequestInfoContainer in specified order
 */
enum OrderFunFlags
{
  OF_NOTICE = 1,
  OF_IMP_TRACK = 2,
  OF_ACTION_TRACK = 4,
  OF_REVENUE_TYPE_SHARE = 8,
  OF_IMP_CHANGE_USER_ID = 16,
  OF_NON_FIRST_IMP_CHANGE_USER_ID = 32
};

struct OrderFun
{
  OrderFun(
    int flags,
    int op1, int op2, int op3, int op4,
    const AdvCustomActionInfo& ca_info = AdvCustomActionInfo())
  {
    fill_flags(flags);
    ops[0] = op1;
    ops[1] = op2;
    ops[2] = op3;
    ops[3] = op4;
    sz = 4;
    custom_action_info = ca_info;
  }

  OrderFun(
    int flags,
    int op1, int op2, int op3)
  {
    fill_flags(flags);
    ops[0] = op1;
    ops[1] = op2;
    ops[2] = op3;
    sz = 3;
  }

  OrderFun(
    int flags,
    int op1, int op2, int op3, int op4, int op5)
  {
    fill_flags(flags);
    ops[0] = op1;
    ops[1] = op2;
    ops[2] = op3;
    ops[3] = op4;
    ops[4] = op5;
    sz = 5;
  }

  OrderFun(
    int flags,
    int op1, int op2, int op3, int op4,
    int op5, int op6)
  {
    fill_flags(flags);
    ops[0] = op1;
    ops[1] = op2;
    ops[2] = op3;
    ops[3] = op4;
    ops[4] = op5;
    ops[5] = op6;
    sz = 6;
  }

  OrderFun(
    int flags,
    int op1, int op2, int op3, int op4,
    int op5, int op6, int op7, int op8)
  {
    fill_flags(flags);
    ops[0] = op1;
    ops[1] = op2;
    ops[2] = op3;
    ops[3] = op4;
    ops[4] = op5;
    ops[5] = op6;
    ops[6] = op7;
    ops[7] = op8;
    sz = 8;
  }

  void
  fill_flags(int flags)
  {
    notice = flags & OF_NOTICE;
    imp_track = flags & OF_IMP_TRACK;
    action_track = flags & OF_ACTION_TRACK;
    revenue_type_share = flags & OF_REVENUE_TYPE_SHARE;
    imp_change_user_id = flags & OF_IMP_CHANGE_USER_ID;
    non_first_imp_change_user_id = flags & OF_NON_FIRST_IMP_CHANGE_USER_ID;
  };

  void
  operator ()(
    const AdServer::Commons::RequestId& req_id,
    RequestInfoContainer* request_info_container) const
  {
    RequestInfo request_info;
    request_info.time = Generics::Time::get_time_of_day();
    request_info.request_id = req_id;
    request_info.user_id = generate_user_id(false);
    request_info.user_id = generate_user_id(false);
    request_info.at_flags = 0;
    request_info.enabled_action_tracking = action_track;
    request_info.enabled_impression_tracking = imp_track;
    request_info.enabled_notice = notice;
    if(revenue_type_share)
    {
      request_info.expression = "TEST-SHARE";
    }
    else
    {
      request_info.expression = "TEST-ABSOLUTE";
    }

    request_info.adv_revenue.impression = RevenueDecimal("0.1");
    request_info.adv_revenue.click = RevenueDecimal("0.1");
    request_info.adv_revenue.action = RevenueDecimal("0.1");
    request_info.adv_revenue.currency_rate = RevenueDecimal(1);
    request_info.pub_revenue.impression =
      request_info.enabled_impression_tracking || request_info.enabled_notice ?
      BASE_PUB_IMP_REVENUE : // will be overriden at impr track, 0.9
      (revenue_type_share ? PUB_SHARE_REVENUE : PUB_IMP_REVENUE); // 0.3
    //std::cout << "Set request_info.pub_revenue.impression = " << request_info.pub_revenue.impression << std::endl;

    request_info.pub_revenue.click = RevenueDecimal("0.1");
    request_info.pub_revenue.action = RevenueDecimal("0.1");
    request_info.pub_bid_cost = RevenueDecimal::ZERO;
    request_info.pub_floor_cost = RevenueDecimal::ZERO;

    request_info.pub_revenue.currency_rate = PUB_CURRENCY_RATE;
    request_info.isp_revenue.impression = RevenueDecimal("0.1");
    request_info.isp_revenue.click = RevenueDecimal("0.1");
    request_info.isp_revenue.action = RevenueDecimal("0.1");
    request_info.isp_revenue.currency_rate = RevenueDecimal(1);
    request_info.device_channel_id = imp_change_user_id ? 1 : 0;
    request_info.pub_commission = RevenueDecimal::ZERO;
    request_info.isp_revenue_share = RevenueDecimal::ZERO;
    request_info.ctr = RevenueDecimal::ZERO;
    request_info.conv_rate = RevenueDecimal::ZERO;
    request_info.self_service_commission = RevenueDecimal::ZERO;
    request_info.adv_commission = RevenueDecimal::ZERO;
    request_info.pub_cost_coef = RevenueDecimal::ZERO;

    /*
    std::cout << "process ";
    print(std::cout);
    std::cout << std::endl;
    */

    bool imp_done = false;

    for (int i = 0; i < sz; ++i)
    {
      if (ops[i] == REQUEST_OP)
      {
        request_info_container->process_request(RequestInfo(request_info));
      }
      /*
      else if (ops[i] == CUSTOM_ACT_OP)
      {
        request_info_container->process_custom_action(
          req_id,
          custom_action_info);
      }
      */
      else if (ops[i] == IMPRESSION_OP || ops[i] == NOTICE_OP)
      {
        ImpressionInfo impression_info;
        impression_info.time = Generics::Time::get_time_of_day();
        impression_info.request_id = req_id;
        if(ops[i] == IMPRESSION_OP && (
             imp_change_user_id ||
             (non_first_imp_change_user_id && imp_done)))
        {
          impression_info.user_id = generate_user_id(true);
        }

        if((ops[i] == NOTICE_OP && request_info.enabled_notice) ||
           (ops[i] == IMPRESSION_OP && !request_info.enabled_notice &&
            request_info.enabled_impression_tracking))
        {
          ImpressionInfo::PubRevenue pub_revenue;
          if(revenue_type_share)
          {
            pub_revenue.revenue_type = AdServer::CampaignSvcs::RT_SHARE;
            pub_revenue.impression = PUB_SHARE;
          }
          else
          {
            pub_revenue.revenue_type = AdServer::CampaignSvcs::RT_ABSOLUTE;
            pub_revenue.impression = PUB_IMP_REVENUE;
          }
          impression_info.pub_revenue = pub_revenue;
        }
        impression_info.verify_impression = (ops[i] == IMPRESSION_OP);
        request_info_container->process_impression(impression_info);

        if(ops[i] == IMPRESSION_OP)
        {
          imp_done = true;
        }
      }
      else
      {
        request_info_container->process_action(
          static_cast<RequestContainerProcessor::ActionType>(ops[i]),
          Generics::Time::get_time_of_day(),
          request_info.request_id);
      }
    }
  }

  void
  print(std::ostream& ostr) const
  {
    for (int i = 0; i < sz; ++i)
    {
      if (i != 0) ostr << "->";

      if (ops[i] == REQUEST_OP)
      {
        ostr << "REQ";
      }
      /*
      else if (ops[i] == CUSTOM_ACT_OP)
      {
        ostr << "CUSTOM ACTION";
      }
      */
      else if(ops[i] == NOTICE_OP)
      {
        ostr << "NOTICE";
      }
      else if(ops[i] == IMPRESSION_OP)
      {
        ostr << "IMP";
      }
      else
      {
        switch(ops[i])
        {
        case RequestContainerProcessor::AT_CLICK:
          ostr << "CLICK";
          break;
        case RequestContainerProcessor::AT_ACTION:
          ostr << "ACT";
          break;
        };
      }
    }
  }

  bool imp_track;
  bool action_track;
  bool notice;
  bool revenue_type_share;
  bool imp_change_user_id;
  bool non_first_imp_change_user_id;
  int ops[8];
  int sz;
  AdvCustomActionInfo custom_action_info;
};


/**
 * Wrapper for sequence of test calls to RequestInfoContainer
 */
void
test_seq_wrapper(
  const char* TEST_NAME,
  TestRequestActionProcessorImpl* action_processor,
  RequestInfoContainer* request_info_container,
  const Counter& check_counter,
  const OrderFun& fun)
{
  //std::cerr << ">>>>>>>>> start sequence" << std::endl;
  Generics::Uuid uuid = Generics::Uuid::create_random_based();
  AdServer::Commons::RequestId req_id(uuid.to_string().c_str());

  fun(req_id, request_info_container);

  std::map<AdServer::Commons::RequestId, Counter>& res = *action_processor;

  if (res[req_id] != check_counter)
  {
    std::cerr
      << "Incorrect Counter: "
      << res[req_id] << std::endl
      << "Instead "
      << check_counter
      << " in test '"
      << TEST_NAME << "'"
      << add_error
      << std::endl;
  }
  /*else
  {
    std::cout
      << "Correct counter: "
      << TEST_NAME << std::endl;
  }*/
  //std::cerr << "<<<<<<<<< sequence fin" << std::endl;
}


void
test_order_wrapper(
  const char* TEST_NAME,
  TestRequestActionProcessorImpl* action_processor,
  RequestInfoContainer* request_info_container,
  const OrderFun& fun,
  const Counter& check_counter = Counter(1, 1, 1, 1, 1, 0, 0))
{
  action_processor->set_current_test(TEST_NAME);

  test_seq_wrapper(
    TEST_NAME,
    action_processor,
    request_info_container,
    check_counter,
    fun);
}

void
gen_seq(
  const char* TEST_NAME,
  TestRequestActionProcessorImpl* action_processor,
  RequestInfoContainer* request_info_container,
  unsigned long* counter,
  OrderFun& order_fun,
  const Counter& check_counter,
  long l = 0,
  long r = -1)
{
  if (r == -1)
  {
    r = order_fun.sz - 1;
  }

  if (l == r)
  {
    std::ostringstream test_name_ostr;
    test_name_ostr << TEST_NAME << ": ";
    order_fun.print(test_name_ostr);

    //std::cerr << "To '" << test_name_ostr.str() << "'" << std::endl;

    *counter += 1;
    if(*counter % 1000 == 0)
    {
      //std::cout << "finished " << *counter << " interations" << std::endl;
    }

    return test_seq_wrapper(
      test_name_ostr.str().c_str(),
      action_processor,
      request_info_container,
      check_counter,
      order_fun);
  }

  for (long i = l; i <= r; ++i)
  {
    std::swap(order_fun.ops[i], order_fun.ops[l]);

    gen_seq(
      TEST_NAME,
      action_processor,
      request_info_container,
      counter,
      order_fun,
      check_counter,
      l + 1,
      r);

    std::swap(order_fun.ops[i], order_fun.ops[l]);
  }
}

void
test_all_combinations_wrapper(
  const char* TEST_NAME,
  TestRequestActionProcessorImpl* action_processor,
  RequestInfoContainer* request_info_container,
  const OrderFun& fun,
  const Counter& check_counter = Counter(1, 1, 1, 1, 1, 0, 0))
{
  std::cout << TEST_NAME << std::endl;
  OrderFun fun_copy(fun);
  unsigned long counter = 0;
  gen_seq(
    TEST_NAME,
    action_processor,
    request_info_container,
    &counter,
    fun_copy,
    check_counter);
}

int
request_info_container_test()
{
  try
  {
    system(("rm -r " + *root_path + TEST_FOLDER +
      " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER).c_str());

    AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
      new AdServer::ProfilingCommons::ProfileMapFactory::Cache(1));

    Logging::Logger_var logger(new Logging::Null::Logger);

    // tests check aggregated result of local and delegate processor's
    TestRequestActionProcessorImpl_var action_processor(
      new TestRequestActionProcessorImpl(nullptr, "aggregate"));
    TestRequestActionProcessorImpl_var delegate_action_processor(
      new TestRequestActionProcessorImpl(action_processor, "delegate"));
    TestRequestActionProcessorImpl_var local_action_processor(
      new TestRequestActionProcessorImpl(action_processor, "local"));

    RequestInfoContainer_var delegate_request_info_container(
      new RequestInfoContainer(
        logger,
        delegate_action_processor, // delegate_action_processor,
        0,
        (*root_path + TEST_FOLDER).c_str(),
        "DelegateRequest",
        String::SubString(),
        String::SubString(),
        cache,
        Generics::Time(10), // expire time (sec)
        Generics::Time(2)));

    RequestInfoContainer_var normal_request_info_container(
      new RequestInfoContainer(
        logger,
        local_action_processor,
        RequestOperationProcessor_var(
          new CheckRequestOperationProcessor(
            delegate_request_info_container->request_operation_proxy())),
        (*root_path + TEST_FOLDER).c_str(),
        "Request",
        (*root_path + TEST_FOLDER),
        String::SubString("Bid"),
        cache,
        Generics::Time(10), // expire time (sec)
        Generics::Time(2)));

    CompositeRequestOperationProcessor_var
      self_delegate_request_operation_processor(
        new CompositeRequestOperationProcessor());

    RequestInfoContainer_var self_delegate_request_info_container(
      new RequestInfoContainer(
        logger,
        action_processor,
        RequestOperationProcessor_var(
          new CheckRequestOperationProcessor(
            self_delegate_request_operation_processor)),
        (*root_path + TEST_FOLDER).c_str(),
        "SDRequest",
        String::SubString(),
        String::SubString(),
        cache,
        Generics::Time(10), // expire time (sec)
        Generics::Time(2)));

    self_delegate_request_operation_processor->add_child_processor(
      delegate_request_info_container->request_operation_proxy());

    //multi_thread_test(request_info_container);

    //for(int i = 0; i <= 2; ++i)
    for(int i = 0; i <= 2; ++i)
    //for(int i = 0; i <= 0; ++i)
    {
      std::cout << "STAGE #" << i << std::endl;

      // 0 - moving disabled mode
      // 1 - moving to other container
      // 2 - moving to self mode
      TestRequestActionProcessorImpl_var orig_action_processor;
      RequestInfoContainer_var request_info_container;

      if(i == 0 || i == 1)
      {
        orig_action_processor = local_action_processor;
        request_info_container = normal_request_info_container;
      }
      else
      {
        orig_action_processor = action_processor;
        request_info_container = self_delegate_request_info_container;
      }

      int add_flags = (i == 0 ? 0 : OF_IMP_CHANGE_USER_ID);

      action_processor->clear();
      local_action_processor->clear();
      delegate_action_processor->clear();

      TestRequestActionProcessorImpl_var tgt_action_processor = action_processor;
        //i == 0 ? action_processor : delegate_action_processor;

      if(i != 0)
      {
        // container should delegate all operations excluding requests (
        //   request can be processed localy or delegated)
        local_action_processor->assert_on_non_request(true);
        std::cout << "tests with delegating" << std::endl;
      }

      // localization of scenario (example)
      /*
      test_order_wrapper(
        "REQ->IMP->CLICK",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          add_flags, // no imp track
          REQUEST_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK),
        Counter(1, 1, 1, 0, 1, 1, 0));
      */
      test_all_combinations_wrapper(
        "IMPR TRACK, ACT TRACK - REQ,IMP,CLICK,ACT",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_ACTION_TRACK | add_flags,
          REQUEST_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_ACTION),
        Counter(1, 1, 1, 1, 1, 0, 0));

      /*
      test_all_combinations_wrapper(
        "IMP TRACK WITH REVENUE SHARE, ACT TRACK - REQ,IMP,CLICK",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_ACTION_TRACK | OF_REVENUE_TYPE_SHARE | add_flags,
          REQUEST_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK),
        Counter(1, 1, 1, 0, 1, 0, 0));
      */
      /*
      test_all_combinations_wrapper(
        "IMP TRACK WITH REVENUE SHARE, ACT TRACK - REQ,IMP,CLICK,ACTION",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_ACTION_TRACK | OF_REVENUE_TYPE_SHARE | add_flags,
          REQUEST_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_ACTION),
        Counter(1, 1, 1, 1, 1, 0, 0));
      */

      request_info_container->clear_expired_requests();

      test_all_combinations_wrapper(
        "IMPR TRACK, NO ACT TRACK - REQ,IMP,CLICK,ACT",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | add_flags,
          REQUEST_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_ACTION),
        Counter(1, 1, 1, 0, 1, 0, 0));

      test_all_combinations_wrapper(
        "IMPR TRACK, NOTICE, NO ACT TRACK - REQ,IMP,CLICK,ACT",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_NOTICE | add_flags,
          REQUEST_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_ACTION),
        Counter(1, 0, 0, 0, 1, 0, 0));

      test_all_combinations_wrapper(
        "IMPR TRACK, NOTICE, NO ACT TRACK - REQ,NOTICE,IMP,CLICK,ACT",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_NOTICE | add_flags,
          REQUEST_OP,
          NOTICE_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_ACTION),
        Counter(1, 1, 1, 0, 1, 0, 0));

      test_all_combinations_wrapper(
        "IMPR TRACK, NOTICE, NO ACT TRACK - REQ,NOTICE,2*IMP,CLICK,ACT",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_NOTICE | add_flags,
          REQUEST_OP,
          NOTICE_OP,
          IMPRESSION_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_ACTION),
        Counter(1, 1, 1, 0, 1, 1, 0));

      test_order_wrapper(
        "REQ->IMP->CLICK->ACT (NOTICE disabled)",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_ACTION_TRACK | add_flags,
          IMPRESSION_OP,
          REQUEST_OP,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_ACTION),
        Counter(1, 1, 1, 1, 1, 0, 0));

      test_order_wrapper(
        "REQ->NOTICE->IMP->CLICK (X1)",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_NOTICE | add_flags,
          REQUEST_OP,
          NOTICE_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK),
        Counter(1, 1, 1, 0, 1, 0, 0));

      //std::cout << "================================================" << std::endl;

      test_order_wrapper(
        "REQ->IMP->NOTICE->CLICK (XXX0)",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_NOTICE | add_flags, // no imp track
          REQUEST_OP,
          IMPRESSION_OP,
          NOTICE_OP,
          RequestContainerProcessor::AT_CLICK),
        Counter(1, 1, 1, 0, 1, 0, 0));

      //std::cout << "================================================" << std::endl;

      test_order_wrapper(
        "REQ->IMP->IMP->NOTICE->CLICK (XXX)",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_NOTICE | add_flags, // no imp track
          REQUEST_OP,
          IMPRESSION_OP,
          IMPRESSION_OP,
          NOTICE_OP,
          RequestContainerProcessor::AT_CLICK),
        Counter(1, 1, 1, 0, 1, 1, 0));

      // combinations
      test_all_combinations_wrapper(
        "IMPR TRACK, NO ACT TRACK - REQ,2*IMP,CLICK,ACT",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | add_flags,
          REQUEST_OP,
          IMPRESSION_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_ACTION),
        Counter(1, 1, 1, 0, 1, 1, 0));

      // doubles test
      /*
      test_all_combinations_wrapper(
        "DOUBLES TEST (2*REQ -> 2*IMP -> 2*CLICK -> 2*ACT)",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | OF_ACTION_TRACK | add_flags,
          REQUEST_OP,
          REQUEST_OP,
          IMPRESSION_OP,
          IMPRESSION_OP,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_CLICK,
          RequestContainerProcessor::AT_ACTION,
          RequestContainerProcessor::AT_ACTION),
        Counter(1, 1, 1, 2, 1, 1, 1));
      */

      test_all_combinations_wrapper(
        "DISABLED ACTION TRACK TEST (ACT -> CLICK -> IMP -> REQ)",
        tgt_action_processor,
        request_info_container,
        OrderFun(
          OF_IMP_TRACK | add_flags,
          RequestContainerProcessor::AT_ACTION,
          RequestContainerProcessor::AT_CLICK,
          -3,
          REQUEST_OP),
        Counter(1, 1, 1, 0, 1, 0, 0));

      if(i == 0)
      {
        /*
        // tests with disabled impression track - can't be delegated
        test_all_combinations_wrapper(
          "NO IMPR TRACK, NO ACT TRACK - REQ,IMP,CLICK",
          tgt_action_processor,
          request_info_container,
          OrderFun(
            add_flags,
            REQUEST_OP,
            IMPRESSION_OP,
            RequestContainerProcessor::AT_CLICK),
          Counter(1, 1, 1, 0, 1, 1, 0)); // double imp because disabled impr track

        test_all_combinations_wrapper(
          "NO IMPR TRACK, ACT TRACK - REQ,IMP,CLICK",
          tgt_action_processor,
          request_info_container,
          OrderFun(
            OF_ACTION_TRACK | add_flags,
            REQUEST_OP,
            IMPRESSION_OP,
            RequestContainerProcessor::AT_CLICK),
          Counter(1, 1, 1, 0, 1, 1, 0)); // double imp because disabled impr track

        test_all_combinations_wrapper(
          "NO IMPR TRACK, NO ACT TRACK - REQ,IMP,CLICK",
          tgt_action_processor,
          request_info_container,
          OrderFun(
            add_flags,
            REQUEST_OP,
            IMPRESSION_OP,
            RequestContainerProcessor::AT_CLICK),
          Counter(1, 1, 1, 0, 1, 1, 0)); // double imp because disabled impr track

        test_all_combinations_wrapper(
          "NO IMPR TRACK, ACT TRACK - REQ,IMP,CLICK,ACT",
          tgt_action_processor,
          request_info_container,
          OrderFun(
            OF_ACTION_TRACK | OF_REVENUE_TYPE_SHARE | add_flags,
            REQUEST_OP,
            -3,
            RequestContainerProcessor::AT_CLICK,
            RequestContainerProcessor::AT_ACTION),
          Counter(1, 1, 1, 1, 1, 1, 0)); // double imp because disabled impr track

        test_all_combinations_wrapper(
          "NO IMPR TRACK, NO ACT TRACK - REQ,IMP,CLICK,ACT",
          tgt_action_processor,
          request_info_container,
          OrderFun(
            add_flags,
            REQUEST_OP,
            -3,
            RequestContainerProcessor::AT_CLICK,
            RequestContainerProcessor::AT_ACTION),
          Counter(1, 1, 1, 0, 1, 1, 0)); // double imp because disabled impr track

        // second impression must be ignored
        test_order_wrapper(
          "DOUBLES TEST (REQ -> 2*IMP)",
          tgt_action_processor,
          request_info_container,
          OrderFun(
            OF_IMP_TRACK | OF_NON_FIRST_IMP_CHANGE_USER_ID,
            REQUEST_OP,
            IMPRESSION_OP,
            IMPRESSION_OP),
          Counter(1, 1, 0, 0, 1, 1, 0));

        test_order_wrapper(
          "DISABLED IMPRESSION TRACK TEST (ACT -> CLICK -> REQ)",
          tgt_action_processor,
          request_info_container,
          OrderFun(
            OF_ACTION_TRACK | add_flags,
            RequestContainerProcessor::AT_ACTION,
            RequestContainerProcessor::AT_CLICK,
            RequestContainerProcessor::AT_CLICK,
            REQUEST_OP),
          Counter(1, 1, 1, 1, 1, 0, 1));

        test_order_wrapper(
          "DISABLED ALL TRACK TEST (ACT -> 2*CLICK -> REQ)",
          tgt_action_processor,
          request_info_container,
          OrderFun(
            add_flags,
            RequestContainerProcessor::AT_ACTION,
            RequestContainerProcessor::AT_CLICK,
            RequestContainerProcessor::AT_CLICK,
            REQUEST_OP),
          Counter(1, 1, 1, 0, 1, 0, 1));
        */
      }

      if(i == 0)
      {
        if(!delegate_action_processor->empty())
        {
          std::cerr << add_error <<
            "delegate_action_processor isn't empty after non delegate cases" << std::endl;
        }
      }
      else
      {
        /*
        Counter sum = local_action_processor->sum();
        Counter check_counter(0, 0, 0, 0, sum.prime_requests, 0, 0); // prime_requests can be non zero

        if(sum != check_counter)
        {
          std::cerr << add_error <<
            "action_processor have incorrect counter after all tests: " <<
            sum << std::endl <<
            "Instead " << check_counter << std::endl;
        }
        */
      }
    }
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << add_error << ex.what() << std::endl;
  }

  return get_errors(std::cerr);
}

bool
init(int& argc, char**& argv) /*throw(eh::Exception)*/
{
  using namespace Generics::AppUtils;
  Args args;
  CheckOption opt_help;

  args.add(
    equal_name("path") ||
    short_name("p"),
    root_path);
  args.add(
    equal_name("help") ||
    short_name("h"),
    opt_help);

  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return false;
  }
  return true;
}

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    if (!init(argc, argv))
    {
      return 0;
    }
    return request_info_container_test();
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return -1;
}
