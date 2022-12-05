
package CreativeTemplateGranularUpdateTest::TestCase;

use strict;
use warnings;

sub create_templates
{
  my ($self, $args) = @_;

  foreach my $arg (@$args)
  {
    die "$self->{case_name}: Template '$arg->{name}' is redefined!"
      if exists $self->{templates}->{$arg->{name}};

    my $name = $arg->{name};

    my $template = $self->{ns}->create(Template => $arg);

    $self->{templates}->{$name} = $template;

    $self->{ns}->output("$name/TEMPLATE_ID", $template->{template_id});
    $self->{ns}->output("$name/TEMPLATE_FILE_ID", $template->{template_file_id});
    $self->{ns}->output("$name/NAME", $template->{name});
  }
}

sub new
{
  my ($self, $ns, $case_name) = @_;

  unless (ref $self)
  { $self = bless {}, $self; }

  $self->{ns} = $ns->sub_namespace($case_name);

  $self->{templates} = {};

  return $self;
}

1;

package CreativeTemplateGranularUpdateTest;

use strict;
use warnings;
use DB::Defaults;
use DB::Util;

sub testcase_add_template
{
  my ($self) = @_;

  my $subspace = $self->{ns}->sub_namespace("InsertedTemplate");
  $subspace->output('Template1/NAME', make_autotest_name($subspace, 'Template1'));
}

sub testcase_change_template
{
  my ($self) = @_;

  my $test_case =
    new CreativeTemplateGranularUpdateTest::TestCase($self->{ns}, "ChangedTemplate");

  $test_case->create_templates([
    { name => "Template1",
      template_file => $self->{file1},
      size_id => DB::Defaults::instance()->size_300x250(),
      flags => 1,
      app_format_id => $self->{appformat},
      template_file_type => 'T' }
  ]);
}

sub testcase_delete_template
{
  my ($self) = @_;

  my $test_case =
    new CreativeTemplateGranularUpdateTest::TestCase($self->{ns}, "DeletedTemplateFile");

  $test_case->create_templates([
    { name => "Template1",
      template_file => $self->{file2},
      size_id => DB::Defaults::instance()->size_300x250(),
      flags => 1,
      app_format_id => $self->{appformat},
      template_file_type => 'T' }
  ]);
}

sub testcase_del_status_template
{
  my ($self) = @_;

  my $test_case =
    new CreativeTemplateGranularUpdateTest::TestCase($self->{ns}, "DeletedTemplate");

  $test_case->create_templates([
    { name => "Template1",
      template_file => $self->{file3},
      size_id => DB::Defaults::instance()->size(),
      flags => 1,
      app_format_id => $self->{appformat},
      template_file_type => 'T' }
  ]);
}

sub init
{
  my ($self, $ns) = @_;

  $self->{ns} = $ns;

  $self->{appformat} = DB::Defaults::instance()->app_format_no_track;

  $self->{file1} = "UnitTests/1_TemplateGranularUpdateTest";
  $self->{file2} = "UnitTests/2_TemplateGranularUpdateTest";
  $self->{file3} = "UnitTests/3_TemplateGranularUpdateTest";

  $ns->output('AppFormat', $self->{appformat});
  $ns->output('AppFormatName', $self->{appformat}->{name});
  $ns->output('Size/300x250/ID', DB::Defaults::instance()->size_300x250());
  $ns->output('Size/300x250/NAME', DB::Defaults::instance()->size_300x250()->{name});
  $ns->output('Size/468x60/ID', DB::Defaults::instance()->size());
  $ns->output('Size/468x60/NAME', DB::Defaults::instance()->size()->{name});
  $ns->output('File1', $self->{file1});
  $ns->output('File2', $self->{file2});
  $ns->output('File3', $self->{file3});

  $self->testcase_add_template();
  $self->testcase_change_template();
  $self->testcase_delete_template();
  $self->testcase_del_status_template();
}

1;
