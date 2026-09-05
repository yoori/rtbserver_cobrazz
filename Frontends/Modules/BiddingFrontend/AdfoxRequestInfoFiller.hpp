#pragma once

#include <memory>
#include <string_view>

#include <Commons/FastJsonParser.hpp>
#include <Frontends/FrontendCommons/HttpRequest.hpp>
#include <Generics/MonoAllocator.hpp>

namespace AdServer::Bidding
{
  struct RequestInfo;
  class RequestInfoFiller;

  struct AdfoxRequestContext
  {
    enum class CodeType
    {
      Banner,
      Combo,
      InPage,
      PreRoll,
      MidRoll,
      PostRoll
    };

    struct Size
    {
      unsigned long width = 0;
      unsigned long height = 0;
    };

    struct Place
    {
      explicit Place(Generics::MonoAllocatorArena* arena)
        : sizes(arena)
      {}

      Place(Place&&) noexcept = default;
      Place& operator=(Place&&) noexcept = default;

      std::string_view id;
      std::string_view placement_id;
      std::string_view code_type_name;
      std::string_view target_ref;
      CodeType code_type = CodeType::Banner;
      Generics::MonoVector<Size> sizes;
    };

    explicit AdfoxRequestContext(Generics::MonoAllocatorArena* arena)
      : arena_(arena),
        places(arena)
    {}

    AdfoxRequestContext(const AdfoxRequestContext&) = delete;
    AdfoxRequestContext& operator=(const AdfoxRequestContext&) = delete;
    AdfoxRequestContext(AdfoxRequestContext&&) noexcept = default;
    AdfoxRequestContext& operator=(AdfoxRequestContext&&) = delete;

    void
    clear() noexcept;

    Generics::MonoAllocatorArena* arena_;
    Generics::MonoVector<Place> places;
    std::string_view currency;
    unsigned long window_width = 0;
    unsigned long window_height = 0;
    bool places_present = false;
    bool settings_present = false;
    bool window_size_present = false;
  };

  class AdfoxRequestInfoFiller
  {
  public:
    explicit AdfoxRequestInfoFiller(RequestInfoFiller* request_info_filler);

    void
    fill(
      RequestInfo& request_info,
      AdfoxRequestContext& context,
      const FCGI::HttpRequest& request,
      std::string&& body) const;

  private:
    using FastJsonParser = AdServer::Commons::FastJsonParser<Generics::MonoString>;

    RequestInfoFiller* request_info_filler_;
    std::unique_ptr<FastJsonParser> parser_;
  };
}
