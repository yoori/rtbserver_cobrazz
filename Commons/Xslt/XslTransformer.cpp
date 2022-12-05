// @file Xslt/XslTransformer.cpp

#include "XslTransformer.hpp"

#include "LibxsltTransformer.hpp"

namespace AdServer
{

  //
  // XslTransformer class implementation
  //

  XslTransformer::XslTransformer(XsltEngine /*engine*/) noexcept
    : pimpl_(
      static_cast<XslTransformerBase*>(new LibxslTransformer))
  {
  }

  XslTransformer::XslTransformer(std::istream& xsl,
    const char* base_path,
    XsltEngine /*engine*/)
    /*throw(Exception)*/
    : pimpl_(static_cast<XslTransformerBase*>(
        new LibxslTransformer(xsl, base_path)))
  {
  }

  XslTransformer::XslTransformer(const char* xsl_file,
    const char* base_path,
    XsltEngine /*engine*/)
    /*throw(FileNotExists, Exception)*/
    : pimpl_(static_cast<XslTransformerBase*>(
        new LibxslTransformer(xsl_file, base_path)))
  {
  }

  void
  XslTransformer::open(const char* xsl_file,
    const char* base_path)
    /*throw(FileNotExists, Exception)*/
  {
    pimpl_->open(xsl_file, base_path);
  }

  void
  XslTransformer::transform(std::istream& input,
    std::ostream& output,
    const XslParameters* parameters)
    /*throw(Exception)*/
  {
    pimpl_->transform(input, output, parameters);
  }

  void
  XslTransformer::register_external_fun(
    const char* fun_namespace,
    const char* fun_name,
    XslFunction* fun) /*throw(Exception)*/
  {
    pimpl_->register_external_fun(fun_namespace, fun_name, fun);
  }

} // namespace AdServer
