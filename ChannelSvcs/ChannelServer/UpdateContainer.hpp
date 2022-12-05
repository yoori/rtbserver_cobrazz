
#ifndef AD_SERVER_UPDATE_CONTAINER
#define AD_SERVER_UPDATE_CONTAINER

#include <map>
#include <set>
#include <deque>
#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <ChannelSvcs/ChannelCommons/TriggerParser.hpp>
#include "ChannelContainer.hpp"
#include "DictionaryMatcher.hpp"

namespace AdServer
{
namespace ChannelSvcs
{

  class ChannelContainerBase; 

  class UpdateContainer:
    public TriggerParser::MergeContainer,
    public ChannelContainerBase
  {
  public:
    UpdateContainer(
      ChannelContainer* cont,
      DictionaryMatcher* dict)
      noexcept;

    ~UpdateContainer() noexcept {};

    /*Callback of parser
     * add new triggers to container */
    virtual size_t add_trigger(MergeAtom& merge_atom)
      /*throw(TriggerParser::Exception)*/;

    /*Build list of removed trigger
     * @info - actual channel information
     * Build list of removed trigger*/
    size_t check_actual(ChannelIdToMatchInfo& info) noexcept;

    /* list of updated channels*/
    const ExcludeContainerType& get_updated() noexcept;

    /* list of removed channels*/
    const ExcludeContainerType& get_removed() noexcept;

    /* list of new channels*/
    const ExcludeContainerType& get_new() noexcept;

    /*select triggers for parsing and mark removed triggers
     * @channel_id - channel
     * @triggers - list of triggers for parsing
     * @load_once - this channel can't be updated
     * just loaded for one time
     */
    size_t select_parsed_triggers(
      unsigned long channel_id,
      TriggerList& triggers,
      bool load_once)
      noexcept;

    virtual bool ready() const noexcept;

    const ChannelContainer* get_helper() const noexcept;

    /* fill cache of lexemes for unmerged triggers */
    void prepare_merge(const UnmergedMap& unmerged) /*throw(Exception)*/;

    /* get lexemes for new triggers
     * @lang - language
     * @parts - parts of triggers
     * @lexemes - founded lexemes for triggers
     * */
    void fill_lexemes(
      unsigned short lang,
      const SubStringVector& parts,
      LexemesPtrVector& lexemes) const
      noexcept;

    /* get old information about channels
     * @returns pointer to information
     * */
    ChannelIdToMatchInfo_var get_old_info() noexcept;

    typedef std::map<unsigned int, MatterItem> Matters;

    Matters& get_matters() noexcept;

    const Matters& get_matters() const noexcept;

    typedef std::map<
      unsigned short, DictionaryMatcher::LexemeCache> Lexemes;

  private:

    void cleanup_() noexcept;

  private:
    Matters matters_;
    ExcludeContainerType new_channels_;
    ExcludeContainerType updated_channels_;
    ExcludeContainerType removed_channels_;
    const ChannelContainer* helper_;
    DictionaryMatcher* dict_matcher_;
    Lexemes lexemes_;
  };

}
}

namespace AdServer
{
  namespace ChannelSvcs
  {

  inline
  UpdateContainer::Matters& UpdateContainer::get_matters() noexcept
  {
    return matters_;
  }

  inline
  const UpdateContainer::Matters& UpdateContainer::get_matters() const noexcept
  {
    return matters_;
  }


  inline
  bool UpdateContainer::ready() const noexcept
  {
    if(!dict_matcher_)
    {
      return true;
    }
    return dict_matcher_->ready();
  }

  inline
  const ChannelContainer* UpdateContainer::get_helper() const noexcept
  {
    return helper_;
  }

  inline
  const ExcludeContainerType& UpdateContainer::get_removed() noexcept
  {
    return removed_channels_;
  }

  inline
  const ExcludeContainerType& UpdateContainer::get_updated() noexcept
  {
    return updated_channels_;
  }

  inline
  const ExcludeContainerType& UpdateContainer::get_new() noexcept
  {
    return new_channels_;
  }

  }//namespace ChannelSvcs
}

#endif //AD_SERVER_UPDATE_CONTAINER

