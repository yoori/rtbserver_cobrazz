// @file Commons/Xslt/XslTransformAdmin.cpp

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include <Generics/Time.hpp>
#include <Generics/AppUtils.hpp>

#include <Stream/MemoryStream.hpp>

#include "XslTransformer.hpp"
#include "LibxsltExFunctions.hpp"

namespace
{
  const char XSLT_EXT_NAMESPACE[] = "http://foros.com/foros/xslt-template";

  const char DEFAULT_BASE_PATH[] = "";
  Generics::AppUtils::Option<std::string> base_path(DEFAULT_BASE_PATH);

  const char DEFAULT_PARAMS[] = "";
  Generics::AppUtils::Option<std::string> xslt_params(DEFAULT_PARAMS);

  const char DEFAULT_VALUES[] = "";
  Generics::AppUtils::Option<std::string> xslt_values(DEFAULT_VALUES);

  const unsigned long DEFAULT_ITERATIONS = 1;
  Generics::AppUtils::Option<unsigned long> iterations(DEFAULT_ITERATIONS);

  Generics::AppUtils::Option<std::string> xslt_file("");
  Generics::AppUtils::Option<std::string> xml_file("");

  Generics::AppUtils::CheckOption verbose;

  const char USAGE[] = "Usage:\n"
    "  XslTransformAdmin [-b base_path -p params -v values -i -h]"
    " <xslt file> <xml file>\n"
    "  -b, --base base_path Base path for XSL document, using to resolve "
    "relative path\n"
    "  -p, --params params The string with parameters for the XSL "
    "stylesheet, separated by ':'\n"
    "  -v, --values values The string with values of parameters for"
    " the XSL stylesheet, separated by ':'\n"
    "  -i, --info Allow verbose output, print debug information "
    "about transformation\n"
    "  -c, --count Repeat transformation count times and print performance"
    " statistics\n"
    "  -h, --help           Show this help.";

  typedef AdServer::XslTransformer::XslParameters XslParameters;
  XslParameters parameters;
}

bool
init(int& argc, char**& argv)
  /*throw(eh::Exception)*/
{
  using namespace Generics::AppUtils;
  Args args;
  CheckOption opt_help;

  args.add(
    equal_name("base") ||
    short_name("b"),
    base_path);
  args.add(
    equal_name("params") ||
    short_name("p"),
    xslt_params);
  args.add(
    equal_name("values") ||
    short_name("v"),
    xslt_values);
  args.add(
    equal_name("info") ||
    short_name("i"),
    verbose);
  args.add(
    equal_name("count") ||
    short_name("c"),
    iterations);
  args.add(
    equal_name("help") ||
    short_name("h"),
    opt_help);

  if (argc > 3)
  {
    args.parse(argc - 3, argv + 1);
  }

  typedef std::vector<std::string> Strings;
  Strings params, values;
  String::StringManip::SplitColon tokenizer(*xslt_params);
  String::SubString name;
  while (tokenizer.get_token(name))
  {
    params.push_back(name.str());
  }

  if (opt_help.enabled() || argc <= 2)
  {
    std::cout << USAGE << std::endl;
    return false;
  }

  if (!params.empty())
  {
    String::StringManip::SplitColon tokenizer_values(*xslt_values);
    while (tokenizer_values.get_token(name))
    {
      values.push_back(name.str());
    }

    for (std::size_t i = 0; i < params.size(); ++i)
    {
      std::string value = (i < values.size() ? values[i] : "");
      value = '\'' + value;
      value = value + '\'';
      parameters.insert(XslParameters::value_type(params[i],
        value ));
    }
    if (verbose.enabled())
    {
      std::cout << "XSLT parameters is following:\n";
      for (XslParameters::const_iterator cit = parameters.begin();
        cit != parameters.end(); ++cit)
      {
        std::clog << cit->first << " := " << cit->second << std::endl;
      }
    }
  }

  return true;
}

int
main(int argc, char* argv[])
  noexcept
{
  ::setlocale(LC_CTYPE, "en_US.utf8");

  std::string stage;

  try
  {
    if (!init(argc, argv))
    {
      return -1;
    }

    stage = "init xslt";

    /* ADSC-8624 reproducer
    std::unique_ptr<AdServer::XslTransformer> tmp_transformer(
      new AdServer::XslTransformer(
        argv[argc - 2],
        (*base_path).empty() ? 0 : (*base_path).c_str()));

    tmp_transformer->register_external_fun(
      XSLT_EXT_NAMESPACE,
      "escape-js-unicode",
      AdServer::XsltExt::JSEscapeUnicodeFun::create());
    tmp_transformer->register_external_fun(
      XSLT_EXT_NAMESPACE,
      "escape-js",
      AdServer::XsltExt::JSEscapeFun::create());
    tmp_transformer->register_external_fun(
      XSLT_EXT_NAMESPACE,
      "escape-xml",
      AdServer::XsltExt::XmlEscapeFun::create());
    */

    AdServer::XslTransformer transformer(argv[argc - 2],
      (*base_path).empty() ? 0 : (*base_path).c_str()
      );

    stage = "init external functions";

    transformer.register_external_fun(
      XSLT_EXT_NAMESPACE,
      "escape-js-unicode",
      AdServer::XsltExt::JSEscapeUnicodeFun::create());
    transformer.register_external_fun(
      XSLT_EXT_NAMESPACE,
      "escape-js",
      AdServer::XsltExt::JSEscapeFun::create());
    transformer.register_external_fun(
      XSLT_EXT_NAMESPACE,
      "escape-xml",
      AdServer::XsltExt::XmlEscapeFun::create());

    /* ADSC-8624 reproducer
    tmp_transformer.reset(0);
    */

    stage = "transform";

    std::ifstream xml_file(argv[argc - 1], std::ios::in);
    std::string buffer;
    // Try get file size. We cannot get file size for some streams
    // e.g. /dev/stdin
    std::ifstream::pos_type xml_file_size = xml_file.tellg();
    if (!xml_file_size)
    {
      xml_file.seekg(0, std::ios::end);
      std::ifstream::pos_type xml_file_size = xml_file.tellg();

      if (!xml_file_size || !xml_file.seekg(0, std::ios::beg))
      {
        std::cerr << "Cannot get XML content of " << argv[argc - 1]
          << ", file size = " << xml_file_size << std::endl;
      }
      buffer.reserve(static_cast<std::size_t>(xml_file_size));
    }
    std::copy(std::istreambuf_iterator<char>(xml_file),
      std::istreambuf_iterator<char>(),
      std::back_inserter(buffer));

    std::ostringstream result;

    XslParameters* parameters_ptr = (parameters.empty() ? 0 : &parameters);
    Generics::CPUTimer timer;
    timer.start();
    for (unsigned long i = 0; i < *iterations; ++i)
    {
      Stream::Parser xml_file(buffer.c_str(), buffer.size());
      result.str("");
      transformer.transform(xml_file, result, parameters_ptr);
    }
    timer.stop();
    if (iterations.installed() && *iterations)
    {
      // Provide average time for transformations
      std::cout << timer.elapsed_time() / *iterations << std::endl;
    }
    else
    {
      std::cout << result.str();
    }
  }
  catch (const AdServer::XslTransformer::Exception& ex)
  {
    std::cerr << "at stage '"
      << stage;
    if (stage == "transform")
    {
      std::cerr << "'. Broken XML file: " << argv[argc - 1];
    }
    else
    {
      std::cerr << "'. Broken XSL file: " << argv[argc - 2];
    }
    std::cerr  << ". Caught XslTransformer::Exception: " << ex.what();
    return -1;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "at stage '" << stage;
    if (stage == "transform")
    {
      std::cerr << "'. Broken XML file: " << argv[argc - 1];
    }
    else
    {
      std::cerr << "'. Broken XSL file: " << argv[argc - 2];
    }
    std::cerr << ". Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }
  return 0;
}
