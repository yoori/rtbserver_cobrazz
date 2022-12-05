// @file Xslt/LibxsltExFunctions.hpp

#ifndef LIBXSLTEXFUNCTIONS_HPP_INCLUDED
#define LIBXSLTEXFUNCTIONS_HPP_INCLUDED

#include <string>

#include <libxml/xpathInternals.h>

#include <libxslt/extensions.h>
#include <libxslt/xsltutils.h>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/PosixLock.hpp>

#include <String/StringManip.hpp>
#include <Stream/MemoryStream.hpp>


namespace AdServer
{

  /**
   * Base abstraction for external XSLT function
   */
  class XslFunction :
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    /**
     * Perform registrate fun_namespace::fun_name in XSLT engine
     * namespace and function name must be stored to do unregistration
     * @param fun_namespace The namespace for XSLT function
     * @param fun_name The name of external XSLT function
     */
    virtual void
    registrate(const char* fun_namespace, const char* fun_name)
      /*throw(Exception)*/ = 0;

    /**
     * Perform unregistration previously registered function
     */
    virtual void
    unregistrate()
      /*throw(Exception)*/ = 0;

    /**
     * @return The pointer to C callback function that can be registered
     * as external XSLT function
     */
    virtual xmlXPathFunction
    get_function() noexcept = 0;

    /**
     * Useful RAII wrapper for string allocated in libxslt
     */
    struct LibxsltCharDestroyer
    {
      void
      operator ()(xmlChar* text)
      {
        xmlFree(text);
      }
    };

    typedef std::unique_ptr<xmlChar, LibxsltCharDestroyer> CharPtr;

  protected:
    /**
     * reference countable - protected destructor
     */
    virtual
    ~XslFunction() noexcept
    {
    }
  };

  typedef ReferenceCounting::SmartPtr<XslFunction> XslFunction_var;

  /**
   * Implement reg/unreg abilities in libxslt engine
   */
  class XslFunctionRegistrar : public XslFunction
  {
    public:
      typedef Sync::PosixGuard Guard;
      typedef Sync::PosixMutex Mutex;

      XslFunctionRegistrar() noexcept;

      XslFunctionRegistrar(const char* fun_namespace,
        const char* fun_name) /*throw(Exception)*/
        : fun_namespace_(fun_namespace),
          fun_name_(fun_name)
      {
        registrate(fun_namespace_.c_str(), fun_name_.c_str());
      }

      /**
       * Register function in libxslt engine. Body function will
       * be defined in descendants, through virtual get_function call.
       * @param fun_namespace The namespace for XSLT function
       * @param fun_name The name of external XSLT function
       */
      virtual void
      registrate(const char* fun_namespace, const char* fun_name)
        /*throw(Exception)*/
      {
        const char FUN[] = "XslFunctionRegistrar::registrate()";

        int res;
        {
          Guard lock(mutex);
          res =
            xsltRegisterExtModuleFunction(
              reinterpret_cast<const xmlChar*>(fun_name),
              reinterpret_cast<const xmlChar*>(fun_namespace),
              get_function());
        }
        if (res)
        {
          Stream::Error ostr;
          ostr << FUN << ": XSLT external function registration fail. "
            "Namespace: " << fun_namespace << ", name: " << fun_name
            << ", result code: " << res;
          throw Exception(ostr);
        }
        fun_namespace_ = fun_namespace;
        fun_name_ = fun_name;
      }

      /**
       * Perform unregistration previously registered function
       */
      virtual void
      unregistrate() /*throw(Exception)*/
      {
        const char FUN[] = "XslFunctionRegistrar::unregistrate()";

        int res;
        {
          Guard lock(mutex);
          res =
            xsltUnregisterExtModuleFunction(
              reinterpret_cast<const xmlChar*>(fun_name_.c_str()),
              reinterpret_cast<const xmlChar*>(fun_namespace_.c_str()));
        }
        fun_namespace_.clear();
        fun_name_.clear();
        if (res)
        {
          Stream::Error ostr;
          ostr << FUN << ": XSLT external function unregistration fail. "
            "Namespace: " << fun_namespace_ << ", name: " << fun_name_
            << ", result code: " << res;
          throw Exception(ostr);
        }
      }

      static Mutex mutex;
    protected:

      /**
       * Perform unregistration, protected as refcountable
       */
      virtual
      ~XslFunctionRegistrar() noexcept
      {
        try
        {
          unregistrate();
        }
        catch (const Exception&)
        {
        }
      }
    private:

      std::string fun_namespace_;
      std::string fun_name_;
  };

  /**
   * Implement need for us external XSLT functions (conversion routine).
   */
  template <typename Functor>
  class StringConvertFunction :
    public XslFunctionRegistrar
  {
  public:
    typedef XslFunctionRegistrar::CharPtr CharPtr;

    /**
     * Create unregistred function
     */
    StringConvertFunction();

    /**
     * Create and register function
     * @param fun_namespace The namespace for XSLT function
     * @param fun_name The name of external XSLT function
     */
    StringConvertFunction(const char* fun_namespace, const char* fun_name);

    /**
     * Make-function
     * @param fun_namespace The namespace for XSLT function
     * @param fun_name The name of external XSLT function
     * @return already registred XSLT function.
     */
    static ReferenceCounting::SmartPtr<StringConvertFunction<Functor> >
    create(const char* fun_namespace, const char* fun_name) noexcept
    {
      return new StringConvertFunction<Functor>(fun_namespace, fun_name);
    }

    /**
     * Make-function
     * @return Not registered XSLT function
     */
    static ReferenceCounting::SmartPtr<StringConvertFunction<Functor> >
    create() noexcept
    {
      return new StringConvertFunction<Functor>();
    }

    virtual xmlXPathFunction
    get_function() noexcept
    {
      return execute;
    }

    /**
     * Stateless callback called from libxslt code, cannot raise exception,
     * otherwise C-code of libxslt will leak resources.
     * @param context XPath context
     * @param nargs The number of arguments of XSLT function
     * @return Use some libxslt function to return values in XSL
     * stylesheet
     */
    static void
    execute(xmlXPathParserContextPtr context, int nargs) noexcept
    {
      const char FUN[] = "StringConvertFunction<..>::execute()";
      xsltTransformContextPtr transform_context = 0;

      try
      {
        transform_context = xsltXPathGetTransformContext(context);
        if (nargs != 1)
        {
          xsltTransformError(transform_context, NULL, NULL, "%s%s%d", FUN,
            ": arguments count isn't equal 1. Its value: ", nargs);
          return;
        }

        CharPtr input(xmlXPathPopString(context));
        std::string result_str;
        Functor::calculate(reinterpret_cast<char*>(input.get()), result_str);

        xmlChar* result =
          xmlStrdup(reinterpret_cast<const xmlChar*>(result_str.c_str()));
        xmlXPathReturnString(context, result);
      }
      catch (const eh::Exception& ex)
      {
        xsltTransformError(transform_context, NULL, NULL, "%s%s",
          FUN, ex.what());
      }
      catch (...)
      {
        xsltTransformError(transform_context, NULL, NULL, "%s%s", FUN,
          ": Unknown exception.");
      }
    }

    protected:
      /**
       * refcountable, protected
       */
      virtual
      ~StringConvertFunction() noexcept;
  };


  namespace XsltExt
  {
    /**
     * Encodes string with JavaScript rules (\\xXX form)
     */
    struct JSEncodeFunctor
    {
      static void
      calculate(const char* value, std::string& encoded)
      {
        String::StringManip::js_encode(value, encoded);
      }
    };

    /**
     * Encodes string with JavaScript unicode rules (\\uXXXX form)
     */
    struct JSUnicodeEncodeFunctor
    {
      static void
      calculate(const char* value, std::string& encoded)
      {
        String::StringManip::js_unicode_encode(value, encoded);
      }
    };

    /**
     *
     */
    struct XmlEncodeFunctor
    {
      static void
      calculate(const char* value, std::string& encoded)
      {
        String::StringManip::xml_encode(value, encoded);
      }
    };

    typedef
      StringConvertFunction<JSEncodeFunctor>
      JSEscapeFun;

    typedef
      StringConvertFunction<JSUnicodeEncodeFunctor>
      JSEscapeUnicodeFun;

    typedef
      StringConvertFunction<XmlEncodeFunctor>
      XmlEscapeFun;
  } // namespace XsltEx
} // namespace AdServer

//
// Inlines implementation
//

namespace AdServer
{

  inline
  XslFunctionRegistrar::XslFunctionRegistrar() noexcept
  {
  }

  template <typename Functor>
  StringConvertFunction<Functor>::StringConvertFunction()
  {
  }

  template <typename Functor>
  StringConvertFunction<Functor>::StringConvertFunction(
    const char* fun_namespace, const char* fun_name)
    : XslFunctionRegistrar(fun_namespace, fun_name)
  {
  }

  template <typename Functor>
  StringConvertFunction<Functor>::~StringConvertFunction() noexcept
  {
  }

} // namespace AdServer

#endif // XSLTEXFUNCTIONS_HPP_INCLUDED
