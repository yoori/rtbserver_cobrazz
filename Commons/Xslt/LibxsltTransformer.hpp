// @file Xslt/LibxsltTransformer.hpp

#ifndef LIBXSLTTRANSFORMER_HPP_INCLUDED
#define LIBXSLTTRANSFORMER_HPP_INCLUDED

#include <libxslt/xsltInternals.h>

#include <string>
#include <list>

#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Sync/SyncPolicy.hpp>

#include "XslTransformer.hpp"

namespace AdServer
{
  class XslFunction;
  typedef ReferenceCounting::SmartPtr<XslFunction> XslFunction_var;

  class LibxsltHolder : public ReferenceCounting::AtomicImpl
  {
  public:
    static LibxsltHolder*
    instance() /*throw(eh::Exception)*/;

    void
    register_external_fun(
      const char* fun_namespace,
      const char* fun_name,
      XslFunction* fun) /*throw(XslTransformerBase::Exception)*/;

  protected:
    virtual
    ~LibxsltHolder() noexcept;

  private:
    LibxsltHolder() noexcept;

    void
    initialize_i_() noexcept;

    void
    deinitialize_() noexcept;

  private:
    typedef std::map<std::string, XslFunction_var> XslFunctionMap;
    typedef Sync::Policy::PosixThread SyncPolicy;
    typedef SyncPolicy::Mutex Mutex;
    typedef SyncPolicy::WriteGuard Guard;

    Mutex ext_functions_lock_;
    XslFunctionMap ext_functions_;

    static Mutex lock_;
    static ReferenceCounting::SmartPtr<LibxsltHolder> holder_;
  };

  typedef ReferenceCounting::SmartPtr<LibxsltHolder> LibxsltHolder_var;

  /**
   * Tested only two scenarios of using in multi-threaded environment
   * 1. Create transformer and do transform of different XML in some threads.
   * 2. Create transformer, transform in some threads.
   *   Other class methods is not thread safe.
   */
  class LibxslTransformer : public XslTransformerBase
  {
  public:

    /**
     * Creates and all
     */
    LibxslTransformer() noexcept;

    /**
     * @param xsl The input stream with XSL data
     * @param base_path The base URI to resolve relative addresses in
     * XSL documents. Sample of valid base URI:
     * ./data/templates/
     * If null exception raise.
     */
    LibxslTransformer(std::istream& xsl, const char* base_path = "")
      /*throw(Exception)*/;

    /**
     * @param xsl_file The URL to XSL file
     * @param base_path root path for xsl_file, all xsl:include in file
     * work as file placed on base_path. If equal zero, as the base path
     * using xsl file name.
     */
    LibxslTransformer(const char* xsl_file, const char* base_path = 0)
      /*throw(FileNotExists, Exception)*/;

    /**
     * empty, all resources free through smart pointers
     */
    virtual
    ~LibxslTransformer() noexcept;

    /**
     * Not thread-safe!
     * @param xsl_file The file with XSL that will applying when
     * do transform(). As the base path using xsl file name.
     * @param base_path The path from which should work xsl:include's
     */
    void
    open(const char* xsl_file, const char* base_path = 0)
      /*throw(FileNotExists, Exception)*/;

    /**
     * Apply stylesheet to XML
     * @param input The XML data
     * @param output Result will here
     * @param parameters Optional parameters for XSL stylesheet
     */
    void
    transform(std::istream& input, std::ostream& output,
      const XslParameters* parameters = 0)
      /*throw(Exception)*/;

    /**
     * Registrate external XSL function in libxslt engine
     * Note: You should register external function once, but
     * sharing XslTransformer between threads and simultaneously
     * registering external functions on it is NOT thread safe!
     * @param fun_namespace The namespace for XSLT function
     * @param fun_name The name of external XSLT function
     * @param fun Pointer to functional object
     */
    void
    register_external_fun(
      const char* fun_namespace,
      const char* fun_name,
      XslFunction* fun) /*throw(Exception)*/;

  private:

    struct LibxsltContextParserDestroyer
    {
      void
      operator ()(const xmlParserCtxtPtr parser_context) noexcept
      {
        xmlFreeParserCtxt(parser_context);
      }
    };

    typedef std::unique_ptr<
      xmlParserCtxt,
      LibxsltContextParserDestroyer> ParserContextPtr;

    struct LibxsltStylesheetDestroyer
    {
      void
      operator()(const xsltStylesheetPtr stylesheet) noexcept
      {
        if(stylesheet)
        {
          stylesheet->doc->URL = 0;
          xsltFreeStylesheet(stylesheet);
        }
      }
    };

    typedef std::unique_ptr<
      xsltStylesheet,
      LibxsltStylesheetDestroyer> StylesheetPtr;

    /**
     * Parse stream that produce XML
     * Set URL of document to base_path_, this will be important
     * when apply XSL stylesheet to document with includes.
     * @param istr XML data producing stream
     * @return The smart pointer to super context, aggregate libxslt
     * context and user data in one
     */
    ParserContextPtr
    parse_xml_stream_(std::istream& istr) /*throw(Exception)*/;

    /**
     * Init libxslt library globals, parse XSL stylesheet from
     * input data stream and save compiled stylesheet
     * @param istr This stream should contain XSLT code, that will be
     * applying in transform calls
     */
    void
    init_(std::istream& istr) /*throw(Exception)*/;

    /// libxslt globals load/unload guard
    LibxsltHolder_var libxslt_holder_;
    /// Store all registered external XSL function, release while
    /// destroying transformer
    typedef std::list<XslFunction_var> ExternalFunList;
    ExternalFunList ext_functions_;
    /// Compiled stylesheet
    StylesheetPtr stylesheet_;
    /// Provide point from which should work xsl:include
    /// while applying stylesheet
    std::string base_path_;
  };

}

#endif // LIBXSLTTRANSFORMER_HPP_INCLUDED
