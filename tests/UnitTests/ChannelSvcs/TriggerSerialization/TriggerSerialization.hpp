
namespace AdServer
{
namespace UnitTests
{
  class TriggerSerializationTest
  {
  public:

    TriggerSerializationTest() : trigger_type_(0) {};

    struct SoftWord
    {
      unsigned int channel_trigger_id;
      std::string trigger;
      ChannelSvcs::Parts parts;
      char type;
      bool exact;
    };

    int run(int argc, char* argv[]) noexcept;

    void generate_soft_trigger_word(
      size_t iteration,
      size_t max_parts,
      size_t max_word_len,
      SoftWord& word,
      std::string& trigger) noexcept;

  private:
    int regular_test_case_() noexcept;
    int print_compary_result_(
      const SoftWord& word1,
      const std::string& trigger_in,
      char type,
      const std::string& trigger,
      bool exact,
      bool negative) noexcept;
    int print_compary_parts_(
      size_t iteration,
      const ChannelSvcs::Parts& words1,
      const ChannelSvcs::SubStringVector& parts2,
      const char *result,
      size_t len) noexcept;

    std::string trigger_;
    char trigger_type_;

  };
}
}

