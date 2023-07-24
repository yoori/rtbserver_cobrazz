// STD
#include <iostream>

// THIS
#include "Application.hpp"

int main(int argc, char** argv)
{
  try
  {
    Application{}.run(argc, argv);
    return EXIT_SUCCESS;
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << "Fatal error: "
              << exc.what();
  }

  return EXIT_FAILURE;
}
