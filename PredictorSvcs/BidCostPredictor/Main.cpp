// STD
#include <iostream>

// THIS
#include "Application.hpp"

int main(int argc, char** argv)
{
  using namespace PredictorSvcs::BidCostPredictor;

  try
  {
    return Application().run(argc, argv);
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << exc.what()
              << '\n';
    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Unknown error"
              << '\n';
    return EXIT_FAILURE;
  }
}
