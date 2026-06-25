#include <cassert>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <string_view>
#include <type_traits>

#include <Commons/DecimalUtils.hpp>
#include <String/StringManip.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer::Bidding
{
  namespace
  {
    using FastJsonParser = AdServer::Commons::FastJsonParser;
    using ValueProcessor = FastJsonParser::ValueProcessor;
    using RequestParams = AdServer::Bidding::CampaignManager::RequestParams;
    using AdSlotContext = JsonAdSlotProcessingContext;
    using Banner = AdSlotContext::Banner;
    using BannerFormat = AdSlotContext::BannerFormat;
    using Native = AdSlotContext::Native;
    using NativeAsset = Native::Asset;

    struct FastOpenRtbState
    {
      RequestParams* request_params = nullptr;
      JsonProcessingContext* context = nullptr;
      AdSlotContext* ad_slot = nullptr;
      Banner* banner = nullptr;
      BannerFormat* banner_format = nullptr;
      AdSlotContext::Deal* deal = nullptr;
      AdSlotContext::Metric* metric = nullptr;
      JsonProcessingContext::Segment* segment = nullptr;
      JsonProcessingContext::UserEid* user_eid = nullptr;
      JsonProcessingContext::UserEidUid* user_eid_uid = nullptr;
      Native* native = nullptr;
      Native::Data* native_data = nullptr;
      Native::Image* native_image = nullptr;
      Native::Video* native_video = nullptr;
      NativeAsset* native_asset = nullptr;
      long pending_native_asset_id = 0;
      bool pending_native_asset_required = false;
      bool pending_native_asset_id_present = false;
      bool pending_native_asset_required_present = false;
      bool matching_ad_processed = false;
    };

    template<typename Target>
    void
    assign_string(Target& target, std::string_view value)
    {
      target.assign(value.data(), value.size());
    }

    std::string
    integer_to_string(int64_t value)
    {
      char buf[32];
      const auto result = std::to_chars(buf, buf + sizeof(buf), value);
      return std::string(buf, result.ptr);
    }

    std::string
    float_to_string(double value)
    {
      char buf[64];
      const int size = std::snprintf(buf, sizeof(buf), "%.17g", value);
      return size > 0 ? std::string(buf, static_cast<std::size_t>(size)) :
        std::string();
    }

    template<typename NumberType>
    bool
    parse_integer(std::string_view value, NumberType& result)
    {
      const auto parsed = std::from_chars(
        value.data(),
        value.data() + value.size(),
        result);
      return parsed.ec == std::errc() && parsed.ptr == value.data() + value.size();
    }

    template<typename DecimalType>
    DecimalType
    parse_decimal(
      std::string_view value,
      Generics::DecimalMulRemainder round_type,
      const DecimalType& invalid_value = DecimalType())
    {
      try
      {
        return AdServer::Commons::extract_decimal<DecimalType>(
          String::SubString(value.data(), value.size()),
          round_type);
      }
      catch(const typename DecimalType::Exception&)
      {
        return invalid_value;
      }
    }

    template<typename NumberType>
    bool
    convert_integer(int64_t value, NumberType& result)
    {
      if constexpr(std::is_unsigned_v<NumberType>)
      {
        if(value < 0 ||
          static_cast<uint64_t>(value) >
            static_cast<uint64_t>(std::numeric_limits<NumberType>::max()))
        {
          return false;
        }
      }
      else
      {
        if(value < static_cast<int64_t>(std::numeric_limits<NumberType>::min()) ||
          value > static_cast<int64_t>(std::numeric_limits<NumberType>::max()))
        {
          return false;
        }
      }

      result = static_cast<NumberType>(value);
      return true;
    }

    template<typename NumberType>
    bool
    convert_integer(double value, NumberType& result)
    {
      if(!std::isfinite(value) || std::trunc(value) != value)
      {
        return false;
      }

      const auto min_value = static_cast<long double>(
        std::numeric_limits<NumberType>::lowest());
      const auto max_value = static_cast<long double>(
        std::numeric_limits<NumberType>::max());
      const auto check_value = static_cast<long double>(value);
      if(check_value < min_value || check_value > max_value)
      {
        return false;
      }

      result = static_cast<NumberType>(value);
      return true;
    }

    void
    fill_url(std::string_view value, HTTP::HTTPAddress& target)
    {
      String::SubString value_view(value.data(), value.size());
      JsonContextURLParamProcessor<JsonProcessingContext>::fill_url(
        value_view,
        target);
    }

    void
    set_state_number(
      AdSlotContext::ULValueStateHolder& target,
      std::string_view value)
    {
      unsigned long result = 0;
      if(parse_integer(value, result))
      {
        target = result;
      }
      else
      {
        target.set_state(AdSlotContext::ULValueStateHolder::S_FAIL);
      }
    }

    template<typename HolderType>
    void
    set_state_number(HolderType& target, int64_t value)
    {
      typename HolderType::ValueType result = 0;
      if(convert_integer(value, result))
      {
        target = result;
      }
      else
      {
        target.set_state(HolderType::S_FAIL);
      }
    }

    template<typename HolderType>
    void
    set_state_number(HolderType& target, double value)
    {
      typename HolderType::ValueType result = 0;
      if(convert_integer(value, result))
      {
        target = result;
      }
      else
      {
        target.set_state(HolderType::S_FAIL);
      }
    }

    void
    set_state_number(
      AdSlotContext::LValueStateHolder& target,
      std::string_view value)
    {
      long result = 0;
      if(parse_integer(value, result))
      {
        target = result;
      }
      else
      {
        target.set_state(AdSlotContext::LValueStateHolder::S_FAIL);
      }
    }

    template<typename SetType>
    inline void
    insert_string(SetType& target, std::string_view value)
    {
      typename SetType::value_type item(target.get_allocator().resource());
      assign_string(item, value);
      target.emplace(std::move(item));
    }

    template<typename VectorType>
    inline void
    append_string(VectorType& target, std::string_view value)
    {
      typename VectorType::value_type item(target.get_allocator().resource());
      assign_string(item, value);
      target.emplace_back(std::move(item));
    }

    template<typename HolderType>
    inline void
    insert_number(HolderType& holder, std::string_view value)
    {
      typename HolderType::ValueType::value_type result = 0;
      if(parse_integer(value, result))
      {
        if(!holder.present())
        {
          holder = typename HolderType::ValueType();
        }
        holder->insert(holder->end(), result);
      }
      else
      {
        holder.set_state(HolderType::S_FAIL);
      }
    }

    template<typename HolderType>
    inline void
    insert_number(HolderType& holder, int64_t value)
    {
      typename HolderType::ValueType::value_type result = 0;
      if(convert_integer(value, result))
      {
        if(!holder.present())
        {
          holder = typename HolderType::ValueType();
        }
        holder->insert(holder->end(), result);
      }
      else
      {
        holder.set_state(HolderType::S_FAIL);
      }
    }

    template<typename HolderType>
    inline void
    insert_number(HolderType& holder, double value)
    {
      typename HolderType::ValueType::value_type result = 0;
      if(convert_integer(value, result))
      {
        if(!holder.present())
        {
          holder = typename HolderType::ValueType();
        }
        holder->insert(holder->end(), result);
      }
      else
      {
        holder.set_state(HolderType::S_FAIL);
      }
    }

    inline FastOpenRtbState&
    state_from_context(void* context)
    {
      return *static_cast<FastOpenRtbState*>(context);
    }

    class IgnoringValueProcessor: public ValueProcessor
    {
    public:
      void
      object_started(std::string_view, void*) const override
      {}

      void
      array_started(std::string_view, void*) const override
      {}

      void
      process_integer(int64_t, std::string_view, void*) const override
      {}

      void
      process_float(double, std::string_view, void*) const override
      {}

      void
      process_string(std::string_view, std::string_view, void*) const override
      {}

      void
      process_string(std::string&&, std::string_view, void*) const override
      {}

      void
      process_bool(bool, std::string_view, void*) const override
      {}

      void
      process_null(std::string_view, void*) const override
      {}
    };

    template<typename ProcessorType>
  void
  add_processor(
    FastJsonParser& parser,
    std::string_view path,
    std::shared_ptr<ProcessorType> processor,
    bool as_string = false)
  {
    parser.add_processor(path, std::move(processor), as_string);
  }

    template<typename Context, typename Field>
    class StringFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      StringFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        Field Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        process_(value, context);
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        process_(value, context);
      }

    private:
      void
      process_(std::string_view value, void* context) const
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          assign_string(target->*field_, value);
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      Field Context::* field_;
    };

    template<typename Context, typename Field>
    class UrlFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      UrlFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        Field Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          fill_url(value, target->*field_);
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      Field Context::* field_;
    };

    template<typename Context, typename NumberType>
    class IntegerFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      IntegerFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        NumberType Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          target->*field_ = static_cast<NumberType>(value);
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      NumberType Context::* field_;
    };

    template<typename Context>
    class OptionalLongFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      OptionalLongFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        AdServer::Commons::Optional<long> Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          target->*field_ = static_cast<long>(value);
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      AdServer::Commons::Optional<long> Context::* field_;
    };

    template<typename Context>
    class BoolFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      BoolFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        bool Context::* field)
        : get_context_(get_context),
          field_(field)
      {}

      void
      process_bool(bool value, std::string_view, void* context) const override
      {
        set_(value, context);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        set_(value != 0, context);
      }

    private:
      void
      set_(bool value, void* context) const
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          target->*field_ = value;
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      bool Context::* field_;
    };

    template<typename Context, typename DecimalType>
    class DecimalFieldProcessor final: public IgnoringValueProcessor
    {
    public:
      DecimalFieldProcessor(
        Context* (*get_context)(FastOpenRtbState&),
        DecimalType Context::* field,
        Generics::DecimalMulRemainder round_type,
        const DecimalType& invalid_value)
        : get_context_(get_context),
          field_(field),
          round_type_(round_type),
          invalid_value_(invalid_value)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        parse_(value, context);
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        parse_(value, context);
      }

      void
      process_number(
        std::string_view,
        bool,
        std::string_view,
        void*) const override
      {
        assert(false && "DecimalFieldProcessor must be registered as string");
      }

      void
      process_integer(int64_t, std::string_view, void*) const override
      {
        assert(false && "DecimalFieldProcessor must be registered as string");
      }

      void
      process_float(double, std::string_view, void*) const override
      {
        assert(false && "DecimalFieldProcessor must be registered as string");
      }

    private:
      void
      parse_(std::string_view value, void* context) const
      {
        if(Context* target = get_context_(state_from_context(context)))
        {
          target->*field_ = parse_decimal(
            value,
            round_type_,
            invalid_value_);
        }
      }

    private:
      Context* (*get_context_)(FastOpenRtbState&);
      DecimalType Context::* field_;
      Generics::DecimalMulRemainder round_type_;
      DecimalType invalid_value_;
    };

    std::string
    make_path(std::string_view prefix, std::string_view suffix)
    {
      if(prefix.empty())
      {
        return std::string(suffix);
      }

      std::string path;
      path.reserve(prefix.size() + 1 + suffix.size());
      path.append(prefix);
      path += '.';
      path.append(suffix);
      return path;
    }

    template<typename Context, typename Field>
    void
    add_string(
      FastJsonParser& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      Field Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<StringFieldProcessor<Context, Field>>(
          get_context,
          field));
    }

    template<typename Context, typename Field>
    void
    add_url(
      FastJsonParser& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      Field Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<UrlFieldProcessor<Context, Field>>(get_context, field));
    }

    template<typename Context, typename NumberType>
    void
    add_integer(
      FastJsonParser& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      NumberType Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<IntegerFieldProcessor<Context, NumberType>>(
          get_context,
          field));
    }

    template<typename Context>
    void
    add_optional_long(
      FastJsonParser& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      AdServer::Commons::Optional<long> Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<OptionalLongFieldProcessor<Context>>(get_context, field));
    }

    template<typename Context>
    void
    add_bool(
      FastJsonParser& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      bool Context::* field)
    {
      add_processor(
        parser,
        path,
        std::make_shared<BoolFieldProcessor<Context>>(get_context, field));
    }

    template<typename Context, typename DecimalType>
    void
    add_decimal(
      FastJsonParser& parser,
      std::string_view path,
      Context* (*get_context)(FastOpenRtbState&),
      DecimalType Context::* field,
      Generics::DecimalMulRemainder round_type,
      const DecimalType& invalid_value = DecimalType())
    {
      add_processor(
        parser,
        path,
        std::make_shared<DecimalFieldProcessor<Context, DecimalType>>(
          get_context,
          field,
          round_type,
          invalid_value),
        true);
    }

    JsonProcessingContext*
    root_context(FastOpenRtbState& state)
    {
      return state.context;
    }

    AdSlotContext*
    ad_slot_context(FastOpenRtbState& state)
    {
      return state.ad_slot;
    }

    Banner*
    banner_context(FastOpenRtbState& state)
    {
      return state.banner;
    }

    BannerFormat*
    banner_format_context(FastOpenRtbState& state)
    {
      return state.banner_format;
    }

    AdSlotContext::Deal*
    deal_context(FastOpenRtbState& state)
    {
      return state.deal;
    }

    AdSlotContext::Metric*
    metric_context(FastOpenRtbState& state)
    {
      return state.metric;
    }

    JsonProcessingContext::Segment*
    segment_context(FastOpenRtbState& state)
    {
      return state.segment;
    }

    JsonProcessingContext::UserEid*
    user_eid_context(FastOpenRtbState& state)
    {
      return state.user_eid;
    }

    JsonProcessingContext::UserEidUid*
    user_eid_uid_context(FastOpenRtbState& state)
    {
      return state.user_eid_uid;
    }

    Native*
    native_context(FastOpenRtbState& state)
    {
      return state.native;
    }

    Native::Data*
    native_data_context(FastOpenRtbState& state)
    {
      return state.native_data;
    }

    Native::Image*
    native_image_context(FastOpenRtbState& state)
    {
      return state.native_image;
    }

    void
    start_ad_slot(FastOpenRtbState& state)
    {
      state.context->ad_slots.emplace_back(state.context->resource());
      state.ad_slot = &state.context->ad_slots.back();
      state.banner = nullptr;
      state.banner_format = nullptr;
      state.deal = nullptr;
      state.metric = nullptr;
      state.native = nullptr;
    }

    void
    start_banner(FastOpenRtbState& state)
    {
      if(!state.ad_slot)
      {
        return;
      }

      state.ad_slot->banners.emplace_back(state.ad_slot->resource());
      state.banner = &state.ad_slot->banners.back();
      state.banner_format = nullptr;
      state.matching_ad_processed = false;
    }

    BannerFormat*
    ensure_banner_format(FastOpenRtbState& state)
    {
      if(!state.banner)
      {
        return nullptr;
      }

      if(!state.banner_format)
      {
        state.banner->formats.emplace_back(state.banner->resource());
        state.banner_format = &state.banner->formats.back();
      }

      return state.banner_format;
    }

    void
    start_banner_format(FastOpenRtbState& state)
    {
      if(!state.banner)
      {
        return;
      }
      state.banner->formats.emplace_back(state.banner->resource());
      state.banner_format = &state.banner->formats.back();
    }

    void
    start_deal(FastOpenRtbState& state)
    {
      if(!state.ad_slot)
      {
        return;
      }
      state.ad_slot->deals.emplace_back(state.ad_slot->resource());
      state.deal = &state.ad_slot->deals.back();
    }

    void
    start_metric(FastOpenRtbState& state)
    {
      if(!state.ad_slot)
      {
        return;
      }
      state.ad_slot->metrics.emplace_back(state.ad_slot->resource());
      state.metric = &state.ad_slot->metrics.back();
    }

    void
    start_segment(FastOpenRtbState& state)
    {
      state.context->segments.emplace_back(state.context->resource());
      state.segment = &state.context->segments.back();
    }

    void
    start_user_eid(FastOpenRtbState& state)
    {
      state.context->user_eids.emplace_back(state.context->resource());
      state.user_eid = &state.context->user_eids.back();
      state.user_eid_uid = nullptr;
    }

    void
    start_user_eid_uid(FastOpenRtbState& state)
    {
      if(!state.user_eid)
      {
        return;
      }
      state.user_eid->uids.emplace_back(
        state.user_eid->uids.get_allocator().resource());
      state.user_eid_uid = &state.user_eid->uids.back();
    }

    void
    start_native(FastOpenRtbState& state)
    {
      if(!state.ad_slot)
      {
        return;
      }
      state.ad_slot->native = AdSlotContext::Native_var(
        new Native(state.ad_slot->resource()));
      state.native = state.ad_slot->native.in();
    }

    void
    start_native_asset(FastOpenRtbState& state)
    {
      state.native_data = nullptr;
      state.native_image = nullptr;
      state.native_video = nullptr;
      state.native_asset = nullptr;
      state.pending_native_asset_id = 0;
      state.pending_native_asset_required = false;
      state.pending_native_asset_id_present = false;
      state.pending_native_asset_required_present = false;
    }

    template<typename Asset>
    void
    apply_pending_native_asset(FastOpenRtbState& state, Asset& asset)
    {
      if(state.pending_native_asset_id_present)
      {
        asset.id = state.pending_native_asset_id;
      }

      if(state.pending_native_asset_required_present)
      {
        asset.required = state.pending_native_asset_required;
      }

      state.native_asset = &asset;
    }

    void
    start_native_title(FastOpenRtbState& state)
    {
      if(!state.native)
      {
        return;
      }
      state.native->data_assets.emplace_back(state.native->resource());
      state.native_data = &state.native->data_assets.back();
      state.native_data->data_type = Native::NDTE_TITLE;
      apply_pending_native_asset(state, *state.native_data);
    }

    void
    start_native_data(FastOpenRtbState& state)
    {
      if(!state.native)
      {
        return;
      }
      state.native->data_assets.emplace_back(state.native->resource());
      state.native_data = &state.native->data_assets.back();
      apply_pending_native_asset(state, *state.native_data);
    }

    void
    start_native_image(FastOpenRtbState& state)
    {
      if(!state.native)
      {
        return;
      }
      state.native->image_assets.emplace_back(state.native->resource());
      state.native_image = &state.native->image_assets.back();
      apply_pending_native_asset(state, *state.native_image);
    }

    void
    start_native_video(FastOpenRtbState& state)
    {
      if(!state.native)
      {
        return;
      }
      state.native->video_assets.emplace_back(state.native->resource());
      state.native_video = &state.native->video_assets.back();
      apply_pending_native_asset(state, *state.native_video);
    }

    struct EmptyStateHandler
    {
      void
      operator()(FastOpenRtbState&) const
      {}
    };

    template<typename ObjectHandler, typename ArrayHandler>
    class ObjectProcessor final: public IgnoringValueProcessor
    {
    public:
      ObjectProcessor(ObjectHandler object_handler, ArrayHandler array_handler)
        : object_handler_(std::move(object_handler)),
          array_handler_(std::move(array_handler))
      {}

      void
      object_started(std::string_view, void* context) const override
      {
        object_handler_(state_from_context(context));
      }

      void
      array_started(std::string_view, void* context) const override
      {
        array_handler_(state_from_context(context));
      }

    private:
      ObjectHandler object_handler_;
      ArrayHandler array_handler_;
    };

    template<typename Append>
    class StringArrayProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      StringArrayProcessor(Append append)
        : append_(std::move(append))
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        append_(state_from_context(context), value);
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        append_(state_from_context(context), value);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        append_(state_from_context(context), integer_to_string(value));
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        append_(state_from_context(context), float_to_string(value));
      }

    private:
      Append append_;
    };

    template<typename HolderGetter>
    class StateValueNumberProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      StateValueNumberProcessor(HolderGetter get_holder)
        : get_holder_(std::move(get_holder))
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          set_state_number(*holder, value);
        }
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          set_state_number(*holder, value);
        }
      }

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        process_string_(value, context);
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        process_string_(value, context);
      }

    private:
      void
      process_string_(std::string_view value, void* context) const
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          set_state_number(*holder, value);
        }
      }

    private:
      HolderGetter get_holder_;
    };

    template<typename HolderGetter>
    class StateNumberSetProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      StateNumberSetProcessor(HolderGetter get_holder)
        : get_holder_(std::move(get_holder))
      {}

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          insert_number(*holder, value);
        }
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          insert_number(*holder, value);
        }
      }

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        process_string_(value, context);
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        process_string_(value, context);
      }

    private:
      void
      process_string_(std::string_view value, void* context) const
      {
        if(auto* holder = get_holder_(state_from_context(context)))
        {
          insert_number(*holder, value);
        }
      }

    private:
      HolderGetter get_holder_;
    };

    template<typename Handler>
    class StringHandlerProcessor final: public IgnoringValueProcessor
    {
    public:
      explicit
      StringHandlerProcessor(Handler handler)
        : handler_(std::move(handler))
      {}

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        handler_(state_from_context(context), value);
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        handler_(state_from_context(context), value);
      }

    private:
      Handler handler_;
    };

    class MatchingAdProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      array_started(std::string_view, void* context) const override
      {
        state_from_context(context).matching_ad_processed = false;
      }

      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        process_(value, context);
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        process_(value, context);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        process_(integer_to_string(value), context);
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        process_(float_to_string(value), context);
      }

    private:
      void
      process_(std::string_view value, void* context) const
      {
        FastOpenRtbState& state = state_from_context(context);
        if(state.banner && !state.matching_ad_processed)
        {
          assign_string(state.banner->matching_ad, value);
          state.matching_ad_processed = true;
        }
      }
    };

    template<bool Width>
    class BannerSizeProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        process_(value, context);
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        process_(value, context);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        process_(integer_to_string(value), context);
      }

    private:
      void
      process_(std::string_view value, void* context) const
      {
        if(BannerFormat* format = ensure_banner_format(state_from_context(context)))
        {
          if constexpr(Width)
          {
            assign_string(format->width, value);
          }
          else
          {
            assign_string(format->height, value);
          }
        }
      }
    };

    class NativeAssetIdProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        FastOpenRtbState& state = state_from_context(context);
        if(state.native_asset)
        {
          state.native_asset->id = static_cast<long>(value);
        }
        else
        {
          state.pending_native_asset_id = static_cast<long>(value);
          state.pending_native_asset_id_present = true;
        }
      }
    };

    class NativeAssetRequiredProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_bool(bool value, std::string_view, void* context) const override
      {
        set_(value, context);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        set_(value != 0, context);
      }

    private:
      void
      set_(bool value, void* context) const
      {
        FastOpenRtbState& state = state_from_context(context);
        if(state.native_asset)
        {
          state.native_asset->required = value;
        }
        else
        {
          state.pending_native_asset_required = value;
          state.pending_native_asset_required_present = true;
        }
      }
    };

    class MetricValueProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      process_string(std::string_view value, std::string_view, void* context)
        const override
      {
        if(AdSlotContext::Metric* metric = state_from_context(context).metric)
        {
          assign_string(metric->value, value);
        }
      }

      void
      process_string(std::string&& value, std::string_view, void* context)
        const override
      {
        process_string(std::string_view(value), {}, context);
      }

      void
      process_integer(int64_t value, std::string_view, void* context)
        const override
      {
        process_number_(static_cast<double>(value), context);
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        process_number_(value, context);
      }

    private:
      void
      process_number_(double value, void* context) const
      {
        if(AdSlotContext::Metric* metric = state_from_context(context).metric)
        {
          char buf[64];
          const int size = std::snprintf(buf, sizeof(buf), "%.3f", value);
          assign_string(
            metric->value,
            std::string_view(buf, static_cast<std::size_t>(size)));
        }
      }
    };

    class VideoStartedProcessor final: public IgnoringValueProcessor
    {
    public:
      void
      object_started(std::string_view, void* context) const override
      {
        if(AdSlotContext* ad_slot = state_from_context(context).ad_slot)
        {
          ad_slot->video = true;
        }
      }
    };

    template<typename ObjectHandler, typename ArrayHandler = EmptyStateHandler>
    void
    add_object_processor(
      FastJsonParser& parser,
      std::string_view path,
      ObjectHandler object_handler,
      ArrayHandler array_handler = {})
    {
      add_processor(
        parser,
        path,
        std::make_shared<ObjectProcessor<ObjectHandler, ArrayHandler>>(
          std::move(object_handler),
          std::move(array_handler)));
    }

    template<typename Append>
    void
    add_string_array(
      FastJsonParser& parser,
      std::string_view path,
      Append append)
    {
      add_processor(
        parser,
        path,
        std::make_shared<StringArrayProcessor<Append>>(std::move(append)));
    }

    template<typename HolderGetter>
    void
    add_state_value_number(
      FastJsonParser& parser,
      std::string_view path,
      HolderGetter get_holder)
    {
      add_processor(
        parser,
        path,
        std::make_shared<StateValueNumberProcessor<HolderGetter>>(
          std::move(get_holder)));
    }

    template<typename HolderGetter>
    void
    add_state_number_set(
      FastJsonParser& parser,
      std::string_view path,
      HolderGetter get_holder)
    {
      add_processor(
        parser,
        path,
        std::make_shared<StateNumberSetProcessor<HolderGetter>>(
          std::move(get_holder)));
    }

    void
    init_banner_processors(FastJsonParser& parser, std::string_view base)
    {
      add_object_processor(parser, base, start_banner, [](FastOpenRtbState&) {});
      add_string(parser, std::string(base) + ".pos", banner_context, &Banner::pos);
      add_string_array(
        parser,
        std::string(base) + ".battr",
        [](FastOpenRtbState& state, std::string_view value)
        {
          if(state.banner)
          {
            append_string(state.banner->exclude_categories, value);
          }
        });
      add_string_array(
        parser,
        std::string(base) + ".ext.bcont",
        [](FastOpenRtbState& state, std::string_view value)
        {
          if(state.banner)
          {
            append_string(state.banner->exclude_categories, value);
          }
        });
      add_integer(
        parser,
        std::string(base) + ".ext.overlay.hpos",
        banner_context,
        &Banner::ext_hpos);
      add_processor(
        parser,
        std::string(base) + ".ext.matching_ad_id",
        std::make_shared<MatchingAdProcessor>());

      add_object_processor(parser, std::string(base) + ".format", start_banner_format);
      add_object_processor(
        parser,
        std::string(base) + ".format",
        start_banner_format,
        [](FastOpenRtbState&) {});

      add_processor(
        parser,
        std::string(base) + ".w",
        std::make_shared<BannerSizeProcessor<true>>());
      add_processor(
        parser,
        std::string(base) + ".h",
        std::make_shared<BannerSizeProcessor<false>>());

      add_string(parser, std::string(base) + ".format.w", banner_format_context, &BannerFormat::width);
      add_string(parser, std::string(base) + ".format.h", banner_format_context, &BannerFormat::height);
      add_string(parser, std::string(base) + ".format.ext.type", banner_format_context, &BannerFormat::ext_type);
      add_string(parser, std::string(base) + ".format.ext.fmt", banner_format_context, &BannerFormat::ext_format);
    }

    void
    init_native_payload_processors(FastJsonParser& parser, std::string_view prefix)
    {
      add_string(parser, make_path(prefix, "ver"), native_context, &Native::version);
      add_optional_long(
        parser,
        make_path(prefix, "plcmttype"),
        native_context,
        &Native::placement);
      add_object_processor(
        parser,
        make_path(prefix, "assets"),
        start_native_asset,
        [](FastOpenRtbState&) {});

      add_processor(
        parser,
        make_path(prefix, "assets.id"),
        std::make_shared<NativeAssetIdProcessor>());
      add_processor(
        parser,
        make_path(prefix, "assets.required"),
        std::make_shared<NativeAssetRequiredProcessor>());

      add_object_processor(
        parser,
        make_path(prefix, "assets.title"),
        start_native_title);
      add_integer(
        parser,
        make_path(prefix, "assets.title.len"),
        native_data_context,
        &Native::Data::len);

      add_object_processor(
        parser,
        make_path(prefix, "assets.data"),
        start_native_data);
      add_integer(
        parser,
        make_path(prefix, "assets.data.type"),
        native_data_context,
        &Native::Data::data_type);
      add_integer(
        parser,
        make_path(prefix, "assets.data.len"),
        native_data_context,
        &Native::Data::len);

      add_object_processor(
        parser,
        make_path(prefix, "assets.img"),
        start_native_image);
      add_integer(
        parser,
        make_path(prefix, "assets.img.type"),
        native_image_context,
        &Native::Image::image_type);
      add_integer(
        parser,
        make_path(prefix, "assets.img.w"),
        native_image_context,
        &Native::Image::width);
      add_integer(
        parser,
        make_path(prefix, "assets.img.wmin"),
        native_image_context,
        &Native::Image::width);
      add_integer(
        parser,
        make_path(prefix, "assets.img.h"),
        native_image_context,
        &Native::Image::height);
      add_integer(
        parser,
        make_path(prefix, "assets.img.hmin"),
        native_image_context,
        &Native::Image::height);
      add_string_array(
        parser,
        make_path(prefix, "assets.img.mimes"),
        [](FastOpenRtbState& state, std::string_view value)
        {
          if(state.native_image)
          {
            insert_string(state.native_image->mimes, value);
          }
        });

      add_object_processor(
        parser,
        make_path(prefix, "assets.video"),
        start_native_video);
      add_string_array(
        parser,
        make_path(prefix, "assets.video.mimes"),
        [](FastOpenRtbState& state, std::string_view value)
        {
          if(state.native_video)
          {
            insert_string(state.native_video->mimes, value);
          }
        });
      add_state_value_number(
        parser,
        make_path(prefix, "assets.video.minduration"),
        [](FastOpenRtbState& state)
          -> AdSlotContext::ULValueStateHolder*
        {
          return state.native_video ? &state.native_video->min_duration : nullptr;
        });
      add_state_value_number(
        parser,
        make_path(prefix, "assets.video.maxduration"),
        [](FastOpenRtbState& state)
          -> AdSlotContext::ULValueStateHolder*
        {
          return state.native_video ? &state.native_video->max_duration : nullptr;
        });
      add_state_number_set(
        parser,
        make_path(prefix, "assets.video.protocols"),
        [](FastOpenRtbState& state)
          -> AdSlotContext::ULSetStateHolder*
        {
          return state.native_video ? &state.native_video->protocols : nullptr;
        });
    }

    void
    parse_native_request(FastOpenRtbState& state, std::string_view value)
    {
      if(!state.native)
      {
        return;
      }

      FastJsonParser native_parser(true);
      init_native_payload_processors(native_parser, "");
      init_native_payload_processors(native_parser, "native");
      native_parser.parse(value, &state);
    }

    void
    init_native_processors(FastJsonParser& parser, std::string_view base)
    {
      add_object_processor(parser, base, start_native);

      add_processor(
        parser,
        std::string(base) + ".request",
        std::make_shared<StringHandlerProcessor<decltype(&parse_native_request)>>(
          &parse_native_request));

      init_native_payload_processors(parser, std::string(base) + ".request");
      init_native_payload_processors(parser, std::string(base) + ".request.native");
    }
  }

  void
  RequestInfoFiller::init_fast_json_processors_()
  {
    auto parser = std::make_unique<FastJsonParser>(true);

    add_string(*parser, "id", root_context, &JsonProcessingContext::request_id);
    add_string_array(
      *parser,
      "cur",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->currencies, value);
      });
    add_string_array(
      *parser,
      "bcat",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->exclude_categories, value);
      });
    add_bool(*parser, "test", root_context, &JsonProcessingContext::test);
    add_bool(*parser, "ext.is_test", root_context, &JsonProcessingContext::test);
    add_bool(*parser, "ext.secure", root_context, &JsonProcessingContext::secure);
    add_string(*parser, "ext.category", root_context, &JsonProcessingContext::required_category);
    add_string(*parser, "ext.udi.idfa", root_context, &JsonProcessingContext::ifa);
    add_string(*parser, "ext.udi.gaid", root_context, &JsonProcessingContext::ifa);

    add_object_processor(
      *parser,
      "imp",
      start_ad_slot,
      [](FastOpenRtbState&) {});
    add_string(*parser, "imp.id", ad_slot_context, &AdSlotContext::id);
    add_decimal(
      *parser,
      "imp.bidfloor",
      ad_slot_context,
      &AdSlotContext::min_cpm_price,
      Generics::DMR_CEIL,
      AdServer::CampaignSvcs::RevenueDecimal::MAXIMUM);
    add_string(
      *parser,
      "imp.bidfloorcur",
      ad_slot_context,
      &AdSlotContext::min_cpm_price_currency_code);
    add_bool(*parser, "imp.secure", ad_slot_context, &AdSlotContext::secure);
    add_string(*parser, "imp.tagid", ad_slot_context, &AdSlotContext::tagid);
    add_string(*parser, "imp.ext.type", ad_slot_context, &AdSlotContext::imp_ext_type);
    add_optional_long(
      *parser,
      "imp.pmp.private_auction",
      ad_slot_context,
      &AdSlotContext::private_auction);
    add_object_processor(
      *parser,
      "imp.pmp.deals",
      start_deal,
      [](FastOpenRtbState&) {});
    add_string(*parser, "imp.pmp.deals.id", deal_context, &AdSlotContext::Deal::id);
    add_decimal(
      *parser,
      "imp.pmp.deals.bidfloor",
      deal_context,
      &AdSlotContext::Deal::cpm_price,
      Generics::DMR_CEIL,
      AdServer::CampaignSvcs::RevenueDecimal::MAXIMUM);
    add_string(
      *parser,
      "imp.pmp.deals.bidfloorcur",
      deal_context,
      &AdSlotContext::Deal::currency_code);

    add_object_processor(
      *parser,
      "imp.metric",
      start_metric,
      [](FastOpenRtbState&) {});
    add_string(*parser, "imp.metric.type", metric_context, &AdSlotContext::Metric::type);
    add_processor(
      *parser,
      "imp.metric.value",
      std::make_shared<MetricValueProcessor>());

    init_banner_processors(*parser, "imp.banner");
    init_banner_processors(*parser, "imp.banners");

    add_processor(
      *parser,
      "imp.video",
      std::make_shared<VideoStartedProcessor>());
    add_state_value_number(
      *parser,
      "imp.video.w",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_width : nullptr;
      });
    add_state_value_number(
      *parser,
      "imp.video.h",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_height : nullptr;
      });
    add_string(*parser, "imp.video.pos", ad_slot_context, &AdSlotContext::video_pos);
    add_string_array(
      *parser,
      "imp.video.mimes",
      [](FastOpenRtbState& state, std::string_view value)
      {
        if(state.ad_slot)
        {
          insert_string(state.ad_slot->video_mimes, value);
        }
      });
    add_string_array(
      *parser,
      "imp.video.battr",
      [](FastOpenRtbState& state, std::string_view value)
      {
        if(state.ad_slot)
        {
          append_string(state.ad_slot->video_exclude_categories, value);
        }
      });
    add_state_value_number(
      *parser,
      "imp.video.minduration",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_min_duration : nullptr;
      });
    add_state_value_number(
      *parser,
      "imp.video.maxduration",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_max_duration : nullptr;
      });
    add_state_number_set(
      *parser,
      "imp.video.protocol",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULSetStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_protocols : nullptr;
      });
    add_state_number_set(
      *parser,
      "imp.video.protocols",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULSetStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_protocols : nullptr;
      });
    add_state_number_set(
      *parser,
      "imp.video.playbackmethod",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULSetStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_playbackmethods : nullptr;
      });
    add_state_number_set(
      *parser,
      "imp.video.api",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULSetStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_api : nullptr;
      });
    add_state_value_number(
      *parser,
      "imp.video.startdelay",
      [](FastOpenRtbState& state)
        -> AdSlotContext::LValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_start_delay : nullptr;
      });
    add_state_value_number(
      *parser,
      "imp.video.linearity",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_linearity : nullptr;
      });
    add_state_value_number(
      *parser,
      "imp.video.skip",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_skip : nullptr;
      });
    add_state_value_number(
      *parser,
      "imp.video.reward",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_reward : nullptr;
      });
    add_optional_long(*parser, "imp.video.placement", ad_slot_context, &AdSlotContext::video_placement);
    add_state_value_number(
      *parser,
      "imp.video.ext.adtype",
      [](FastOpenRtbState& state)
        -> AdSlotContext::ULValueStateHolder*
      {
        return state.ad_slot ? &state.ad_slot->video_adtype : nullptr;
      });

    init_native_processors(*parser, "imp.native");

    add_object_processor(
      *parser,
      "site",
      [](FastOpenRtbState& state) { state.context->site = true; });
    add_url(*parser, "site.page", root_context, &JsonProcessingContext::site_page);
    add_string(*parser, "site.name", root_context, &JsonProcessingContext::site_name);
    add_url(*parser, "site.domain", root_context, &JsonProcessingContext::site_domain);
    add_string(*parser, "site.id", root_context, &JsonProcessingContext::site_id);
    add_string(*parser, "site.search", root_context, &JsonProcessingContext::site_search);
    add_url(*parser, "site.ref", root_context, &JsonProcessingContext::site_ref);
    add_url(*parser, "site.referer", root_context, &JsonProcessingContext::site_referer);
    add_url(*parser, "site.rereferer", root_context, &JsonProcessingContext::site_rereferer);
    add_string(*parser, "site.keywords", root_context, &JsonProcessingContext::site_keywords);
    add_string_array(*parser, "site.pagecat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->site_pagecat, value); });
    add_string_array(*parser, "site.sectioncat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->site_sectioncat, value); });
    add_string_array(*parser, "site.cat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->site_cat, value); });
    add_bool(*parser, "site.ext.ssl_enabled", root_context, &JsonProcessingContext::secure);
    add_string(*parser, "site.ext.puid1", root_context, &JsonProcessingContext::puid1);
    add_string(*parser, "site.ext.puid2", root_context, &JsonProcessingContext::puid2);
    add_object_processor(*parser, "site.content", [](FastOpenRtbState& state) { state.context->site_content = true; });
    add_string(*parser, "site.content.keywords", root_context, &JsonProcessingContext::content_keywords);
    add_string(*parser, "site.content.title", root_context, &JsonProcessingContext::content_title);
    add_string(*parser, "site.content.series", root_context, &JsonProcessingContext::content_series);
    add_string(*parser, "site.content.season", root_context, &JsonProcessingContext::content_season);
    add_string_array(*parser, "site.content.cat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->content_cat, value); });
    add_object_processor(*parser, "site.content.producer", [](FastOpenRtbState& state) { state.context->site_content_producer = true; });
    add_string_array(
      *parser,
      "site.content.producer.name",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->content_producer_name, value);
      });
    add_object_processor(*parser, "site.publisher", [](FastOpenRtbState& state) { state.context->site_publisher = true; });
    add_string(*parser, "site.publisher.name", root_context, &JsonProcessingContext::publisher_name);
    add_string(*parser, "site.publisher.id", root_context, &JsonProcessingContext::publisher_id);
    add_string_array(*parser, "site.publisher.cat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->publisher_cat, value); });

    add_object_processor(
      *parser,
      "app",
      [](FastOpenRtbState& state) { state.context->app = true; });
    add_string(*parser, "app.id", root_context, &JsonProcessingContext::app_id);
    add_string(*parser, "app.name", root_context, &JsonProcessingContext::app_name);
    add_string(*parser, "app.bundle", root_context, &JsonProcessingContext::app_bundle);
    add_url(*parser, "app.domain", root_context, &JsonProcessingContext::app_domain);
    add_url(*parser, "app.storeurl", root_context, &JsonProcessingContext::app_store_url);
    add_string(*parser, "app.keywords", root_context, &JsonProcessingContext::app_keywords);
    add_string_array(*parser, "app.pagecat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->app_pagecat, value); });
    add_string_array(*parser, "app.sectioncat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->app_sectioncat, value); });
    add_string_array(*parser, "app.cat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->app_cat, value); });
    add_object_processor(*parser, "app.content", [](FastOpenRtbState& state) { state.context->app_content = true; });
    add_string(*parser, "app.content.keywords", root_context, &JsonProcessingContext::content_keywords);
    add_string(*parser, "app.content.title", root_context, &JsonProcessingContext::content_title);
    add_string(*parser, "app.content.series", root_context, &JsonProcessingContext::content_series);
    add_string(*parser, "app.content.season", root_context, &JsonProcessingContext::content_season);
    add_string_array(*parser, "app.content.cat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->content_cat, value); });
    add_object_processor(*parser, "app.content.producer", [](FastOpenRtbState& state) { state.context->app_content_producer = true; });
    add_string_array(
      *parser,
      "app.content.producer.name",
      [](FastOpenRtbState& state, std::string_view value)
      {
        append_string(state.context->content_producer_name, value);
      });
    add_object_processor(*parser, "app.publisher", [](FastOpenRtbState& state) { state.context->app_publisher = true; });
    add_string(*parser, "app.publisher.name", root_context, &JsonProcessingContext::publisher_name);
    add_string(*parser, "app.publisher.id", root_context, &JsonProcessingContext::publisher_id);
    add_string_array(*parser, "app.publisher.cat", [](FastOpenRtbState& state, std::string_view value) { append_string(state.context->publisher_cat, value); });

    add_string(*parser, "device.ip", root_context, &JsonProcessingContext::ip);
    add_string(*parser, "device.ipv6", root_context, &JsonProcessingContext::ipv6);
    add_string(*parser, "device.ua", root_context, &JsonProcessingContext::user_agent);
    add_string(*parser, "device.userdata", root_context, &JsonProcessingContext::user_id);
    add_string(*parser, "device.ifa", root_context, &JsonProcessingContext::ifa);
    add_string(*parser, "device.didmd5", root_context, &JsonProcessingContext::didmd5);
    add_string(*parser, "device.didsha1", root_context, &JsonProcessingContext::didsha1);
    add_string(*parser, "device.dpidmd5", root_context, &JsonProcessingContext::dpidmd5);
    add_string(*parser, "device.dpisha1", root_context, &JsonProcessingContext::dpisha1);
    add_string(*parser, "device.macsha1", root_context, &JsonProcessingContext::macsha1);
    add_string(*parser, "device.macmd5", root_context, &JsonProcessingContext::macmd5);
    add_string(*parser, "device.language", root_context, &JsonProcessingContext::language);
    add_string(*parser, "device.carrier", root_context, &JsonProcessingContext::carrier);
    add_integer(*parser, "device.devicetype", root_context, &JsonProcessingContext::ssp_devicetype);
    add_string(*parser, "device.geo.country", root_context, &JsonProcessingContext::ssp_country);
    add_string(*parser, "device.geo.region", root_context, &JsonProcessingContext::ssp_region);
    add_string(*parser, "device.geo.city", root_context, &JsonProcessingContext::ssp_city);

    add_object_processor(*parser, "user", [](FastOpenRtbState& state) { state.context->user = true; });
    add_string(*parser, "user.id", root_context, &JsonProcessingContext::external_user_id);
    add_string(*parser, "user.buyeruid", root_context, &JsonProcessingContext::user_id);
    add_string(*parser, "user.buyerid", root_context, &JsonProcessingContext::user_id);
    add_string(*parser, "user.keywords", root_context, &JsonProcessingContext::user_keywords);
    add_string(*parser, "user.gender", root_context, &JsonProcessingContext::user_gender);
    add_integer(*parser, "user.yob", root_context, &JsonProcessingContext::user_yob);
    add_object_processor(
      *parser,
      "user.data.segment",
      start_segment,
      [](FastOpenRtbState&) {});
    add_string(*parser, "user.data.segment.id", segment_context, &JsonProcessingContext::Segment::id);
    add_string(*parser, "user.data.segment.name", segment_context, &JsonProcessingContext::Segment::name);
    add_string(*parser, "user.data.segment.value", segment_context, &JsonProcessingContext::Segment::value);
    add_object_processor(
      *parser,
      "user.ext.eids",
      start_user_eid,
      [](FastOpenRtbState&) {});
    add_string(*parser, "user.ext.eids.source", user_eid_context, &JsonProcessingContext::UserEid::source);
    add_object_processor(
      *parser,
      "user.ext.eids.uids",
      start_user_eid_uid,
      [](FastOpenRtbState&) {});
    add_string(*parser, "user.ext.eids.uids.id", user_eid_uid_context, &JsonProcessingContext::UserEidUid::id);
    add_string(*parser, "user.ext.eids.uids.stableid", user_eid_uid_context, &JsonProcessingContext::UserEidUid::stable_id);

    add_bool(*parser, "regs.coppa", root_context, &JsonProcessingContext::regs_coppa);

    fast_json_parser_ = std::move(parser);
  }

  void
  RequestInfoFiller::parse_openrtb_request_(
    RequestParams& request_params,
    JsonProcessingContext& context,
    const char* bid_request) const
  {
    static const char* FUN = "RequestInfoFiller::parse_openrtb_request_()";

    FastOpenRtbState state;
    state.request_params = &request_params;
    state.context = &context;

    try
    {
      fast_json_parser_->parse(std::string_view(bid_request), &state);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": parsing error: " << e.what();
      throw InvalidParamException(ostr);
    }

    for(auto& ad_slot : context.ad_slots)
    {
      ad_slot.metrics.remove_if(
        [](const AdSlotContext::Metric& metric)
        {
          return metric.type.empty() || metric.value.empty();
        });
    }
  }
}
