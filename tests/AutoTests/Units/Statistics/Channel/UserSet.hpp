#ifndef _UNITTEST__USERSET_HPP
#define _UNITTEST__USERSET_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace Statistics
  {
    /**
     * @class UserSet
     * @brief User collection, using in statistics test
     */
    class UserSet
    {

      DECLARE_EXCEPTION(InvalidIndex, Exception);

      typedef AutoTest::AdClient AdClient;

      typedef std::vector<AdClient> Users;

    public:

      /**
       * @brief Initialize collection.
       *
       * Creates collection of persistent users.
       * @param test, used collection.
       * @param collection size.
       */
      void initialize(
        BaseUnit* test,
        unsigned long size);

      /**
       * @brief Send requests  from each 
       *  user, who have index in range [start_index, stop_index].
       *
       * Sends GET requests to the server
       * @param request sending request.
       * @param fisrt user index.
       * @param last user index.
       * @param detail_info description for this request.
       */
      void process_request(
        const BaseRequest& request,
        unsigned long start_index = 0,
        unsigned long stop_index = 0)
          /*throw(eh::Exception)*/;
      

      /**
       * @brief Send request to the server from one
       *  of the collection's user.
       *
       * Sends GET requests to the server
       * @param user index.
       * @param request sending request.
       */
      void process_request_i(
        unsigned long index,
        const BaseRequest& request)
          /*throw(eh::Exception)*/;

      /**
       * @brief Access operator.
       *
       * Get access to user by index.
       * @param key param index.
       * @return user.
       */
      AdClient& operator[](unsigned long index);

      /**
       * @brief Get collection size
       *
       * @return collection size
       */
      size_t size() const;
      
    private:
      Users users_;
      void check_index(unsigned long index);
    };

    inline AdClient& UserSet::operator[](unsigned long index)
    {
      check_index(index);
      return users_[index];
    }

    inline size_t UserSet::size() const
    {
      return users_.size();
    }
  }
}


#endif  // _UNITTEST__USERSET_HPP
