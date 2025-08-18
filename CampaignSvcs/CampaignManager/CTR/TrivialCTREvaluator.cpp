#include <fstream>

#include "TrivialCTREvaluator.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  namespace
  {
    const String::SubString GENERIC_DOMAIN("?");
  }

  const TrivialCTREvaluator::KeyHashAdapter TrivialCTREvaluator::DEFAULT_KEY_(0, GENERIC_DOMAIN);

  // TrivialCTREvaluator::KeyHashAdapter
  TrivialCTREvaluator::KeyHashAdapter::KeyHashAdapter(
    unsigned long tag_id_val, const String::SubString& domain_val)
    : tag_id(tag_id_val),
      domain(domain_val.str())
  {
    Generics::Murmur64Hash hash(hash_);
    Generics::hash_add(hash, tag_id);
    Generics::hash_add(hash, domain);
  }

  bool
  TrivialCTREvaluator::KeyHashAdapter::operator==(const KeyHashAdapter& right) const
  {
    return tag_id == right.tag_id && domain == right.domain;
  }

  size_t
  TrivialCTREvaluator::KeyHashAdapter::hash() const throw ()
  {
    return hash_;
  }

  // TrivialCTREvaluator
  TrivialCTREvaluator::TrivialCTREvaluator(const String::SubString& file)
  {
    std::ifstream istr(file.str().c_str());
    load_(istr);
  }

  TrivialCTREvaluator::~TrivialCTREvaluator() noexcept
  {}

  RevenueDecimal
  TrivialCTREvaluator::get_ctr(
    const ModelTraits& /*model*/,
    const CampaignSelectParams* request_params,
    const Creative* /*creative*/,
    const HashArray* /*request_hashes*/,
    const HashArray* /*auction_hashes*/,
    const HashArray* /*candidate_hashes*/,
    const HashArray* /*opt_hashes*/) const
  {
    const unsigned long tag_id = request_params->tag ? request_params->tag->tag_id : 0;

    auto it = ctr_map_.find(KeyHashAdapter(tag_id, request_params->referer_hostname));

    if(it != ctr_map_.end())
    {
      return it->second;
    }

    it = ctr_map_.find(KeyHashAdapter(tag_id, GENERIC_DOMAIN));
    if(it != ctr_map_.end())
    {
      return it->second;
    }

    it = ctr_map_.find(DEFAULT_KEY_);
    if(it != ctr_map_.end())
    {
      return it->second;
    }

    return RevenueDecimal::ZERO;
  }

  void
  TrivialCTREvaluator::load_(std::istream& istr)
  {
    const String::SubString EMPTY_DOMAIN("-");
    CTRMap ctr_map;

    // load
    while(!istr.eof())
    {
      std::string line;
      std::getline(istr, line);

      if(!line.empty())
      {
        String::StringManip::Splitter<
          String::AsciiStringManip::SepComma> tokenizer(line);

        String::SubString tag_id_str;
        String::SubString domain;
        String::SubString ctr_str;
        unsigned long tag_id;
        RevenueDecimal ctr;

        if(!tokenizer.get_token(tag_id_str) ||
          !tokenizer.get_token(domain) ||
          !tokenizer.get_token(ctr_str) ||
          !String::StringManip::str_to_int(tag_id_str, tag_id))
        {
          Stream::Error ostr;
          ostr << "Invalid line in trivial ctr config: line ='" << line << "'";
          throw InvalidConfig(ostr);
        }

        try
        {
          ctr = RevenueDecimal(ctr_str);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "Invalid CTR in trivial ctr config: line ='" << line << "'";
          throw InvalidConfig(ostr);
        }

        ctr_map.emplace(
          KeyHashAdapter(tag_id, domain == EMPTY_DOMAIN ? String::SubString() : domain),
          ctr);
      }
    }

    ctr_map_.swap(ctr_map);
  }
}
