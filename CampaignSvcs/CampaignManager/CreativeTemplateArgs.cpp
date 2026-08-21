#include <sstream>

#include <HTTP/UrlAddress.hpp>
#include <String/StringManip.hpp>

#include "CreativeTemplateArgs.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  void
  TokenValueMap::set_value(std::string_view name, std::string&& value)
  {
    values_[std::string(name)] = std::move(value);
  }

  void
  TokenValueMap::set_value(std::string_view name, const std::string& value)
  {
    values_[std::string(name)] = value;
  }

  void
  TokenValueMap::set_value(std::string_view name, std::string_view value)
  {
    values_[std::string(name)] = std::string(value);
  }

  void
  TokenValueMap::set_value(std::string_view name, const char* value)
  {
    values_[std::string(name)] = value ? value : "";
  }

  bool
  TokenValueMap::get_argument(
    const String::SubString& key,
    std::string& result,
    bool value) const
  {
    if (!value)
    {
      result.assign(key.data(), key.size());
      return true;
    }

    const std::string_view name(key.data(), key.size());
    const auto it = values_.find(name);
    if (it == values_.end())
    {
      return false;
    }

    result = it->second;
    return true;
  }

  bool
  TokenValueMap::empty() const noexcept
  {
    return values_.empty();
  }

  std::size_t
  TokenValueMap::size() const noexcept
  {
    return values_.size();
  }

  void
  TokenValueMap::clear() noexcept
  {
    values_.clear();
  }

  bool CreativeInstantiateRule::instantiate_relative_protocol_url(
    std::string& url) const noexcept
  {
    if (RELATIVE_URL_PREFIX.start(url))
    {
      url.insert(0, url_prefix.c_str(), url_prefix.size());
      return true;
    }
    return false;
  }

  class TemplateArgsHelper: public String::TextTemplate::ArgsCallback
  {
  public:
    TemplateArgsHelper(
      const TokenProcessorMap& token_processors,
      const TokenSet& insert_restrictions,
      const TokenOptionValueProvider& token_values,
      const CreativeInstantiateRule& rule,
      const CreativeInstantiateArgs& creative_args,
      unsigned long max_depth)
      : token_processors_(token_processors),
        insert_restrictions_(insert_restrictions),
        token_values_(token_values),
        rule_(rule),
        creative_args_(creative_args),
        max_depth_(max_depth)
    {}

  private:
    bool
    get_argument(const String::SubString& key, std::string& result,
      bool value = true) const override
      /*throw(BaseTokenProcessor::InvalidValue, eh::Exception)*/
    {
      if (max_depth_ == 0)
      {
        Stream::Error ostr;
        ostr << "Max depth exceeded.";
        throw BaseTokenProcessor::InvalidValue(ostr);
      }

      const std::string_view key_view(key.data(), key.size());
      std::string key_str(key_view);
      if (insert_restrictions_.find(key_str) == insert_restrictions_.end())
      {
        Stream::Error ostr;
        ostr << "Invalid use token (" << key.size() << ")'" << key << "'.";
        throw BaseTokenProcessor::InvalidValue(ostr);
      }

      //FIXME
      if (!value)
      {
        result.assign(key.data(), key.size());
        return true;
      }

      BaseTokenProcessor_var token_processor;
      TokenOptionValue token_value;
      if (token_values_.get(key_view, token_value))
      {
        TokenProcessorMap::const_iterator it = token_processors_.find(
          token_value.option_id);

        if (it != token_processors_.end())
        {
          token_processor = it->second;
        }
      }

      if (!token_processor.in())
      {
        token_processor = BaseTokenProcessor::default_token_processor(
          key_str.c_str());
      }

      return token_processor->instantiate(
        token_values_,
        token_processors_,
        rule_,
        creative_args_,
        result,
        max_depth_ - 1);
    }

  private:
    const TokenProcessorMap& token_processors_;
    const TokenSet& insert_restrictions_;
    const TokenOptionValueProvider& token_values_;
    const CreativeInstantiateRule& rule_;
    const CreativeInstantiateArgs& creative_args_;
    unsigned long max_depth_;
  };

  TokenOptionValueProvider::TokenOptionValueProvider(
    const String::TextTemplate::ArgsCallback* request_args,
    const OptionTokenValueMap& creative_args)
    noexcept
    : request_args_(request_args),
      creative_args_(creative_args)
  {}

  TokenOptionValueProvider::TokenOptionValueProvider(
    const TokenValueMap& request_args,
    const OptionTokenValueMap& creative_args)
    noexcept
    : request_args_(&request_args),
      creative_args_(creative_args)
  {}

  TokenOptionValueProvider::TokenOptionValueProvider(
    const OptionTokenValueMap& creative_args)
    noexcept
    : request_args_(nullptr),
      creative_args_(creative_args)
  {}

  bool
  TokenOptionValueProvider::get(
    std::string_view token,
    TokenOptionValue& value) const
  {
    OptionTokenValueMap::const_iterator creative_it =
      creative_args_.find(token);
    if (creative_it != creative_args_.end())
    {
      value.option_id = creative_it->second.option_id;
      value.value = creative_it->second.value;
      return true;
    }

    if (request_args_)
    {
      std::string request_value;
      if (request_args_->get_argument(
          String::SubString(token.data(), token.size()),
          request_value))
      {
        value.option_id = 0;
        value.value = request_value;
        return true;
      }
    }

    return false;
  }

  /** BaseTokenProcessor */
  BaseTokenProcessor::BaseTokenProcessor(
    const char* token_name,
    const TokenSet& insert_restrictions)
    /*throw(eh::Exception)*/
    : token_(token_name),
      insert_restrictions_(insert_restrictions)
  {}

  bool
  BaseTokenProcessor::instantiate(
    const TokenOptionValueProvider& token_values,
    const TokenProcessorMap& token_processors,
    const CreativeInstantiateRule& rule,
    const CreativeInstantiateArgs& creative_args,
    std::string& result,
    int max_depth)
    const /*throw(eh::Exception)*/
  {
    TokenOptionValue token_value;
    const bool has_value = token_values.get(token_, token_value);

    try
    {
      TemplateArgsHelper sub_args(
        token_processors,
        insert_restrictions_,
        token_values,
        rule,
        creative_args,
        max_depth);

      String::TextTemplate::DefaultValue default_cont(&sub_args);
      String::TextTemplate::ArgsEncoder encoder(&default_cont);

      if (!has_value && !rule.use_empty_values_)
      {
        return false;
      }

      String::TextTemplate::Basic templ(
        has_value ?
          String::SubString(token_value.value.data(), token_value.value.size()) :
          String::SubString(),
        TokenTemplateProperties::START_TOKEN,
        TokenTemplateProperties::STOP_TOKEN);

      result = templ.instantiate(encoder);

      post_process(rule, creative_args, result);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't init token '" << token_ <<
        "'='" <<
        (has_value ? std::string(token_value.value) : "") <<
        "': " << ex.what();
      throw Exception(ostr);
    }

    return true;
  }

  bool
  BaseTokenProcessor::instantiate(
    const OptionTokenValueMap& token_values,
    const TokenProcessorMap& token_processors,
    const CreativeInstantiateRule& rule,
    const CreativeInstantiateArgs& creative_args,
    std::string& result,
    int max_depth)
    const /*throw(eh::Exception)*/
  {
    const TokenOptionValueProvider token_value_provider(token_values);
    return instantiate(
      token_value_provider,
      token_processors,
      rule,
      creative_args,
      result,
      max_depth);
  }

  BaseTokenProcessor*
  BaseTokenProcessor::default_token_processor(
    const char* token)
  {
    return new BaseTokenProcessor(token, TokenSet());
  }

  /* LinkTokenProcessor */
  LinkTokenProcessor::LinkTokenProcessor(
    const char* token_name,
    const TokenSet& insert_restrictions)
    /*throw(eh::Exception)*/
    : BaseTokenProcessor(
        token_name,
        insert_restrictions)
  {}

  void
  LinkTokenProcessor::post_process(
    const CreativeInstantiateRule& rule,
    const CreativeInstantiateArgs& /*creative_args*/,
    std::string& value) const
  {
    if (!value.empty())
    {
      rule.instantiate_relative_protocol_url(value);
    }

    if (!value.empty())
    {
      try
      {
        HTTP::BrowserAddress url(value);
        value = url.url();
      }
      catch(...)
      {}
    }
  }

  /* UrlTokenProcessor */
  UrlTokenProcessor::UrlTokenProcessor(
    const char* token_name,
    const TokenSet& insert_restrictions)
    /*throw(eh::Exception)*/
    : BaseTokenProcessor(
        token_name,
        insert_restrictions)
  {}

  void
  UrlTokenProcessor::post_process(
    const CreativeInstantiateRule& rule,
    const CreativeInstantiateArgs& /*creative_args*/,
    std::string& value) const
  {
    if (!value.empty() &&
       !rule.instantiate_relative_protocol_url(value) &&
       !HTTP::HTTP_PREFIX.start(value) &&
       !HTTP::HTTPS_PREFIX.start(value))
    {
      value = rule.image_url + value;
    }

    if (!value.empty())
    {
      try
      {
        HTTP::BrowserAddress url(value);
        value = url.url();
      }
      catch(...)
      {}
    }
  }

  /* FileTokenProcessor */
  FileTokenProcessor::FileTokenProcessor(
    const char* token_name,
    const TokenSet& insert_restrictions)
    /*throw(eh::Exception)*/
    : BaseTokenProcessor(
        token_name,
        insert_restrictions)
  {}

  void
  FileTokenProcessor::post_process(
    const CreativeInstantiateRule& rule,
    const CreativeInstantiateArgs& /*creative_args*/,
    std::string& value) const
  {
    if (!value.empty())
    {
      value = rule.image_url + value;

      try
      {
        HTTP::BrowserAddress url(value);
        value = url.url();
      }
      catch(...)
      {}
    }
  }

  /* PublisherUrlTokenProcessor */
  PublisherUrlTokenProcessor::PublisherUrlTokenProcessor(
    const char* token_name,
    const TokenSet& insert_restrictions)
    /*throw(eh::Exception)*/
    : BaseTokenProcessor(
        token_name,
        insert_restrictions)
  {}

  void
  PublisherUrlTokenProcessor::post_process(
    const CreativeInstantiateRule& rule,
    const CreativeInstantiateArgs& /*creative_args*/,
    std::string& value) const
  {
    if (!value.empty() &&
       ::strncmp(value.c_str(), "http://", 7) != 0 &&
       ::strncmp(value.c_str(), "https://", 8) != 0)
    {
      value = rule.publ_url + value;
    }
  }

  /* PublisherFileTokenProcessor */
  PublisherFileTokenProcessor::PublisherFileTokenProcessor(
    const char* token_name,
    const TokenSet& insert_restrictions)
    /*throw(eh::Exception)*/
    : BaseTokenProcessor(
        token_name,
        insert_restrictions)
  {}

  void
  PublisherFileTokenProcessor::post_process(
    const CreativeInstantiateRule& rule,
    const CreativeInstantiateArgs& /*creative_args*/,
    std::string& value) const
  {
    if (!value.empty())
    {
      value = rule.publ_url + value;
    }
  }

  // DynamicContentUrlTokenProcessor
  DynamicContentUrlTokenProcessor::DynamicContentUrlTokenProcessor(
    const char* token_name,
    const TokenSet& insert_restrictions)
    /*throw(eh::Exception)*/
    : BaseTokenProcessor(
        token_name,
        insert_restrictions)
  {}

  void
  DynamicContentUrlTokenProcessor::post_process(
    const CreativeInstantiateRule& /*rule*/,
    const CreativeInstantiateArgs& creative_args,
    std::string& value) const
  {
    if (!value.empty() &&
       value[0] == '/' &&
       (value.size() < 2 || value[1] != '/'))
    {
      std::string mime_file;
      String::StringManip::mime_url_encode(value, mime_file);

      std::ostringstream res_ostr;
      res_ostr << creative_args.full_dynamic_creative_prefix <<
        "&file=" << mime_file <<
        creative_args.last_c_param;

      value = res_ostr.str();
    }
  }
}
}
