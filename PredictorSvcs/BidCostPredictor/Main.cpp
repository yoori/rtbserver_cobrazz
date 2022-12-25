// STD
#include <iostream>

// THIS
#include "Application.hpp"

int main(int argc, char** argv)
{
  try
  {
    const int exit_code
        = PredictorSvcs::BidCostPredictor::Application().run(argc, argv);
    return exit_code;
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << exc.what();
    return EXIT_FAILURE;
  }
}
