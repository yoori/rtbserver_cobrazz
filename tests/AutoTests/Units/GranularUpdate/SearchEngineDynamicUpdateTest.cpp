#include "SearchEngineDynamicUpdateTest.hpp"
 
REFLECT_UNIT(SearchEngineDynamicUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef ORM::PQ::SearchEngine SearchEngine;
  typedef AutoTest::SearchEngineChecker SearchEngineChecker;
  typedef AutoTest::WaitChecker<SearchEngineChecker> SearchEngineWaitChecker;
}

void
SearchEngineDynamicUpdateTest::add_scenario()
{
  std::string description("New engine.");
  add_descr_phrase(description);

  ORM::ORMRestorer<SearchEngine>* engine =
    create<SearchEngine>();

  engine->decoding_depth = fetch_int("ADDENGINE/DEC_DEPTH");
  engine->encoding = fetch_string("ADDENGINE/ENCODING");
  engine->host = fetch_string("ADDENGINE/HOST");
  engine->name = fetch_string("ADDENGINE/NAME");
  engine->post_encoding =  fetch_string("ADDENGINE/POST_ENCODING");
  engine->regexp = fetch_string("ADDENGINE/REGEXP");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      engine->insert()),
    "must insert new search engine");

  add_checker(
    description,
    SearchEngineWaitChecker(
      SearchEngineChecker(
        this,
        engine->search_engine_id(),
        SearchEngineChecker::Expected().
          id(strof(engine->search_engine_id())).
          encoding(fetch_string("ADDENGINE/ENCODING")).
          decoding_depth(
            fetch_string("ADDENGINE/DEC_DEPTH")).
          regexp("\\Q" +
            fetch_string("ADDENGINE/REGEXP") +
              "\\E"))));
}

template<typename AdminField, typename ValueType>
void
SearchEngineDynamicUpdateTest::update_scenario(
  const std::string& description,
  unsigned long engine_id,
  AdminField admin_field,
  ExpectedSetter diff_setter,
  const ValueType& old_value,
  const ValueType& new_value)
 {
   add_descr_phrase(description);

   ORM::ORMRestorer<SearchEngine>* engine =
     create<SearchEngine>(engine_id);

   {
     SearchEngineChecker::Expected expected;

     (expected.*(diff_setter))(
       "\\Q" + strof(old_value) + "\\E");
     
     FAIL_CONTEXT(
       SearchEngineWaitChecker(
         SearchEngineChecker(
           this,
           engine_id,
           expected)).check(),
       description + " Initial");
     
   }

   engine->*(admin_field) = new_value;

   FAIL_CONTEXT(
     AutoTest::predicate_checker(
       engine->update()),
     "must update search engine");

   {
     SearchEngineChecker::Expected expected;

     (expected.*(diff_setter))(
       "\\Q" + strof(new_value) + "\\E");
     
     add_checker(
       description,
       SearchEngineWaitChecker(
         SearchEngineChecker(
           this,
           engine_id,
           expected)));
   }
 }

void
SearchEngineDynamicUpdateTest::delete_scenario()
{
  std::string description("Delete engine.");
  add_descr_phrase(description);

  FAIL_CONTEXT(
    SearchEngineWaitChecker(
      SearchEngineChecker(
        this,
        fetch_int("DELENGINE/ENGINE"),
        SearchEngineChecker::Expected().
          id(fetch_string("DELENGINE/ENGINE")))).check(),
    description + " Initial");

  ORM::ORMRestorer<SearchEngine>* engine =
    create<SearchEngine>(fetch_int("DELENGINE/ENGINE"));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      engine->delet()),
    "must delete search engine");

  add_checker(
    description,
    SearchEngineWaitChecker(
      SearchEngineChecker(
        this,
        fetch_int("DELENGINE/ENGINE"),
        SearchEngineChecker::Expected(),
        AutoTest::AEC_NOT_EXISTS)));
}

bool 
SearchEngineDynamicUpdateTest::run()
{
  NOSTOP_FAIL_CONTEXT(add_scenario());
  
  NOSTOP_FAIL_CONTEXT(
    update_scenario(
      "Update regex.",
      fetch_int("UPDATEREGEX/ENGINE"),
      &AutoTest::ORM::PQ::SearchEngine::regexp,
      &SearchEngineChecker::Expected::regexp,
      fetch_string("UPDATEREGEX/OLD_REGEXP"),
      fetch_string("UPDATEREGEX/NEW_REGEXP")));
  
  NOSTOP_FAIL_CONTEXT(
    update_scenario(
      "Update encoding.",
      fetch_int("UPDATEENC/ENGINE"),
      &AutoTest::ORM::PQ::SearchEngine::encoding,
      &SearchEngineChecker::Expected::encoding,
      fetch_string("UPDATEENC/OLD_ENCODING"),
      fetch_string("UPDATEENC/NEW_ENCODING")));

  NOSTOP_FAIL_CONTEXT(
    update_scenario(
      "Update encoding.",
      fetch_int("UPDATEDEPTH/ENGINE"),
      &AutoTest::ORM::PQ::SearchEngine::decoding_depth,
      &SearchEngineChecker::Expected::decoding_depth,
      fetch_int("UPDATEDEPTH/OLD_DEPTH"),
      fetch_int("UPDATEDEPTH/NEW_DEPTH")));
  
  NOSTOP_FAIL_CONTEXT(delete_scenario());

  return true;
}

void
SearchEngineDynamicUpdateTest::set_up()
{
  add_descr_phrase("Setup.");
}

void
SearchEngineDynamicUpdateTest::tear_down()
{
  add_descr_phrase("Tear down.");
}

