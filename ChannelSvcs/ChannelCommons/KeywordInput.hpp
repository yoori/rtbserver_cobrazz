#pragma once

#include <string_view>
#include <Commons/Constants.hpp>
#include <Language/SegmentorCommons/SegmentorInterface.hpp>

namespace AdServer::ChannelSvcs
{
  enum ParseMode
  {
    PM_SIMPLIFY,
    PM_NO_SIMPLIFY
  };

  struct KeywordInput
  {
    std::string_view text;
    ParseMode mode = PM_NO_SIMPLIFY;
    std::string_view separators = ",\n";
    std::size_t max_length = Commons::DEFAULT_MAX_HARD_WORD_SEQ;
    const Language::Segmentor::SegmentorInterface* segmentor = nullptr;
    bool use_keyword_hooks = true;
  };
}
