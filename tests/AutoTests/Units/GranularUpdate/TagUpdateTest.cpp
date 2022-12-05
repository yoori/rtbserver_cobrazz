#include "TagUpdateTest.hpp"

REFLECT_UNIT(TagUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::SelectedCreativesCCID SelectedCreativesCCID;

  namespace ORM = ::AutoTest::ORM;

  typedef AutoTest::TagChecker TagChecker;
  typedef AutoTest::WaitChecker<TagChecker> TagWaitChecker;
  typedef AutoTest::SelectedCreativeChecker SelectedCreativeChecker;

  std::string size_regexp(
    const std::string id,
    const std::string name)
  {
    return
      std::string(
        "^\\[" + id + ",'" + name + "',\\d+\\]$");
  }
}

void TagUpdateTest::set_up()
{
  add_descr_phrase("Setup");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_MANAGER)),
    "CampaignManager must set in the configuration file");

  other_size_type_ = fetch_int("SizeType");
  site_id_ = fetch_int("SiteId");
  size_468x60_id_ = fetch_int("Size1");
  size_468x60_name_ = fetch_string("Size1Name");
}

void TagUpdateTest::tag_create_case()
{
  // Create tag
  ORM::ORMRestorer<ORM::PQ::Tags>* tag =
    create<ORM::PQ::Tags>();
  tag->name = fetch_string("InsertTag/Tag1/NAME");
  tag->flags = 0;
  tag->size_type_id = other_size_type_;
  tag->site = site_id_;
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      tag->insert()),
    "inserting tag");

  // Create tag pricing
  ORM::ORMRestorer<ORM::RatedTagPricing>* tag_pricing =
    create<ORM::RatedTagPricing>();
  tag_pricing->status = "A";
  tag_pricing->tag = tag->id();
  tag_pricing->rate.rate_type = "CPM";
  tag_pricing->rate.rate = 10;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      tag_pricing->insert()),
    "inserting tag_pricing");

  ORM::ORMRestorer<ORM::PQ::Tag_tagsize>* tag_size =
    create<ORM::PQ::Tag_tagsize>();

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      tag_size->insert(
        size_468x60_id_,
        tag->id())),
    "inserting tag_tagsize");

  ADD_WAIT_CHECKER(
    "Check changes",
    TagChecker(
      this,
      tag->id(),
      TagChecker::Expected().
        tag_id(tag->id()).
        site_id(site_id_).
        sizes(
          size_regexp(
            strof(size_468x60_id_),
            size_468x60_name_)).
        accepted_categories("").
        rejected_categories("")));
}

void TagUpdateTest::tag_update_creative_category_exclusion_case()
{
  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

  AutoTest::NSLookupRequest request;
  request.referer_kw = fetch_string("Keyword");

  unsigned int cc_id = fetch_int("CCID");
  unsigned int cat_id = fetch_int("VisualCategory");
  unsigned int tag1 = fetch_int("UpdateCategory/Tag1/ID");
  unsigned int tag2 = fetch_int("UpdateCategory/Tag2/ID");
  unsigned int tag3 = fetch_int("UpdateCategory/Tag3/ID");

  // Initial checks
  FAIL_CONTEXT(
    TagWaitChecker(
      TagChecker(
        this,
        tag1,
        TagChecker::Expected().
          tag_id(tag1).
          site_id(site_id_).
          sizes(
            size_regexp(
              strof(size_468x60_id_),
              size_468x60_name_)).
          accepted_categories("").
          rejected_categories(""))).check(),
    "Initial for tag#1");

  FAIL_CONTEXT(
    TagChecker(
      this,
      tag2,
      TagChecker::Expected().
        tag_id(tag2).
        site_id(site_id_).
        sizes(
          size_regexp(
            strof(size_468x60_id_),
            size_468x60_name_)).
        accepted_categories(strof(cat_id)).
        rejected_categories("")).check(),
    "Initial for tag#2");

  FAIL_CONTEXT(
    TagChecker(
      this,
      tag3,
      TagChecker::Expected().
        tag_id(tag3).
        site_id(site_id_).
        sizes(
          size_regexp(
            strof(size_468x60_id_),
            size_468x60_name_)).
        accepted_categories("").
        rejected_categories(strof(cat_id))).check(),
    "Initial for tag#3");

  request.tid = tag1;
  
  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client, request, cc_id).check(),
    "Initial request from tag#1");

  request.tid = tag2;
  
  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client, request, cc_id).check(),
    "Initial request from tag#2");

  request.tid = tag3;
  
  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client, request, 0).check(),
    "Initial request from tag#3");

  // Insert new category
  ORM::ORMRestorer<ORM::PQ::Tagscreativecategoryexclusion>* exclusion1 =
    create<ORM::PQ::Tagscreativecategoryexclusion>();
  exclusion1->approval = "A";
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      exclusion1->insert(cat_id, tag1)),
    "inserting creative category exclusion on tag level");

  ADD_WAIT_CHECKER(
    "Adding approved category for tag#1",
    TagChecker(
      this,
      tag1,
      TagChecker::Expected().
        tag_id(tag1).
        site_id(site_id_).
        sizes(
          size_regexp(
            strof(size_468x60_id_),
            size_468x60_name_)).
        accepted_categories(strof(cat_id)).
        rejected_categories("")));

  request.tid = tag1;
  add_checker(
    "Request for creative from tag#1",
    SelectedCreativeChecker(client, request, cc_id));

  // Update category approval
  ORM::ORMRestorer<ORM::PQ::Tagscreativecategoryexclusion>* exclusion2 =
    create<ORM::PQ::Tagscreativecategoryexclusion>(
      ORM::PQ::Tagscreativecategoryexclusion(pq_conn_, cat_id, tag2));
  exclusion2->approval = "R";

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      exclusion2->update()),
    "updating creative category exclusion approval for tag#2");

  ADD_WAIT_CHECKER(
    "Rejecting category for tag#2",
    TagChecker(
      this,
      tag2,
      TagChecker::Expected().
        tag_id(tag2).
        site_id(site_id_).
        sizes(
          size_regexp(
            strof(size_468x60_id_),
            size_468x60_name_)).
        accepted_categories("").
        rejected_categories(strof(cat_id))));

  request.tid = tag2;
  add_checker(
    "Request for no creative from tag#2",
    SelectedCreativeChecker(client, request, 0));

  // Delete category exclusion
  ORM::ORMRestorer<ORM::PQ::Tagscreativecategoryexclusion>* exclusion3 =
    create<ORM::PQ::Tagscreativecategoryexclusion>(
      ORM::PQ::Tagscreativecategoryexclusion(pq_conn_, cat_id, tag3));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      exclusion3->delet()),
    "deleting creative category exclusion for tag#3");

  ADD_WAIT_CHECKER(
    "Deleting category excludion for tag#3",
    TagChecker(
      this,
      tag3,
      TagChecker::Expected().
        tag_id(tag3).
        site_id(site_id_).
        sizes(
          size_regexp(
            strof(size_468x60_id_),
            size_468x60_name_)).
        accepted_categories("").
        rejected_categories("")));

  request.tid = tag3;
  add_checker(
    "Request for creative from tag#3",
    SelectedCreativeChecker(client, request, cc_id));
}

void TagUpdateTest::tag_update_size_case()
{

  int size_type = fetch_int("Size2Type");
  int size = fetch_int("Size2");
  std::string size_name = fetch_string("Size2Name");
  
  ORM::ORMRestorer<ORM::PQ::Tags>* tag =
    create<ORM::PQ::Tags>(fetch_int("UpdateSize/Tag1/ID"));

  FAIL_CONTEXT(
    TagWaitChecker(
      TagChecker(
        this,
        tag->id(),
        TagChecker::Expected().
          tag_id(tag->id()).
          site_id(site_id_).
          sizes(
            size_regexp(
              strof(size_468x60_id_),
              size_468x60_name_)).
          accepted_categories("").
          rejected_categories(""))).check(),
    "Initial");

  tag->size_type_id = size_type;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      tag->update()),
    "updating tag size");

  {
    ORM::ORMRestorer<ORM::PQ::Tag_tagsize>* tag_size =
      create(
        ORM::PQ::Tag_tagsize(
          pq_conn_, size_468x60_id_, tag->id()));

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        tag_size->delet()),
    "delete old tag_tagsize");
  }

  {
    ORM::ORMRestorer<ORM::PQ::Tag_tagsize>* tag_size =
      create<ORM::PQ::Tag_tagsize>();

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        tag_size->insert(size, tag->id())),
    "insert new tag_tagsize");
  }


  ADD_WAIT_CHECKER(
    "Check changes",
    TagChecker(this,
      tag->id(),
      TagChecker::Expected().
        tag_id(tag->id()).
        site_id(site_id_).
        sizes(
          size_regexp(
            fetch_string("Size2"),
            size_name)).
        accepted_categories("").
        rejected_categories("")));
}

void TagUpdateTest::tag_remove_case()
{
  ORM::ORMRestorer<ORM::PQ::Tags>* tag =
    create<ORM::PQ::Tags>(fetch_int("DeleteTag/Tag1/ID"));

  FAIL_CONTEXT(TagWaitChecker(TagChecker(this, tag->id(),
        TagChecker::Expected().
          tag_id(tag->id()).
          site_id(site_id_).
          sizes(
            size_regexp(
              strof(size_468x60_id_),
              size_468x60_name_)).
          accepted_categories("").
          rejected_categories(""))).check(),
    "Initial");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      tag->del()),
    "deleting tag");

  ADD_WAIT_CHECKER(
    "Check changes",
    TagChecker(
      this, tag->id(),
      TagChecker::Expected(),
      AutoTest::AEC_NOT_EXISTS));
}

void TagUpdateTest::tear_down()
{
  add_descr_phrase("Tear down");
}

bool 
TagUpdateTest::run()
{
  AUTOTEST_CASE(
    tag_create_case(),
    "Tag create case");

  AUTOTEST_CASE(
    tag_update_creative_category_exclusion_case(),
    "Update category case");
  
  AUTOTEST_CASE(
    tag_update_size_case(),
    "Update size case");

  AUTOTEST_CASE(
    tag_remove_case(),
    "Tag delete case");

  return true;
}
