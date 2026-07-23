#include <iostream>
#include <memory>
#include <string>
#include <Generics/TaskRunner.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Commons/TextTemplateAsyncCache.hpp>

using namespace AdServer;

unsigned long ONE_MB = 1024 * 1024;

int main(int /*argc*/, char** /*argv*/)
{
  ::system("mkdir ~tmp 2>/dev/null ; echo '%%TEST%%' >~tmp/t");

  using ArgMap = std::map<String::SubString, std::string>;
  ArgMap args_cont;
  args_cont[String::SubString("TEST")] = "XX";
  String::TextTemplate::ArgsContainer<ArgMap> args(&args_cont);
  Logging::ActiveObjectCallbackImpl_var callback(
    new Logging::ActiveObjectCallbackImpl());
  Generics::TaskRunner_var task_runner(
    new Generics::TaskRunner(callback, 1));

  {
    Commons::TextTemplateCachePtr cache =
      std::make_shared<Commons::TextTemplateCache>(
        ONE_MB,
        task_runner.in(),
        Generics::Time::ONE_MINUTE,
        Generics::Time::ONE_SECOND);

    Generics::Timer timer;
    timer.start();

    for(int i = 0; i < 10000; ++i)
    {
      Commons::TextTemplatePtr t =
        cache->get_sync(std::string("~tmp/t"), std::string());
      std::string res = t->instantiate(args);
      /*
      std::cout << res << std::endl;
      */
    }

    timer.stop();
    std::cout << timer.elapsed_time() << std::endl;
  }

  {
    Commons::TextTemplateCachePtr cache =
      std::make_shared<Commons::TextTemplateCache>(
        ONE_MB,
        task_runner.in(),
        Generics::Time::ONE_MINUTE,
        Generics::Time::ZERO);

    Generics::Timer timer;
    timer.start();

    for(int i = 0; i < 10000; ++i)
    {
      Commons::TextTemplatePtr t =
        cache->get_sync(std::string("~tmp/t"), std::string());
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
