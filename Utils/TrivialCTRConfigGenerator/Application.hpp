#pragma once

#include <iostream>
#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>
#include <String/SubString.hpp>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  Application_() noexcept;

  virtual
  ~Application_() noexcept;

  int
  main(int& argc, char** argv);

protected:
  void
  generate_(
    const String::SubString& input_folder,
    const String::SubString& output_folder);
};

typedef Generics::Singleton<Application_> Application;
