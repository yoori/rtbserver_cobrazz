// STD
#include <iostream>

// THIS
#include <Frontends/HttpServer/Application.hpp>

int main(int argc, char** argv)
{
  using Application = AdServer::Frontends::Http::Application;
  using Application_var = AdServer::Frontends::Http::Application_var;

  try
  {
    Application_var application(new Application);
    return application->run(argc, argv);
  }
  catch (const std::exception& exc)
  {
    std::cerr << "HttpServer is failed, reason="
              << exc.what();
    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "HttpServer is failed. "
                 "Unknown error";
    return EXIT_FAILURE;
  }
}