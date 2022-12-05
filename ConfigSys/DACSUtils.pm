package DACSUtils;

sub process_hosts
{
  my ($val) = @_;
  my @ret = ();

  if(ref($val) eq 'ARRAY')
  {
    foreach $i(@$val)
    {
      @ret = @$val;
    }
  }
  else
  {
    @ret = split(",", $val);
  }

  return @ret;
}

sub generate
{
  my $FUN = 'DACSUtils::generate';
  my ($naming, $distribution) = @_;

  my @host_names = process_hosts($naming);

  my $distribution_value = undef;
  my $naming_value = undef;

  my $distribution_dedicated = undef;
  my $distribution_common = undef; 
  my $naming_dedicated = undef;
  my $naming_common = undef;
 
  my %return_locations = ();
  my $hosts_count = scalar(@host_names);

  if(defined($distribution->{$hosts_count}))
  {
    $distribution_value = $distribution->{$hosts_count};
  }
  else
  {
    $distribution_value = $distribution->{'N'}; 
  }  
  
  if(defined($distribution_value->{'dedicated_hosts'}))
  {
    $distribution_dedicated = $distribution_value->{'dedicated_hosts'};
  }

  if(defined($distribution_value->{'other_hosts'}))
  {
    (ref($distribution_value->{'other_hosts'}) eq 'ARRAY') or
      die "$FUN:: error: in hosts destribution for hosts count = $hosts_count " .
          "service list for 'other_hosts' is not of the array type.\n";

    $distribution_common = $distribution_value->{'other_hosts'};
  }

  my $host_number = 0;

  for(my $i = 1; $i <= $hosts_count; $i++) 
  { 
    my $services_on_host = undef; 
    my $host_name = @host_names[$i - 1];

    if (exists($distribution_dedicated->{$i}))
    {
      (ref($distribution_dedicated->{$i}) eq 'ARRAY') or
        die "$FUN:: error: for the host with sequence number = $i service list " .
            "is not of array type.\n";

      $services_on_host = $distribution_dedicated->{$i};
    }
    else
    {
      $services_on_host = $distribution_common;
    }

    $return_locations{$host_name} = $services_on_host;
  }

  return \%return_locations;
}

sub load
{ 
  my ($file_name) = @_;

  require("$file_name") or
    die "Error: Can't load $file_name: $!\n";

  return \%service_locations;
}

sub save
{ 
  my ($dacs_locations, $file_name) = @_;

  open(LOCFILE,"> $file_name") or
    die "Error: Can't open $file_name for writing: $!\n";

  print LOCFILE "\%service_locations = (\n";
 
  while(($host_name, $services_on_host) = each(%$dacs_locations)) 
  {
    print LOCFILE "  '$host_name' => [\n";

    foreach $serv (@$services_on_host) 
    {
      print LOCFILE "      '$serv',\n";
    }

    print LOCFILE "    ],\n";
  }

  print LOCFILE ");\n";

  close(LOCFILE);
}

sub generate_host_vars
{
  my($service_locations, $postfix) = @_;
  my %services_to_host_map = ();

  if(!defined($postfix))
  {
    $postfix = "_hosts";
  }

  while(($host, $services_list) = each(%$service_locations))
  {
    foreach $service (@$services_list)
    {
      $service =~ /.*::(.*)/;
      push( @{$services_to_host_map{"$1$postfix"}}, $host );
    } 
  }

  return \%services_to_host_map; 
}

sub get_hosts
{
  my ($service_locations) = @_;
  my @result_hosts = keys %$service_locations;
  
  return \@result_hosts;
}

sub generate_conf_locations
{
  my $DEFAULT_CONFIGURATOR_NAME = 'AdServer::TemplConfigurator';
  my ($hosts, $conf_name) = @_;
  
  if(!defined($conf_name))
  {
    $conf_name = $DEFAULT_CONFIGURATOR_NAME;
  }

  my %dacs_locations = ();

  foreach $i (@$hosts)
  {
    $dacs_locations{$i} = [ $conf_name ];
  }

  return \%dacs_locations;
}

sub process_components_distribution
{
  # process components distribution configuration
  my ($components, $dacs_directory, $dacs_cluster_filename) = @_;
  
  my %result_hash = ();

  # generate DACS locations for each component
  (ref($components) eq 'HASH') or die "Components tree isn't HASH."; 

  while(my ($component_type, $component_seq) = each(%$components))
  {
    (ref($component_seq) eq 'HASH') or 
      die "Component node with type '$component_type' isn't HASH."; 

    while(my ($component_name, $component_services) = each(%$component_seq))
    {
      $result_hash{$component_name} = $component_services;
    } 
  }

  return \%result_hash;
}

sub generate_local_dacs
{  
  # merge DACS locations for one locations result,
  # that will be used for configuring template files.   
  my ($components, $current_host) = @_;
  my %result_dacs_locations = ();
  my $found = 0;

  while(my ($component_type, $component_seq) = each(%$components))
  {
    my $found = 0;
    my $last_filename = undef;

    (ref($component_seq) eq 'HASH') or 
      die "Component node with type '$component_type' isn't HASH."; 

    while(my ($dacs_filename, $component_services) = each(%$component_seq))
    {
      $last_filename = $dacs_filename;

      if(exists($component_services->{$current_host}))
      {
        if($found == 1)
        {
          die "Components has common hosts.";
        }
        
        $found = 1;
        merge(\%result_dacs_locations, $component_services);
      }
    } 

    if(defined($last_filename) && $found == 0)
    {
      merge(\%result_dacs_locations, $component_seq->{$last_filename});
    }
  }  
  
  return \%result_dacs_locations;
}

sub merge
{
  my ($left_dacs_locations, $right_dacs_locations) = @_;

  while(($hostname, $services) = each(%$right_dacs_locations))
  {
    my %tmp;
    if(exists($left_dacs_locations->{$hostname}))
    {
      @tmp{(@{$left_dacs_locations->{$hostname}}, @$services)} = ();
      @{$left_dacs_locations->{$hostname}} = sort keys %tmp;
    }
    else
    {
      @{$left_dacs_locations->{$hostname}} = @$services;
    }
  }
}

1;
