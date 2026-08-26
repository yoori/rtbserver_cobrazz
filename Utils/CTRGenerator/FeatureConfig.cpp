#include "FeatureConfig.hpp"

#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <string>

#include <Commons/FastJsonParser.hpp>

namespace AdServer::CampaignSvcs
{
  namespace
  {
    using FastJsonParser = Commons::FastJsonParser<>;
    using ValueProcessor = FastJsonParser::ValueProcessor;

    struct ParseState
    {
      CTRGeneratorFeatureConfig config{};
      bool dimension_present = false;
      bool features_array_started = false;
    };

    class DimensionProcessor final: public ValueProcessor
    {
    public:
      void
      process_integer(int64_t value, std::string_view, void* context) const override
      {
        if (value <= 0 ||
          static_cast<uint64_t>(value) >= std::numeric_limits<unsigned long>::digits)
        {
          throw CTRGeneratorFeatureConfig::Exception("Invalid 'features_dimension'");
        }

        auto& state = *static_cast<ParseState*>(context);
        state.config.features_dimension = value;
        state.dimension_present = true;
      }
    };

    class FeaturesProcessor final:
      public ValueProcessor,
      protected CTR::FeatureNameResolver
    {
    public:
      void
      object_started(std::string_view, void*) const override
      {
        throw CTRGeneratorFeatureConfig::Exception("Feature group must be an array");
      }

      void
      array_started(std::string_view, void* context) const override
      {
        auto& state = *static_cast<ParseState*>(context);
        if (!state.features_array_started)
        {
          state.features_array_started = true;
        }
        else
        {
          state.config.features.emplace_back();
        }
      }

      void
      process_string(std::string_view value, std::string_view, void* context) const override
      {
        auto& state = *static_cast<ParseState*>(context);
        if (state.config.features.empty() || value.empty())
        {
          throw CTRGeneratorFeatureConfig::Exception("Invalid basic feature name");
        }

        CTR::BasicFeature basic_feature;
        if (!basic_feature_by_name(basic_feature, value))
        {
          Stream::Error error;
          error << "Invalid basic feature name: '" << value << "'";
          throw CTRGeneratorFeatureConfig::Exception(error);
        }

        auto& feature = state.config.features.back();
        if (!feature.name.empty())
        {
          feature.name += ',';
        }
        feature.name.append(value);
        feature.basic_features.insert(basic_feature);
      }

      void
      process_string(std::string&& value, std::string_view path, void* context) const override
      {
        process_string(std::string_view(value), path, context);
      }
    };
  }

  CTRGeneratorFeatureConfig
  load_ctr_generator_feature_config(std::string_view file_name)
  {
    const std::string file_name_string(file_name);
    std::ifstream input(file_name_string);
    if (!input)
    {
      Stream::Error error;
      error << "Can't open feature config file '" << file_name << "'";
      throw CTRGeneratorFeatureConfig::Exception(error);
    }

    const std::string content{
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};

    FastJsonParser::ProcessorSet processors;
    processors.add_processor("features_dimension", std::make_shared<DimensionProcessor>());
    processors.add_processor("features", std::make_shared<FeaturesProcessor>());
    const FastJsonParser parser(std::move(processors));

    ParseState state;
    try
    {
      parser.parse(content, &state);
    }
    catch(const FastJsonParser::Exception& ex)
    {
      Stream::Error error;
      error << "Can't parse feature config file '" << file_name << "': " << ex.what();
      throw CTRGeneratorFeatureConfig::Exception(error);
    }

    if (!state.dimension_present)
    {
      Stream::Error error;
      error << "Feature config file '" << file_name << "' contains invalid 'features_dimension'";
      throw CTRGeneratorFeatureConfig::Exception(error);
    }

    if (!state.features_array_started || state.config.features.empty())
    {
      Stream::Error error;
      error << "Feature config file '" << file_name <<
        "' must contain a non-empty 'features' array";
      throw CTRGeneratorFeatureConfig::Exception(error);
    }

    for (const auto& feature : state.config.features)
    {
      if (feature.basic_features.empty())
      {
        Stream::Error error;
        error << "Feature config file '" << file_name << "' contains an empty feature group";
        throw CTRGeneratorFeatureConfig::Exception(error);
      }
    }

    return std::move(state.config);
  }
}
