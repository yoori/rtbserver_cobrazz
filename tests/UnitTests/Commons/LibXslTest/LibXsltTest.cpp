// @file LibXslTest/LibXsltTest.cpp

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <Generics/Rand.hpp>
#include <TestCommons/MTTester.hpp>
#include <Commons/Xslt/XslTransformer.hpp>
#include <Commons/Xslt/LibxsltExFunctions.hpp>

DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

const std::string&
get_root_path() /*throw(eh::Exception)*/;

namespace
{
  typedef AdServer::XslTransformer::XslParameters XslParameters;
  typedef std::vector<std::string> Files;

  const char XSLT_EXT_NAMESPACE[] = "http://foros.com/foros/xslt-template";
}

struct Transformer
{
  Transformer(
    const Files& files,
    const XslParameters& parameters);

  void
  operator ()() noexcept;

private:
//  typedef Sync::PosixMutex Mutex;
//  typedef Sync::PosixGuard Guard;
//  static Mutex mutex_;

  const Files& FILES_;
  const XslParameters& PARAMETERS_;
  AdServer::XslTransformer transformer_;
};

void
mt_transform_test(const XslParameters& parameters,
  const Files& files);

struct CreateTransformer
{
  CreateTransformer(
    const Files& files,
    const char* xsl_file,
    const XslParameters& parameters);

  void
  operator ()() noexcept;

private:
  typedef Sync::PosixMutex Mutex;
  typedef Sync::PosixGuard Guard;
  static Mutex mutex_;

  const Files& FILES_;
  const std::string XSL_FILE_;
  const XslParameters& PARAMETERS_;
};

CreateTransformer::Mutex CreateTransformer::mutex_;

void
mt_constructor_test(const XslParameters& parameters,
  const char* xsl_file, const Files& files);

//
// Test body below
//
int
main()
{
  try
  {
    std::cout << "libxslt multithreading test started.." << std::endl;

    XslParameters parameters;
    Files files;

    files.push_back(get_root_path() + "test.xml");
    files.push_back(get_root_path() + "test2.xml");
    files.push_back(get_root_path() + "pass_birds.xml");
    files.push_back(get_root_path() + "pass_test.xml");

    parameters.insert(XslParameters::value_type("test_xsl_parameter1",
      "'SetValue1'"));
    parameters.insert(XslParameters::value_type("test_xsl_parameter3",
      "'SetValue3'"));

    mt_constructor_test(parameters, "test.xsl", files);
    std::cout << "Valid xsl test complete" << std::endl;
    mt_constructor_test(parameters, "invalid.xsl", files);
    std::cout << "Invalid xsl test complete" << std::endl;
    mt_constructor_test(parameters, "", files); // random case
    std::cout << "Valid/Invalid xsl test complete" << std::endl;

    mt_transform_test(parameters, files);

    std::cout << "SUCCESS" << std::endl;
  }
  catch (eh::Exception& e)
  {
    std::cout << "\nFAIL: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cout << "\nFAIL: unknown exception" << std::endl;
    throw;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////
// Implementations

const std::string&
get_root_path() /*throw(eh::Exception)*/
{
  static std::string root_path;
  if (root_path.empty())
  {
    char* ev = getenv("TEST_TOP_SRC_DIR");
    root_path = ev ? ev : "../../../../..";
    root_path += "/tests/UnitTests/Commons/LibXslTest/Data/";
  }
  return root_path;
}

void
mt_transform_test(const XslParameters& parameters,
  const Files& files)
{
  Transformer transformers(files, parameters);
  TestCommons::MTTester<Transformer&> mt_tester(
    transformers, 10);

  mt_tester.run(1000, 0, 1000);
}

Transformer::Transformer(
  const Files& files,
  const XslParameters& parameters)
  : FILES_(files),
    PARAMETERS_(parameters),
    transformer_((get_root_path() + "test.xsl").c_str(), 0,
      AdServer::XslTransformer::XE_LIBXSLT)
{
  transformer_.register_external_fun(
    XSLT_EXT_NAMESPACE,
    "escape-js-unicode",
    AdServer::XsltExt::JSEscapeUnicodeFun::create());
  transformer_.register_external_fun(
    XSLT_EXT_NAMESPACE,
    "escape-js",
    AdServer::XsltExt::JSEscapeFun::create());
}

void
Transformer::operator ()() noexcept
{
  try
  {
    std::size_t index = Generics::safe_rand(FILES_.size());
    const char* chosen_file = FILES_[index].c_str();
    std::ifstream xml_file(chosen_file, std::ios::in);
    std::ostringstream result;
    const XslParameters* parameters_ptr =
      (PARAMETERS_.empty() ? 0 : &PARAMETERS_);
    transformer_.transform(xml_file, result, parameters_ptr);
    if (result.str().empty())
    {
      throw Exception("FAIL");
    }
//    Guard lock(mutex_);
//    std::cout << result.str() << std::endl;
  }
  catch (const AdServer::XslTransformerBase::Exception&)
  {
  }
}

void
mt_constructor_test(const XslParameters& parameters,
  const char* xsl_file,
  const Files& files)
{
  CreateTransformer transformers(files, xsl_file, parameters);
  TestCommons::MTTester<CreateTransformer&> mt_tester(
    transformers, 10);

  mt_tester.run(1000, 0, 1000);
}

CreateTransformer::CreateTransformer(
  const Files& files,
  const char* xsl_file,
  const XslParameters& parameters)
  : FILES_(files),
    XSL_FILE_(xsl_file),
    PARAMETERS_(parameters)
{
}

void
CreateTransformer::operator ()() noexcept
{
  try
  {
    const char* XSL_FILES[] =
    {
      "test.xsl",
      "invalid.xsl"
    };

    const char* XSL_FILE = XSL_FILE_.c_str();
    if (XSL_FILE_.empty())
    {
      XSL_FILE = XSL_FILES[Generics::safe_rand(
        sizeof(XSL_FILES)/sizeof(XSL_FILES[0]))];
    }

    AdServer::XslTransformer transformer(AdServer::XslTransformer::XE_LIBXSLT);

    transformer.open((get_root_path() +
      XSL_FILE).c_str());

    transformer.register_external_fun(
      XSLT_EXT_NAMESPACE,
      "escape-js-unicode",
      AdServer::XsltExt::JSEscapeUnicodeFun::create());
    transformer.register_external_fun(
      XSLT_EXT_NAMESPACE,
      "escape-js",
      AdServer::XsltExt::JSEscapeFun::create());

    std::size_t index = Generics::safe_rand(FILES_.size());
    const char* chosen_file = FILES_[index].c_str();
    std::ifstream xml_file(chosen_file, std::ios::in);
    std::ostringstream result;
    const XslParameters* parameters_ptr =
      (PARAMETERS_.empty() ? 0 : &PARAMETERS_);
    transformer.transform(xml_file, result, parameters_ptr);
    if (result.str().empty())
    {
      throw Exception("FAIL");
    }
//    Guard lock(mutex_);
//    std::cout << result.str() << std::endl;
  }
  catch (const AdServer::XslTransformer::Exception&)
  {
  }
}

