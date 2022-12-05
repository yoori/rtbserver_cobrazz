#ifndef AD_SERVER_LOG_PROCESSING_REQUEST_BASE_HPP
#define AD_SERVER_LOG_PROCESSING_REQUEST_BASE_HPP


#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <LogCommons/CsvUtils.hpp>
#include <LogCommons/GenericLogCsvSaverImpl.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer {
namespace LogProcessing {

  typedef std::pair<StringIO<Aux_::ConvertSpacesSeparators, '='>,
    StringIO<Aux_::ConvertSpacesSeparators, ','> > UserProperty;
  typedef std::list<UserProperty> UserPropertyList;
  typedef OutputArchive<Aux_::NoInvariants, ':'> OutputArchiveColon;

  namespace RequestData_V_3_4
  {
    typedef Generics::SimpleDecimal<uint32_t, 6, 5> DeliveryThresholdT;
    typedef OptionalValue<unsigned long> DeviceChannelIdOptional;

    struct CmpChannel
    {
      typedef AdServer::LogProcessing::FixedNumber FixedNum;

      CmpChannel()
        : channel_id(),
          channel_rate_id(),
          imp_revenue(FixedNum::ZERO),
          imp_sys_revenue(FixedNum::ZERO),
          adv_imp_revenue(FixedNum::ZERO),
          click_revenue(FixedNum::ZERO),
          click_sys_revenue(FixedNum::ZERO),
          adv_click_revenue(FixedNum::ZERO)
      {
      }

      bool operator==(const CmpChannel& cmp_channel) const
      {
        if (this == &cmp_channel)
        {
          return true;
        }
        return channel_id == cmp_channel.channel_id &&
          channel_rate_id == cmp_channel.channel_rate_id &&
          imp_revenue == cmp_channel.imp_revenue &&
          imp_sys_revenue == cmp_channel.imp_sys_revenue &&
          adv_imp_revenue == cmp_channel.adv_imp_revenue &&
          click_revenue == cmp_channel.click_revenue &&
          click_sys_revenue == cmp_channel.click_sys_revenue &&
          adv_click_revenue == cmp_channel.adv_click_revenue;
      }

      template <class ARCHIVE_>
      void serialize(ARCHIVE_& ar)
      {
        ar & channel_id;
        ar & channel_rate_id;
        ar & imp_revenue;
        ar & imp_sys_revenue;
        ar & adv_imp_revenue;
        ar & click_revenue;
        ar & click_sys_revenue;
        ar ^ adv_click_revenue;
      }

      unsigned long channel_id;
      unsigned long channel_rate_id;
      FixedNum imp_revenue;
      FixedNum imp_sys_revenue;
      FixedNum adv_imp_revenue;
      FixedNum click_revenue;
      FixedNum click_sys_revenue;
      FixedNum adv_click_revenue;
    };

    typedef std::list<CmpChannel> CmpChannelList;

    struct Revenue
    {
      typedef AdServer::LogProcessing::FixedNumber FixedNum;

      Revenue()
          :
          rate_id(),
          request_revenue(FixedNum::ZERO),
          imp_revenue(FixedNum::ZERO),
          click_revenue(FixedNum::ZERO),
          action_revenue(FixedNum::ZERO)
      {
      }

      bool operator==(const Revenue& revenue) const
      {
        if (this == &revenue)
        {
          return true;
        }
        return rate_id == revenue.rate_id &&
          request_revenue == revenue.request_revenue &&
          imp_revenue == revenue.imp_revenue &&
          click_revenue == revenue.click_revenue &&
          action_revenue == revenue.action_revenue;
      }

      template <class ARCHIVE_>
      void serialize(ARCHIVE_& ar)
      {
        ar & rate_id;
        ar & request_revenue;
        ar & imp_revenue;
        ar & click_revenue;
        ar ^ action_revenue;
      }

      unsigned long rate_id;
      FixedNum request_revenue;
      FixedNum imp_revenue;
      FixedNum click_revenue;
      FixedNum action_revenue;
    };
  } //RequestData_V_3_4_0

  inline
  FixedBufStream<CommaCategory>&
  operator>>(FixedBufStream<CommaCategory>& is, RequestData_V_3_4::CmpChannel& cmp_channel)
  {
    String::SubString token = is.read_token();
    if(is.good())
    {
      FixedBufStream<SemiCategory> fbs(token);
      TokenizerInputArchive<Aux_::NoInvariants, SemiCategory> ia(fbs);
      ia >> cmp_channel;
      is.take_fails(fbs);
    }
    return is;
  }

  inline
  std::ostream&
  operator<<(std::ostream& os, const RequestData_V_3_4::CmpChannel& cmp_channel)
  {
    OutputArchiveColon oa(os);
    oa << cmp_channel;
    return os;
  }

  inline
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestData_V_3_4::Revenue& revenue)
  {
    TokenizerInputArchive<Aux_::NoInvariants> ia(is);
    ia >> revenue;
    return is;
  }

  inline
  std::ostream&
  operator<<(std::ostream& os, const RequestData_V_3_4::Revenue& revenue)
  {
    SimpleTabOutputArchive oa(os);
    oa << revenue;
    return os;
  }

  FixedBufStream<CommaCategory>&
  operator>>(FixedBufStream<CommaCategory>& is, UserProperty& property) /*throw(eh::Exception)*/;

  inline
  std::ostream&
  operator<<(std::ostream& os, const UserProperty& property) /*throw(eh::Exception)*/
  {
    os << property.first << '=' << property.second;
    return os;
  }

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_REQUEST_BASE_HPP */
