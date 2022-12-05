
#include "CreativeCategoryGranularUpdateTest.hpp"
#include <iterator>

REFLECT_UNIT(CreativeCategoryGranularUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace {
  enum CategoryType
  {
    CT_VISUAL  = 0,
    CT_CONTENT = 1,
    CT_TAGS    = 2
  };

  typedef AutoTest::CreativeCategoryChecker CreativeCategoryChecker;
  typedef AutoTest::WaitChecker<CreativeCategoryChecker> CreativeCategoryWaitChecker;
  typedef AutoTest::CreativeChecker CreativeChecker;
  typedef AutoTest::WaitChecker<CreativeChecker> CreativeWaitChecker;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::SelectedCreativeChecker SelectedCreativeChecker;
}


void
CreativeCategoryGranularUpdateTest::set_up()
{
  add_descr_phrase("SetUp");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_MANAGER)),
    "CampaignManager must prisent in the configuration file");
 
}

bool
CreativeCategoryGranularUpdateTest::run()
{
  create_categories_();
  unlink_category_();
  add_tags_exclusion_();
  del_tags_exclusion_();
  return true;
}

void
CreativeCategoryGranularUpdateTest::tear_down()
{
  add_descr_phrase("Tear down");
}

void CreativeCategoryGranularUpdateTest::create_categories_()
{
  std::string description("Create creative categories.");
  add_descr_phrase(description);

  // Initial check

  FAIL_CONTEXT(
    CreativeWaitChecker(
      CreativeChecker(
        this,
        fetch_int("CREATE/CC"),
        CreativeChecker::Expected().
          ccid(fetch_int("CREATE/CC")).
          campaign_id(fetch_int("CREATE/CCG")).
          categories(""))).check(),
    description + " Initial creative");


  // Create & link categories

  const char* CATEGORY_NAMES[] =
  {
    "CREATE/CATEGORYNAME/Visual",
    "CREATE/CATEGORYNAME/Content",
    "CREATE/CATEGORYNAME/Tags"
  };

  std::list<unsigned long> cat_ids;

  for (size_t i = 0; i < countof(CATEGORY_NAMES); ++i)
  {
    ORM::ORMRestorer<ORM::PQ::CreativeCategory>* category =
      create<ORM::PQ::CreativeCategory>();
    CategoryType category_type = static_cast<CategoryType>(i);
    category->type = category_type;
    category->name = fetch_string(CATEGORY_NAMES[i]);
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      category->insert()),
    description + 
      " Cann't insert visual category#" + strof(i+1));

    cat_ids.push_back(category->id());
    
    if (category_type != CT_TAGS)
    {
      ORM::ORMRestorer<ORM::PQ::CreativeCategory_Creative>* cc_category =
        create<ORM::PQ::CreativeCategory_Creative>();
      FAIL_CONTEXT(
        AutoTest::predicate_checker(
          cc_category->insert(
            category->id(),
            fetch_int("CREATE/CREATIVE"))),
        description + 
          "Cann't create link to category#" + strof(i+1));
    }

    // Initialize checker for category
    add_checker(
      description + " Creative category#" + strof(i+1),
      CreativeCategoryWaitChecker(
        CreativeCategoryChecker(
          this,
          category->id(),
          CreativeCategoryChecker::Expected().
          creative_category_id(
            category->id()))));
  }

  // Initialize checker for creative categories
  std::ostringstream expected_categories;

  std::list<unsigned long>::iterator last = --cat_ids.end();
  
  std::copy(cat_ids.begin(), last,
    std::ostream_iterator<unsigned long>(
      expected_categories, ","));

  expected_categories << *last;

  add_checker(
    description + " Creative categories",
    CreativeWaitChecker(
      CreativeChecker(
        this,
        fetch_int("CREATE/CC"),
        CreativeChecker::Expected().
          categories(expected_categories.str()))));

}

void CreativeCategoryGranularUpdateTest::unlink_category_()
{
  std::string description("Unlink creative category.");
  add_descr_phrase(description);

  unsigned long tag_category = fetch_int("UNLINK/CATEGORY/Tags");
  unsigned long cc = fetch_int("UNLINK/CC");

  FAIL_CONTEXT(
    CreativeCategoryWaitChecker(
      CreativeCategoryChecker(
        this,
        tag_category,
        CreativeCategoryChecker::Expected().
          creative_category_id(tag_category))).check(),
    description + " Initial category");

  FAIL_CONTEXT(
    CreativeWaitChecker(
      CreativeChecker(
        this,
        cc,
        CreativeChecker::Expected().
          categories(strof(tag_category)))).check(),
    description + " Initial creative");

  ORM::ORMRestorer<ORM::PQ::CreativeCategory>* category =
    create<ORM::PQ::CreativeCategory>(tag_category);

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      category->delet()),
    description +
      " Cann't delete tags category");

  add_checker(
    description + " Creative tags category",
    CreativeCategoryWaitChecker(
      CreativeCategoryChecker(
        this,
        tag_category,
        CreativeCategoryChecker::Expected(),
        AutoTest::AEC_NOT_EXISTS)));

  add_checker(
    description + " Creative categories",
    CreativeWaitChecker(
      CreativeChecker(
        this,
        cc,
        CreativeChecker::Expected().categories(""))));
}

void CreativeCategoryGranularUpdateTest::add_tags_exclusion_()
{
  std::string description("Add tag exclusion category.");
  add_descr_phrase(description);

  unsigned long cc = fetch_int("ADDEXCLUSION/CC");
  unsigned long category = fetch_int("ADDEXCLUSION/EXCLUSIONCATEGORY");

  {
    std::string expected_categories =
      map_objects(
        "ADDEXCLUSION/CATEGORY/Visual,"
        "ADDEXCLUSION/CATEGORY/Content");

    FAIL_CONTEXT(
      CreativeWaitChecker(
        CreativeChecker(
          this,
          cc,
          CreativeChecker::Expected().
            categories(expected_categories))).check(),
      description + " Initial creative");
  }

  // Test request
  NSLookupRequest request;
  request.referer_kw = fetch_string("ADDEXCLUSION/KWD");
  request.tid = fetch_string("ADDEXCLUSION/TAG");
    
  AdClient client(AdClient::create_user(this));

  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client, request, cc).check(),
    description + " Initial request for CC");

  ORM::ORMRestorer<ORM::PQ::CreativeCategory_Creative>* cc_category =
    create<ORM::PQ::CreativeCategory_Creative>();
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      cc_category->insert(
        category,
        fetch_int("ADDEXCLUSION/CREATIVE"))),
    description + 
      "Cann't create link to exclusion category");


  {
    std::string expected_categories =
      map_objects(
        "ADDEXCLUSION/CATEGORY/Visual,"
        "ADDEXCLUSION/CATEGORY/Content,"
        "ADDEXCLUSION/EXCLUSIONCATEGORY");

    add_checker(
      description + " Creative creative",
      CreativeWaitChecker(
        CreativeChecker(
          this,
          cc,
          CreativeChecker::Expected().
            categories(expected_categories))));
  }

  add_checker(
    description + " Request for CC",
    SelectedCreativeChecker(client, request, 0));
}

void CreativeCategoryGranularUpdateTest::del_tags_exclusion_()
{
  std::string description("Delete tag exclusion category.");
  add_descr_phrase(description);

  unsigned long cc = fetch_int("DELEXCLUSION/CC");
  unsigned long category = fetch_int("DELEXCLUSION/EXCLUSIONCATEGORY");

  {
    std::string expected_categories =
      map_objects(
        "DELEXCLUSION/CATEGORY/Visual,"
        "DELEXCLUSION/CATEGORY/Content,"
        "DELEXCLUSION/EXCLUSIONCATEGORY");

    FAIL_CONTEXT(
      CreativeWaitChecker(
        CreativeChecker(
          this,
          cc,
          CreativeChecker::Expected().
            categories(expected_categories))).check(),
      description + " Initial creative");
  }

  // Test request
  NSLookupRequest request;
  request.referer_kw = fetch_string("DELEXCLUSION/KWD");
  request.tid = fetch_string("DELEXCLUSION/TAG");
    
  AdClient client(AdClient::create_user(this));

  FAIL_CONTEXT(
    SelectedCreativeChecker(
      client, request, 0).check(),
    description + " Initial request for CC");

  ORM::PQ::CreativeCategory_Creative cc_category_(
    pq_conn_,
    category,
    fetch_int("DELEXCLUSION/CREATIVE"));

  ORM::ORMRestorer<ORM::PQ::CreativeCategory_Creative>* cc_category =
    create(
      ORM::PQ::CreativeCategory_Creative(
        pq_conn_,
        category,
        fetch_int("DELEXCLUSION/CREATIVE")));
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      cc_category->delet()),
    description + 
      "Cann't unlink from exclusion category");

  {
    std::string expected_categories =
      map_objects(
        "DELEXCLUSION/CATEGORY/Visual,"
        "DELEXCLUSION/CATEGORY/Content");

    add_checker(
      description + " Creative creative",
      CreativeWaitChecker(
        CreativeChecker(
          this,
          cc,
          CreativeChecker::Expected().
            categories(expected_categories))));
  }

  add_checker(
    description + " Request for CC",
    SelectedCreativeChecker(client, request, cc));
}
 
 
