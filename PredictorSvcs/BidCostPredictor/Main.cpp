// STD
#include <iostream>

// THIS
#include "Application.hpp"

/*#define BOOST_TEST_MODULE Simple testcases
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE(simple_test) {
  BOOST_CHECK_EQUAL(2+2, 4);
}*/

int main(int argc, char** argv)
{
  using namespace PredictorSvcs::BidCostPredictor;
  try
  {
    return Application().run(argc, argv);
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << exc.what() << "\n";
    return EXIT_FAILURE;
  }
}
