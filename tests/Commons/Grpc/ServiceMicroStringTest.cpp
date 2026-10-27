#include <cstdint>
#include <iostream>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <google/protobuf/arena.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>

#include <CampaignSvcs/CampaignManager/CampaignManagerGrpc.pb.h>
#include <ChannelSvcs/ChannelServer/ChannelServerGrpc.pb.h>

namespace
{
  namespace CM = adserver::campaign_svcs::campaign_manager;
  namespace CS = adserver::channel_svcs::channel_server;
  namespace PB = google::protobuf;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void check_storage(
    std::string_view value, std::string_view expected, std::string_view block,
    std::string_view context = {})
  {
    require(value == expected, "string value changed");
    if (value.size() >= 32)
    {
      const auto begin = reinterpret_cast<std::uintptr_t>(block.data());
      const auto address = reinterpret_cast<std::uintptr_t>(value.data());
      if (address < begin || address + value.size() > begin + block.size())
      {
        throw std::runtime_error("string payload is outside Arena: " + std::string(context));
      }
    }
  }

  std::size_t check_message(const PB::Descriptor* descriptor)
  {
    constexpr std::size_t block_size = 1024 * 1024;
    const auto block = std::make_unique<char[]>(block_size);
    PB::ArenaOptions options;
    options.initial_block = block.get();
    options.initial_block_size = block_size;
    const auto* prototype = PB::MessageFactory::generated_factory()->GetPrototype(descriptor);
    PB::DynamicMessageFactory factory;
    const auto* reference_prototype = factory.GetPrototype(descriptor);
    std::size_t fields = 0;

    for (int index = 0; index < descriptor->field_count(); ++index)
    {
      const auto* field = descriptor->field(index);
      if (field->cpp_type() != PB::FieldDescriptor::CPPTYPE_STRING)
      {
        continue;
      }

      ++fields;
      for (const std::size_t size : {0, 8, 32, 4096, 65536})
      {
        PB::Arena arena(options);
        auto* message = prototype->New(&arena);
        auto* parsed = prototype->New(&arena);
        const auto* reflection = message->GetReflection();
        std::string input(size, 'x');
        if (size > 2)
        {
          input[size / 2] = '\0';
        }

        const auto expected = input;
        if (field->is_repeated())
        {
          reflection->AddString(message, field, input);
          reflection->AddString(message, field, input);
        }
        else
        {
          reflection->SetString(message, field, input);
        }

        PB::Reflection::ScratchSpace scratch;
        const auto get = [field, reflection, &scratch](const PB::Message& value)
        {
          return field->is_repeated() ?
            reflection->GetRepeatedStringView(value, field, 0, scratch) :
            reflection->GetStringView(value, field, scratch);
        };
        input.assign(size, 'y');
        require(get(*message) == expected, "field value changed");

        // Compare generated serialization with a reflection-based reference.
        std::unique_ptr<PB::Message> reference(reference_prototype->New());
        if (field->is_repeated())
        {
          reference->GetReflection()->AddString(reference.get(), field, expected);
          reference->GetReflection()->AddString(reference.get(), field, expected);
        }
        else
        {
          reference->GetReflection()->SetString(reference.get(), field, expected);
        }

        // Singular SetString(std::string) may adopt its heap buffer instead of copying to Arena.
        if (field->is_repeated())
        {
          check_storage(get(*message), expected, {block.get(), block_size}, field->full_name());
        }

        const auto wire = message->SerializeAsString();
        require(wire == reference->SerializeAsString(), "wire format changed");
        require(parsed->ParseFromString(wire), "parse failed");
        if (field->cpp_string_type() == PB::FieldDescriptor::CppStringType::kView)
        {
          check_storage(get(*parsed), expected, {block.get(), block_size}, field->full_name());
        }
        else
        {
          require(get(*parsed) == expected, "parsed value changed");
        }

        std::string text;
        require(PB::TextFormat::PrintToString(*message, &text), "text serialization failed");
        require(PB::TextFormat::ParseFromString(text, parsed), "text parsing failed");
        require(parsed->SerializeAsString() == wire, "text round trip changed data");
        std::string json;
        require(PB::util::MessageToJsonString(*message, &json).ok(), "JSON serialization failed");
        parsed->Clear();
        require(PB::util::JsonStringToMessage(json, parsed).ok(), "JSON parsing failed");
        require(parsed->SerializeAsString() == wire, "JSON round trip changed data");

        std::unique_ptr<PB::Message> heap_copy(prototype->New());
        heap_copy->CopyFrom(*message);
        require(heap_copy->SerializeAsString() == wire, "Arena-to-heap copy changed data");
        message->Clear();
        require(field->is_repeated() ? reflection->FieldSize(*message, field) == 0 :
          get(*message).empty(), "clear failed");
        message->CopyFrom(*heap_copy);
        require(get(*message) == expected, "field value changed");
      }
    }

    return fields;
  }

  void check_nested_setters()
  {
    constexpr std::size_t block_size = 1024 * 1024;
    const auto block = std::make_unique<char[]>(block_size);
    PB::ArenaOptions options;
    options.initial_block = block.get();
    options.initial_block_size = block_size;
    PB::Arena arena(options);
    auto* request = PB::Arena::Create<CM::GetCampaignCreativeRequest>(&arena);
    auto* common = request->mutable_request_params()->mutable_common_info();
    auto* token = common->add_tokens();
    auto* response = PB::Arena::Create<CM::GetCampaignCreativeResponse>(&arena);
    auto* slot = response->mutable_request_result()->add_ad_slots();
    auto* channel = PB::Arena::Create<CS::MatchRequest>(&arena);
    const std::string value(4096, 'a');
    common->set_referer(value);
    token->set_value(value);
    common->add_urls(value);
    auto* request_slot = request->mutable_request_params()->add_ad_slots();
    request_slot->mutable_sizes()->Add()->assign(value.data(), value.size());
    request_slot->add_currency_codes(value);
    request_slot->add_exclude_categories(value);
    request_slot->add_required_categories(value);
    slot->add_track_pixel_urls(value);
    slot->add_external_visual_categories(value);
    slot->add_external_content_categories(value);
    slot->set_creative_body(value);
    channel->set_pwords(value);
    for (const auto actual :
      std::initializer_list<std::string_view>{
        common->referer(), token->value(), slot->creative_body(), channel->pwords(),
        common->urls(0), request_slot->sizes(0), request_slot->currency_codes(0),
        request_slot->exclude_categories(0), request_slot->required_categories(0),
        slot->track_pixel_urls(0), slot->external_visual_categories(0),
        slot->external_content_categories(0)})
    {
      check_storage(actual, value, {block.get(), block_size});
    }
  }
}

int main()
{
  try
  {
    std::size_t fields = check_message(CS::MatchRequest::descriptor());
    const auto* file = CM::GetCampaignCreativeRequest::descriptor()->file();
    for (int index = 0; index < file->message_type_count(); ++index)
    {
      fields += check_message(file->message_type(index));
    }

    check_nested_setters();
    PB::ShutdownProtobufLibrary();
    std::cout << "PASS: " << fields
      << " string/bytes fields: wire/text/JSON round trips, copies; nested setter Arena storage\n";
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
