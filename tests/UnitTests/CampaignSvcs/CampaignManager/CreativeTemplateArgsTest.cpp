// @file CampaignManager/CreativeTemplateArgsTest.cpp
#include <iostream>
#include <CampaignSvcs/CampaignManager/CreativeTemplateArgs.hpp>

using namespace AdServer::CampaignSvcs;

int
main() noexcept
{
  int ret_code = 0;

  try
  {
    CreativeInstantiateRule rule;
    OptionTokenValueMap token_values;
    TokenProcessorMap token_processor_map;

    {
      TokenSet restrictions;
      restrictions.insert("TEST2");
      restrictions.insert("TEST3");

      BaseTokenProcessor_var token_processor =
        new BaseTokenProcessor(
          "TEST",
          restrictions);
      token_processor_map[1] = token_processor;
    }

    {
      TokenSet restrictions;
      restrictions.insert("TEST3");

      BaseTokenProcessor_var token_processor =
        new BaseTokenProcessor(
          "TEST2",
          restrictions);
      token_processor_map[2] = token_processor;
    }

    {
      TokenSet restrictions;

      BaseTokenProcessor_var token_processor =
        new BaseTokenProcessor(
          "TEST3",
          restrictions);
      token_processor_map[3] = token_processor;
    }

    {
      token_values["TEST"] = OptionValue(1, "##TEST2## ##TEST3##");
      token_values["TEST2"] = OptionValue(2, "##TEST3##");
      token_values["TEST3"] = OptionValue(3, "EEEE");

      std::string result;

      token_processor_map[1]->instantiate(
        token_values,
        token_processor_map,
        rule,
        CreativeInstantiateArgs(),
        result);

      if (result != "EEEE EEEE")
      {
        std::cerr << "Test#1: incorrect instantiate result: '" << result
          << "'." << std::endl;
        ret_code = -1;
      }
      else
      {
        std::cout << "Test#1: OK" << std::endl;
      }
    }

    {
      token_values["TEST"] = OptionValue(1, "##TEST2## ##TEST3##");
      token_values["TEST2"] = OptionValue(2, "123");
      token_values["TEST3"] = OptionValue(3, "");

      std::string result;

      token_processor_map[1]->instantiate(
        token_values,
        token_processor_map,
        rule,
        CreativeInstantiateArgs(),
        result);

      if (result != "123 ")
      {
        std::cerr << "Test#2: incorrect instantiate result: '" << result
          << "'." << std::endl;
        ret_code = -1;
      }
      else
      {
        std::cout << "Test#2: OK" << std::endl;
      }
    }

    try
    {
      token_values["TEST"] = OptionValue(1, "##TEST2## ##TEST3##");
      token_values["TEST2"] = OptionValue(2, "##TEST3## ##TEST##");
      token_values["TEST3"] = OptionValue(3, "EEEE");

      std::string result;

      token_processor_map[1]->instantiate(
        token_values,
        token_processor_map,
        rule,
        CreativeInstantiateArgs(),
        result);

      std::cerr << "Test#3: must be thrown InvalidValue exception."
        << std::endl;
      ret_code = -1;
    }
    catch (const eh::Exception& ex)
    {
      std::cout << "Test#3: OK." << std::endl;
    }
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    ret_code = -1;
  }

  return ret_code;
}
