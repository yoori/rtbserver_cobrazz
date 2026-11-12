#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

#include <DTree/FloatFeatures.hpp>

#include <CampaignSvcs/CampaignManager/CTR/VangaCTREvaluator.hpp>

namespace
{
  using namespace AdServer::CampaignSvcs::CTR;

  void check_prediction(
    const char* name,
    const VangaCTREvaluator& evaluator,
    const Vanga::DTree& native_model,
    const std::array<const HashArray*, 4>& hashes,
    const char* svm_row)
  {
    Vanga::FloatFeatureArray svm_features;
    std::istringstream input(svm_row);
    if (!Vanga::load_float_features(input, svm_features))
    {
      throw std::runtime_error("Missing LibSVM row");
    }

    const double expected = native_model.fpredict(svm_features);
    const double actual = evaluator.predict(
      ModelTraits{}, nullptr, nullptr, hashes[0], hashes[1], hashes[2], hashes[3]);
    if (!std::isfinite(actual) || std::abs(actual - expected) > 1e-7)
    {
      throw std::runtime_error(
        std::string(name) + ": expected=" + std::to_string(expected) +
        ", actual=" + std::to_string(actual));
    }
  }
}

int main(int argc, char** argv)
{
  try
  {
    if (argc != 3)
    {
      throw std::runtime_error("Usage: VangaCTREvaluatorTest <binary model> <numeric model>");
    }

    const VangaCTREvaluator evaluator{String::SubString(argv[1])};
    std::ifstream model_file(argv[1]);
    const Vanga::DTree_var native_model = Vanga::DTree::load(model_file);

    Generics::MonoAllocatorArena arena;
    HashArray request{HashArray::allocator_type(arena)};
    HashArray auction{HashArray::allocator_type(arena)};
    HashArray candidate{HashArray::allocator_type(arena)};
    HashArray opt{HashArray::allocator_type(arena)};
    HashArray adjacent{HashArray::allocator_type(arena)};
    request.emplace_back(0, 1.0f);
    auction.emplace_back(99, 1.0f);
    candidate.emplace_back(3350, 1.0f);
    opt.emplace_back(16383, 1.0f);
    adjacent.emplace_back(1, 1.0f);

    check_prediction(
      "request index zero", evaluator, *native_model,
      {&request, nullptr, nullptr, nullptr}, "0 1:1");
    check_prediction(
      "auction", evaluator, *native_model, {nullptr, &auction, nullptr, nullptr}, "0 100:1");
    check_prediction(
      "candidate", evaluator, *native_model, {nullptr, nullptr, &candidate, nullptr}, "0 3351:1");
    check_prediction(
      "last index", evaluator, *native_model, {nullptr, nullptr, nullptr, &opt}, "0 16384:1");
    check_prediction(
      "adjacent index", evaluator, *native_model, {&adjacent, nullptr, nullptr, nullptr}, "0 2:1");

    for (unsigned int i = 0; i < 3; ++i)
    {
      check_prediction(
        "all feature levels", evaluator, *native_model,
        {&request, &auction, &candidate, &opt}, "0 1:1 100:1 3351:1 16384:1");
      check_prediction(
        "cleared feature buffer", evaluator, *native_model,
        {nullptr, nullptr, nullptr, nullptr}, "0");
    }

    const VangaCTREvaluator numeric_evaluator{String::SubString(argv[2])};
    std::ifstream numeric_model_file(argv[2]);
    const Vanga::DTree_var numeric_model = Vanga::DTree::load(numeric_model_file);
    request[0].second = 0.25f;
    auction[0].second = -1.0f;
    candidate[0].second = 2.5f;
    opt[0].second = 0.75f;

    check_prediction(
      "request fraction", numeric_evaluator, *numeric_model,
      {&request, nullptr, nullptr, nullptr}, "0 1:0.25");
    check_prediction(
      "auction negative sentinel", numeric_evaluator, *numeric_model,
      {nullptr, &auction, nullptr, nullptr}, "0 100:-1");
    check_prediction(
      "candidate value above one", numeric_evaluator, *numeric_model,
      {nullptr, nullptr, &candidate, nullptr}, "0 3351:2.5");
    check_prediction(
      "optional fraction", numeric_evaluator, *numeric_model,
      {nullptr, nullptr, nullptr, &opt}, "0 16384:0.75");

    for (unsigned int i = 0; i < 3; ++i)
    {
      check_prediction(
        "all numeric feature levels", numeric_evaluator, *numeric_model,
        {&request, &auction, &candidate, &opt}, "0 1:0.25 100:-1 3351:2.5 16384:0.75");
      check_prediction(
        "cleared numeric feature buffer", numeric_evaluator, *numeric_model,
        {nullptr, nullptr, nullptr, nullptr}, "0");
    }

    request[0].second = 0.5f;
    check_prediction(
      "threshold equality", numeric_evaluator, *numeric_model,
      {&request, nullptr, nullptr, nullptr}, "0 1:0.5");
    request[0].second = std::nextafter(0.5f, 1.0f);
    check_prediction(
      "float immediately above threshold", numeric_evaluator, *numeric_model,
      {&request, nullptr, nullptr, nullptr}, "0 1:0.500000059604644775390625");
    request[0].second = 0.75f;
    check_prediction(
      "distinct fraction", numeric_evaluator, *numeric_model,
      {&request, nullptr, nullptr, nullptr}, "0 1:0.75");
    candidate.clear();
    candidate.emplace_back(0, 0.0f);
    check_prediction(
      "candidate overwrites request with zero", numeric_evaluator, *numeric_model,
      {&request, nullptr, &candidate, nullptr}, "0 1:0");
    request[0].second = std::numeric_limits<float>::quiet_NaN();
    opt[0].second = std::numeric_limits<float>::quiet_NaN();
    check_prediction(
      "NaN branch directions", numeric_evaluator, *numeric_model,
      {&request, nullptr, nullptr, &opt}, "0 1:nan 16384:nan");
    check_prediction(
      "cleared NaN feature buffer", numeric_evaluator, *numeric_model,
      {nullptr, nullptr, nullptr, nullptr}, "0");

    std::cout << "Vanga raw predictions match the native model" << std::endl;
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "VangaCTREvaluatorTest failed: " << ex.what() << std::endl;
    return 1;
  }
}
