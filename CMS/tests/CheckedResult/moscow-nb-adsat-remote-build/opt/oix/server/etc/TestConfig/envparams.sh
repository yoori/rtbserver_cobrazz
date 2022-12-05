
export newsgate_port=10080
export unix_commons_root=/opt/foros/server

export tested_port=10180

export tested_host=adsr-nbadsat0.ocslab.com
export server_root=/opt/foros/server
export workspace_root=/opt/foros/server/var
export config_root=/opt/foros/server/etc
export test_config_root=$workspace_root/run/Tests
export test_log_path=$workspace_root/log/Tests

export cluster_type=remote
export adserver_ssh_identity=/home/aduser/.ssh/adkey

export PATH=$PATH:$server_root/bin
export PATH=$PATH:$server_root/build/bin
export PATH=$PATH:$server_root:$server_root/tests/AutoTests/PerlUtils
export PATH=$PATH:$server_root/lib/utils/tests/AutoTests/PerlUtils
export PATH=$PATH:$server_root/tests/AutoTests/PerlUtils/NightlyScripts
export PATH=$PATH:$server_root/lib/utils/tests/AutoTests/PerlUtils/NightlyScripts
export PATH=$PATH:$server_root/tests/PerformanceTests/PerlUtils
export PATH=$PATH:$server_root/lib/utils/tests/PerformanceTests/PerlUtils
export PATH=$PATH:$unix_commons_root/bin:$unix_commons_root/build/bin:$unix_commons_root
export PATH=$PATH:$server_root/lib/utils/tests/AutoTests/PyUtils/NewsGateEmu
export PATH=$PATH:$server_root/tests/AutoTests/PyUtils/NewsGateEmu

export ORACLE_HOME=/opt/oracle/product/9.2.0

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$server_root/build/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$unix_commons_root/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$unix_commons_root/build/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/oracle/product/10g/instantclient/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/perl5/5.8.8/i386-linux-thread-multi/CORE
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64/perl5/5.8.8/x86_64-linux-thread-multi/CORE

export NLS_LANG=.AL32UTF8
export NLS_NCHAR=AL32UTF8


export auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils
export auto_test_path=$auto_test_path:$server_root/lib/utils/tests/AutoTests/PerlUtils
export auto_test_path=$auto_test_path:$server_root/tests/AutoTests/PerlUtils/NightlyScripts
export auto_test_path=$auto_test_path:$server_root/lib/utils/tests/AutoTests/PerlUtils/NightlyScripts

export test_commons=$server_root/tests/Commons/Perl
export test_commons=$test_commons:$server_root/lib/utils/tests/Commons/Perl

export performance_test_path=$server_root/tests/PerformanceTests/PerlUtils
export performance_test_path=$performance_test_path:$server_root/lib/utils/tests/PerformanceTests/PerlUtils

export PERL5LIB=$PERL5LIB:$test_commons:$auto_test_path:$performance_test_path:$server_root/DACS

export PYTHONPATH=$PYTHONPATH:$server_root/lib/utils/PyCommons:$server_root/lib/utils/tests/AutoTests/PyUtils/NewsGateEmu:$server_root/PyCommons:$server_root/tests/AutoTests/PyUtils/NewsGateEmu

export db_schema=//ora-nb/addbnb.ocslab.com
export db_user=BS_NIGHTLY_BUILDS_ADS_AUTO
export db_password=adserver

export url=http://xnb.ocslab.com
export http_root=/opt/Bamboo/bamboo-home/xml-data/build-dir/ADS-TESTADSAT/test-results
export http_test_path=
export history_path=/opt/Bamboo/bamboo-home/xml-data/builds/ADS-TESTADSAT/download-data/artifacts
export dst_sub_path=build-XX/Results

export adserver_host=adsr-nbadsat0
export server_start_prefix="true"

export colocation_name=moscow-nb-adsat-remote

export nb_hosts="adsr-nbadsat0 adsp-nbadsat0 adsp-nbadsat1 adsc-nbadsat0 adsc-nbadsat1 node2-nb"

