// GTEST
#include <gtest/gtest.h>

// BOOST
#include <boost/algorithm/string.hpp>

// STD
#include <filesystem>
#include <fstream>

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>

// THIS
#include <PredictorML/Predictor.hpp>

namespace
{

std::string get_path_ml_script()
{
  const auto& path = std::filesystem::canonical("/proc/self/exe");
  auto parent = path.parent_path();
  while (
    !parent.empty() &&
    !parent.filename().empty() &&
    parent.filename() != "rtbserver_cobrazz")
  {
    parent = parent.parent_path();
  }

  if (parent.filename().empty())
  {
    throw std::runtime_error(
      "Logic error. Not exist rtbserver_cobrazz directory");
  }
  parent += "/tests/UnitTestsNew/PredictorML/ClasifierTest.py";

  if (!std::filesystem::exists(parent))
  {
    throw std::runtime_error("Not exist machine learning script");
  }

  return parent.string();
}

using Logger_var = Logging::Logger_var;
Logger_var logger = new Logging::OStream::Logger(
  Logging::OStream::Config(
    std::cerr,
    Logging::Logger::CRITICAL));

} // namespace

TEST(PredictorML, Test1)
{
  using Predictor = AdServer::PredictorML::Predictor;
  EXPECT_ANY_THROW(Predictor("/temp/not_exist", logger.in()));
}

void test_ml(const std::size_t number_tags)
{
  using Predictor = AdServer::PredictorML::Predictor;

  const auto ml_script_path = get_path_ml_script();
  const std::string path_model = "/tmp/test_model";
  const std::string path_probabilities = "/tmp/test_probabilities";
  const float zero = AdServer::PredictorML::convert_int(0);

  std::ostringstream command;
  command << "python3 "
          << ml_script_path
          << " -n "
          << number_tags
          << " -p -o "
          << path_model
          << " -r "
          << path_probabilities;
  EXPECT_EQ(std::system(command.str().c_str()), EXIT_SUCCESS);

  Predictor predictor(path_model, logger.in());
  const auto& labels = predictor.labels();
  EXPECT_EQ(labels.size(), number_tags);
  for (std::size_t i = 0; i < number_tags; i += 1)
  {
    EXPECT_EQ(labels[i], (i + 1) * 1000);
  }

  for (std::size_t i = 0; i < number_tags; i += 1)
  {
    auto predict_label = predictor.predict(
      {std::to_string(i + 1), "0", "0", "0", "0"},
      {.0f, .0f, .0f, .0f, .0f});
    EXPECT_EQ(predict_label, (i + 1) * 1000);

    const auto int_tag = AdServer::PredictorML::convert_int(i + 1);
    auto flat_predict_label = predictor.flat_predict(
      {int_tag, zero, zero, zero, zero,
       .0f, .0f, .0f, .0f, .0f});
    EXPECT_EQ(flat_predict_label, (i + 1) * 1000);
  }

  if (number_tags == 2)
  {
    const auto int_tag1 = AdServer::PredictorML::convert_int(1);
    auto features1 = std::vector<float>{
      int_tag1, zero, zero, zero, zero, .0f, .0f, .0f, .0f, .0f};
    const auto int_tag2 = AdServer::PredictorML::convert_int(2);
    auto features2 = std::vector<float>{
      int_tag2, zero, zero, zero, zero, .0f, .0f, .0f, .0f, .0f};

    auto labels = predictor.flat_predict({features1, features2});
    EXPECT_EQ(labels.size(), 2);
    EXPECT_EQ(labels[0], 1000);
    EXPECT_EQ(labels[1], 2000);

    auto labels2 = predictor.predict(
      std::vector<std::vector<std::string>>{
        {"1", "0", "0", "0", "0"},
        {"2", "0", "0", "0", "0"},
      },
      std::vector<std::vector<float>>{
        {.0f, .0f, .0f, .0f, .0f},
        {.0f, .0f, .0f, .0f, .0f}});
    EXPECT_EQ(labels2.size(), 2);
    EXPECT_EQ(labels2[0], 1000);
    EXPECT_EQ(labels2[1], 2000);
  }

  std::ifstream prob_file(path_probabilities);
  EXPECT_TRUE(prob_file);

  std::vector<std::vector<double>> proba_list_file;
  for (std::string line; std::getline(prob_file, line, '\n');)
  {
    std::vector<std::string> strs;
    boost::split(strs, line, boost::is_any_of(" "));
    EXPECT_EQ(strs.size(), number_tags + 1);
    if (strs.size() == number_tags + 1)
    {
      auto probabilities = predictor.predict_proba(
        {strs[0], "0", "0", "0", "0"},
        {.0f, .0f, .0f, .0f, .0f});
      EXPECT_EQ(probabilities.size(), number_tags);

      const auto int_tag = AdServer::PredictorML::convert_int(
        std::atoi(strs[0].c_str()));
      auto flat_probabilities = predictor.flat_predict_proba(
        {int_tag, zero, zero, zero, zero,
         .0f, .0f, .0f, .0f, .0f});
      EXPECT_EQ(flat_probabilities.size(), number_tags);

      std::vector<double> proba;
      for (std::size_t i = 0; i < number_tags; i += 1)
      {
        const double p = std::stod(strs[i + 1]);
        proba.emplace_back(p);
        EXPECT_TRUE(std::abs(p - probabilities[i]) / p < 0.000001);
        EXPECT_TRUE(std::abs(p - flat_probabilities[i]) / p < 0.000001);
      }
      proba_list_file.emplace_back(std::move(proba));
    }
  }

  if (number_tags == 2)
  {
    const auto int_tag1 = AdServer::PredictorML::convert_int(1);
    auto features1 = std::vector<float>{
      int_tag1, zero, zero, zero, zero, .0f, .0f, .0f, .0f, .0f};
    const auto int_tag2 = AdServer::PredictorML::convert_int(2);
    auto features2 = std::vector<float>{
      int_tag2, zero, zero, zero, zero, .0f, .0f, .0f, .0f, .0f};

    auto proba_list1 = predictor.flat_predict_proba({features1, features2});
    EXPECT_EQ(proba_list1.size(), proba_list_file.size());
    for (std::size_t i = 0; i < proba_list1.size(); i += 1)
    {
      EXPECT_TRUE(std::abs(proba_list1[i][0] - proba_list_file[i][0]) / proba_list1[i][0] < 0.000001);
    }

    auto proba_list2 = predictor.predict_proba(
      std::vector<std::vector<std::string>>{
        {"1", "0", "0", "0", "0"},
        {"2", "0", "0", "0", "0"},
      },
      std::vector<std::vector<float>>{
        {.0f, .0f, .0f, .0f, .0f},
        {.0f, .0f, .0f, .0f, .0f}});
    EXPECT_EQ(proba_list2.size(), proba_list_file.size());
    for (std::size_t i = 0; i < proba_list2.size(); i += 1)
    {
      EXPECT_TRUE(std::abs(proba_list2[i][0] - proba_list_file[i][0]) / proba_list2[i][0] < 0.000001);
    }
  }
}

TEST(PredictorML, Test2)
{
  for (std::size_t i = 2; i <= 4; i += 1)
  {
    test_ml(i);
  }
}