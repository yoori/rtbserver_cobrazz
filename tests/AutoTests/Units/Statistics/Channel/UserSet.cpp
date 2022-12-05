
#include "UserSet.hpp"

namespace AutoTest
{
  namespace Statistics
  {

    // class UserSet
    void
    UserSet::initialize(BaseUnit* test,
                        unsigned long size)
    {
      for (unsigned long i = 0; i < size; ++i)
      {
        users_.push_back(AdClient::create_user(test));
      }
    }

    void
    UserSet::process_request(
      const BaseRequest& request,
      unsigned long start_index,
      unsigned long stop_index)
      /*throw(eh::Exception)*/
    {
      unsigned long stop_index_ = stop_index?
        std::min(stop_index, users_.size()): users_.size();
      for (unsigned long i = std::min(start_index, stop_index_);
           i < std::max(start_index, stop_index_); ++i)
      {
        process_request_i(i, request);
      }
    }
      
    void
    UserSet::process_request_i(unsigned long index,
                               const BaseRequest& request)
      /*throw(eh::Exception)*/
    {
      check_index(index);
      users_[index].process_request(request);
    }

    void UserSet::check_index(unsigned long index)
    {
      if (index >= users_.size())
      {
        Stream::Error error;
        error << "Invalid user collection index (index=" <<
            index << ", size=" << users_.size() << ")" ;
        throw InvalidIndex(error);
      }
    }
  }
}

