#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <String/TextTemplate.hpp>

#include "CreativeTemplate.hpp"
#include "CreativeTextGenerator.hpp"

namespace
{
  const Generics::Time UPDATE_PERIOD(60); // 1 min

  bool
  read_file_state(
    const char* file,
    AdServer::CampaignSvcs::CreativeTemplateFactory::State& state) noexcept
  {
    struct stat file_stat;
    if (::stat(file, &file_stat) != 0)
    {
      state.initialized = false;
      return false;
    }

    state.modification_time = file_stat.st_mtim.tv_sec;
    state.modification_time_nanoseconds = file_stat.st_mtim.tv_nsec;
    state.file_size = file_stat.st_size;
    state.initialized = true;
    return true;
  }
}

namespace AdServer::CampaignSvcs
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
    TemplateArgsCallback(const TokenValueMap& request_args, const TokenValueMap* creative_args)
      : request_args_(request_args),
        creative_args_(creative_args)
    {}

  private:
    bool
    get_argument(const String::SubString& key, std::string& result, bool value) const override
      /*throw(eh::Exception)*/
    {
      if (!value)
      {
        result.assign(key.data(), key.size());
        return true;
      }

      if (request_args_.get_argument(key, result))
      {
        return true;
      }

      if (creative_args_ && creative_args_->get_argument(key, result))
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

    virtual std::string instantiate(const String::TextTemplate::ArgsCallback& args)
      /*throw(InvalidParams,
            InvalidTemplate,
            ImplementationException)*/;

    virtual bool
    key_used(const String::SubString& key) const
      noexcept;

    const std::shared_ptr<const std::vector<std::string>>&
    expected_post_actions() const noexcept override;

  protected:
    virtual ~TextTemplate() noexcept
    {}

  protected:
    String::TextTemplate::IStream text_template_;
    String::TextTemplate::Keys keys_;
    std::shared_ptr<const std::vector<std::string>> expected_post_actions_;
  };

  /**
   * TextTemplate implementation
   */
  TextTemplate::TextTemplate(const char* file)
    /*throw(Template::FileNotExists, Exception)*/
  {
    static const char* FUN = "TextTemplate::TextTemplate()";

    std::fstream fstr(file, std::ios::in);
    if (!fstr.is_open())
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

      std::vector<std::string> expected_post_actions;
      for (const auto& post_action : CreativeTokens::VIDEO_POST_ACTION_TOKENS)
      {
        if (keys_.find(post_action.token) != keys_.end())
        {
          expected_post_actions.emplace_back(post_action.action_name);
        }
      }
      expected_post_actions_ = expected_post_actions.empty() ?
        empty_expected_post_actions() :
        std::make_shared<const std::vector<std::string>>(std::move(expected_post_actions));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init template, caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  std::string
  Template::instantiate(const TemplateParams* request_params, const TemplateParamsList& params)
    /*throw(InvalidParams,
      InvalidTemplate,
      ImplementationException)*/
  {
    const TokenValueMap* creative_args = nullptr;
    TemplateParamsList::const_iterator cr_it = params.begin();
    if (cr_it != params.end())
    {
      TemplateParamsList::const_iterator next_it = cr_it;
      ++next_it;
      if (next_it == params.end())
      {
        creative_args = (*cr_it).in();
      }
    }

    TemplateArgsCallback args_cont(*request_params, creative_args);
    return instantiate(args_cont);
  }

  std::string
  TextTemplate::instantiate(const String::TextTemplate::ArgsCallback& args)
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
      ostr << "Can't instantiate creative. Caught UnknownName: " << ex.what();
      throw InvalidParams(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't instantiate creative. Caught eh::Exception: " << ex.what();
      throw ImplementationException(ostr);
    }
  }

  bool
  TextTemplate::key_used(const String::SubString& key)
    const noexcept
  {
    return keys_.find(key.str()) != keys_.end();
  }

  const std::shared_ptr<const std::vector<std::string>>&
  TextTemplate::expected_post_actions() const noexcept
  {
    return expected_post_actions_;
  }

  void
  Template::get_keys(String::TextTemplate::Keys& keys, const String::SubString& text)
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
    state.check_time = Generics::Time::get_time_of_day();
    read_file_state(creative_template_handler.file.c_str(), state);

    if (creative_template_handler.type == CreativeTemplateFactory::Handler::CTT_TEXT)
    {
      try
      {
        return new TextTemplate(creative_template_handler.file.c_str());
      }
      catch (const TextTemplate::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "CreativeTemplateFactory:create(): "
                "Can't init text template. Caught Exception: " << ex.what();
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
    return Generics::Time::get_time_of_day() - state.check_time > UPDATE_PERIOD;
  }

  Template*
  CreativeTemplateFactory::update(Template* templ, const Handler& handler, State& state) const
  {
    State current_state;
    current_state.check_time = Generics::Time::get_time_of_day();
    if (!read_file_state(handler.file.c_str(), current_state))
    {
      state.check_time = current_state.check_time;
      return ReferenceCounting::add_ref(templ);
    }

    if (state.same_file(current_state))
    {
      state.check_time = current_state.check_time;
      return ReferenceCounting::add_ref(templ);
    }

    try
    {
      Template_var result = create(handler, current_state);
      state = current_state;
      return result.retn();
    }
    catch(const Template::FileNotExists&)
    {
      /* keep old template state if file disappeared */
      state.check_time = current_state.check_time;
      return ReferenceCounting::add_ref(templ);
    }
  }

} // namespace AdServer::CampaignSvcs
