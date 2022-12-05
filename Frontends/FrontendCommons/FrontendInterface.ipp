
namespace FrontendCommons
{
  inline
  const std::string&
  FrontendInterface::Configuration::path() const
  {
    return config_path_;
  }

  inline
  const FrontendInterface::Configuration::FeConfig&
  FrontendInterface::Configuration::get() const
    /*throw(InvalidConfiguration)*/
  {
    if (config_)
    {
      return *config_;
    }
    Stream::Error err;
    err << "Configuration file '" << config_path_ << "' did not" <<
      " initialize correctly";
    throw InvalidConfiguration(err);
  }

}

