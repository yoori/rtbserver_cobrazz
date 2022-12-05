
#ifndef _AUTOTESTS_COMMONS_PROTOBUFRESPONSE_HPP
#define _AUTOTESTS_COMMONS_PROTOBUFRESPONSE_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <Generics/Uncopyable.hpp>
#include <tests/AutoTests/Commons/AdClient.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>
#include <tests/AutoTests/Commons/Checkers/CompositeCheckers.hpp>
#include <tests/AutoTests/Commons/Checkers/ExpectedValue.hpp>

namespace AutoTest
{
  namespace ProtoBuf
  {
    /**
     * @class FieldChecker
     * @brief Protobuf field checker.
     */
    template <typename ProtoBufChecker, typename T, typename Getter>
    class FieldChecker: public Checker
    {
      ProtoBufChecker& checker_;
      T exp_;
      Getter getter_;
      
    public:
      /**
       * @brief Constructor.
       *
       * @param parent checker.
       * @param expected value.
       * @param pointer to field getter function.
       */
      FieldChecker(
        ProtoBufChecker& checker,
        const T& exp,
        Getter getter);
      
      /**
       * @brief Copy constructor.
       *
       * Need to fix RepeatedPtrField
       * copy constructor warning.
       */
      FieldChecker(
        const FieldChecker& other);
      
      /**
       * @brief Destructor.
       */
      virtual ~FieldChecker() noexcept;
      
      /**
       * @brief Check.
       * @param throw on error flag.
       */
      bool
      check(
        bool throw_error = true)
        /*throw(eh::Exception)*/;
    };

    /**
     * @class FieldExistChecker
     * @brief Protobuf field exist checker.
     */
    template <typename ProtoBufChecker, typename Getter>
    class FieldExistChecker: public Checker
    {
      ProtoBufChecker& checker_;
      bool exist_;
      Getter getter_;
      
    public:
      /**
       * @brief Constructor.
       *
       * @param parent checker.
       * @param exist or not.
       * @param pointer to exist check function.
       */
      FieldExistChecker(
        ProtoBufChecker& checker,
        bool exist,
        Getter getter);
      
      /**
       * @brief Destructor.
       */
      virtual ~FieldExistChecker() noexcept;
      
      /**
       * @brief Check.
       * @param throw on error flag.
       */
      bool
      check(
        bool throw_error = true)
        /*throw(eh::Exception)*/;
    };


    /**
     * @class ExpectedUtils
     * @brief Protobuf utils for checking expected.
     */
    template <typename Message>
    struct ExpectedUtils
    {
      /**
       * @brief Add simple checker.
       *
       * @param parent checker.
       * @param field name.
       * @param expected value.
       * @param pointer to field getter function.
       */
      template <typename ProtoBufChecker, typename T, typename Getter>
      const ExpValue<T>&
      add_checker(
        ProtoBufChecker& checker,
        const std::string& name,
        const ExpValue<T>& val,
        Getter getter);

      /**
       * @brief Add sequence (Repeated...) field checker.
       *
       * @param parent checker.
       * @param field name.
       * @param expected value.
       * @param pointer to field getter function.
       */
      template <typename ProtoBufChecker, typename SeqIn, typename SeqOut>
      const ExpValue<SeqIn>&
      add_seq_checker(
        ProtoBufChecker& checker,
        const std::string& name,
        const ExpValue<SeqIn>& val,
        const SeqOut& (Message::*getter)(void) const);

      /**
       * @brief Add field exist checker.
       *
       * @param parent checker.
       * @param field name.
       * @param expected (exist or not).
       * @param pointer to check exist (has_...) function.
       */
      template <typename ProtoBufChecker, typename T>
      const ExpValue<bool>&
      add_exist_checker(
        ProtoBufChecker& checker,
        const std::string& name,
        const ExpValue<bool>& val,
        T (Message::*getter)(void) const);
      
    };

    template <typename Tag>
    struct BaseTraits;

    /**
     * @class AdChecker
     * @brief Base class for protobuf Ad checkers.
     */
    template <typename Tag>
    class AdChecker: public Checker
    {
    public:
      typedef std::pair<std::string, AutoTest::Internals::CheckerHolder_var> CheckerPair;
      typedef std::list<CheckerPair> CheckerList;
      typedef typename BaseTraits<Tag>::Response Response;
      typedef typename BaseTraits<Tag>::Ad Ad;
      typedef typename BaseTraits<Tag>::Expected Expected;
      typedef typename ::google::protobuf::RepeatedPtrField<Ad>::size_type size_type;
      
    public:
      /**
       * @brief Constructor.
       *
       * @param client (user).
       * @param response expectation.
       * @param checked response creative.
       */
      AdChecker(
        const AdClient& client,
        const Expected& expected,
        size_t creative_num);
      
      /**
       * @brief Copy constructor.
       */
      AdChecker(
        const AdChecker& other);
      
      /**
       * @brief Destructor.
       */
      virtual ~AdChecker() noexcept;
      
      /**
       * @brief Check.
       * @param throw on error flag.
       */
      virtual
      bool
      check(
        bool throw_error = true)
        /*throw(CheckFailed, eh::Exception)*/;

      /**
       * @brief Get response Ad count
       * @return advertising size
       */
      virtual
      size_type
      ad_size() const = 0;
    
    protected:
      
      friend class ExpectedUtils<Ad>;
      
      CheckerList checkers_;
      Response response_;
      Expected expected_;
      size_type creative_num_;
    };
  }

   /**
   * @class TypeTraits
   * @brief RepeatedPtrField specialization 
   */
  template<typename T>
  struct TypeTraits< google::protobuf::RepeatedPtrField<T> >
  {
    typedef typename google::protobuf::RepeatedPtrField<T> SeqType;

    /**
     * @brief Compare.
     *
     * @param expected
     * @param got
     * @return true if equal
     */
    static
    bool
    equal(
      const SeqType& exp,
      const SeqType& got);

    /**
     * @brief Convert to string.
     *
     * @param value
     * @return value string representation
     */
    static
    std::string
    to_string(
      const SeqType& val);
  };

  /**
   * @class TypeTraits
   * @brief RepeatedField specialization 
   */
  template<typename T>
  struct TypeTraits< google::protobuf::RepeatedField<T> >
  {
    typedef typename google::protobuf::RepeatedField<T> SeqType;

    static
    bool
    equal(
      const SeqType& exp,
      const SeqType& got);

    static
    std::string
    to_string(
      const SeqType& val);
  };
}

#include "ProtoBufResponse.tpp"

#endif  // _AUTOTESTS_COMMONS_PROTOBUFRESPONSE_HPP
