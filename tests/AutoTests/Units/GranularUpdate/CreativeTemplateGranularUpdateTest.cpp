
#include "CreativeTemplateGranularUpdateTest.hpp"

REFLECT_UNIT(CreativeTemplateGranularUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  const char* template_type_name = "Text";
  typedef AutoTest::CreativeTemplatesChecker CreativeTemplatesChecker;
  typedef AutoTest::WaitChecker<CreativeTemplatesChecker> CreativeTemplatesWaitChecker;
  const char ASPECT[] = "CreativeTemplateGranularUpdateTest";
  const unsigned long INFO = Logging::Logger::INFO;
}

void CreativeTemplateGranularUpdateTest::set_up()
{
  add_descr_phrase("Setup");

  file1_    = fetch_string("File1");
  file2_    = fetch_string("File2");
  file3_    = fetch_string("File3");
  app_format_id_ = fetch_int ("AppFormat");
  size_300x250_id_       = fetch_int ("Size/300x250/ID");
  size_468x60_id_       = fetch_int ("Size/468x60/ID");

  format_name_  = fetch_string("AppFormatName");
  size_300x250_name_    = fetch_string("Size/300x250/NAME");
  size_468x60_name_    = fetch_string("Size/468x60/NAME");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_config().check_service(CTE_ALL, STE_CAMPAIGN_MANAGER)),
    "CampaignManager need for this test");
}

void CreativeTemplateGranularUpdateTest::testcase_add_template()
{
  std::string description("Adding new templates");
  add_descr_phrase(description);

  std::string template_name = fetch_string("InsertedTemplate/Template1/NAME");

  ORM::ORMRestorer<ORM::PQ::Template>* creative_template =
    create<ORM::PQ::Template>();

  creative_template->name   = template_name;
  creative_template->status = "A";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      creative_template->insert()),
    "inserting new template");

  ORM::ORMRestorer<ORM::PQ::Templatefile>* template_file1 =
    create<ORM::PQ::Templatefile>();

  template_file1->app_format_id = app_format_id_;
  template_file1->template_id = creative_template->template_id();
  template_file1->flags = 1;
  template_file1->size_id = size_300x250_id_;
  template_file1->template_file = file1_;
  template_file1->template_type = "T";

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      template_file1->insert()),
    "inserting template file #1");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      creative_template->select()),
    "selecting template");
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      template_file1->select()),
    "selecting template file #1");

  AutoTest::Logger::thlog().stream(INFO, ASPECT)
    << "\ntemplate db version before insertion template file #2: " << creative_template->version
    << "\ntemplate file #1 db version before insertion template file #2: " << template_file1->version;

  ORM::ORMRestorer<ORM::PQ::Templatefile>* template_file2 =
    create<ORM::PQ::Templatefile>();

  template_file2->app_format_id = app_format_id_;
  template_file2->template_id = creative_template->template_id();
  template_file2->flags = 1;
  template_file2->size_id = size_468x60_id_;
  template_file2->template_file = file2_;
  template_file2->template_type = "T";

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      template_file2->insert()),
    "inserting template file #2");

  // Select DB version
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      creative_template->select()),
    "selecting template");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      template_file1->select()),
    "selecting template file #1");
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      template_file2->select()),
    "selecting template file #2");

  AutoTest::Logger::thlog().stream(INFO, ASPECT)
    << "\ntemplate file #2 db version: " << template_file2->version
    << "\ntemplate db version after insertion template file #2: " << creative_template->version
    << "\ntemplate file #1 db version after insertion template file #2: " << template_file1->version;

  add_checker(
    description,
    CreativeTemplatesWaitChecker(
      CreativeTemplatesChecker(
        this,
        CreativeTemplatesChecker::Expected().
          creative_format(template_name).
          creative_size(size_300x250_name_).
          app_format(format_name_).
          template_file(AutoTest::ANY + file1_).
          type(template_type_name))));

  add_checker(
    description,
    CreativeTemplatesWaitChecker(
      CreativeTemplatesChecker(
        this,
        CreativeTemplatesChecker::Expected().
          creative_format(template_name).
          creative_size(size_468x60_name_).
          app_format(format_name_).
          template_file(AutoTest::ANY + file2_).
          type(template_type_name))));
}

void CreativeTemplateGranularUpdateTest::testcase_change_template()
{
  std::string description("Changing existing templates");
  add_descr_phrase(description);

  std::string template_name = fetch_string("ChangedTemplate/Template1/NAME");

  ORM::ORMRestorer<ORM::PQ::Template>* creative_template =
    create<ORM::PQ::Template>(
      fetch_int("ChangedTemplate/Template1/TEMPLATE_ID"));

  ORM::ORMRestorer<ORM::PQ::Templatefile>* template_file =
    create<ORM::PQ::Templatefile>(
      fetch_int("ChangedTemplate/Template1/TEMPLATE_FILE_ID"));

  FAIL_CONTEXT(
    CreativeTemplatesWaitChecker(
      CreativeTemplatesChecker(
        this,
        CreativeTemplatesChecker::Expected().
          creative_format(template_name).
          creative_size(size_300x250_name_).
          app_format(format_name_).
          template_file(AutoTest::ANY + file1_).
          type(template_type_name))).check(),
    description + " - initial");

  std::string template_name_new = template_name + "-new";
  creative_template->name = template_name_new;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      creative_template->update()),
    "updating template name");

  template_file->template_file = file3_;
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      template_file->update()),
    "updating template file");

  add_checker(
    description,
    CreativeTemplatesWaitChecker(
      CreativeTemplatesChecker(
        this,
        CreativeTemplatesChecker::Expected().
          creative_format(template_name_new).
          creative_size(size_300x250_name_).
          app_format(format_name_).
          template_file(AutoTest::ANY + file3_).
          type(template_type_name))));
}

void CreativeTemplateGranularUpdateTest::testcase_delete_template()
{
  std::string description("Deleting existing template file");
  add_descr_phrase(description);

  std::string template_name = fetch_string("DeletedTemplateFile/Template1/NAME");

  ORM::ORMRestorer<ORM::PQ::Templatefile>* template_file =
    create<ORM::PQ::Templatefile>(
      fetch_int("DeletedTemplateFile/Template1/TEMPLATE_FILE_ID"));

  FAIL_CONTEXT(
    CreativeTemplatesWaitChecker(
      CreativeTemplatesChecker(
        this,
        CreativeTemplatesChecker::Expected().
          creative_format(template_name).
          creative_size(size_300x250_name_).
          app_format(format_name_).
          template_file(AutoTest::ANY + file2_).
          type(template_type_name))).check(),
    description + " - initial");

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      template_file->delet()), "deleting template file");

  add_checker(
    description,
    CreativeTemplatesWaitChecker(
      CreativeTemplatesChecker(
        this,
        CreativeTemplatesChecker::Expected().
          creative_format(template_name).
          template_file(AutoTest::ANY + file2_),
        AutoTest::AEC_NOT_EXISTS)));
}

void CreativeTemplateGranularUpdateTest::testcase_del_status_template()
{
  std::string description("Set template status to 'D'");
  add_descr_phrase(description);

  std::string template_name = fetch_string("DeletedTemplate/Template1/NAME");

  ORM::ORMRestorer<ORM::PQ::Template>* creative_template =
    create<ORM::PQ::Template>(
      fetch_int("DeletedTemplate/Template1/TEMPLATE_ID"));

  FAIL_CONTEXT(
    CreativeTemplatesWaitChecker(
      CreativeTemplatesChecker(
        this,
        CreativeTemplatesChecker::Expected().
          creative_format(template_name).
          creative_size(size_468x60_name_).
          app_format(format_name_).
          template_file(AutoTest::ANY + file3_).
          type(template_type_name))).check(),
    description + " - initial");

  creative_template->status = "D";
  FAIL_CONTEXT(
    AutoTest::predicate_checker(creative_template->update()), "updating template status");

  add_checker(
    description,
    CreativeTemplatesWaitChecker(
      CreativeTemplatesChecker(
        this,
        CreativeTemplatesChecker::Expected().
          creative_format(template_name).
          template_file(AutoTest::ANY + file3_),
        AutoTest::AEC_NOT_EXISTS)));
}

void CreativeTemplateGranularUpdateTest::tear_down ()
{
  add_descr_phrase("Tear down");
}

bool CreativeTemplateGranularUpdateTest::run()
{
  testcase_add_template();
  testcase_change_template();
  testcase_delete_template();
  testcase_del_status_template();

  add_descr_phrase("Check changes");
  return true;
}
