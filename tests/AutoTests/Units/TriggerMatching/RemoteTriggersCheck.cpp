#include "RemoteTriggersCheck.hpp"
#include <set>
 
REFLECT_UNIT(RemoteTriggersCheck) (
  "TriggerMatching",
  AUTO_TEST_FAST,
  AUTO_TEST_SERIALIZE
);

namespace
{
  typedef AutoTest::ChannelCheckAdmin CheckAdmin;
  typedef AutoTest::TriggerAdmin TriggerAdmin;
  typedef AutoTest::AdminCmd::values_type ValuesType;
  typedef std::set<std::string> SortedVector;
  typedef std::map<std::string, ValuesType> TriggerLists;

  const char FAKE_STAMP[] = "1970-01-01-00:00:00.000000";

  DECLARE_EXCEPTION(CompareFailed, eh::DescriptiveException);

  template<typename Admin>
  class TrigerListsComparator
  {
  public:
    TrigerListsComparator(Admin& central_admin, Admin& remote_admin)
      : central_admin_(central_admin),
        remote_admin_(remote_admin)
    {};

    void check() /*throw(CompareFailed, eh::Exception)*/
    {
      Stream::Error error;
      unsigned short error_num = 1;

      // Create triggers map based on central admin output
      TriggerLists central_triggers_list;
      central_admin_.fetch();
      for (unsigned int i = 0; i < central_admin_.size(); ++i)
      {
        central_admin_[i][TriggerAdmin::Expected::STAMP] = FAKE_STAMP;
        central_triggers_list.insert(std::make_pair(
          central_admin_[i][0],
          sort_values_(central_admin_[i])));
      }

      // Fetch triggers list of remote cluster
      // and compare it with central triggers list
      remote_admin_.fetch();

      for (unsigned int i = 0; i < remote_admin_.size(); ++i)
      {
        remote_admin_[i][TriggerAdmin::Expected::STAMP] = FAKE_STAMP;
        TriggerLists::const_iterator pos =
          central_triggers_list.find(remote_admin_[i][0]);
        if (pos == central_triggers_list.end())
        {
          error << error_num++ << ". Can't find trigger list with id = '" <<
            remote_admin_[i][0] << "' "
            "in central ChannelController on '" << central_admin_.address() <<
            "' (that present in remote ChannelController on '" <<
            remote_admin_.address() << "')" << std::endl;
        }
        else if (pos->second != sort_values_(remote_admin_[i]))
        {
          error << error_num++ << ". Got different values for trigger list with id='"
                << remote_admin_[i][0] << "'." << std::endl
                << "Central (ChannelController on '" << central_admin_.address()
                << "'):" << std::endl << "{" << std::endl;
          for (size_t j = 0; j < remote_admin_.slice_size(); ++j)
          {
            if (j == TriggerAdmin::Expected::STAMP) { continue; }
            error << "  " << remote_admin_.field_name(j) << " = " <<
              pos->second[j] << std::endl;
          }
          error << "}" << std::endl << std::endl
                << "Remote (ChannelController on '" << remote_admin_.address()
                << "'):" << std::endl << "{" << std::endl;
          for (size_t j = 0; j < remote_admin_.slice_size(); ++j)
          {
            if (j == TriggerAdmin::Expected::STAMP) { continue; }
            error << "  " << remote_admin_.field_name(j) << " = "
                  << remote_admin_[i][j] << std::endl;
          }
          error << "}" << std::endl << std::endl;
        }
      }

      if (!error.str().empty())
      {
        throw CompareFailed(error.str());
      }
    }

  private:
    ValuesType& sort_values_(ValuesType& values)
    {
      for (unsigned int i = 0; i < values.size(); ++i)
      {
        String::StringManip::SplitComma tokenizer(values[i]);
        String::SubString token;
        SortedVector sorted_tokens;
        while (tokenizer.get_token(token))
        {
          String::StringManip::trim(token);
          sorted_tokens.insert(token.str());
        }
        values[i].clear();
        for (SortedVector::const_iterator p = sorted_tokens.begin();
             p != sorted_tokens.end();)
        {
          values[i] += *p;
          if (++p != sorted_tokens.end())
          {
            values[i] += ", ";
          }
        }
      }
      return values;
    }

  private:
    Admin& central_admin_;
    Admin& remote_admin_;
  };
}

bool 
RemoteTriggersCheck::run_test()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_CENTRAL, STE_CHANNEL_CONTROLLER)),
    "Test require Central.ChannelController in AutoTest config!");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_REMOTE1, STE_CHANNEL_CONTROLLER) |
      get_config().check_service(CTE_PROFILING, STE_CHANNEL_CONTROLLER)),
    "Test require Remote#1.ChannelController or "
    "Profiling.ChannelController in AutoTest config!")

  std::string central_channel_controller =
    get_config().get_service(CTE_CENTRAL, STE_CHANNEL_CONTROLLER).address;
  std::string remote_channel_controller =
    get_config().check_service(CTE_REMOTE1, STE_CHANNEL_CONTROLLER)
      ? get_config().get_service(CTE_REMOTE1, STE_CHANNEL_CONTROLLER).address
      : get_config().get_service(CTE_PROFILING, STE_CHANNEL_CONTROLLER).address;

  CheckAdmin c_admin(remote_channel_controller.c_str());
  c_admin.fetch();
  std::string trigger_list = AutoTest::get_field_list(c_admin,
    CheckAdmin::Expected::ID);

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !trigger_list.empty()),
    "Admin command returns empty channel list!");

  TriggerAdmin u_admin_central(central_channel_controller.c_str(), trigger_list);
  TriggerAdmin u_admin_remote(remote_channel_controller.c_str(), trigger_list);

  TrigerListsComparator<TriggerAdmin> comparator(u_admin_central, u_admin_remote);
  FAIL_CONTEXT(
    comparator.check(),
    "must get expected trigger lists for central cluster");

  return true;
}
