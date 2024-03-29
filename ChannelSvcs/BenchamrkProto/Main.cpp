// STD
#include <chrono>
#include <iostream>

// UNIXCOMMONS
#include <Generics/Uncopyable.hpp>

// Protobuf
#include <google/protobuf/arena.h>
#include "Test.pb.h"

class Timer final : private Generics::Uncopyable
{
private:
  using TimePoint = std::chrono::time_point<
    std::chrono::high_resolution_clock>;

public:
  explicit Timer() = default;

  ~Timer() = default;

  void start() noexcept
  {
    begin_time_ = std::chrono::high_resolution_clock::now();
  }

  void stop() noexcept
  {
    end_time_ = std::chrono::high_resolution_clock::now();
  }

  std::size_t elapsed_time() noexcept
  {
    return std::chrono::duration<double, std::milli>(end_time_ - begin_time_).count();
  }

private:
  TimePoint begin_time_;

  TimePoint end_time_;
};

class Benchmark final : private Generics::Uncopyable
{
public:
  Benchmark(
    const std::size_t number_iterations,
    const int channels_count,
    const std::string& data_string,
    const std::string& time_string)
    : number_iterations_(number_iterations),
      channels_count_(channels_count),
      data_string_(data_string),
      time_string_(time_string)
  {
  }

  void run()
  {
    run_default();
    run_arena();
  }

private:
  void run_default()
  {
    Timer timer;
    timer.start();
    for (std::size_t i = 1; i <= number_iterations_; ++i)
    {
      Proto::MatchRequest request;
      fill_request_proto(request);
      Proto::MatchResponse response;
      fill_response_proto(response);
    }
    timer.stop();
    std::cout << "time(Default)="
              << timer.elapsed_time()
              << std::endl;
  }

  void run_arena()
  {
    google::protobuf::ArenaOptions arena_options;
    arena_options.initial_block_size = 256 * 10;
    arena_options.start_block_size = 256 * 10;
    arena_options.max_block_size = 8192 * 10;
    google::protobuf::Arena arena(arena_options);

    Timer timer;
    timer.start();
    for (std::size_t i = 1; i <= number_iterations_; ++i)
    {
      auto* request = google::protobuf::Arena::CreateMessage<Proto::MatchRequest>(&arena);
      fill_request_proto(*request);
      auto* response = google::protobuf::Arena::CreateMessage<Proto::MatchResponse>(&arena);
      fill_response_proto(*response);

      if (/*i % 100 == 0*/true)
      {
        arena.Reset();
      }
    }
    timer.stop();
    std::cout << "time(arena)="
              << timer.elapsed_time()
              << std::endl;
  }

  void fill_request_proto(Proto::MatchRequest& request)
  {
    auto* query = request.mutable_query();
    query->set_request_id(data_string_);
    query->set_first_url(data_string_);
    query->set_first_url_words(data_string_);
    query->set_urls(data_string_);
    query->set_urls_words(data_string_);
    query->set_pwords(data_string_);
    query->set_swords(data_string_);
    query->set_uid(data_string_);
    query->set_statuses("ab");
    query->set_non_strict_word_match(true);
    query->set_non_strict_url_match(true);
    query->set_return_negative(true);
    query->set_simplify_page(true);
    query->set_fill_content(true);
  }

  void fill_response_proto(Proto::MatchResponse& response)
  {
    auto* info = response.mutable_info();

    auto* content_channels = info->mutable_content_channels();
    content_channels->Reserve(channels_count_);
    for (int i = 0; i < channels_count_; ++i)
    {
      auto* content_channel_atom = content_channels->Add();
      content_channel_atom->set_id(100);
      content_channel_atom->set_weight(100);
    }

    auto* matched_channels = info->mutable_matched_channels();

    auto* page_channels = matched_channels->mutable_page_channels();
    page_channels->Reserve(channels_count_);
    for (int i = 0; i < channels_count_; ++i)
    {
      auto* atom = page_channels->Add();
      atom->set_id(10000000);
      atom->set_trigger_channel_id(10000000);
    }

    auto* search_channels = matched_channels->mutable_search_channels();
    search_channels->Reserve(channels_count_);
    for (int i = 0; i < channels_count_; ++i)
    {
      auto* atom = search_channels->Add();
      atom->set_id(10000000);
      atom->set_trigger_channel_id(10000000);
    }

    auto* url_channels = matched_channels->mutable_url_channels();
    url_channels->Reserve(channels_count_);
    for (int i = 0; i < channels_count_; ++i)
    {
      auto* atom = url_channels->Add();
      atom->set_id(10000000);
      atom->set_trigger_channel_id(10000000);
    }

    auto* url_keyword_channels = matched_channels->mutable_url_keyword_channels();
    url_keyword_channels->Reserve(channels_count_);
    for (int i = 0; i < channels_count_; ++i)
    {
      auto* atom = url_keyword_channels->Add();
      atom->set_id(10000000);
      atom->set_trigger_channel_id(10000000);
    }

    auto* uid_channels = matched_channels->mutable_uid_channels();
    uid_channels->Reserve(channels_count_);
    for (int i = 0; i < channels_count_; ++i)
    {
      uid_channels->Add(10000000);
    }

    info->set_match_time(time_string_);
    info->set_no_adv(true);
    info->set_no_track(true);
  }

private:
  const std::size_t number_iterations_;

  const int channels_count_;

  const std::string data_string_;

  const std::string time_string_;
};

int main(int argc, char* argv[])
{
  const std::size_t number_iterations = 1000000;
  const int channels_count = 30;
  const std::string data_string(100, 'a');
  const std::string time_string(15, 'a');

  Benchmark benchmark(
    number_iterations,
    channels_count,
    data_string,
    time_string);
  benchmark.run();

  return EXIT_SUCCESS;
}