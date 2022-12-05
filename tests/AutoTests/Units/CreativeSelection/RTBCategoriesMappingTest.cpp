
#include "RTBCategoriesMappingTest.hpp"

REFLECT_UNIT(RTBCategoriesMappingTest) (
  "CreativeSelection",
  AUTO_TEST_FAST );

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::OpenRTBRequest OpenRTBRequest;
  typedef AutoTest::UserBindRequest UserBindRequest;
  typedef AutoTest::TanxRequest TanxRequest;
  typedef AutoTest::OpenRTBResponseChecker OpenRTBResponseChecker;
  typedef AutoTest::TanxResponseChecker TanxResponseChecker;

  const unsigned long FIELD_NOT_EXIST = std::numeric_limits<unsigned long>::max();
  const char EMPTY[] = "empty";

  template<typename T>
  struct FetchTypeMapping
  {
    typedef T FetchType;
  };

  template<>
  struct FetchTypeMapping<const std::string&>
  {
    typedef std::string FetchType;
  };

  template<>
  struct FetchTypeMapping<unsigned int>
  {
    typedef unsigned long FetchType;
  };

  template<>
  struct FetchTypeMapping<int>
  {
    typedef unsigned long FetchType;
  };

  struct RTBTraits
  {
    typedef AutoTest::OpenRTBRequest Request;
    typedef AutoTest::OpenRTBResponseChecker Checker;
    typedef AutoTest::OpenRTBResponseChecker::ExpectedList Expected;
  };

  struct TanxTraits
  {
    typedef AutoTest::TanxRequest Request;
    typedef AutoTest::TanxResponseChecker Checker;
    typedef AutoTest::TanxResponseChecker::Expected Expected;
  };

  struct BaiduTraits
  {
    typedef AutoTest::BaiduRequest Request;
    typedef AutoTest::BaiduResponseChecker Checker;
    typedef AutoTest::BaiduResponseChecker::Expected Expected;
  };

  const char BODY[] = "body";
  const char BODY_OPENX[] = "body-openx";
  const char BODY_ALLYES[] = "body-allyes";

  const RTBCategoriesMappingTest::RTBTestCase<1> TEST1[] =
  {
    /*1*/
    {
      "2 visual categories with the same IAB key",
      BODY, {{ "250x250", 0 }}, 0, {{ "RTB/250x250", "vis-3/iab", 0 }}
    },
    /*2*/
    {
      "visual categories w/o IAB key",
      BODY, {{ "300x250", 0 }}, 0,
      {{ "RTB/300x250", EMPTY, 0 }}
    },
    /*3*/
    { "2 visual categories with different IAB keys",
      BODY, {{ "160x600", 0 }}, 0,
      {{ "RTB/160x600", "vis-1/iab,vis-2/iab", 0 }}
    },
    /*4*/
    { "OpenX: 2 content categories with the same",
      BODY_OPENX, {{ "250x250", 0 }}, 0,
      {{ "RTB/250x250", "vis-3/openx", "cnt-3/openx" }}
    },
    /*5*/
    { "OpenX: 2 content categories with different",
      BODY_OPENX, {{ "240x400", 0 }}, 0,
      {{ "RTB/240x400", "vis-1/openx", "cnt-1/openx,cnt-2/openx" }}
    },
    /*6*/
    { "Visual category not excluded",
      BODY, {{ "120x240", "vis-1/iab" }}, 0,
      {{ "RTB/120x240", EMPTY, 0 }}
    },
    /*7*/
    { "Visual category excluded (string IAB value)",
      BODY, {{ "120x240", "vis-1-1/iab" }}, 0,
      {{ "RTB-RON/120x240", EMPTY, 0 }}
    },
    /*8*/
    { "Content category not excluded",
      BODY, {{ "120x600", 0 }}, "cnt-1/iab",
      {{ "RTB/120x600", EMPTY, 0 }}
    },
    /*9*/
    { "Content category excluded",
      BODY, {{ "120x600", 0 }}, "cnt-1-1/iab",
      {{ "RTB-RON/120x600", EMPTY, 0 }}
    },
    /*10*/
    { "2 visual category in creative, excluded by 1",
      BODY, {{ "160x600", "vis-2/iab" }}, 0,
      {{ "RTB-RON/160x600", EMPTY, 0 }}
    },
    /*11*/
    { "2 content category in creative, excluded by 1",
      BODY, {{ "240x400", 0 }}, "cnt-2/iab",
      {{ "RTB-RON/240x400", EMPTY, 0 }}
    },
    /*12*/
    { "multiple visual categories in request, excluded by one",
      BODY, {{ "250x250", "vis-6/iab,vis-3/iab,vis-7/iab" }}, 0,
      {{ "RTB-RON/250x250", EMPTY, 0 }}
    },
    /*13*/
    { "multiple contentcategories in request, excluded by one",
      BODY, {{ "250x250", 0 }}, "cnt-6/iab,cnt-7/iab,cnt-3/iab",
      {{ "RTB-RON/250x250", EMPTY, 0 }}
    },
    /*14*/
    { "Visual category passed in content request field",
      BODY, {{ "120x240", 0 }}, "vis-1-1/iab",
      {{ "RTB-RON/120x240", EMPTY, 0 }}
    },
    /*15*/
    { "Content category passed in visual request field",
      BODY, {{ "120x600", "cnt-1-1/iab" }}, 0,
      {{ "RTB-RON/120x600", EMPTY, 0 }}
    },
    /*16*/
    { "Tanx id in visual request field",
      BODY, {{ "160x600", "vis-1/tanx" }}, 0,
      {{ "RTB/160x600", "vis-1/iab,vis-2/iab", 0 }}
    },
    /*17*/
    { "Tanx id in content request field",
      BODY, {{ "160x600", 0 }}, "cnt-1/tanx",
      {{ "RTB/160x600", "vis-1/iab,vis-2/iab", 0 }}
    },
    /*18*/
    { "OpenX id in visual request field (openx request type)",
      BODY_OPENX, {{ "160x600", "vis-1/openx" }}, 0,
      {{ "RTB-RON/160x600", EMPTY, EMPTY }}
    },
    /*19*/
    { "IAB id in visual request field (openx request type)",
      BODY_OPENX, {{ "160x600", "vis-1/iab" }}, 0,
      {{ "RTB/160x600", "vis-1/openx,vis-2/openx" , "cnt-1/openx" }}
    },
    /*20*/
    { "IAB id in content request field (openx request type)",
      BODY_OPENX, {{ "160x600", 0 }}, "cnt-1/iab",
      {{ "RTB/160x600", "vis-1/openx,vis-2/openx", "cnt-1/openx" }}
    },
    /*21*/
    { "OpenX id in content request field (openx request type)",
      BODY_OPENX, {{ "160x600", 0 }}, "cnt-1/openx",
      {{ "RTB-RON/160x600", EMPTY, EMPTY }}
    },
    /*22*/
    { "Special json characters in request: Creative matching integrity check",
      BODY, {{ "728x90", 0 }}, 0,
      {{ "RTB/728x90", EMPTY, 0 }}
    },
    /*23*/
    { "Special json characters in request: \"'\" char",
      BODY, {{ "728x90", 0 }}, "cnt-s-1/iab",
      {{ "RTB-RON/728x90", EMPTY, 0 }}
    },
    /*24*/
    { "Special json characters in request: '\' char", 
      BODY, {{ "728x90", 0 }}, "cnt-s-2/iab",
      {{ "RTB-RON/728x90", EMPTY, 0 }}
    },
    /*25*/
    { "Special json characters in request: 'ы' char", 
      BODY, {{ "728x90", 0 }}, "cnt-s-3/iab",
      {{ "RTB-RON/728x90", EMPTY, 0 }}
    },
    /*26*/
    { "Special json characters in request: '\\n' char", 
      BODY, {{ "728x90", 0 }}, "cnt-s-4/iab",
      {{ "RTB-RON/728x90", EMPTY, 0 }}
    },
    /*27*/
    { "Text ads: response filling",
      BODY, {{ "468x61", 0 }}, 0,
      {{ "TEXT/468x61", "text/iab", 0 }}
    },
    /*28*/
    { "Text ads: visual category filtration",
      BODY, {{ "468x61", "text/iab" }}, 0,
      {{ "RTB-RON/468x61", EMPTY, 0 }}
    }
  };

  const RTBCategoriesMappingTest::AllyesTestCase<1> ALLYES_TEST1[] =
  {
    /*1*/
    { "Allyes: visual & content categories w/o allyes key",
      BODY_ALLYES, {{ "120x240", 0 }}, 0,
      {{ "RTB/120x240", FIELD_NOT_EXIST, EMPTY }}
    },
    /*2*/
    { "Allyes: 2 visual categories with different allyes keys",
      BODY_ALLYES, {{ "160x600", 0 }}, 0,
      {{ "RTB/160x600", 1,  "cnt-1/allyes"}}
    },
    /*3*/
    { "Allyes: 2 content categories with different allyes keys",
      BODY_ALLYES, {{ "240x400", 0 }}, 0,
      {{ "RTB/240x400", 1, "cnt-1/allyes,cnt-2/allyes" }}
    },
    /*4*/
    { "Allyes: visual & content categories with the same allyes keys",
      BODY_ALLYES, {{ "250x250", 0 }}, 0,
      {{ "RTB/250x250", 34, "cnt-4/allyes" }}
    },
    /*5*/
    { "Allyes: 2 visual & 2 content categories; 1st category (with lower id) has no allyes key",
      BODY_ALLYES, {{ "350x180", 0 }}, 0,
      {{ "RTB/350x180", 1, "cnt-1/allyes" }}
    },
    /*6*/
    { "Allyes: visual category exclusion via banner.battr",
      BODY_ALLYES, {{ "160x600", "vis-1/allyes" }}, 0,
      {{ "RTB-RON/160x600", FIELD_NOT_EXIST, EMPTY }}
    },
    /*7*/
    { "Allyes: visual category exclusion via banner.battr",
      BODY_ALLYES, {{ "240x400", 0 }}, "cnt-1/allyes",
      {{ "RTB-RON/240x400", FIELD_NOT_EXIST, EMPTY }}
    }
  };
  
  const RTBCategoriesMappingTest::RTBTestCase<2> TEST2[] =
  {
    /*1*/
    { "Creative matching integrity check",
      BODY, {{ "120x240", 0 }, { "160x600", 0 }}, 0,
      {{ "RTB/120x240", EMPTY, EMPTY },
       { "RTB/160x600", "vis-1/iab,vis-2/iab", EMPTY }}
    },
    /*2*/
    { "banner.battr should be applied to own banner only",
      BODY,
      {{ "120x240", "vis-1/iab" },
       { "160x600", "vis-1-1/iab" }}, 0,
      {{ "RTB/120x240", EMPTY, EMPTY },
       { "RTB/160x600", "vis-1/iab,vis-2/iab", EMPTY }}
    },
    /*3*/
    { "banner.battr should be applied to own banner only",
      BODY,
      {{ "120x240", "vis-1-1/iab" },
       { "160x600", "vis-1/iab" }}, 0,
      {{ "RTB-RON/120x240", EMPTY, EMPTY },
       { "RTB-RON/160x600", EMPTY, EMPTY }}  },
    /*5*/
    { "OpenX ad_ox_cats array filling",
      BODY_OPENX,
      {{ "240x400", 0 }, { "160x600", 0 }}, 0, // request
      {{ "RTB/240x400", "vis-1/openx", "cnt-1/openx,cnt-2/openx" },
       { "RTB/160x600", "vis-1/openx,vis-2/openx", "cnt-1/openx" }} } // response
  };

  const RTBCategoriesMappingTest::AllyesTestCase<2> ALLYES_TEST2[] =
  {
    /*6*/
    { "Creative matching integrity check",
      BODY_ALLYES, {{ "240x400", 0 }, { "160x600", 0 }}, 0,
      {{ "RTB/240x400", 1, "cnt-1/allyes,cnt-2/allyes" },
       { "RTB/160x600", 1, "cnt-1/allyes" }}
    }
  };
  
  const RTBCategoriesMappingTest::RTBTestCase<3> TEST2_4[] =
  {
    /*4*/
    { "TanxRequest.bcat should be applied to all banners",
      BODY,
      {{ "240x400", 0 }, { "120x240", 0 }, { "120x600", 0 }}, "cnt-1-1/iab,cnt-2/iab", // request
      // response
      {{ "RTB-RON/240x400", EMPTY, EMPTY },
       { "RTB/120x240", EMPTY, EMPTY },
       { "RTB-RON/120x600", EMPTY, EMPTY }} 
    } 
  };
  
// a:* - for excluded_ad_category request param filling
// s:* - for excluded_sensitive_category request param filling
  const RTBCategoriesMappingTest::TanxTestCase TEST3[] =
  {
    /*1*/
    { "2 categories with the same TanX key",
      { "250x250", 0 }, 0,
      { "TANX/250x250", "vis-3/tanx", "cnt-3/tanx" }
    },
    /*2*/
    { "categories w/o TanX key",
      { "300x250", 0 }, 0, { "TANX/300x250", EMPTY, EMPTY }
    },
    /*3*/
    { "2 visual categories with TanX key",
      { "160x600", 0 }, 0,
      { "TANX/160x600", "vis-1/tanx,vis-2/tanx", "cnt-1/tanx" }
    },
    /*4*/
    { "2 content categories with Tanx key",
      { "240x400", 0 }, 0,
      { "TANX/240x400", "vis-1/tanx", "cnt-1/tanx,cnt-2/tanx" }
    },
    /*5*/
    { "120x240 creative integrity check",
      { "120x240", 0 }, 0,
      { "TANX/120x240", "vis-1-2/tanx", EMPTY }
    },
    /*6*/
    { "120x600 creative integrity check",
      { "120x600", 0 }, 0,
      { "TANX/120x600", EMPTY, "cnt-1-2/tanx" }
    },
    /*7*/
    { "excluded by visual category",
      { "120x240", "vis-1-2/tanx" }, 0,
      { "TANX-RON/120x240", 0, 0 }
    },
    /*8*/
    { "excluded by content category in excluded_sensitive_category field",
      { "120x600", 0 },
      "s:cnt-1-2/tanx", { "TANX-RON/120x600", 0, 0 }
    },
    /*9*/
    { "excluded by content category in excluded_ad_category field",
      { "120x600", 0 },
      "a:cnt-1-2/tanx", { "TANX-RON/120x600", 0, 0 }
    },
    /*10*/
    { "multiple visual categories in request, excluded by one",
      { "160x600", "vis-7/tanx,vis-1/tanx" }, 0,
      { 0, 0, 0 }
    },
    /*11*/
    { "multiple content categories in request, excluded by one",
      { "240x400", 0 },
      "s:cnt-6/tanx,s:cnt-2/tanx,s:cnt-7/tanx", { 0, 0, 0 }
    },
    /*12*/
    { "multiple content categories in 2 request fields, excluded by one",
      { "240x400", 0 },
      "a:cnt-6/tanx,s:cnt-1/tanx", { 0, 0, 0 }
    },
    /*13*/
    { "multiple content categories in 2 request fields, excluded by one",
      { "240x400", 0 },
      "a:cnt-1/tanx,s:cnt-6/tanx", { 0, 0, 0 }
    },
    /*14*/
    { "IAB visual category key",
      { "160x600", "vis-1/iab" }, 0,
      { "TANX/160x600", 0, 0 }
    },
    /*15*/
    { "OpenX visual category key",
      { "160x600", "vis-1/openx" },
      0, { "TANX/160x600", 0, 0 }
    },
    /*16*/
    { "OpenX content category key",
      { "160x600", 0 },
      "s:cnt-1/openx", { "TANX/160x600", 0, 0 }
    }
  };

  const RTBCategoriesMappingTest::BaiduTestCase TEST4_1[] =
  {
    {
      "2 categories with the same Baidu key",
      { "240x400", 0 },
      { "BAIDU/240x400", "vis-1/baidu", "cnt-3/baidu" }
    },
    {
      "Categories w/o Baidu key",
      { "300x250", 0 },
      { "BAIDU/300x250", 0, 0 }
    },
    {
      "2 visual categories with Baidu key",
      { "250x250", 0 },
      { "BAIDU/250x250", "vis-3/baidu", "cnt-1/baidu" }
    },
    {
      "2 content categories with Baidu key",
      { "336x280", 0 },
      { "BAIDU/336x280", "vis-3/baidu", "cnt-3/baidu" }
    }
  };

  const RTBCategoriesMappingTest::BaiduTestCase TEST4_2[] =
  {
    {
      "Excluded by content category",
      { "250x250", "cnt-1/baidu" },
      { 0, 0, 0 }
    },
    {
      "Excluded by visual category",
      { "250x250", "vis-2/baidu" },
      { 0, 0, 0 }
    },
    {
      "Multiple content categories in request, excluded by one",
      { "250x250", "unexist,cnt-1/baidu" },
      { 0, 0, 0 }
    },
    {
      "Multiple content categories in request, excluded by one",
      { "250x250", "unexist" },
      { "BAIDU/250x250", "vis-3/baidu", "cnt-1/baidu" }
    }
  };
}

template<typename Object, typename Param>
void RTBCategoriesMappingTest::set_category(
  Object& obj,
  Param Object::* setter,
  const std::string& cat)
{
  typedef typename Param::Type Type;
  typename FetchTypeMapping<Type>::FetchType value;
  fetch(value, "CATEGORIES/" + cat);
  (obj.*setter)(value);
}

template<typename Object, typename RetVal, typename T>
void
RTBCategoriesMappingTest::set_category(
  Object& obj,
  RetVal (Object::*setter)(T),
  const std::string& cat)
{
  typename FetchTypeMapping<T>::FetchType value;
  fetch(value, "CATEGORIES/" + cat);
  (obj.*setter)(value);
}

template<typename Object, typename Setter>
void RTBCategoriesMappingTest::set_categories(
  const char* cat_list,
  Object& obj,
  Setter setter)
{
  if (cat_list)
  {
    String::SubString cat;
    String::SubString list(cat_list);
    String::StringManip::SplitComma tokenizer(list);
    while (tokenizer.get_token(cat))
    {
      String::StringManip::trim(cat);
      set_category(obj, setter, cat.str());
    }
  }
}

template<typename Object, typename Setter, typename Touch>
void
RTBCategoriesMappingTest::set_expected_categories(
  const char* cat_list,
  Object& obj,
  Setter setter,
  Touch touch)
{
  if (cat_list)
  {
    if (strcmp(cat_list, EMPTY) == 0)
    {
      (obj.*touch)();
    }
    else
    {
      set_categories(cat_list, obj, setter);
    }
  }
}

void
RTBCategoriesMappingTest::prepare_request(
  TanxRequest& request,
  const TanxTestCase& test_case)
{
  request.
    url(fetch_string("SEARCH")).
    debug_size(fetch_string(std::string("Sizes/") + test_case.banner.size)).
    debug_ccg(fetch_string("TANX/CCG_ID")).
    aid(fetch_string("TanX/ACCOUNT_ID")).
    min_cpm_price(1);

  if (test_case.banner.battr)
  {
    set_categories(
      test_case.banner.battr,
      request,
      &TanxRequest::excluded_filter);
  }
  if (test_case.bcat)
  {
    String::SubString ccat;
    String::StringManip::SplitComma tokenizer(String::SubString(test_case.bcat));
    while (tokenizer.get_token(ccat))
    {
      String::SubString::SizeType sep = ccat.find(':');
      if (sep == String::SubString::NPOS)
      {
        request.excluded_ad_category(fetch_int("CATEGORIES/" + ccat.str()));
      }
      else
      {
        std::string excl_field = ccat.substr(0, sep).str();
        int cat_id = fetch_int("CATEGORIES/" + ccat.substr(sep + 1).str());
        if (excl_field == "a")
        {
          request.excluded_ad_category(cat_id);
        }
        else if (excl_field == "s")
        {
          request.excluded_sensitive_category(cat_id);
        }
      }
    }
  }
}

void
RTBCategoriesMappingTest::prepare_request(
  BaiduRequest& request,
  const BaiduTestCase& test_case)
{
  request.
    debug_ccg(fetch_string("BAIDU/CCG_ID")).
    aid(fetch_string("Baidu/ACCOUNT_ID")).
    url(fetch_string("SEARCH")).
    id("BAIDU").
    minimum_cpm(1);

  if (test_case.banner.size)
  {
    request.debug_size(
      fetch_string(std::string("Sizes/") + test_case.banner.size));
  }

  if (test_case.banner.battr)
  {
    set_categories(
      test_case.banner.battr,
      request,
      &BaiduRequest::excluded_product_category);
  }
}

template<typename TestCase>
void
RTBCategoriesMappingTest::prepare_request(
  OpenRTBRequest& request,
  const TestCase& test_case)
{
  request.
    referer(fetch_string("SEARCH")).
    min_cpm_price(0).
    debug_ccg(fetch_string("TEXT/CCG_ID")).
    src(test_case.source).
    aid(
      strcmp(test_case.source, BODY) == 0?
        fetch_string("iab/ACCOUNT_ID"):
          fetch_string("OpenX/ACCOUNT_ID"));

  if (test_case.bcat)
  {
    set_categories(
      test_case.bcat,
      request,
      &OpenRTBRequest::bcat);
  }

  for (size_t slot = 0; slot < countof(test_case.banners); ++slot)
  {
    request.debug_size[slot + 1] =
      fetch_string(std::string("Sizes/") + test_case.banners[slot].size);

    set_categories(test_case.banners[slot].battr,
      request.imp[slot], &OpenRTBRequest::ImpGroup::battr);
  }
}

template<size_t Slots>
void
RTBCategoriesMappingTest::prepare_checker(
  OpenRTBResponseChecker::ExpectedList& expected,
  const RTBTestCase<Slots>& test_case)
{
  for (size_t slot = 0; slot < Slots; ++slot)
  {
    OpenRTBResponseChecker::Expected e;
    if (test_case.bids[slot].cc_id)
    {
      unsigned long cc_id = fetch_int(
        std::string("CREATIVEIDS/") + test_case.bids[slot].cc_id);
      e.adid(cc_id);
    }
    if (test_case.bids[slot].visual_categories)
    {
      set_expected_categories(
        test_case.bids[slot].visual_categories,
        e,
        &OpenRTBResponseChecker::Expected::attr,
        &OpenRTBResponseChecker::Expected::attr_checked);
    }
    else
    {
       e.attr_exist(false);
    }
    if (test_case.bids[slot].content_categories &&
      (strcmp(test_case.source, BODY_OPENX) == 0))
    {
      set_expected_categories(
        test_case.bids[slot].content_categories,
        e,
        &OpenRTBResponseChecker::Expected::ad_ox_cats,
        &OpenRTBResponseChecker::Expected::ad_ox_cats_checked);
    }
    else
    {
      e.ad_ox_cats_exist(false);
    }
    expected.push_back(e);
  }
}

template<size_t Slots>
void
RTBCategoriesMappingTest::prepare_checker(
  OpenRTBResponseChecker::ExpectedList& expected,
  const AllyesTestCase<Slots>& test_case)
{
   for (size_t slot = 0; slot < Slots; ++slot)
  {
    OpenRTBResponseChecker::Expected e;
    if (test_case.bids[slot].cc_id)
    {
      unsigned long cc_id = fetch_int(
        std::string("CREATIVEIDS/") + test_case.bids[slot].cc_id);
      e.adid(cc_id);
    }
    if (test_case.bids[slot].cat)
    {
      set_expected_categories(
        test_case.bids[slot].cat,
        e,
        &OpenRTBResponseChecker::Expected::cat,
        &OpenRTBResponseChecker::Expected::cat_checked);
    }
    else
    {
      e.cat_exist(false);
    }
    if (test_case.bids[slot].fmt != FIELD_NOT_EXIST)
    {
      e.fmt(test_case.bids[slot].fmt);
    }
    else
    {
      e.fmt_exist(false);
    }
    expected.push_back(e);
  }
}

void
RTBCategoriesMappingTest::prepare_checker(
  TanxResponseChecker::Expected& expected,
  const TanxTestCase& test_case)
{
  // NOTE: empty Expected object means that we expects no bids;
  if (test_case.bid.cc_id)
  {
    std::string creative_id = fetch_string(
      std::string("CREATIVE/") + test_case.bid.cc_id  + "/TANX");
    expected.creative_id(creative_id);
  }
  
  set_expected_categories(
    test_case.bid.visual_categories,
    expected,
    &TanxResponseChecker::Expected::creative_type,
    &TanxResponseChecker::Expected::creative_type_checked);

  set_expected_categories(
    test_case.bid.content_categories,
    expected,
    &TanxResponseChecker::Expected::category,
    &TanxResponseChecker::Expected::category_checked);
}

void
RTBCategoriesMappingTest::prepare_checker(
  BaiduResponseChecker::Expected& expected,
  const BaiduTestCase& test_case)
{
  // NOTE: empty Expected object means that we expects no bids;
  if (test_case.bid.cc_id)
  {
    unsigned long creative_id = fetch_int(
      std::string("CREATIVE/") + test_case.bid.cc_id + "/BAIDU");
    expected.creative_id(creative_id);

    if (!test_case.bid.visual_categories)
    {
      expected.type_exist(false);
    }
    
    if (!test_case.bid.content_categories)
    {
      expected.category_exist(false);
    }
  }
  if (test_case.bid.visual_categories)
  {
    expected.type(
      fetch_int(
        std::string("CATEGORIES/") +
        test_case.bid.visual_categories));
  }
  if (test_case.bid.content_categories)
  {
    expected.category(
      fetch_int(
        std::string("CATEGORIES/") +
        test_case.bid.content_categories));
  }
}

template<typename Traits, typename CaseType, size_t Cases>
void RTBCategoriesMappingTest::perform_case_(
  const CaseType (&cases)[Cases])
{
  AdClient client(AdClient::create_nonoptin_user(this));

  for (size_t i = 0; i < Cases; ++i)
  {
    // Prepare request;
    typename Traits::Request request;
    prepare_request(request, cases[i]);
      
    // Send request
    client.process_post(request);
    // Check response

    typename Traits::Expected expected;

    prepare_checker(expected, cases[i]);
      
    NOSTOP_FAIL_CONTEXT(
      typename Traits::Checker(
        client, expected).check(),
      strof(i+1) + ". " + cases[i].description);
  }
}

bool
RTBCategoriesMappingTest::run_test()
{
  AUTOTEST_CASE(
    perform_case_<RTBTraits>(TEST1),
    "OpenRTB single slot requests");

  AUTOTEST_CASE(
    perform_case_<RTBTraits>(ALLYES_TEST1),
    "OpenRTB single slot requests");

  AUTOTEST_CASE(
    perform_case_<RTBTraits>(TEST2),
    "OpenRTB multi-slot requests");

  AUTOTEST_CASE(
    perform_case_<RTBTraits>(ALLYES_TEST2),
    "OpenRTB multi-slot requests");
  
  AUTOTEST_CASE(
    perform_case_<RTBTraits>(TEST2_4),
    "OpenRTB multi-slot requests");
    
  AUTOTEST_CASE(
    perform_case_<TanxTraits>(TEST3),
    "TanX categories exclusion");

  AUTOTEST_CASE(
    perform_case_<BaiduTraits>(TEST4_1),
    "Baidu categories exclusion");

  AUTOTEST_CASE(
    perform_case_<BaiduTraits>(TEST4_2),
    "Baidu categories exclusion");
  
  return true;
}
