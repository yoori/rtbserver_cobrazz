namespace AdServer
{
  namespace CampaignSvcs
  {
    // CalcParamFillerIntImpl
    template<typename IntType>
    CalculateParamsFiller::CalcParamFillerIntImpl<IntType>::CalcParamFillerIntImpl(
      IntType CTRGenerator::CalculateParams::* field)
        : field_(field)
    { }

    template<typename IntType>
    void
    CalculateParamsFiller::CalcParamFillerIntImpl<IntType>::set_value(
      CTRGenerator::CalculateParams& calc_params,
      const String::SubString& str)
    {
      if(!String::StringManip::str_to_int(str, calc_params.*field_))
      {
        Stream::Error ostr;
        ostr << "invalid feature value '" << str << "'";
        throw CTRGenerator::InvalidConfig(ostr);
      }
    }
  }
}
