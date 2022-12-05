#include <fstream>
#include <iostream>
#include <sstream>
#include <String/TextTemplate.hpp>
#include <Commons/Xslt/XslTransformer.hpp>
#include <Commons/Xslt/LibxsltExFunctions.hpp>

#include "CreativeTemplate.hpp"

namespace
{
  const Generics::Time UPDATE_PERIOD(60); // 1 min
  const char XSLT_EXT_NAMESPACE[] = "http://foros.com/foros/xslt-template";
}

namespace AdServer
{
namespace CampaignSvcs
{
  class KeyArgsCallback: public String::TextTemplate::ArgsCallback
  {
    virtual
    bool
    get_argument(const String::SubString& key, std::string& result,
      bool /*value*/) const /*throw(eh::Exception)*/
    {
      key.assign_to(result);
      return true;
    }
  };

  /* Concrete template implementations */
  /** TextTemplate */
  class TextTemplate: public Template
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    TextTemplate(const char* file)
      /*throw(Template::FileNotExists, Exception)*/;

    virtual void instantiate(
      const TemplateParams* request_params,
      const TemplateParamsList& creative_params,
      std::ostream& ostr)
      /*throw(InvalidParams,
            InvalidTemplate,
            ImplementationException)*/;

    virtual bool
    key_used(const String::SubString& key) const
      noexcept;

  protected:
    virtual ~TextTemplate() noexcept
    {}

  protected:
    String::TextTemplate::IStream text_template_;
    String::TextTemplate::Keys keys_;
  };

  /** XsltTemplate */
  class XsltTemplate: public Template
  {
  public:
    XsltTemplate(const char* file)
      /*throw(FileNotExists, XslTransformer::Exception)*/;

    virtual void instantiate(
      const TemplateParams* request_params,
      const TemplateParamsList& creative_params,
      std::ostream& ostr)
      /*throw(InvalidParams,
        InvalidTemplate,
        ImplementationException)*/;

    virtual bool
    key_used(const String::SubString& key) const
      noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;
    XslTransformer xsl_transformer_;
  };

  /**
   * TextTemplate implementation
   */
  TextTemplate::TextTemplate(const char* file)
    /*throw(Template::FileNotExists, Exception)*/
  {
    static const char* FUN = "TextTemplate::TextTemplate()";

    std::fstream fstr(file, std::ios::in);
    if(!fstr.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't open file '" << file << "'";
      throw Template::FileNotExists(ostr);
    }

    try
    {
      std::stringstream ostr;
      ostr << fstr.rdbuf();
      text_template_.init(
        ostr,
        TokenTemplateProperties::START_TOKEN,
        TokenTemplateProperties::STOP_TOKEN);

      // fill keys
      KeyArgsCallback null_args;
      String::TextTemplate::DefaultValue default_cont(&null_args);
      String::TextTemplate::ArgsEncoder encoder(&default_cont);
      text_template_.keys(encoder, keys_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't init template, caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  TextTemplate::instantiate(
    const TemplateParams* request_params,
    const TemplateParamsList& params,
    std::ostream& ostr)
    /*throw(InvalidParams,
      InvalidTemplate,
      ImplementationException)*/
  {
    try
    {
      TokenValueMap args;
      // request tokens override creative tokens
      args.insert(request_params->begin(), request_params->end());

      if(++params.begin() == params.end())
      {
        const TokenValueMap& creative_args = *(*params.begin());
        args.insert(creative_args.begin(), creative_args.end());
      }

      String::TextTemplate::ArgsContainer<TokenValueMap,
        String::TextTemplate::ArgsContainerStringAdapter> args_cont(&args);
      String::TextTemplate::DefaultValue default_cont(&args_cont);
      String::TextTemplate::ArgsEncoder encoder(&default_cont);
      ostr << text_template_.instantiate(encoder);
    }
    catch(const String::TextTemplate::UnknownName& ex)
    {
      Stream::Error ostr;
      ostr << "Can't instantiate creative. Caught UnknownName: " <<
        ex.what();
      throw InvalidParams(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't instantiate creative. Caught eh::Exception: " <<
        ex.what();
      throw ImplementationException(ostr);
    }
  }

  bool
  TextTemplate::key_used(const String::SubString& key)
    const noexcept
  {
    return keys_.find(key.str()) != keys_.end();
  }

  /**
   * XsltTemplate implementation
   */
  XsltTemplate::XsltTemplate(const char* file)
    /*throw(Template::FileNotExists, XslTransformer::Exception)*/
  {
    try
    {
      xsl_transformer_.open(file);

      xsl_transformer_.register_external_fun(
        XSLT_EXT_NAMESPACE,
        "escape-js-unicode",
        AdServer::XsltExt::JSEscapeUnicodeFun::create());

      xsl_transformer_.register_external_fun(
        XSLT_EXT_NAMESPACE,
        "escape-js",
        AdServer::XsltExt::JSEscapeFun::create());

      xsl_transformer_.register_external_fun(
        XSLT_EXT_NAMESPACE,
        "escape-xml",
        AdServer::XsltExt::XmlEscapeFun::create());
    }
    catch(const XslTransformer::FileNotExists& ex)
    {
      throw Template::FileNotExists("");
    }
  }

  void
  XsltTemplate::instantiate(
    const TemplateParams* request_params,
    const TemplateParamsList& params,
    std::ostream& ostr)
    /*throw(InvalidParams,
      InvalidTemplate,
      ImplementationException)*/
  {
    /* generate xml */
    std::stringstream xml_params;

    xml_params << "<impression>" << std::endl;

    for(TemplateParamsList::const_iterator cr_it = params.begin();
        cr_it != params.end(); ++cr_it)
    {
      const TokenValueMap& args = *(*cr_it);

      xml_params << "  <creative>" << std::endl;

      for(TokenValueMap::const_iterator it = args.begin();
          it != args.end(); ++it)
      {
        xml_params <<
          "    <token name=\"" << it->first << "\">" <<
          "<![CDATA[" << it->second << "]]>" <<
          "</token>" << std::endl;
      }

      xml_params << "  </creative>" << std::endl;
    }

    for(TokenValueMap::const_iterator it = request_params->begin();
        it != request_params->end(); ++it)
    {
      xml_params <<
        "  <token name=\"" << it->first << "\">" <<
        "<![CDATA[" << it->second << "]]>" <<
        "</token>" << std::endl;
    }

    xml_params << "</impression>" << std::endl;

    /*
    std::cerr << ">>>>>>>>>>>>>>>>>" << std::endl <<
      xml_params.str() <<
      "<<<<<<<<<<<<<<<<<<<" << std::endl;
    */

    try
    {
      xsl_transformer_.transform(xml_params, ostr);
    }
    catch(const XslTransformer::Exception& ex)
    {
      /*
      std::cout << "==============" << std::endl;
      std::cout << xml_params.str() << std::endl;
      std::cout << "==============" << std::endl;
      */
      throw ImplementationException(ex.what());
    }
  }

  bool
  XsltTemplate::key_used(const String::SubString& /*key*/)
    const noexcept
  {
    return true;
  }

  void
  Template::get_keys(
    String::TextTemplate::Keys& keys,
    const String::SubString& text)
    noexcept
  {
    std::stringstream ostr;
    ostr << text.str();

    String::TextTemplate::IStream text_template;
    text_template.init(
      ostr,
      TokenTemplateProperties::START_TOKEN,
      TokenTemplateProperties::STOP_TOKEN);

    KeyArgsCallback null_args;
    String::TextTemplate::DefaultValue default_cont(&null_args);
    String::TextTemplate::ArgsEncoder encoder(&default_cont);
    text_template.keys(encoder, keys);
  }

  /**
   * CreativeTemplateFactory implementation
   */
  Template*
  CreativeTemplateFactory::create(
    const CreativeTemplateFactory::Handler& creative_template_handler,
    State& state)
    const
    /*throw(
      Template::FileNotExists,
      Template::InvalidTemplate,
      ImplementationException)*/
  {
    state = Generics::Time::get_time_of_day();

    if(creative_template_handler.type == CreativeTemplateFactory::Handler::CTT_TEXT)
    {
      try
      {
        return new TextTemplate(
          creative_template_handler.file.c_str());
      }
      catch (const TextTemplate::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "CreativeTemplateFactory:create(): "
                "Can't init text template. Caught Exception: "
             << ex.what();
        throw Template::InvalidTemplate(ostr);
      }
    }
    else if(creative_template_handler.type == CreativeTemplateFactory::Handler::CTT_XSLT)
    {
      try
      {
        return new XsltTemplate(
          creative_template_handler.file.c_str());
      }
      catch(const XslTransformer::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "CreativeTemplateFactory::create(): "
                "Can't init xslt template. Caught Exception: "
             << ex.what();
        throw Template::InvalidTemplate(ostr);
      }
    }

    throw Template::InvalidTemplate("Unknown template type.");
  }

  bool
  CreativeTemplateFactory::need_update(
    const Handler& /*handler*/, const State& state) const
    /*throw(Template::InvalidTemplate, ImplementationException)*/
  {
    return Generics::Time(state) - Generics::Time::get_time_of_day() >
      UPDATE_PERIOD;
  }

  Template*
  CreativeTemplateFactory::update(
    Template* templ,
    const Handler& handler,
    State& state) const
  {
    state = Generics::Time::get_time_of_day();

    try
    {
      return create(handler, state);
    }
    catch(const Template::FileNotExists&)
    {
      /* keep old template state if file disappeared */
      return ReferenceCounting::add_ref(templ);
    }
  }

} /* CampaignSvcs */
} /* AdServer */
