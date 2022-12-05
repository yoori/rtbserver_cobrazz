

  export unix_commons_root=/opt/foros/server
  export adserver_ssh_identity=/home/aduser/.ssh/adkey

  export server_root=/opt/foros/server
  export server_bin_root=/opt/foros/server
  export config_root=/opt/foros/server/etc/telstra-stage-central/adproxycluster-1
  export data_root=/opt/foros/server/var/www
  export current_config_dir=$config_root/CurrentEnv
  export workspace_root=/opt/foros/server/var
  export log_root=$workspace_root/log

  

  export ps_data_root=$config_root/www/PageSense
  export dacs_root=$config_root/DACS

  export auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils
  export auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils/NightlyScripts

  export PATH=$PATH:$server_bin_root
  export PATH=$PATH:$server_root/bin
  export PATH=$PATH:$server_root/build/bin
  export PATH=$PATH:$server_root/ConfigSys:$auto_test_path
  export PATH=$PATH:$unix_commons_root/bin:$unix_commons_root/build/bin:$unix_commons_root

  export PERL5LIB=$PERL5LIB:$server_root/bin
  export PERL5LIB=$PERL5LIB:$server_root/DACS
  export PERL5LIB=$PERL5LIB:$server_root/ConfigSys:$auto_test_path

  export PERL5LIB=$PERL5LIB:$config_root/telstra-stage-central/adproxycluster-1/DACS
  export PERL5LIB=$PERL5LIB:$dacs_root

  export ORACLE_HOME=/opt/oracle/product/9.2.0

  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/lib
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/build/lib
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$unix_commons_root/build/lib
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/oracle/product/10g/instantclient/lib
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$ORACLE_HOME/lib
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/perl5/5.8.8/i386-linux-thread-multi/CORE
  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64/perl5/5.8.8/x86_64-linux-thread-multi/CORE

  export NLS_LANG=.AL32UTF8
  export NLS_NCHAR=AL32UTF8

