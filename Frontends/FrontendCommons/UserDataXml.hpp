#ifndef _USERDATAXML_HPP_
#define _USERDATAXML_HPP_

#include <iostream>
#include <eh/Exception.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>

namespace FrontendCommons
{
  void print_user_data_xml(
    std::ostream& out,
    const char* token,
    const AdServer::UserInfoSvcs::UserPropertySeq& properties,
    const char* prefix = "")
    /*throw(eh::Exception)*/;
}

#endif /*_USERDATAXML_HPP_*/
