
export unix_commons_root=/home/jurij_kuznecov/projects/unixcommons/trunk

export tested_port=12080

export tested_host=xen.ocslab.com
export server_root=/home/jurij_kuznecov/projects/foros/server/trunk
export workspace_root=/home/jurij_kuznecov/projects/foros/run/trunk/var
export config_root=/home/jurij_kuznecov/projects/foros/run/trunk/etc
export test_config_root=$workspace_root/run/Tests
export test_log_path=$workspace_root/log/Tests

export cluster_type=central
export adserver_ssh_identity=/home/jurij_kuznecov/.ssh/adkey

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

export db_schema=//oracle/addbpt.ocslab.com
export db_user=BS_ADS_DEV
export db_password=adserver

export url=http://xen.ocslab.com
export http_root=/var/www/html
export http_test_path=tests

export mail_list=server@ocslab.com

export adserver_host=xen.ocslab.com
export server_start_prefix="true"

export colocation_name=devel

export nb_hosts="xen.ocslab.com"

