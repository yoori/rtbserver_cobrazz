// BOOST
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// STD
#include <fstream>
#include <iostream>

// THIS
#include "Benchmark.hpp"
#include "TimeMeter.hpp"

Benchmark::Benchmark(
  Logger* logger,
  const std::string& model_path,
  const std::string& csv_path,
  const std::uint32_t number_line)
  : logger_(ReferenceCounting::add_ref(logger)),
    model_path_(model_path),
    csv_path_(csv_path),
    number_line_(number_line)
{
}

int Benchmark::run() noexcept
{
  {
    std::ostringstream stream;
    stream << FNS
           << "Start loading model="
           << model_path_;
    logger_->info(stream.str());
  }

  TimeMeter time_meter;
  time_meter.start();
  predictor_ = std::make_unique<Predictor>(
    model_path_,
    logger_.in());
  const auto prepare_elapsed_time = time_meter.stop();

  {
    std::ostringstream stream;
    stream << FNS
           << "Model is successfully loaded, "
              "elapsed_time="
           << prepare_elapsed_time
           << "[ms]";
    logger_->info(stream.str());
  }

  {
    std::ostringstream stream;
    stream << FNS
           << "Start prepare data from csv_file="
           << csv_path_;
    logger_->info(stream.str());
  }

  Data data;
  prepare_date(data);

  {
    std::ostringstream stream;
    stream << FNS
           << "Data prepare successfully";
    logger_->info(stream.str());
  }

  model_process(data);
  model_process_bunch(data);

  return EXIT_SUCCESS;
}

void Benchmark::model_process(const Data& data)
{
  TimeMeter time_meter;

  {
    std::ostringstream stream;
    stream << FNS
           << "Start process model"
           << csv_path_
           << ". Number line="
           << data.cat_features_list.size();
    logger_->info(stream.str());
  }

  time_meter.start();
  const auto number = data.cat_features_list.size();
  for (std::size_t i = 0; i < number; i += 1)
  {
    auto proba = predictor_->predict_proba(
      data.cat_features_list[i],
      data.float_features_list[i]);
    if (proba.size() != 2)
    {
      throw std::runtime_error("Logic error");
    }
  }

  const auto model_elapsed_time = time_meter.stop();
  {
    std::ostringstream stream;
    stream << FNS
           << "Model elapsed time="
           << model_elapsed_time
           << "[ms]";
    logger_->info(stream.str());
  }

  {
    std::ostringstream stream;
    stream << FNS
           << "Model processing is successfully stopped.";
    logger_->info(stream.str());
  }
}

void Benchmark::model_process_bunch(const Data& data)
{
  TimeMeter time_meter;

  {
    std::ostringstream stream;
    stream << FNS
           << "Start process model(bunch)"
           << csv_path_
           << ". Number line="
           << data.cat_features_list.size();
    logger_->info(stream.str());
  }

  time_meter.start();
  auto proba = predictor_->predict_proba(
    data.cat_features_list,
    data.float_features_list);
  if (proba.size() != data.cat_features_list.size())
  {
    throw std::runtime_error("Wrong predictor");
  }

  const auto model_elapsed_time = time_meter.stop();
  {
    std::ostringstream stream;
    stream << FNS
           << "Model(bunch) elapsed time="
           << model_elapsed_time
           << "[ms]";
    logger_->info(stream.str());
  }

  {
    std::ostringstream stream;
    stream << FNS
           << "Model(bunch) processing "
              "is successfully stopped.";
    logger_->info(stream.str());
  }
}

void Benchmark::prepare_date(Data& data)
{
  const auto number_cat_features =
    predictor_->cat_features_count();
  const auto number_float_features =
    predictor_->float_features_count();
  std::size_t total_number =
    1 + number_cat_features + number_float_features;

  data.cat_features_list.reserve(number_line_);
  data.float_features_list.reserve(number_line_);

  std::ifstream file(csv_path_);
  if (!file)
  {
    std::ostringstream stream;
    stream << "Can't open file="
           << csv_path_;
    logger_->critical(
      stream.str());
    throw std::runtime_error(stream.str());
  }

  std::string line;
  line.reserve(50000);
  std::vector<std::string> strings;
  std::uint32_t i = 1;
  while (i <= number_line_ + 1 && std::getline(file, line))
  {
    if (i == 1)
    {
      i += 1;
      continue;
    }

    boost::split(
      strings,
      line,
      boost::is_any_of(","));
    if (strings.size() != total_number)
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct csv file. "
                "Total number elements in line must be equal "
             << strings.size();
      throw std::runtime_error(stream.str());
    }

    std::vector<std::string> cat_features;
    cat_features.reserve(number_cat_features);
    for (std::size_t j = 1; j <= number_cat_features; j += 1)
    {
      cat_features.emplace_back(
        std::move(strings[j]));
    }
    data.cat_features_list.emplace_back(
      std::move(cat_features));

    std::vector<float> float_features;
    float_features.reserve(number_float_features);
    for (std::size_t j = 1; j <= number_float_features; j += 1)
    {
      float_features.emplace_back(
        boost::lexical_cast<float>(
          strings[number_cat_features + j]));
    }
    data.float_features_list.emplace_back(
      std::move(float_features));

    if (i % 1000 == 0)
    {
      std::ostringstream stream;
      stream << "Process "
             << (i * 100) / number_line_
             << "%";
      logger_->info(stream.str());
    }

    i += 1;
  }

  if (i < number_line_ + 1)
  {
    std::ostringstream stream;
    stream << FNS
           << "Number line in csv file="
           << csv_path_
           << " is less then requirement="
           << number_line_;
    logger_->error(stream.str());
  }
}