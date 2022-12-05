
  unix_commons_root=/opt/foros/server/
  export unix_commons_root
  

  adserver_ssh_identity=/home/aduser/.ssh/adkey
  export adserver_ssh_identity

  server_root=/opt/foros/server/
  server_bin_root=/opt/foros/server/
  config_root=/opt/foros/server/etc/moscow-test/adcluster
  cache_root=/opt/foros/server/var/cache
  current_config_dir=$config_root/CurrentEnv
  workspace_root=/opt/foros/server/var
  data_root=/opt/foros/server/var/www
  log_root=$workspace_root/log
  TNS_ADMIN=$config_root/OcciAdmin/

  export server_root
  export server_bin_root
  export config_root
  export cache_root
  export current_config_dir
  export workspace_root
  export data_root
  export log_root
  export TNS_ADMIN

  

  ps_data_root=/opt/foros/server/etc/www/PageSense
  webwise_discover_data_root=/opt/foros/server/etc/www/WebwiseDiscover
  dacs_root=$config_root/DACS
  export ps_data_root
  export webwise_discover_data_root
  export dacs_root

  auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils
  auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils/NightlyScripts
  export auto_test_path

  PATH=$PATH:$server_root/bin
  PATH=$PATH:$server_root/build/bin
  PATH=$PATH:$server_root/ConfigSys:$auto_test_path
  PATH=$PATH:$unix_commons_root/bin:$unix_commons_root/build/bin:$unix_commons_root
  export PATH

  PERL5LIB=$PERL5LIB:$server_root/bin
  PERL5LIB=$PERL5LIB:$server_root/DACS
  PERL5LIB=$PERL5LIB:$server_root/ConfigSys:$auto_test_path

  PERL5LIB=$PERL5LIB:$config_root/moscow-test/adcluster/DACS
  PERL5LIB=$PERL5LIB:$dacs_root
  export PERL5LIB

  ORACLE_HOME=/opt/oracle/product/9.2.0
  export ORACLE_HOME

  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/lib
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/build/lib
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$unix_commons_root/build/lib
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/oracle/product/10g/instantclient/lib
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ORACLE_HOME/lib
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/perl5/5.8.8/i386-linux-thread-multi/CORE
  LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64/perl5/5.8.8/x86_64-linux-thread-multi/CORE

  export LD_LIBRARY_PATH

  NLS_LANG=.AL32UTF8
  export NLS_LANG
  NLS_NCHAR=AL32UTF8
  export NLS_NCHAR

