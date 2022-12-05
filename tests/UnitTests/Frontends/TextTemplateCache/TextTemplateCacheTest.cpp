#include <iostream>
#include <string>
#include <Commons/TextTemplateCache.hpp>

using namespace AdServer;

unsigned long ONE_MB = 1024 * 1024;

int main(int /*argc*/, char** /*argv*/)
{
  ::system("mkdir ~tmp 2>/dev/null ; echo '%%TEST%%' >~tmp/t");

  typedef std::map<String::SubString, std::string> ArgMap;
  ArgMap args_cont;
  args_cont[String::SubString("TEST")] = "XX";
  String::TextTemplate::ArgsContainer<ArgMap> args(&args_cont);

  {
    Commons::TextTemplateCache_var cache(new Commons::TextTemplateCache(
      ONE_MB,
      Generics::Time::ONE_MINUTE,
      Commons::TextTemplateCacheConfiguration<Commons::TextTemplate>(
        Generics::Time::ONE_SECOND)));

    Generics::Timer timer;
    timer.start();

    for(int i = 0; i < 10000; ++i)
    {
      Commons::TextTemplate_var t = cache->get("~tmp/t");
      std::string res = t->instantiate(args);
      /*
      std::cout << res << std::endl;
      */
    }

    timer.stop();
    std::cout << timer.elapsed_time() << std::endl;
  }

  {
    Commons::TextTemplateCache_var cache(new Commons::TextTemplateCache(
      ONE_MB,
      Generics::Time::ONE_MINUTE,
      Commons::TextTemplateCacheConfiguration<Commons::TextTemplate>(
        Generics::Time::ZERO)));

    Generics::Timer timer;
    timer.start();

    for(int i = 0; i < 10000; ++i)
    {
      Commons::TextTemplate_var t = cache->get("~tmp/t");
      std::string res = t->instantiate(args);
      /*
      std::cout << res << std::endl;
      */
    }

    timer.stop();
    std::cout << timer.elapsed_time() << std::endl;
  }

  return 0;
}
