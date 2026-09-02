#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <Utils/Predictor/FeatureCorrelationPruner/CorrelationPruner.hpp>

namespace
{
  using Pruner = AdServer::Predictor::FeatureCorrelationPruner;

  void
  require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  const Pruner::Drop&
  find_drop(const Pruner::Result& result, Pruner::FeatureIndex index)
  {
    for (const auto& drop : result.drops)
    {
      if (drop.index == index)
      {
        return drop;
      }
    }
    throw std::runtime_error("Expected dropped feature is absent");
  }

  void
  test_greedy_does_not_remove_through_dropped_feature()
  {
    Pruner pruner({1, 2, 3}, 0.97);
    for (unsigned long row = 0; row < 1000; ++row)
    {
      bool first = row % 2 == 0;
      bool second = first;
      if (row < 10)
      {
        second = !second;
      }
      bool third = second;
      if (row >= 10 && row < 20)
      {
        third = !third;
      }
      pruner.add_row({
        {1, first ? 1.0F : 0.0F},
        {2, second ? 1.0F : 0.0F},
        {3, third ? 1.0F : 0.0F},
      });
    }

    const auto result = pruner.result();
    require(result.kept == Pruner::FeatureIndexes({1, 3}), "Features 1 and 3 must be kept");
    require(result.dropped == Pruner::FeatureIndexes({2}), "Only feature 2 must be dropped");
    const auto& drop = find_drop(result, 2);
    require(drop.representative == 1, "Feature 1 must represent feature 2");
    require(std::abs(drop.correlation - 0.98) < 1e-12, "Unexpected feature correlation");
  }

  void
  test_absolute_numeric_correlation_and_constant_feature()
  {
    Pruner pruner({10, 20, 30, 40}, 0.999);
    for (unsigned long row = 0; row < 1000; ++row)
    {
      const float value = static_cast<float>(row) / 1000;
      pruner.add_row({
        {10, value},
        {20, 2 * value + 5},
        {30, -value},
        {40, 1},
      });
    }

    const auto result = pruner.result();
    require(result.kept == Pruner::FeatureIndexes({10, 40}), "Numeric representatives mismatch");
    require(
      result.dropped == Pruner::FeatureIndexes({20, 30}),
      "Perfectly correlated numeric features must be dropped");
    require(find_drop(result, 20).representative == 10, "Feature 20 representative mismatch");
    require(find_drop(result, 30).representative == 10, "Feature 30 representative mismatch");
  }

  void
  test_multiple_svm_sources_and_missing_values()
  {
    Pruner pruner({1, 2, 3}, 0.98);
    std::ostringstream first_data;
    std::ostringstream second_data;
    for (unsigned long row = 0; row < 1000; ++row)
    {
      std::ostream& output = row < 500 ? first_data : second_data;
      output << "0";
      const bool value = row % 2 == 0;
      if (value)
      {
        output << " 1:1 2:1";
      }
      if (row % 4 == 0)
      {
        output << " 3:1";
      }
      output << '\n';
    }

    std::istringstream first(first_data.str());
    std::istringstream second(second_data.str());
    pruner.add_svm(first, "first.libsvm");
    pruner.add_svm(second, "second.libsvm");

    const auto result = pruner.result();
    require(result.rows == 1000, "Rows from all LibSVM sources must be counted");
    require(result.kept == Pruner::FeatureIndexes({1, 3}), "LibSVM kept features mismatch");
    require(result.dropped == Pruner::FeatureIndexes({2}), "LibSVM dropped features mismatch");
  }

  void
  test_invalid_svm_reports_source_and_line()
  {
    Pruner pruner({1}, 0.98);
    std::istringstream input("0 1:1\n0 1:not-a-number\n");
    try
    {
      pruner.add_svm(input, "broken.libsvm");
    }
    catch(const Pruner::Exception& exception)
    {
      const std::string message = exception.what();
      require(
        message.find("broken.libsvm:2") != std::string::npos,
        "Parse error must contain source and line");
      return;
    }
    throw std::runtime_error("Invalid LibSVM value must fail");
  }

  void
  test_empty_input_keeps_all_features()
  {
    Pruner pruner({7, 9}, 0.98);
    const auto result = pruner.result();
    require(result.rows == 0, "Empty input row count mismatch");
    require(result.kept == Pruner::FeatureIndexes({7, 9}), "Empty input must keep all features");
    require(result.dropped.empty(), "Empty input must not drop features");
  }
}

int main()
{
  try
  {
    test_greedy_does_not_remove_through_dropped_feature();
    test_absolute_numeric_correlation_and_constant_feature();
    test_multiple_svm_sources_and_missing_values();
    test_invalid_svm_reports_source_and_line();
    test_empty_input_keeps_all_features();
    std::cout << "FeatureCorrelationPrunerTest passed" << std::endl;
    return 0;
  }
  catch(const std::exception& exception)
  {
    std::cerr << "FeatureCorrelationPrunerTest failed: " << exception.what() << std::endl;
  }
  return 1;
}
