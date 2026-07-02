#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <CampaignSvcs/CampaignManager/InstantiateAd/CacheArgs.hpp>
#include <CampaignSvcs/CampaignManager/InstantiateAd/InstantiateAdCommonCreativeArgs.hpp>
#include <CampaignSvcs/CampaignManager/InstantiateAd/InstantiateAdCreativeArgs.hpp>
#include <CampaignSvcs/CampaignManager/InstantiateAd/InstantiateAdRequestArgs.hpp>
#include <CampaignSvcs/CampaignManager/InstantiateAd/TokenProcessorArgs.hpp>
#include <CampaignSvcs/CampaignManager/InstantiateAd/UnionArgs.hpp>

namespace
{
  using namespace AdServer::CampaignSvcs::InstantiateAd;

  bool
  get_arg(
    String::TextTemplate::ArgsCallback& args,
    const char* name,
    std::string& value,
    bool need_value = true)
  {
    return args.get_argument(String::SubString(name), value, need_value);
  }

  void
  check(bool condition, const char* message)
  {
    if(!condition)
    {
      std::cerr << message << std::endl;
      std::exit(1);
    }
  }
}

int
main()
{
  auto request_manager = std::make_shared<InstantiateAdRequestArgsManager>();
  auto common_manager =
    std::make_shared<InstantiateAdCommonCreativeArgsManager>();
  auto creative_manager = std::make_shared<InstantiateAdCreativeArgsManager>();

  unsigned long request_token_calls = 0;
  unsigned long common_token_calls = 0;
  unsigned long creative_token_calls = 0;

  request_manager->add_processor(
    "request-token",
    [&request_token_calls](
      const InstantiateAdRequestArgsProvider&)
    {
      ++request_token_calls;
      return std::optional<std::string>("request-value");
    });

  request_manager->add_processor(
    "collision-token",
    [](
      const InstantiateAdRequestArgsProvider&)
    {
      return std::optional<std::string>("request-collision");
    });

  request_manager->add_processor(
    "REQ",
    [](
      const InstantiateAdRequestArgsProvider&)
    {
      return std::optional<std::string>("request");
    });

  common_manager->add_processor(
    "common-token",
    [&common_token_calls](
      const InstantiateAdCommonCreativeArgsProvider&)
    {
      ++common_token_calls;
      return std::optional<std::string>("common-value");
    });

  creative_manager->add_processor(
    "creative-token",
    [&creative_token_calls](
      const InstantiateAdCreativeArgsProvider& provider)
    {
      ++creative_token_calls;
      return std::optional<std::string>(
        provider.context().app_format);
    });

  creative_manager->add_processor(
    "collision-token",
    [](
      const InstantiateAdCreativeArgsProvider&)
    {
      return std::optional<std::string>("creative-collision");
    });

  auto request_context = std::make_shared<InstantiateAdContext>();
  auto request_union = std::make_shared<UnionArgs>();
  request_union->add_provider(
    request_manager->create_provider(request_context));
  request_union->add_provider(
    common_manager->create_provider(request_context));
  auto request_level_args = std::make_shared<CacheArgs>(request_union);

  std::string value;
  check(
    get_arg(*request_level_args, "request-token", value) &&
      value == "request-value",
    "request token resolution failed");
  check(
    get_arg(*request_level_args, "request-token", value) &&
      value == "request-value",
    "request token cache lookup failed");
  check(request_token_calls == 1, "request token wasn't cached");

  check(
    get_arg(*request_level_args, "common-token", value) &&
      value == "common-value",
    "common token resolution failed");
  check(
    get_arg(*request_level_args, "common-token", value) &&
      value == "common-value",
    "common token cache lookup failed");
  check(common_token_calls == 1, "common token wasn't cached");

  const char* creative_values[] = {"creative-1", "creative-2"};
  for(const char* creative_value : creative_values)
  {
    auto creative_context = std::make_shared<InstantiateAdContext>();
    creative_context->app_format = creative_value;

    auto creative_union = std::make_shared<UnionArgs>();
    creative_union->add_provider(
      creative_manager->create_provider(
        creative_context,
        request_level_args));
    auto creative_level_args = std::make_shared<CacheArgs>(creative_union);

    auto final_args = std::make_shared<UnionArgs>();
    final_args->add_provider(request_level_args);
    final_args->add_provider(creative_level_args);

    check(
      get_arg(*final_args, "collision-token", value) &&
        value == "request-collision",
      "provider priority is broken");

    check(
      get_arg(*final_args, "creative-token", value) &&
        value == creative_value,
      "creative token resolution failed");
    check(
      get_arg(*final_args, "creative-token", value) &&
        value == creative_value,
      "creative token cache lookup failed");
  }

  check(creative_token_calls == 2, "creative token cache scope is broken");

  {
    using namespace AdServer::CampaignSvcs;

    auto creative_tokens = std::make_shared<OptionTokenValueMap>();
    (*creative_tokens)["lazy-token"] =
      OptionValue(7, "prefix-##REQ##-##nested-token##");
    (*creative_tokens)["nested-token"] =
      OptionValue(0, "creative");

    TokenProcessorMap token_processors;
    TokenSet insert_restrictions;
    insert_restrictions.insert("REQ");
    insert_restrictions.insert("nested-token");
    token_processors[7] = new BaseTokenProcessor(
      "lazy-token",
      insert_restrictions);
    CreativeInstantiateRule instantiate_rule;
    CreativeInstantiateArgs creative_args;

    auto lazy_creative_args = std::make_shared<CacheArgs>(
      std::make_shared<TokenProcessorArgs>(
        creative_tokens,
        token_processors,
        instantiate_rule,
        creative_args,
        request_level_args));

    check(
      get_arg(*lazy_creative_args, "lazy-token", value) &&
        value == "prefix-request-creative",
      "lazy token processor resolution failed");
    check(
      get_arg(*lazy_creative_args, "lazy-token", value) &&
        value == "prefix-request-creative",
      "lazy token processor cache lookup failed");
  }

  check(
    get_arg(*request_level_args, "request-token", value, false) &&
      value == "request-token",
    "value=false handling is broken");
  check(request_token_calls == 1, "value=false should not compute value");

  std::cout << "InstantiateAdArgsTest passed" << std::endl;
  return 0;
}
