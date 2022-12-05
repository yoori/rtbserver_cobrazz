// @file Commons/Xslt/XslTransformer.hpp

#ifndef XSL_TRANSFORMER_HPP_INCLUDED
#define XSL_TRANSFORMER_HPP_INCLUDED

#include <map>
#include <memory>

#include <eh/Exception.hpp>

namespace AdServer
{
  class XslFunction;

  /**
   * Base interface for some xslt transformers implementation
   */
  struct XslTransformerBase
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(FileNotExists, Exception);

    typedef std::map<std::string, std::string> XslParameters;

    virtual void
    open(const char* xsl_file, const char* base_path = 0)
      /*throw(FileNotExists, Exception)*/ = 0;

    /**
     * Perform XSL transformation
     * @param input The stream with XML data that will be
     *   applied XSL stylesheet
     * @param output The stream for transformation results
     * @param parameters Optional parameters for XSL stylesheet
     */
    virtual void
    transform(std::istream& input, std::ostream& output,
      const XslParameters* parameters = 0)
      /*throw(Exception)*/ = 0;

    virtual void
    register_external_fun(
      const char* fun_namespace,
      const char* fun_name,
      XslFunction* fun) /*throw(Exception)*/ = 0;

    virtual
    ~XslTransformerBase() noexcept;
  };


  /**
   * pimpl'ied mediator between some transformation engine
   * and our code.
   */
  class XslTransformer : public XslTransformerBase
  {
  public:
    enum XsltEngine
    {
      XE_LIBXSLT
    };

    typedef XslTransformerBase::Exception Exception;
    typedef XslTransformerBase::FileNotExists FileNotExists;
    typedef XslTransformerBase::XslParameters XslParameters;

    XslTransformer(XsltEngine engine = DEFAULT_ENGINE_) noexcept;

    /**
     * @param xsl The input stream with XSL data
     * @param base_path The base URI to resolve relative addresses in
     * XSL documents. You must use base URI that fully resolved and
     * have slash '/' at end of path. Sample of valid base URI:
     * file:///data/templates/
     * @param engine Selector for XSLT processor will be used for
     *   documents processing
     */
    XslTransformer(std::istream& xsl, const char* base_path = 0,
      XsltEngine engine = DEFAULT_ENGINE_)
      /*throw(Exception)*/;

    /**
     * @param xsl_file The URL to XSL file
     * @param base_path The base URI to resolve relative addresses in
     * XSL documents. You must use base URI that fully resolved and
     * have slash '/' at end of path. Sample of valid base URI:
     * file:///data/templates/
     * @param engine Selector for XSLT processor will be used for
     *   documents processing
     */
    XslTransformer(const char* xsl_file, const char* base_path = 0,
      XsltEngine engine = DEFAULT_ENGINE_)
      /*throw(FileNotExists, Exception)*/;

    virtual
    ~XslTransformer() noexcept;

    void
    open(const char* xsl_file, const char* base_path = 0)
      /*throw(FileNotExists, Exception)*/;

    void
    transform(std::istream& input, std::ostream& output,
      const XslParameters* parameters = 0)
      /*throw(Exception)*/;

    void
    register_external_fun(
      const char* fun_namespace,
      const char* fun_name,
      XslFunction* fun) /*throw(Exception)*/;

  private:
    typedef std::unique_ptr<XslTransformerBase> TransformerImplPtr;
    TransformerImplPtr pimpl_;

    static const XsltEngine DEFAULT_ENGINE_ = XE_LIBXSLT;
  };
}

//
// Inlines implementations
//
namespace AdServer
{
  inline
  XslTransformerBase::~XslTransformerBase() noexcept
  {
  }

  inline
  XslTransformer::~XslTransformer() noexcept
  {
  }
}

#endif // XSL_TRANSFORMER_HPP_INCLUDED
