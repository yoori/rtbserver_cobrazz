#include <arpa/inet.h>

#include <ProfilingCommons/PlainStorage3/FileReader.hpp>

#include "FTRLCTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  FTRLCTREvaluator::FTRLCTREvaluator(const String::SubString& file)
  {
    static const char* FUN = "FTRLCTREvaluator::FTRLCTREvaluator()";

    try
    {
      AdServer::ProfilingCommons::FileReader reader(
        file.str().c_str(),
        1024 * 1024);

      if(reader.file_size() % sizeof(float) != 0)
      {
        Stream::Error ostr;
        ostr << FUN << ": incorrect file size = " << reader.file_size();
        throw Exception(ostr);
      }

      uint32_t loaded_segment;
      while(reader.read(&loaded_segment, sizeof(loaded_segment)))
      {
        loaded_segment = htonl(loaded_segment);
        float weight;
        memcpy(&weight, &loaded_segment, sizeof(weight));
        feature_weights_.emplace_back(weight);
      }
    }
    catch(const AdServer::ProfilingCommons::FileReader::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught FileReader::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  RevenueDecimal
  FTRLCTREvaluator::get_ctr(
    const ModelTraits& /*model*/,
    const CampaignSelectParams* /*request_params*/,
    const Creative* /*creative*/,
    const HashArray* request_hashes,
    const HashArray* auction_hashes,
    const HashArray* candidate_hashes,
    const HashArray* opt_hashes) const
  {
    float weight = 0;

    const HashArray* HASH_SETS[] = {
      request_hashes,
      auction_hashes,
      candidate_hashes,
      opt_hashes
    };

    for (unsigned int i = 0; i < sizeof(HASH_SETS) / sizeof(HASH_SETS[0]); ++i)
    {
      if (HASH_SETS[i])
      {
        weight += eval_weight_(*HASH_SETS[i]);
      }
    }

    return Generics::convert_float<RevenueDecimal>(1. / (1. + ::expf(-weight)));
  }

  float
  FTRLCTREvaluator::eval_weight_(const HashArray& auction_hashes) const
  {
    float res_weight = 0;

    for (const auto& [hash_index, _]: auction_hashes)
    {
      auto index = hash_index % feature_weights_.size();
      res_weight += feature_weights_[index - 1];
    }

    return res_weight;
  }
}
