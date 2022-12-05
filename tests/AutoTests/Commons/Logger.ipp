
namespace AutoTest
{
  inline
  unsigned long
  Logger::log_level()
    noexcept
  {
    return Base::log_level();
  }

  inline
  void
  Logger::log_level(unsigned long value)
    noexcept
  {
    Base::log_level(value);
  }

  inline
  const std::string&
  Logger::log_name() const
    noexcept
  {
    return log_name_;
  }

  inline
  bool
  Logger::empty() const noexcept
  {
    return empty_;
  }

  template <typename Text>
  bool
  Logger::log(Text text, unsigned long severity)
    noexcept
  {
    return Base::log(String::SubString(text), severity, log_name_.c_str());
  }

}
