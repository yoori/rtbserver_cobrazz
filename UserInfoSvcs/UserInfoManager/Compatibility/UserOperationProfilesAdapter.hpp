#ifndef USEROPERATIONPROFILESADAPTER_HPP
#define USEROPERATIONPROFILESADAPTER_HPP

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    const unsigned long PREF_GET_OPERATION_PROFILE_VERSION = 1; 
    const unsigned long FRAUD_OPERATION_PROFILE_VERSION = 1;
    const unsigned long MATCH_OPERATION_PROFILE_VERSION = 340;
    const unsigned long MERGE_OPERATION_PROFILE_VERSION = 350;
    const unsigned long FC_UPDATE_OPERATION_PROFILE_VERSION = 351;
    const unsigned long FC_CONFIRM_OPERATION_PROFILE_VERSION = 340;
    const unsigned long PREF_UPDATE_OPERATION_PROFILE_VERSION = 1;
    const unsigned long PROP_UPDATE_OPERATION_PROFILE_VERSION = 1;
    const unsigned long ADD_AUDIENCE_CHANNELS_OPERATION_PROFILE_VERSION = 1;
    const unsigned long REMOVE_AUDIENCE_CHANNELS_OPERATION_PROFILE_VERSION = 1;

    struct UserFreqCapUpdateOperationProfilesAdapter
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      Generics::SmartMemBuf_var
      operator()(Generics::SmartMemBuf* smart_mem_buf) /*throw(Exception)*/;
    };

    struct UserFreqCapConfirmOperationProfilesAdapter
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      Generics::SmartMemBuf_var
      operator()(Generics::SmartMemBuf* smart_mem_buf) /*throw(Exception)*/;
    };

    struct UserMatchOperationProfilesAdapter
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      template<typename InputIterator, typename OutputContainerType>
      void fill_channel_trigger_seq(
        InputIterator first,
        InputIterator last,
        OutputContainerType& out);
      
      Generics::SmartMemBuf_var
      operator()(Generics::SmartMemBuf* smart_mem_buf) /*throw(Exception)*/;
    };

    struct UserMergeOperationProfilesAdapter
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      Generics::SmartMemBuf_var
      operator()(Generics::SmartMemBuf* smart_mem_buf) /*throw(Exception)*/;
    };

  }
}

#endif /* USEROPERATIONPROFILESADAPTER_HPP */
