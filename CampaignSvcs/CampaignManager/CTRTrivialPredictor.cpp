#include <fstream>
#include <String/StringManip.hpp>
#include <Generics/Hash.hpp>

#include "CTRTrivialPredictor.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
namespace CTR
{
  namespace
  {
    const String::SubString GENERIC_DOMAIN("?");
  }

  const TrivialPredictor::KeyHashAdapter TrivialPredictor::DEFAULT_KEY_(0, GENERIC_DOMAIN);

  // TrivialPredictor::KeyHashAdapter
  TrivialPredictor::KeyHashAdapter::KeyHashAdapter(
    unsigned long tag_id_val, const String::SubString& domain_val)
    : tag_id(tag_id_val),
      domain(domain_val.str())
  {
    Generics::Murmur64Hash hash(hash_);
    Generics::hash_add(hash, tag_id);
    Generics::hash_add(hash, domain);
  }

  bool
  TrivialPredictor::KeyHashAdapter::operator==(const KeyHashAdapter& right) const
  {
    return tag_id == right.tag_id && domain == right.domain;
  }

  size_t
  TrivialPredictor::KeyHashAdapter::hash() const noexcept
  {
    return hash_;
  }

  // TrivialPredictor
  TrivialPredictor::TrivialPredictor(CTRMap ctr_map) noexcept
  {
    ctr_map_.swap(ctr_map);
  }

  TrivialPredictor::~TrivialPredictor() noexcept
  {}

  TrivialPredictor_var
  TrivialPredictor::load(std::istream& istr)
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

    return TrivialPredictor_var(new TrivialPredictor(std::move(ctr_map)));
  }

  RevenueDecimal
  TrivialPredictor::predict(unsigned long tag_id, const String::SubString& domain) const
  {
    auto it = ctr_map_.find(KeyHashAdapter(tag_id, domain));

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
}
}
}

