#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include <LogProcessing/LogGeneralizer/LogGeneralizerHttp.hpp>

namespace
{
  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void test()
  {
    Generics::Values_var stats = new Generics::Values;
    auto handler = AdServer::LogProcessing::make_log_generalizer_stats_http_handler(stats.in());
    const auto request = [&handler](const std::string& method)
    {
      return AdServer::Commons::sync_wait(handler({method, "/stats", "/stats", {}}));
    };
    require(request("GET").body == "{}\n", "empty statistics is not an empty JSON object");

    stats->set("unsigned", std::numeric_limits<unsigned long>::max());
    stats->set("signed", -42L);
    stats->set("fraction", 1.25);
    stats->set("nonfinite", std::numeric_limits<double>::infinity());
    stats->set("quoted\"key", std::string("line\n\"value\""));
    const auto response = request("GET");
    require(response.status == 200, "GET status");
    require(response.content_type == "application/json", "GET content type");
    const auto contains = [&response](const std::string& text)
    {
      return response.body.find(text) != std::string::npos;
    };
    require(contains("\"unsigned\": " + std::to_string(std::numeric_limits<unsigned long>::max())),
      "unsigned counter lost precision");
    require(contains("\"signed\": -42"), "signed counter");
    require(contains("\"fraction\": 1.25"), "floating point counter");
    require(contains("\"nonfinite\": null"), "nonfinite number produced invalid JSON");
    require(contains("\"quoted\\\"key\": \"line\\n\\\"value\\\"\""), "JSON escaping");
    require(request("GET").body == response.body, "GET reset statistics");
    require(request("POST").status == 405, "POST was not rejected");

    stats->add("signed", 1L);
    stats.reset();
    require(request("GET").body.find("\"signed\": -41") != std::string::npos,
      "handler did not retain live statistics");
  }
}

int main()
{
  try
  {
    test();
    std::cout << "PASS: LogGeneralizer /stats serialization and ownership\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
