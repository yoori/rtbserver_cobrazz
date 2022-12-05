
#include "BaiduResponseChecker.hpp"

namespace AutoTest
{
  
  namespace ProtoBuf
  {
    // Expected
    
    BaseTraits<BaiduResponseChecker>::Expected::Expected()
    { }

    BaseTraits<BaiduResponseChecker>::Expected::Expected(
      BaiduResponseChecker& checker,
      const Expected& other) :
      creative_id_(
        Utils().add_checker(
          checker, "creative_id",
          other.creative_id_, &Ad::creative_id)),
      target_url_(
        Utils().add_seq_checker(
          checker, "target_url",
          other.target_url_, &Ad::target_url)),
      advertiser_id_(
        Utils().add_checker(
          checker, "advertiser_id",
          other.advertiser_id_, &Ad::advertiser_id)),
      category_(
        Utils().add_checker(
          checker, "category",
          other.category_, &Ad::category)),
      type_(
        Utils().add_checker(
          checker, "type",
          other.type_, &Ad::type)),
      category_exist_(
        Utils().add_exist_checker(
          checker, "category",
          other.category_exist_, &Ad::has_category)),
      type_exist_(
        Utils().add_exist_checker(
          checker, "type",
          other.type_exist_, &Ad::has_type))
    { }
    
    BaseTraits<BaiduResponseChecker>::Expected&
    BaseTraits<BaiduResponseChecker>::Expected::creative_id(
      unsigned long val)
    {
      creative_id_ = val;
      
      return *this;
    }
    
    BaseTraits<BaiduResponseChecker>::Expected&
    BaseTraits<BaiduResponseChecker>::Expected::target_url(
      const std::string& val)
    {
      target_url_->push_back(val);
      target_url_.is_set(true);
    
      return *this;
    }
    
    BaseTraits<BaiduResponseChecker>::Expected&
    BaseTraits<BaiduResponseChecker>::Expected::advertiser_id(
      unsigned long val)
    {
      advertiser_id_ = val;
      
      return *this;
    }

    BaseTraits<BaiduResponseChecker>::Expected&
    BaseTraits<BaiduResponseChecker>::Expected::category(
      int val)
    {
      category_ = val;
      
      return *this;
    }

    BaseTraits<BaiduResponseChecker>::Expected&
    BaseTraits<BaiduResponseChecker>::Expected::category_exist(
      bool exist)
    {
      category_exist_ = exist;
      
      return *this;
    }

    
    BaseTraits<BaiduResponseChecker>::Expected&
    BaseTraits<BaiduResponseChecker>::Expected::type(
      int val)
    {
      type_ = val;
      
      return *this;
    }

    BaseTraits<BaiduResponseChecker>::Expected&
    BaseTraits<BaiduResponseChecker>::Expected::type_exist(
      bool exist)
    {
      type_exist_ = exist;
      
      return *this;
    }
  }
   
  // BaiduResponseChecker
  BaiduResponseChecker::BaiduResponseChecker(
    const AdClient& client,
    const Expected& expected,
    size_t creative_num) :
    Base(client, expected, creative_num)
  { }

  BaiduResponseChecker::~BaiduResponseChecker() noexcept
  { }

  const BaiduResponseChecker::Ad&
  BaiduResponseChecker::ad() const
  {
    return response_.ad(creative_num_);
  }

  BaiduResponseChecker::size_type
  BaiduResponseChecker::ad_size() const
  {
    return response_.ad_size();
  }
}

