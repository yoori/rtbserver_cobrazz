#include <fstream>
#include <iostream>
#include <sstream>
#include <String/TextTemplate.hpp>

#include "CreativeTemplate.hpp"

namespace
{
  const Generics::Time UPDATE_PERIOD(60); // 1 min
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

  class TemplateArgsCallback: public String::TextTemplate::ArgsCallback
  {
  public:
    TemplateArgsCallback(
      const TokenValueMap& request_args,
      const TokenValueMap* creative_args)
      : request_args_(request_args),
        creative_args_(creative_args)
    {}

  private:
    bool
    get_argument(
      const String::SubString& key,
      std::string& result,
      bool value) const override
      /*throw(eh::Exception)*/
    {
      if(!value)
      {
        result.assign(key.data(), key.size());
        return true;
      }

      if(request_args_.get_argument(key, result))
      {
        return true;
      }

      if(creative_args_ && creative_args_->get_argument(key, result))
      {
        return true;
      }

      return false;
    }

  private:
    const TokenValueMap& request_args_;
    const TokenValueMap* creative_args_;
  };

  /* Concrete template implementations */
  /** TextTemplate */
  class TextTemplate: public Template
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    TextTemplate(const char* file)
      /*throw(Template::FileNotExists, Exception)*/;

    virtual std::string instantiate(
      const String::TextTemplate::ArgsCallback& args)
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

  std::string
  Template::instantiate(
    const TemplateParams* request_params,
    const TemplateParamsList& params)
    /*throw(InvalidParams,
      InvalidTemplate,
      ImplementationException)*/
  {
    const TokenValueMap* creative_args = nullptr;
    TemplateParamsList::const_iterator cr_it = params.begin();
    if(cr_it != params.end())
    {
      TemplateParamsList::const_iterator next_it = cr_it;
      ++next_it;
      if(next_it == params.end())
      {
        creative_args = (*cr_it).in();
      }
    }

    TemplateArgsCallback args_cont(*request_params, creative_args);
    return instantiate(args_cont);
  }

  std::string
  TextTemplate::instantiate(
    const String::TextTemplate::ArgsCallback& args)
    /*throw(InvalidParams,
      InvalidTemplate,
      ImplementationException)*/
  {
    try
    {
      String::TextTemplate::DefaultValue default_cont(&args);
      String::TextTemplate::ArgsEncoder encoder(&default_cont);
      return text_template_.instantiate(encoder);
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
