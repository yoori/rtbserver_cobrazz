
#ifndef _AUTOTESTS_COMMONS_REQUEST_OPENRTBREQUEST_HPP
#define _AUTOTESTS_COMMONS_REQUEST_OPENRTBREQUEST_HPP

#include <Generics/Uncopyable.hpp>
#include "BaseRequest.hpp"
#include "DebugSizeParam.hpp"

namespace AutoTest
{

  namespace OpenRtb
  {
    /**
     * @struct TagDescriptor
     * @brief Open RTB parameter tag descriptor.
     */
    struct TagDescriptor
    {
      // tag open string
      const char* begin;
      // tag close string
      const char* end;
    };

    enum EscapeJSON
    {
      JSON_ESCAPE,
      JSON_RAW
    };

    enum RequestFlags
    {
      RF_SET_DEFS = 0x01,   // set params to defaults
      RF_SEND_BANNER = 0x02 // send banner object in request even if it's empty
    };

    // Tag for empty (raw) parameter
    extern const TagDescriptor EMPTY_TAG;
    // Tag for string (quoted) parameter
    extern const TagDescriptor STRING_TAG;
    // Tag for struct (group) parameter
    extern const TagDescriptor STRUCT_TAG;
    // Tag for array of structs (group) parameter
    extern const TagDescriptor ARRAY_TAG;

    typedef const TagDescriptor& TagConst;
  }

  /**
   * @class OpenRTBRequest
   * @brief Presentation of openRTB bid request.
   */
  class OpenRTBRequest : public BaseRequest
  {

    class Parameter :
      public StringParam
    {
    protected:
      OpenRtb::TagConst tag_;
      OpenRtb::EscapeJSON escape_;
    public:

      /**
       * @brief Constructor.
       * @param parent parameter
       * @param parameter name
       * @param parameter tag
       */
      Parameter(
        BaseParamsContainer* container,
        const char* name,
        OpenRtb::TagConst tag,
        OpenRtb::EscapeJSON escape = OpenRtb::JSON_ESCAPE);

      /**
       * @brief Copy constructor.
       * @param request request which param belongs to
       * @param other param
       */
      Parameter(
        BaseParamsContainer* container,
        const Parameter& other);

      /**
       * @brief Destructor.
       */
      virtual ~Parameter() noexcept;

      /**
       * @brief Print parameter.
       *
       * @param out stream where param will be written to.
       * @param indentation.
       */
      virtual void print(
        std::ostream& out,
        unsigned long indent,
        bool print_name) const;

      /**
       * @brief Print request parameter.
       *
       * @param out stream where param will be written to.
       * @param parameter prefix.
       * @param eql equal symbol for this param
       * @return false always.
       */
      virtual
      bool print (
        std::ostream& out,
        const char* prefix,
        const char* eql) const;

      OpenRtb::TagConst tag() const;
    };

    /**
     * @class Group
     * @brief Presentation of openRTB group (struct) parameter.
     */
    class Group :
       public Parameter,
       virtual public BaseParamsContainer
    {
      DECLARE_EXCEPTION(GroupAccessError, eh::DescriptiveException);

    public:

      typedef void Type;
      
      /**
       * @brief Constructor.
       * @param parent parameter
       * @param parameter name
       * @param parameter tag
       */
      Group(
        BaseParamsContainer* container,
        const char* name,
        bool required = false);

      /**
       * @brief Destructor.
       */
      virtual ~Group() noexcept;

      /**
       * @brief Print parameter.
       *
       * @param out stream where param will be written to.
       * @param indentation.
       */
      virtual void print(
        std::ostream& out,
        unsigned long indent,
        bool print_name) const;

      /**
       * @brief Check parameter empty.
       * @return true if param value is empty, false otherwise.
       */
      virtual bool empty() const;

      /**
       * @brief Returns encode flag.
       *
       * @return false always.
       */
      virtual bool need_encode() const;

      virtual void set_param_val(
        const String::SubString& val);

    private:
      bool required_;
    };

    template<class ParamType>
    struct ParamsGenerator
    {
      template<typename... Args>
      ParamType*
      operator()(
        BaseParamsContainer* request,
        Args&&... args);
    };

    /**
     * @class ParamsArray
     * @brief Presentation of openrtb array parameters.
     */
    template<class ParamType>
    class ParamsArray :
          public ComplexParam< OpenRTBRequest, size_t, ParamType, ParamsGenerator<ParamType> >
    {
      typedef ComplexParam< OpenRTBRequest, size_t, ParamType, ParamsGenerator<ParamType> > Base;

      /// Default group id for parameters
      static const size_t DEFAULT_INDEX = 0;

      size_t current_index_;

    public:

      typedef typename ParamType::Type Type;

      /**
       * @brief Constructor.
       *
       * @param request
       * @param group
       * @param name
       * @param defs
       * @param defs flag
       */
      template <class T>
      ParamsArray(
        OpenRTBRequest* request,
        BaseParamsContainer* group,
        const char* name,
        const T& defs,
        unsigned short flags =
          OpenRtb::RF_SET_DEFS | OpenRtb::RF_SEND_BANNER);
        //bool set_defs = true);

      /**
       * @brief Constructor.
       *
       * @param request
       * @param group
       * @param name
       */
      ParamsArray(
        OpenRTBRequest* request,
        BaseParamsContainer* group,
        const char* name);

      /**
       * @brief Constructor.
       *
       * @param size
       * @param request
       * @param group
       * @param name
       * @param set_defs
       */
      ParamsArray(
        size_t size,
        OpenRTBRequest* request,
        BaseParamsContainer* group,
        const char* name,
        unsigned short flags =
          OpenRtb::RF_SET_DEFS | OpenRtb::RF_SEND_BANNER);
        //bool set_defs = true);

      /**
       * @brief Copy constructor.
       * @param request
       * @param group
       * @param other param
       */
      ParamsArray(
        OpenRTBRequest* request,
        BaseParamsContainer* group,
        const ParamsArray& other);

      /**
       * @brief Assigment operator
       *
       * Allows to assign 1st group's param value with '=' operator.
       * @param val param's value to assign
       * @return reference to the current object.
       */
      template <class T>
      ParamsArray& operator= (const T& val);

      /**
       * @brief Assignment operator.
       *
       * Allows to assign param value with indicated group_id with '()' operator.
       * @param val assignable value
       * @param group_id id in group
       * @param encode tells whether or not
       * to encode param value before assignment
       * @return reference to request which param belongs to
       */
      template <class T>
      OpenRTBRequest& operator() (
        const T& val,
        size_t index);

      template <class T>
      OpenRTBRequest& operator() (
        const T& val);

      /**
       * @brief Clear param value for group.
       *
       * @param group id
       */
      virtual bool clear(
        unsigned int index);

      virtual void clear();

      /**
       * @brief Check if param value is empty.
       * @return true if param value is empty, false otherwise.
       */
      bool
      empty() const;

      /**
       * @brief Set param value.
       *
       * @param val string representation of param value.
       * @param encode if this flag is true encode param value with mime.
       */
      virtual void
      set_param_val(
        const String::SubString& val);

      /**
       * @brief Print request parameter to body.
       *
       * @param out stream where param will be written to.
       * @param indentation.
       */
      virtual
      bool print(
        std::ostream& out,
        const char* prefix,
        const char* eql) const;

      /**
       * @brief Print parameter.
       *
       * @param out stream where param will be written to.
       * @param indentation.
       */
      void
      print(
        std::ostream& out,
        unsigned long indent,
        bool print_name) const;

    };

    Group body_;

    /**
     * @class ProxyParam
     * @brief Use for access to group parameters.
     */
    template <class ParamType>
    class ProxyParam : public BaseParam
    {
      ParamType& param_;

    public:

      typedef typename ParamType::Type Type;

      /**
       * @brief Constructor.
       *
       * @param request
       * @param internal parameter
       */
      ProxyParam(
        OpenRTBRequest* request,
        ParamType& param);

      /**
       * @brief Destructor.
       */
      virtual ~ProxyParam() noexcept;

      /**
       * @brief Set param value.
       *
       * This function sets value to parameter,
       * @param value
       */
      virtual void set_param_val(
        const String::SubString& val);

      /**
     * @brief Clear param value.
     *
     * Make param value empty.
     */
      virtual void clear();

      /**
       * @brief Check if param value is empty.
       * @return true if param value is empty, false otherwise.
       */
      virtual bool empty () const;

      /**
       * @brief Returns param value.
       *
       * @return param value as string
       */
      virtual std::string str() const;

      /**
       * @brief Returns raw param value (without any encoding).
       *
       * @return raw param value as string
       */
      virtual std::string raw_str() const;

      /**
       * @brief Print request parameter.
       *
       * Write param in outstream in format:
       * @param out.
       * @param parameter prefix.
       * @param eql equal symbol.
       * @return false always.
       */
      virtual
      bool print (
        std::ostream& out,
        const char* prefix,
        const char* eql) const;

      /**
       * @brief Assigment operator
       *
       * Allows to assign parameter value.
       * @param value.
       * @return parameter.
       */
      template <class T>
      ParamType& operator= (const T& val);

      /**
       * @brief Assigment operator
       *
       * Allows to assign parameter value.
       * @param value.
       * @return request.
       */
      template <class T>
      OpenRTBRequest& operator() (const T& val);
    };

    typedef DebugSizeParam<OpenRTBRequest> DebugSize;

  public:

    template<OpenRtb::TagConst Tag>
    struct OpenRtbTraits
    {
      typedef std::string Type;
    };

    /**
     * @class Param
     * @brief Presentation of openRTB request parameter.
     */
    template <OpenRtb::TagConst Tag = OpenRtb::EMPTY_TAG,
              OpenRtb::EscapeJSON escape = OpenRtb::JSON_ESCAPE>
    class Param :
      public Parameter
    {
    protected:
      OpenRTBRequest* request_;

    public:

      typedef typename OpenRtbTraits<Tag>::Type Type;
      
      /**
       * @brief Constructor.
       * @param request request which this param belongs to
       * @param group
       * @param name Param's name
       * @param defs default value for parameter
       * @param set_defs this flag tells Param whether or not
       * to use defs as default value for parameter
       */
      template <class T>
      Param(
        OpenRTBRequest* request,
        BaseParamsContainer* group,
        const char* name,
        const T& defs,
        bool set_defs = true);

      /**
       * @brief Copy constructor.
       * @param request request which param belongs to
       * @param group
       * @param other param
       */
      Param(
        OpenRTBRequest* request,
        BaseParamsContainer* group,
        const Param& other);

      /**
       * Creates Param and determines its name.
       * @param request request which this param belongs to
       * @param name Param's name
       */
      Param(
        OpenRTBRequest* request,
        BaseParamsContainer* group,
        const char* name);

      /**
       * @brief Destructor.
       *
       * Destructor for Param object.
       */
      virtual ~Param() noexcept;

      /**
       * @brief Assigment operator
       *
       * Allows to assign parameter value.
       * @param value.
       * @return parameter.
       */
      template <class T>
      Param&
      operator=(
        const T& val);

      /**
       * @brief Assigment operator
       *
       * Allows to assign parameter value.
       * @param value.
       * @return request.
       */
      template <class T>
      OpenRTBRequest&
      operator() (
        const T& val);
    };

    typedef Param<> OpenRTBNumber;
    typedef Param<OpenRtb::EMPTY_TAG, OpenRtb::JSON_RAW> OpenRTBRaw;
    typedef Param<OpenRtb::STRING_TAG> OpenRTBString;
    typedef ParamsArray<OpenRTBNumber> OpenRTBArray;
    typedef ParamsArray<OpenRTBString> OpenRTBStringArray;
    typedef ParamsArray<OpenRTBRaw> OpenRTBRawArray;
    typedef RequestParam<OpenRTBRequest> DebugParam;

    class ImpGroup : public Group
    {
      Group banner_;
      Group ext_;
      Group video_;
      Group video_ext_;
    protected:
      OpenRTBRequest* request_;
    public:

      /**
       * @brief Constructor.
       * @param request
       * @param set_defs
       */
      ImpGroup(
        OpenRTBRequest* request,
        BaseParamsContainer* group,
        const char* name,
        unsigned short flags =
          OpenRtb::RF_SET_DEFS | OpenRtb::RF_SEND_BANNER);

      /**
       * @brief Constructor.
       * @param request
       * @param other imp group
       */
      ImpGroup(
        OpenRTBRequest* request,
        BaseParamsContainer* group,
        const ImpGroup& other);

      /**
       * @brief Destructor.
       */
      virtual ~ImpGroup() noexcept;

      /**
       * @brief Represents OpenRTB adslot width.
       */
      OpenRTBNumber width;

      /**
       * @brief Represents OpenRTB adslot height.
       */
      OpenRTBNumber height;

      /**
       * @brief Represents OpenRTB adslot attributes.
       */
      OpenRTBStringArray battr;

      /**
       * @brief Represents OpenRTB adslot pos.
       */
      OpenRTBNumber pos;

      /**
       * @brief Represents OpenRTB adslot min_cpm_price.
       */
      OpenRTBNumber min_cpm_price;

      /**
       * @brief Represents OpenRTB adslot min_cpm_price_currency_code.
       */
      OpenRTBString min_cpm_price_currency_code;

      /**
       * @brief Represents OpenRTB.id.
       */
      OpenRTBString id;

      /**
       * @brief Represents request.imp.banner.ext.matching_ad_id
       */
      OpenRTBRawArray matching_ad_id;

      /**
       * @brief Represents request.imp.banner.ext.type.
       */
      OpenRTBNumber type;

      /**
       * @brief Represents request.imp.secure
       */
      OpenRTBNumber secure;

      /**
       * @brief Represents request.imp.video.mimes
       */
      OpenRTBRawArray mimes;

      /**
       * @brief Represents request.imp.video.minduration
       */
      OpenRTBRaw minduration;

      /**
       * @brief Represents request.imp.video.maxduration
       */
      OpenRTBRaw maxduration;

      /**
       * @brief Represents request.imp.video.protocol
       */
      OpenRTBRawArray protocol;

      /**
       * @brief Represents request.imp.video.playbackmethod
       */
      OpenRTBRawArray playbackmethod;

      /**
       * @brief Represents request.imp.video.companionad
       */
      OpenRTBRaw companionad;

      /**
       * @brief Represents request.imp.video.startdelay
       */
      OpenRTBRaw startdelay;

      /**
       * @brief Represents request.imp.video.linearity
       */
      OpenRTBRaw linearity;

      /**
       * @brief Represents request.imp.video.h
       */
      OpenRTBNumber video_height;

      /**
       * @brief Represents request.imp.video.w
       */
      OpenRTBNumber video_width;

      /**
       * @brief Represents request.imp.video.pos
       */
      OpenRTBNumber video_pos;

      /**
       * @brief Represents request.imp.video.battr
       */
      OpenRTBArray video_battr;

      /**
       * @brief Represents request.imp.video.ext.adtype
       */
      OpenRTBRaw ext_adtype;
    };

    typedef ParamsArray<ImpGroup> ImpGroups;

    ImpGroups imp;

  private:
    Group site_;
    Group site_ext_;
    Group device_;
    Group user_;
    Group ext_;

  public:
    /// Base url for all bid requests
    static const char* BASE_URL;
    static const char* DEFAULT_IP;

    //OpenRTBRequest members
    typedef RequestParamSetter<OpenRTBRequest> Member;
    typedef RequestParamSetter<ImpGroup> ImpMember;

    /**
     * @brief Constructor.
     *
     * Create the openRTB bid request
     * and sets default values for params.
     * @param flags bid request flags
     */
    explicit OpenRTBRequest(unsigned short flags =
      OpenRtb::RF_SET_DEFS | OpenRtb::RF_SEND_BANNER);

    /**
     * @brief Copy constructor.
     *
     * @param other request
     */
    OpenRTBRequest(const OpenRTBRequest& other);

    /**
     * @brief Destructor.
     */
    virtual ~OpenRTBRequest() noexcept;

    /**
     * @brief Get request body.
     *
     * @return request HTTP body
     */
    virtual std::string body() const;


    /**
     * @brief Represents 'Content-Type' HTTP header.
     */
    HeaderParam<OpenRTBRequest> content_type;

    /**
     * @brief Represents OpenRTB adslot width.
     */
    ProxyParam<OpenRTBNumber> width;

    /**
     * @brief Represents OpenRTB adslot height.
     */
    ProxyParam<OpenRTBNumber> height;

    /**
     * @brief Represents OpenRTB adslot attributes.
     */
    ProxyParam<OpenRTBStringArray> battr;

    /**
     * @brief Represents OpenRTB adslot pos.
     */
    ProxyParam<OpenRTBNumber> pos;

    /**
     * @brief Represents OpenRTB adslot min_cpm_price.
     */
    ProxyParam<OpenRTBNumber> min_cpm_price;

    /**
     * @brief Represents OpenRTB adslot min_cpm_price_currency_code.
     */
    ProxyParam<OpenRTBString> min_cpm_price_currency_code;

    /**
     * @brief Represents OpenRTB.id.
     */
    ProxyParam<OpenRTBString> id;

    /**
     * @brief Represents OpenRTB blocked creative categories
     */
    OpenRTBStringArray bcat;

    /**
     * @brief Represents OpenRTB site categories.
     */
    OpenRTBStringArray cat;

    /**
     * @brief Represents OpenRTB.referer.
     */
    OpenRTBString referer;

    /**
     * @brief Represents OpenRTB.ip.
     */
    OpenRTBString ip;

    /**
     * @brief Represents OpenRTB.user_agent.
     */
    OpenRTBString user_agent;

    /**
     * @brief Represents OpenRTB.external_user_id.
     */
    OpenRTBString external_user_id;

    /**
     * @brief Represents OpenRTB.user_id.
     */
    OpenRTBString user_id;

    /**
     * @brief Represents OpenRTB.ext.is_test param (OpenX extension)
     */
    OpenRTBNumber is_test;

    /**
     * @brief Represents OpenRTB.ext.is_test string param
     *
     * Used only for test purposes. Adserver must ignore string param
     */
    OpenRTBString is_test_string;

    /**
     * @brief Represents OpenRTB.request_id.
     */
    OpenRTBString request_id;

    /**
     * @brief Proxy param for request.imp.banner.ext.matching_ad_id
     */
    ProxyParam<OpenRTBRawArray> matching_ad_id;

    /**
     * @brief Proxy param for request.imp.banner.ext.type
     */
    ProxyParam<OpenRTBNumber> type;

    /**
     * @brief Proxy param for request.imp.secure
     */
    ProxyParam<OpenRTBNumber> secure;

    /**
     * @brief Proxy param for request.imp.video.mimes
     */
    ProxyParam<OpenRTBRawArray> mimes;

    /**
     * @brief Proxy param for request.imp.video.minduration
     */
    ProxyParam<OpenRTBRaw> minduration;

    /**
     * @brief Proxy param for request.imp.video.maxduration
     */
    ProxyParam<OpenRTBRaw> maxduration;

    /**
     * @brief Proxy param for request.imp.video.protocol
     */
    ProxyParam<OpenRTBRawArray> protocol;

    /**
     * @brief Proxy param for request.imp.video.playbackmethod
     */
    ProxyParam<OpenRTBRawArray> playbackmethod;

    /**
     * @brief Proxy param for request.imp.video.companionad
     */
    ProxyParam<OpenRTBRaw> companionad;

    /**
     * @brief Proxy param for request.imp.video.startdelay
     */
    ProxyParam<OpenRTBRaw> startdelay;

    /**
     * @brief Proxy param for request.imp.video.linearity
     */
    ProxyParam<OpenRTBRaw> linearity;

    /**
     * @brief Proxy param for request.imp.video.h
     */
    ProxyParam<OpenRTBNumber> video_height;

    /**
     * @brief Proxy param for request.imp.video.w
     */
    ProxyParam<OpenRTBNumber> video_width;

    /**
     * @brief Proxy param for request.imp.video.pos
     */
    ProxyParam<OpenRTBNumber> video_pos;

    /**
     * @brief Proxy param for request.imp.video.battr
     */
    ProxyParam<OpenRTBArray> video_battr;

    /**
     * @brief Proxy param for request.imp.video.ext.adtype
     */
    ProxyParam<OpenRTBRaw> ext_adtype;

    /**
     * @brief Represents site.ext.ssl_enabled
     */
    OpenRTBNumber ssl_enabled;

    /**
     * @brief Represents aid debug parameter.
     *
     * Publisher account id.
     */
    DebugParam aid;

    /**
     * @brief Represents src debug parameter.
     *
     * Name of RTB system.
     */
    DebugParam src;

    /**
     * @brief Represents random parameter.
     *
     * random value.
     */
    DebugParam random;

    /**
     * @brief Represents 'debug.ccg' param.
     *
     * Frontend log XML-result of campaign selection.
     */
    DebugParam debug_ccg;

    /**
     * @brief Represents 'debug.time' param.
     *
     * Determines time of request.
     * Parameter is for debugging/testing purposes only.
     */
    RequestParam<OpenRTBRequest, NewTimeParam> debug_time;

    /**
     * @brief Represents 'debug.size' param.
     *
     * Redefines protocol banner size value for indicated/all
     * banners of request.
     * Usefull for testing - no need to create unique sizes
     * with WIDTHxHEIGHT protocol name.
     */
    DebugSize debug_size;
  };

  template<>
  OpenRTBRequest::ParamsArray<OpenRTBRequest::ImpGroup>::ParamsArray(
    size_t size,
    OpenRTBRequest* request,
    BaseParamsContainer* group,
    const char* name,
    unsigned short flags);
}



#include "OpenRTBRequest.ipp"

#endif  // _AUTOTESTS_COMMONS_REQUEST_OPENRTBREQUEST_HPP

