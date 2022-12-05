#ifndef ENTITYUTILS_HPP
#define ENTITYUTILS_HPP

using namespace AdServer::CampaignSvcs;

struct Holder
{
  std::list<Tag_var> tags;
  std::list<Colocation_var> colocations;
};

inline
Creative_var
create_creative(
  Campaign_var& campaign,
  unsigned long account_id,
  unsigned long advertiser_id,
  unsigned long campaign_id,
  unsigned long ccg_id,
  unsigned long creative_id,
  unsigned long cc_id,
  unsigned long size_id,
  const std::set<uint32_t>& content_categories,
  const std::set<uint32_t>& visual_categories)
{
  Account_var account = new AccountDef();
  account->account_id = account_id;

  Account_var advertiser = new AccountDef();
  advertiser->account_id = advertiser_id;

  campaign = new Campaign();
  campaign->campaign_id = ccg_id;
  campaign->campaign_group_id = campaign_id;
  campaign->account = account;
  campaign->advertiser = advertiser;
  campaign->ctr = RevenueDecimal(0.999);

  Creative_var creative = new Creative(
    campaign,
    cc_id,
    creative_id,
    0,
    0,
    "",
    "",
    OptionValue(),
    "",
    "",
    Creative::CategorySet());

  std::copy(
    content_categories.begin(),
    content_categories.end(),
    std::back_inserter(creative->content_categories));
  std::copy(
    visual_categories.begin(),
    visual_categories.end(),
    std::back_inserter(creative->visual_categories));

  Size_var size = new Size();
  size->size_id = size_id;

  Creative::Size cr_size;
  cr_size.size = size;
  creative->sizes.insert(std::make_pair(size_id, cr_size));

  return creative;
}

inline
void
init_campaign_select_params(
  Holder& holder,
  CampaignSelectParams& campaign_select_params,
  unsigned long colo_id,
  unsigned long publisher_id,
  unsigned long tag_id,
  unsigned long size_id,
  const char* size_protocol_name)
{
  Account_var publisher = new AccountDef();
  publisher->account_id = publisher_id;

  Site_var site = new Site();
  site->site_id = 2;
  site->account = publisher;

  Tag_var tag = new Tag();
  tag->tag_id = tag_id;
  tag->site = site;
  tag->adjustment = RevenueDecimal(false, 1, 0);

  Size_var size = new Size();
  size->size_id = size_id;
  size->protocol_name = size_protocol_name;

  Tag::Size_var tag_size = new Tag::Size();
  tag_size->size = size;
  tag_size->max_text_creatives = 1;
  tag->sizes.insert(std::make_pair(size_id, tag_size));

  Account_var isp = new AccountDef();
  isp->account_id = 4;

  Colocation_var colocation = new Colocation();
  colocation->colo_id = colo_id;
  colocation->account = isp;

  holder.tags.push_back(tag);
  campaign_select_params.tag = tag;
  holder.colocations.push_back(colocation);
  campaign_select_params.colocation = colocation;
  campaign_select_params.tag_sizes = tag->sizes;
}

#endif /*ENTITYUTILS_HPP*/
