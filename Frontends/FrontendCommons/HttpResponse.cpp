// THIS
#include <Frontends/FrontendCommons/FCGI.hpp>

namespace FrontendCommons
{

OutputStream::OutputStream(HttpResponse* owner) noexcept
  : owner_(owner)
{
}

Stream::BinaryOutputStream& OutputStream::write(
  const char_type* s,
  streamsize n)
{
  ssize_t res = owner_->write(String::SubString(s, n));

  if (res < static_cast<ssize_t>(n) && n > 0)
  {
    setstate(std::ios_base::badbit | std::ios_base::failbit);
  }
  else if (n > 0)
  {
    setstate(std::ios_base::eofbit);
  }

  return *this;
}

} // namespace FrontendCommons
