#include <cstdint>
#include <iostream>
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

  void check_storage(std::string_view value, std::string_view expected, std::string_view block)
  {
    require(value == expected, "string value changed");
    if (value.size() >= 32)
    {
      const auto begin = reinterpret_cast<std::uintptr_t>(block.data());
      const auto address = reinterpret_cast<std::uintptr_t>(value.data());
      require(address >= begin && address + value.size() <= begin + block.size(),
        "string payload is outside Arena");
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
      if (field->is_repeated() || field->cpp_type() != PB::FieldDescriptor::CPPTYPE_STRING)
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
        reflection->SetString(message, field, input);
        input.assign(size, 'y');
        // Protobuf 36.2 GetStringView() does not handle MicroString; GetString() does.
        require(reflection->GetString(*message, field) == expected, "field value changed");

        // Compare generated serialization with a reflection-based reference.
        std::unique_ptr<PB::Message> reference(reference_prototype->New());
        reference->GetReflection()->SetString(reference.get(), field, expected);
        const auto wire = message->SerializeAsString();
        require(wire == reference->SerializeAsString(), "wire format changed");
        require(parsed->ParseFromString(wire), "parse failed");
        require(reflection->GetString(*parsed, field) == expected, "parsed value changed");

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
        require(reflection->GetString(*message, field).empty(), "clear failed");
        message->CopyFrom(*heap_copy);
        require(reflection->GetString(*message, field) == expected, "field value changed");
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
    slot->set_creative_body(value);
    channel->set_pwords(value);
    for (const auto actual :
      {common->referer(), token->value(), slot->creative_body(), channel->pwords()})
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
    std::cout << "PASS: " << fields
      << " string/bytes fields: wire/text/JSON round trips, copies; nested setter Arena storage\n";
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }
}
