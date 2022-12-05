package AdServer::RequestInfoSvcs::RequestInfoChunksConfigurator;

use Utils::Functions;
use AdServer::Functions;
use AdServer::Path;

sub start
{
  my ($host, $descr) = @_;

  my $move_command =
    "mkdir -p \${workspace_root}/${AdServer::Path::RUNTIME_CONF_BASE} && ".
    "RIOldChunksConfigProcessor.pl -v ".
      "-conf \${config_root}/${AdServer::Path::XML_FILE_BASE}RIChunksConfig ".
      "-old-conf \${workspace_root}/${AdServer::Path::RUNTIME_CONF_BASE}Old-RIChunksConfig ".
    ">\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}RequestInfoChunksConfigurator.out; ";

  return AdServer::Functions::execute_command($host, $descr, $move_command);


  my $copy_command =
    "mkdir -p \${workspace_root}/${AdServer::Path::RUNTIME_CONF_BASE} && ".
    " cp ".
      "\${config_root}/${AdServer::Path::XML_FILE_BASE}RIChunksConfig ".
      "\${workspace_root}/${AdServer::Path::RUNTIME_CONF_BASE}Old-RIChunksConfig ".
    ">>\${workspace_root}/${AdServer::Path::OUT_FILE_BASE}RequestInfoChunksConfigurator.out; ";

  return AdServer::Functions::execute_command($host, $descr, $copy_command);
}

sub stop
{
  return 0;
}

1;
