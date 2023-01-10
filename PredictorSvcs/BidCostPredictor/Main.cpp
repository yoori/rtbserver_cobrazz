// STD
#include <iostream>

// THIS
#include "Application.hpp"

int main(int argc, char** argv)
{
  try
  {
    return PredictorSvcs::BidCostPredictor::Application().run(argc, argv);
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << exc.what()
              << "\n";
    return EXIT_FAILURE;
  }
}
