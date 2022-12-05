#ifndef __AUTOTESTS_COMMONS_REQUEST_HPP
#define __AUTOTESTS_COMMONS_REQUEST_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include <String/StringManip.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uncopyable.hpp>
#include <HTTP/UrlAddress.hpp>
#include <HTTP/HttpMisc.hpp>

#include <tests/AutoTests/Commons/Utils.hpp>
#include <tests/AutoTests/Commons/GlobalSettings.hpp>
#include <tests/AutoTests/Commons/ClientRequest.hpp>
#include <ReferenceCounting/NullPtr.hpp>

namespace AutoTest
{
  extern const char* DEBUG_TIME_FORMAT;   // 'debug-time' format
  extern const char* DEBUG_TIME_NEW_FORMAT; // 'debug-time' new format
  extern const char* DEFAULT_USER_AGENT;  // Default User-Agent header

  struct OldTime
  {
    static const char* format();
  };

  struct NewTime
  {
    static const char* format();
  };

  /**
   * @class EqualHeaderName
   * @brief HTTP header names comparator.
   */
  struct EqualHeaderName : std::binary_function <HTTP::Header, std::string, bool>
  {
    bool
    operator() (
      const HTTP::Header& header,
      const std::string& name) const;
  };

  /**
   * @class Time
   * @brief AdServer time presentation.
   *
   * Used in requests debug-time parameters and in DB statistics
   */
  class Time : public Generics::Time
  {
  public:

    /**
     * @brief Default constructor.
     */
    Time() noexcept;

    /**
     * @brief Constructor.
     *
     * @param time
     */
    Time(const timeval& time) noexcept;

    /**
     * @brief Constructor.
     *
     * @param seconds
     * @param microseconds
     */
    Time(
      time_t time_sec,
      suseconds_t usec = 0) noexcept;

    /**
     * @brief Constructor.
     *
     * @param string time presentation.
     */
    Time(const std::string& value)
      /*throw(InvalidArgument, Exception, eh::Exception)*/;

    /**
     * @brief Constructor.
     *
     * @param string time presentation.
     */
    Time(const char* value)
      /*throw(InvalidArgument, Exception, eh::Exception)*/;

    /**
     * @brief Constructor.
     *
     * @param string time presentation.
     */
    Time(const String::SubString& value)
      /*throw(InvalidArgument, Exception, eh::Exception)*/;

    /**
     * @brief Copy constructor.
     *
     * @param time
     */
    Time(const Time& t) noexcept;

    /**
     * @brief Constructor.
     *
     * @param time
     */
    Time(const Generics::Time& t) noexcept;

    /**
     * @brief Assignment operator.
     *
     * @param time
     */
    Time& operator= (const Generics::Time& t);

    /**
     * @brief Assignment operator.
     *
     * @param time
     */
    Time& operator= (const Time& t);

    /**
     * @brief Increment operator (right).
     */
    Time& operator++ ();

    /**
     * @brief Increment operator (left).
     */
    Time operator++ (int);

    /**
     * @brief Decrement operator (right).
     */
    Time& operator-- ();

    /**
     * @brief Decrement operator (left).
     */
    Time operator-- (int);
  };

  std::ostream&
  operator <<(
    std::ostream& ostr,
    const Time& time)
    /*throw(eh::Exception)*/;

  class BaseParam;

  class BaseParamsContainer
  {
  public:

    /**
     * @brief Default constructor.
     */
    BaseParamsContainer();

    /**
     * @brief Destructor.
     *
     * Destructor for BaseRequest object.
     */
    virtual ~BaseParamsContainer() noexcept;

    /**
     * @brief Returns encode flag.
     *
     * @return true  - request's parameters need MIME encoding.
     */
    virtual bool need_encode() const = 0;

  protected:

    friend class BaseParam;
    typedef std::list<BaseParam*> ParamsList;
    ParamsList params_; //!< Params list of request

    /**
     * @brief Add new parameter to container.
     *
     * @param parameter.
     */    
    virtual void add_param(BaseParam* param);
  };

  /**
   * @class BaseRequest
   * @brief Base class for requests to AdServer.
   *
   * Build request on basis of base url and params list
   */
  class BaseRequest : public BaseParamsContainer
  {
  public:

    enum RequestType
    {
      RT_ENCODED,    // Request require MIME encoding
      RT_NOT_ENCODED // Request don't need encoding
    };

    /**
     * @brief Constructor.
     *
     * Create request with base url.
     * @param base_url base url for request
     */
    BaseRequest(
      const char* base_url,
      RequestType req_type);

    /**
     * @brief Destructor.
     *
     * Destructor for BaseRequest object.
     */
    virtual ~BaseRequest() noexcept;

    /**
     * @brief Output request into string.
     *
     * Build request url base on base_url and params list
     * and output it into string.
     * @return request url string.
     * @sa print().
     */
    virtual std::string url() const;

    /**
     * @brief Output request into ostream.
     *
     * This function is the same as url(),
     * but output result into ostream.
     * @param out basic_ostream to out request url
     * @return reference to ostream
     * @sa url().
     */
    virtual std::ostream& print(std::ostream& out) const;

    /**
     * @brief Get access to headers.
     *
     * @return request HTTP headers
     */
    const HTTP::HeaderList& headers() const;

    /**
     * @brief Get access to headers.
     *
     * @return request HTTP headers
     */
    HTTP::HeaderList& headers();

    /**
     * @brief Get request body.
     *
     * @return request HTTP body
     */
    virtual std::string body() const;

    /**
     * @brief Returns params prefix.
     *
     * Params prefix separates parameters in params list.
     * @param first parameter sign
     * @return params prefix
     */
    virtual
    const char*
    params_prefix(bool is_first = false) const;

    /**
     * @brief Returns 'equal' separator.
     *
     * Params separator between name and value.
     * @return params prefix
     */
    virtual
    const char* eql() const;

    /**
     * @brief Returns encode flag.
     *
     * @return true  - request's parameters need MIME encoding.
     */
    virtual bool need_encode() const;

    /**
     * @brief Set HTTP request decoders.
     *
     * @param Client request object.
     */
    virtual
    void
    set_decoder(ClientRequest* request) const;

  protected:

    std::ostream& print_params_(std::ostream& out) const;

    /**
     * @brief Decode request from original URL.
     *
     * @return URL
     */
    virtual void decode_(const char* url)
      /*throw(HTTP::URLAddress::InvalidURL, eh::Exception)*/;

    friend class BaseParam;

    std::string url_;           //!< Base url for request
    RequestType req_type_;      //!< Request type
    HTTP::HeaderList headers_;  //!< Request HTTP headers
  };
  
  /**
   * @class BaseParam
   * @brief Base class for HTTP request's parameters.
   *
   * Contains param name, equal symbol and
   * param value. Also contains information about param's
   * request and link to another param in params list
   * of request.
   */
  class BaseParam :
    public virtual ReferenceCounting::Interface,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:

    /**
     * @brief Constructor.
     * @param request request which param belongs to
     * @param name param's name
     */
    BaseParam(
      BaseParamsContainer* request,
      const char* name);

    /**
     * @brief Copy constructor.
     * @param request request which param belongs to
     * @param other param
     */
    BaseParam(
      BaseParamsContainer* request,
      const BaseParam& other);

    /**
     * @brief Destructor.
     *
     * Destructor for BaseParam object.
     */
    virtual ~BaseParam() noexcept;

    /**
     * @brief Clear param value.
     *
     * Make param value empty.
     * Param name and equal symbol stay without modifications,
     * and you can assign a new value to this parameter.
     */
    virtual void clear() = 0;

    /**
     * @brief Check if param value is empty.
     * @return true if param value is empty, false otherwise.
     */
    virtual bool empty () const = 0;


    /**
     * @brief Returns param value.
     *
     * @return param value as string
     */
    virtual std::string str() const = 0;

    /**
     * @brief Returns raw param value (without any encoding).
     *
     * @return raw param value as string
     */
    virtual std::string raw_str() const = 0;

    /**
     * @brief Print request parameter.
     *
     * Write param in outstream in format:
     * prefix + param name + equal symbol + param value.
     * @param out stream where param will be written to.
     * @param parameter prefix.
     * @param eql equal symbol for this param
     * @return true for successfully dump.
     */
    virtual
    bool print (
      std::ostream& out,
      const char* prefix,
      const char* eql) const;

    /**
     * @brief Set param value.
     *
     * This function sets value to parameter and, if necessary,
     * Old param value get lost.
     * @param val string representation of param value.
     * @param encode if this flag is true encode param value with mime.
     */
    virtual void set_param_val(const String::SubString& val) = 0;

  protected:
    friend class BaseRequest;
    BaseParamsContainer* request_;     //!< request of this param
    std::string name_;               //!< param name
  };

  /**
   * @class StringParam
   * @brief String request's parameter.
   */
  class StringParam : public BaseParam
  {
  public:
    typedef std::string value_type;             //!< Type of param value
    typedef const value_type& value_cref_type;  //!< Reference to value_type
  protected:
    value_type param_value_;  //!< string presentation of the param value
    bool       empty_;        //!< empty flag
    bool need_encode_;        //!< encode flag
  public:
    /**
     * @brief Constructor.
     * @param request request which param belongs to
     * @param name param's name
     */
    StringParam(
      BaseParamsContainer* request,
      const char* name);

    /**
     * @brief Copy constructor.
     * @param request request which param belongs to
     * @param other param
     */
    StringParam(
      BaseParamsContainer* request,
      const StringParam& other);

    /**
     * @brief Destructor.
     *
     * Destructor for StringParam object.
     */
    virtual ~StringParam() noexcept;

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
    virtual bool empty () const;

    /**
     * @brief Get param value.
     * @return param value, represented as string, encoded if need.
     */
    virtual std::string str() const;

    /**
     * @brief Returns raw param value (without MIME encoding).
     *
     * @return raw param value as string
     */
    virtual std::string raw_str() const;

    /**
     * @brief Set param value.
     *
     * This function sets value to parameter and, if necessary,
     * Old param value get lost.
     * @param val string representation of param value.
     */
    virtual void set_param_val(const String::SubString& val);

    /**
     * @brief Set param value (alias).
     *
     * @param val string representation of param value.
     */
    void set_param_val(const std::string& val);

    /**
     * @brief Set param value (alias).
     *
     * @param val string representation of param value.
     */    
    void set_param_val(const char* val);

    template <class T>
    StringParam& operator=(const T& val);

    /**
     * @brief Mark parameter as not encoded.
     */
    void not_encode();
    
  protected:

    /**
     * @brief Set param value.
     *
     * Set value of type T as param value. Class T must have
     * overloaded '<<' opertaor to write its value to a stream.
     * @param val value of type T to set as param value
     */
    template <class T>
    void set_param_val(const T& val);
  };


  /**
   * @class SearchParam
   * @brief Parameter that contain search URL.
   */
  class SearchParam : public StringParam
  {

    static const char* SEARCH_URL;
    
  public:
    /**
     * @brief Constructor.
     * @param request request which param belongs to
     * @param name param's name
     */
    SearchParam(
      BaseParamsContainer* request,
      const char* name);

    /**
     * @brief Copy constructor.
     * @param request request which param belongs to
     * @param other param
     */
    SearchParam(
      BaseParamsContainer* request,
      const SearchParam& other);

    /**
     * @brief Destructor.
     *
     * Destructor for StringParam object.
     */
    virtual ~SearchParam() noexcept;

    /**
     * @brief Set param value.
     *
     * This function create search referer from search keyword and
     * set it as parameter value.
     * @param search keyword.
     */
    virtual void set_param_val(const String::SubString& val);

    /**
     * @brief Returns raw search phrase (without search URL wrapper).
     *
     * @return raw param value as string
     */
    virtual std::string raw_str() const;
    
  protected:
    /**
     * @brief Set param value.
     *
     * Set value of type T as param value. Class T must have
     * overloaded '<<' opertaor to write its value to a stream.
     * @param val value of type T to set as param value
     * @param encode if this flag is true encode param value with mime.
     */
    template <class T>
    void set_param_val(const T& val);

  };

  /**
   * @class TimeParamBase
   * @brief Time request's parameter (debug-time).
   */
  template<typename TimeType>
  class TimeParamBase :
    public Time,
    public BaseParam
  {
  public:
    typedef Time value_type;             //!< Type of param value
    typedef const value_type& value_cref_type;  //!< Reference to value_type
  protected:
    bool       empty_;   //!< empty flag
  public:
    /**
     * @brief Constructor.
     * @param request request which param belongs to
     * @param name param's name
     */
    TimeParamBase(
      BaseParamsContainer* request,
      const char* name);

    /**
     * @brief Copy constructor.
     * @param request request which param belongs to
     * @param other param
     */
    TimeParamBase(
      BaseParamsContainer* request,
      const TimeParamBase& other);

    /**
     * @brief Destructor.
     *
     * Destructor for TimeParamBase object.
     */
    virtual ~TimeParamBase() noexcept;

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

    virtual bool empty () const;
    
    /**
     * @brief Get param value.
     * @return param value, represented as string.
     */
    virtual std::string str() const;

    /**
     * @brief Returns raw time string (without MIME encoding).
     *
     * @return raw param value as string
     */
    virtual std::string raw_str() const;

    /**
     * @brief Set param value.
     *
     * Set time in format DD-MM-YYYY:HH-MM-SS as parameter value.
     * Old param value get lost.
     * @param val string representation of param value.
     */
    virtual void set_param_val(const String::SubString& val);

    /**
     * @brief Set param value (alias).
     *
     * @param time as string.
     */
    void set_param_val(const std::string& val);

    /**
     * @brief Set param value (alias).
     *
     * @param time as string.
     */
    void set_param_val(const char* val);
    
    /**
     * @brief Set param value (alias).
     *
     * Set time in format DD-MM-YYYY:HH-MM-SS as parameter value.
     * If necessary, encode it with MIME.
     * Function take a parameter, which is an object of integer time_t
     * @param val time value
     */
    void set_param_val(const time_t& val);
    
    /**
     * @brief Set param value (alias).
     *
     * Set time as parameter value.
     * Function take a parameter, which is an object of class Generics::Time from UnixCommons.
     * @param val time value
     */    
    void set_param_val(const Generics::Time& val);
    
    /**
     * @brief Set param value.
     *
     * Set time  as parameter value.
     * Function take a parameter, which is an object of integer time_t
     * @param val time value
     */
    void set_param_val(const Time& val);

    /**
     * @brief Increment operator (right).
     */
    TimeParamBase& operator++ ();

    /**
     * @brief Increment operator (left).
     */
    TimeParamBase& operator++ (int);

    /**
     * @brief Decrement operator (right).
     */
    TimeParamBase& operator-- ();
    
    /**
     * @brief Decrement operator (left).
     */
    TimeParamBase& operator-- (int);
  };

  typedef TimeParamBase<OldTime> TimeParam;
  typedef TimeParamBase<NewTime> NewTimeParam;

  /**
   * @class RequestParam
   * @brief Request parameter presentation.
   *
   * This class is used for determining params of any request.
   */
  template <class Request, class Base = StringParam> 
  class  RequestParam : public Base
  {
  public:

    /**
     * @brief Constructor.
     *
     * Creates RequestParam and determines its name and default value.
     * @param request request which this param belongs to
     * @param name RequestParam's name
     * @param defs default value for parameter
     * @param set_defs this flag tells RequestParam whether or not
     * to use defs as default value for parameter
     */
    template <class T>
    RequestParam(
      Request* request,
      const char* name,
      T defs,
      bool set_defs);

    /**
     * @brief Constructor.
     */
    template<typename... Args>
    RequestParam(
      Args&&... args);

    /**
     * @brief Destructor.
     *
     * Destructor for RequestParam object.
     */
    virtual ~RequestParam() noexcept;
    
    /**
     * @brief Assignment operator.
     *
     * Allows assign param value with '=' operator.
     * @param val param's value to assign
     */
    template <class T>
    RequestParam& operator= (const T& val);

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
  };

  /**
   * @class HeaderParam
   * @brief HTTP header's parameter.
   */
  template <class Request, class Base = StringParam>
  class HeaderParam : public Base
  {
   
  public:

    /**
     * @brief Constructor.
     * @param request request which this param belongs to
     * @param name HeaderParam's name
     * @param defs default value for parameter
     * @param set_defs this flag tells HeaderParam whether or not
     * to use defs as default value for parameter
     */
    template <class T>
    HeaderParam(
      Request* request,
      const char* name,
      const T& defs,
      bool set_defs = true);

    /**
     * @brief Constructor.
     * @param request request which this param belongs to
     * @param name HeaderParam's name
     * @param defs default value for parameter
     * @param set_defs this flag tells HeaderParam whether or not
     * to use defs as default value for parameter
     */
    HeaderParam(
      Request* request,
      const char* name,
      const String::SubString& defs,
      bool set_defs = true);

    /**
     * @brief Copy constructor.
     * @param request request which param belongs to
     * @param other param
     */
    HeaderParam(Request* request,
      const HeaderParam& other);

    /**
     * Creates HeaderParam and determines its name.
     * @param request request which this param belongs to
     * @param name HeaderParam's name
     */
    HeaderParam(Request* request, const char* name);

    /**
     * @brief Destructor.
     *
     * Destructor for StringParam object.
     */
    virtual ~HeaderParam() noexcept;

    /**
     * @brief Clear param value.
     *
     * Make param value empty.
     * Param name and equal symbol stay without modifications,
     * and you can assign a new value to this parameter.
     */
    virtual void clear();

    /**
     * @brief Set param value.
     *
     * This function create search referer from search keyword and
     * set it as parameter value.
     * @param search keyword.
     */
    virtual void set_param_val(const String::SubString& val);

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
    HeaderParam& operator= (const T& val);

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
     * @brief Assignment operator.
     *
     * Allows assign param value with '=' operator.
     * @param val param's value to assign
     */
    HeaderParam& operator= (const String::SubString& val);

    /**
     * @brief Assignment operator.
     *
     * Allows assign param value with '()' operator.
     * @param val assignable value
     * @param encode tells whether or not
     * to encode param value before assignment
     * @return reference to request which param belongs to 
     */
    Request& operator() (const String::SubString& val);

  };

  /**
   * @struct ParamsGenerator
   * @brief Params factory for ComplexParam
   */
  template <class Param>
  struct ParamsGenerator
  {
    template<typename... Args>
    Param* 
    operator()(
      BaseParamsContainer* request,
      Args&&... args);
  };

  /**
   * @class ComplexParam
   * @brief Base class for complex parameters.
   *
   * This class represents complex http params that have name, index and value.
   * For example, you can use it for parameter 'g1.r=1',
   * that has name='g', index='1' and value='1';
   * or for parameter 'm.l="en"', that has name='m', index='l' and value='en'.
   * Note, that this param has one name and array of 'index-value' elements.
   * Also, it can contain own (without index) value;
   * for example - 'm=somevalue&m.l=indexvalue'
   * Class provides "access by index" semantic.
   */
  template<
      typename Request,
      typename TKey,
      typename TValue = StringParam,
      typename Generator = ParamsGenerator<TValue> >
  class ComplexParam:
    public RequestParam<Request, TValue>,
    virtual public BaseParamsContainer  
  {
    /**
     * @brief Type of base class.
     */
    typedef RequestParam<Request, TValue> Base;
    
    protected:

    typedef ReferenceCounting::SmartPtr<TValue> TValue_var;
    typedef std::map<TKey, TValue_var> ParamsMap;
    ParamsMap parameters_;  //!< list of indexed params

    public:

    /**
     * @brief Constructor.
     */
    template<typename... Args>
    ComplexParam(
      Args&&... args);

    /**
     * @brief Copy constructor.
     * @param request request which param belongs to
     * @param other param
     */
    ComplexParam(
      Request* request,
      const ComplexParam& other);

    /**
     * @brief Destructor.
     *
     * Destructor for ComplexParam object.
     */
    virtual ~ComplexParam() noexcept;

    /**
     * @brief Clear all indexed params.
     *
     * Make list of indexed param values empty.
     */
    virtual void clear_all();

    /**
     * @brief Clear param with indicated index.
     *
     * Make param value with indicated index empty.
     */
    virtual bool clear(const TKey& key);

    /**
     * @brief Clear param own value.
     *
     * Make param own value empty.
     */
    virtual void clear();

    /**
     * @brief Check if list of indexed params is empty.
     * @return true if list is empty, false otherwise.
     */
    virtual bool empty_all () const;

    /**
     * @brief Check if param with indicated index is empty.
     * @return true if param is empty, false otherwise.
     */
    virtual bool empty (const TKey& key) const;

    /**
     * @brief Check if param own value and indexed params list are empty.
     * @return true if param own value and indexed params list are empty,
     * false otherwise.
     */
    virtual bool empty () const;

    /**
     * @brief Access operator.
     *
     * Allows to get access to parameter by index.
     * For example, to get access to parameter '&m.q=car' of some request
     * you can use the following code: request.m["q"].
     * Then you can use some method of Complex::Param class to handle this param.
     * @param key param index.
     * @return special parameter wrapper to make assignement easy.
     */

    TValue& operator[] (TKey key);

    /**
     * @brief Assignment operator for indexed params.
     *
     * Allows assign param value with some index using '()' operator.
     * @param val assignable value
     * @param key param index
     * @param encode tells whether or not
     * to encode param value before assignment
     * @return reference to request which param belongs to
     */
    template <class T>
    Request& operator() (
      const T& val,
      TKey  key);

    /**
     * @brief Assigment operator.
     *
     * It allows to assign param own value using '()' operator.
     * @param val assignable value
     * @return reference to request which param belongs to
     */
    template <class T>
    Request&
    operator() (const T& val);

    /**
     * @brief Assigment operator.
     *
     * It allows to assign param own value using '=' operator.
     * @param val assignable value
     * @return reference to current object.
     */
    template <class T>
    ComplexParam&
    operator= (const T& val);

    /**
     * @brief Set param value.
     *
     * Sets param value with indicated index,
     * and ,if necessary, encode it with MIME.
     * @param val new param value
     * @param key param index
     */
    template <class T>
    void set_param_val(const T& val,  TKey key);

    /**
     * @brief Print request parameter.
     *
     * Write param in outstream with correct form.
     * @param out stream where param will be written to.
     * @param parameter prefix.
     * @param eql equal symbol for parameter
     * @return true for successfully dump.
     */
    virtual
    bool
    print(
      std::ostream& out,
      const char* prefix,
      const char* eql) const = 0;

    /**
     * @brief Returns encode flag.
     *
     * @return false always.
     */
    virtual bool need_encode() const;

  protected:

    /**
     * @brief Set param value.
     *
     * @param val new param value
     * @param key param index
     */
    void set_param_val(const std::string& val, TKey key);

    /**
     * @brief Add new parameter to container.
     *
     * @param parameter.
     */    
    virtual void add_param(BaseParam* param);

  };

  /**
   * @class RequestMemberBase
   * @brief Request member interface.
   *        Using to access request parameters.
   */
  class RequestMemberBase
  {
  public:

    /**
     * @brief Destructor.
     *
     * Destructor for RequestMemberBase object.
     */
    virtual ~RequestMemberBase() noexcept;

    /**
     * @brief Set parameter value.
     *
     * @param request
     * @param value
     */
    virtual
    void
    set_param_val(
      BaseRequest& request,
      const std::string& val) = 0;

    /**
     * @brief Set parameter value.
     *
     * @param request
     * @param value
     */
    virtual
    void
    set_param_val(
      BaseRequest& request,
      unsigned long val) = 0;

    /**
     * @brief Clear parameter.
     *
     * @param request
     */
    virtual
    void
    clear_param(
      BaseRequest& request) = 0;

    /**
     * @brief Clone member.
     *
     * @return cloned member
     */
    virtual
    RequestMemberBase* clone() const = 0;
  };

  /**
   * @class RequestMember
   * @brief Request member implementation.
   *        Using to access request parameters.
   */
  template <typename Request, typename Param>
  class RequestMember : public RequestMemberBase
  {
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
     * @brief Set parameter value.
     *
     * @param request
     * @param value
     */
    virtual
    void
    set_param_val(
      BaseRequest& request,
      const std::string& val);

    /**
     * @brief Set parameter value.
     *
     * @param request
     * @param value
     */
    virtual
    void
    set_param_val(
      BaseRequest& request,
      unsigned long val);

    /**
     * @brief Clear parameter.
     *
     * @param request
     */
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
  
  /**
   * @class RequestParamSetter
   * @brief Use for settinggs parameters.
   */
  template <typename Request>
  class RequestParamSetter
  {
  public:

    /**
     * @brief Default constructor.
     */
    RequestParamSetter(std::nullptr_t member = nullptr);
    
    /**
     * @brief Constructor.
     *
     * Create setter. 
     * @param pointer to Request member (parameter).
     */
    template <typename Param>
    RequestParamSetter(
      Param Request::* member);

    /**
     * @brief Copy constructor.
     *
     * @param other setter
     */
     RequestParamSetter(const RequestParamSetter& other);

    /**
     * @brief Destructor.
     *
     * Destructor for RequestParamSetter object.
     */
    ~RequestParamSetter() noexcept;

    /**
     * @brief Set request param value.
     *
     * @param request
     * @param parameter value
     */
    template <typename T>
    void operator() (
      Request& request,
      const T& val) const
      /*throw(eh::Exception)*/;

    /**
     * @brief Clear request param.
     *
     * @param request
     */
    void clear(
      Request& request) const;

    /**
     * @brief Check setter is empty.
     * @return true if setter is empty, false otherwise.
     */
    bool empty() const;

  private:
    RequestMemberBase* param_;  //!< Parameter name

  private:
    void
    operator =(RequestParamSetter&) noexcept;

    template <typename Param>
    void
    operator =(Param Request::* member) noexcept;
  };
}//namespace AutoTest

#include "BaseRequest.ipp"

#endif  // __AUTOTESTS_COMMONS_REQUEST_HPP
