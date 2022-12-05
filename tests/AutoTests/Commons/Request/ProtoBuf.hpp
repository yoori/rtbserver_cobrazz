
#ifndef _AUTOTESTS_COMMONS_REQUEST_PROTOBUF_HPP
#define _AUTOTESTS_COMMONS_REQUEST_PROTOBUF_HPP

#include <type_traits>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_enum_reflection.h>
#include "BaseRequest.hpp"

namespace AutoTest
{
  namespace ProtoBuf
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    typedef google::protobuf::Reflection Reflection;
    typedef google::protobuf::Message Message;
    typedef google::protobuf::Descriptor Descriptor;
    typedef google::protobuf::FieldDescriptor FieldDescriptor;
    typedef google::protobuf::EnumValueDescriptor EnumValueDescriptor;
    typedef google::protobuf::EnumDescriptor EnumDescriptor;

    const FieldDescriptor*
    get_field(
      const Descriptor* descriptor,
      const std::string& name) /*throw(Exception)*/;
    
    void clear(
      Message* message,
      const std::string& name);
      
    bool empty(
      Message* message,
      const std::string& name);

    /**
     * @class Type
     * @brief Protobuf field wrapper.
     */
    template <typename ST, typename GT>
    struct Type
    {
      typedef ST SetterType;
      typedef GT NestedType;
      typedef void (Reflection::*Setter)(Message*, const FieldDescriptor*, ST) const;
      typedef GT (Reflection::*Getter)(const Message&, const FieldDescriptor*) const;
      
      static const Setter setter_;
      static const Getter getter_;

      /**
       * @brief Set protobuf field value.
       * @param Protobuf message 
       * @param field name
       * @param value
       */
      template <typename T>
      void
      set_value(
        Message* message,
        const std::string& name,
        T value);

      /**
       * @brief Get protobuf field value.
       * @param Protobuf message 
       * @param field name
       */
      GT
      get_value(
        Message* message,
        const std::string& name);
    };

    typedef Type<const EnumValueDescriptor*, const EnumValueDescriptor*> EnumType;

    /**
     * @class Enum
     * @brief Protobuf enum wrapper.
     */
    template <typename T>
    struct Enum
    {
      typedef T SetterType;
      typedef T NestedType;
      static const EnumDescriptor* descriptor_;

      /**
       * @brief Set protobuf enum value.
       * @param Protobuf message 
       * @param field name
       * @param value
       */
      void
      set_value(
        Message* message,
        const std::string& name,
        T value);

      /**
       * @brief Get protobuf enum value.
       * @param Protobuf message 
       * @param field name
       */
      T get_value(
        Message* message,
        const std::string& name);
    };

    typedef Type<bool, bool> Bool;
    typedef Type<int, int> Int;
    typedef Type<unsigned, unsigned> UInt;
    typedef Type<std::string, std::string> String;
    typedef google::protobuf::RepeatedField<int> RepeatedInt;
    typedef Type<int, const RepeatedInt&> IntSeq;
    typedef google::protobuf::RepeatedField<unsigned> RepeatedUInt;
    typedef Type<unsigned, const RepeatedUInt&> UIntSeq;
    typedef google::protobuf::RepeatedPtrField<std::string> RepeatedString;
    typedef Type<std::string, const RepeatedString&> StringSeq;
  }

  template<typename Request, typename Setter>
  class BidParam : public BaseParam
  {
    DECLARE_EXCEPTION(NotSupported, eh::DescriptiveException);
    
    typedef google::protobuf::Message Message;

  public:

    typedef typename Setter::SetterType Type;
    
    /**
     * @brief Constructor.
     * @param request request which this param belongs to
     * @param obj - protobuf message
     * @param parameter name
     * @param defs default value for parameter
     * @param set_defs this flag tells whether or not
     * to use defs as default value for parameter
     */
    template <typename T>
    BidParam(
      Request* request,
      Message* message,
      const char* name,
      T def,
      bool set_defs = true);
    
    /**
     * @brief Constructor.
     * @param request request which this param belongs to
     * @param obj - protobuf message
     * @param parameter name
     */
    BidParam(
      Request* request,
      Message* message,
      const char* name);

    /**
     * @brief Copy constructor.
     * @param request request which this param belongs to
     * @param obj - protobuf message
     * @param other parameter
     */
    BidParam(
      Request* request,
      Message* message,
      const BidParam& other);
    
    /* @brief Destructor.
     *
     * Destructor for BidParam object.
     */
    virtual ~BidParam() noexcept;
    
    /**
     * @brief Clear param value.
     *
     * Make param value empty.
     * Param name and equal symbol stay without modifications,
     * and you can assign a new value to this parameter.
     */
    virtual void clear();
    
    /**
     * @brief Check if param value is empty.
     * @return true if param value is empty, false otherwise.
     */
    virtual bool empty() const;
    
    /**
     * @brief Returns param value.
     *
     * Not supported.
     */
    virtual std::string str() const;
    
    /**
     * @brief Returns raw param value (without any encoding).
     *
     * Not supported.
     */
    virtual std::string raw_str() const;
    
    /**
     * @brief Set param value.
     *
     * Not supported.
     */
    virtual void set_param_val(const String::SubString&);
    
    /**
     * @brief Print request parameter.
     *
     * Write param in outstream in format:
     * param name + equal symbol + param value.
     * @param out stream where param will be written to.
     * @param parameter prefix.
     * @param eql equal symbol for parameter
     * @return true for successfully dump.
     */
    virtual
    bool print(
      std::ostream& out,
      const char* prefix,
      const char* eql) const;
    
    /**
     * @brief Assignment operator.
     *
     * Allows assign param value with '=' operator.
     * @param val param's value to assign
     */
    template <class T>
    BidParam& operator= (const T& val);

    /**
     * @brief Assignment operator.
     *
     * @return stored value.
     */
    typename Setter::NestedType
    operator*();
    
    /**
     * @brief Get stored value.
     *
     * @return stored value.
     */
    typename Setter::NestedType
    get();
    
    /**
     * @brief Assignment operator.
     *
     * Allows assign param value with '()' operator.
     * @param val assignable value
     * @param encode tells whether or not
     * to encode param value before assignment
     * @return reference to request which param belongs to 
     */
    template <class T>
    Request& operator() (const T& val);

    /**
     * @brief Get parameter name.
     *
     * @return reference to request which param belongs to 
     */
    const std::string& name() const;
   
  private:
    
    template <typename T>
    void set_param_val(T val);
    
  private:
    Message* message_;
  };


  /**
   * @class RequestMember (ProtoBuf parameter (BidParam) specialization)
   * @brief Request member implementation.
   */
  template <typename Request, typename Setter>
  class RequestMember< Request, BidParam<Request, Setter > > : public RequestMemberBase
  {
    typedef BidParam<Request, Setter> Param;

    /**
     * @struct ProtoBufMemberSetter
     * @brief BidParam set helper
     */
    struct MemberSetter
    {
      DECLARE_EXCEPTION(InvalidParameter, eh::DescriptiveException);

      /**
       * @brief Set int value (for integral types).
       *
       * @param Bid parameter
       * @param value
       */
      template<typename T, typename Arg>
      void
      set_int(
        T& param,
        Arg arg,
        typename std::enable_if<
          std::is_integral<typename T::Type>::value>::type* = 0);

      /**
       * @brief Set int value (for non-integral types).
       *
       * @param Bid parameter
       * @param value
       */
      template<typename T, typename Arg>
      void
      set_int(
        T& param,
        Arg arg,
        typename std::enable_if<
          !std::is_integral<typename T::Type>::value>::type* = 0);

      /**
       * @brief Set string value (for integral types).
       *
       * @param Bid parameter
       * @param value
       */
      template<typename T>
      void
      set_string(
        T& param,
        const std::string& arg,
        typename std::enable_if<
          std::is_integral<typename T::Type>::value>::type* = 0);

      /**
       * @brief Set string value (for non integral types).
       *
       * @param Bid parameter
       * @param value
       */
      template<typename T>
      void
      set_string(
        T& param,
        const std::string& arg,
        typename std::enable_if<
          !std::is_integral<typename T::Type>::value>::type* = 0);
    };
   
  public:
    /**
     * @brief Constructor.
     *
     * @param pointer to request member
     */
    RequestMember(
      Param Request::* member);

    /**
     * @brief Destructor.
     *
     * Destructor for RequestMember object.
     */
    virtual ~RequestMember() noexcept;

    /**
     * @brief Get parameter for request member.
     *
     * @param request
     * @return request parameter
     */
    virtual
    void
    set_param_val(
      BaseRequest& request,
      const std::string& val);

    virtual
    void
    set_param_val(
      BaseRequest& request,
      unsigned long val);

    virtual
    void
    clear_param(
      BaseRequest& request);

    /**
     * @brief Clone member.
     *
     * @return cloned member
     */
    virtual
    RequestMemberBase* clone() const;
    
  private:
    Param Request::* member_; //!< Pointer to request's parameter member
  };
}

#include "ProtoBuf.ipp"

#endif  //  _AUTOTESTS_COMMONS_REQUEST_PROTOBUF_HPP
