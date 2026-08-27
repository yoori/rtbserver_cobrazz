
#include "ActionRequest.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* ActionRequestTraits::B::base_name_ = "ActionRequest";
  template <> const char* ActionRequestTraits::B::signature_ = "ActionRequest";
  template <> const char* ActionRequestTraits::B::current_version_ = "3.2";

  std::istream&
  operator>>(std::istream& is, ActionRequestKey& key)
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const ActionRequestKey& key)
    /*throw(eh::Exception)*/
  {
    os << key.sdate_ << '\n' << key.colo_id_;
    return os;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ActionRequestKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ActionRequestInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.holder_ = new ActionRequestInnerKey::DataHolder();
    TokenizerInputArchive<> ia(is);
    ia >> *key.holder_;
    key.holder_->calc_hash_();
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const ActionRequestInnerKey& key)
    /*throw(eh::Exception)*/
  {
    TabOutputArchive oa(os);
    oa << *key.holder_;
    return os;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ActionRequestInnerKey& key)
    /*throw(eh::Exception)*/
  {
    BufferTabOutputArchive archive(out);
    archive << *key.holder_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, ActionRequestInnerData& data)
  {
    is >> data.action_request_count_;
    is >> data.cur_value_;
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const ActionRequestInnerData& data)
  {
    os << data.action_request_count_ << '\t';
    os << data.cur_value_;
    return os;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const ActionRequestInnerData& data)
  {
    out << data.action_request_count_ << '\t' << data.cur_value_;
    return out;
  }

} // namespace AdServer::LogProcessing
