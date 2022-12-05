
function db_clean_size_100 ()
{
echo "Cleaning db for size_100..."
db_clean.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --namespace 'PT' \
  --test 'size_100'
check_error $? "db_clean.pl"
echo "Cleaning db for size_100...ok"

}

function make_config_size_100 ()
{

rm -rf ${test_config_root}/PerformanceTest-size_100/*
mkdir -p ${test_config_root}/PerformanceTest-size_100
  echo "Configuring performance size_100..."
  make_config.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --prefix size_100 \
  --server adsc-nbadsat1:10180 \
  --xsd ${server_root}/xsd/tests/PerformanceTests/AdServerTest/AdServerTestConfig.xsd \
  --campaign_config_file ${config_root}/TestConfig/PerformanceTest-size_100/campaigns.cfg \
  --channel_config_file ${config_root}/TestConfig/PerformanceTest-size_100/channels.cfg \
  --create_free_tags \
  --empty_tids_percentage 99 \
  --referer_func random \
  --referer_size 15 \
  --click_rate 0 \
  --action_rate 0 \
  --optout_rate 50 \
  ${test_config_root}/PerformanceTest-size_100/Config.xml
  [ $? -ne 0 ] && echo "command '$2' return error status code $1" >&2 &&  exit 3
  echo "Configuring performance size_100...ok"
}


function db_clean_size_500 ()
{
echo "Cleaning db for size_500..."
db_clean.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --namespace 'PT' \
  --test 'size_500'
check_error $? "db_clean.pl"
echo "Cleaning db for size_500...ok"

}

function make_config_size_500 ()
{

rm -rf ${test_config_root}/PerformanceTest-size_500/*
mkdir -p ${test_config_root}/PerformanceTest-size_500
  echo "Configuring performance size_500..."
  make_config.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --prefix size_500 \
  --server adsc-nbadsat1:10180 \
  --xsd ${server_root}/xsd/tests/PerformanceTests/AdServerTest/AdServerTestConfig.xsd \
  --campaign_config_file ${config_root}/TestConfig/PerformanceTest-size_500/campaigns.cfg \
  --channel_config_file ${config_root}/TestConfig/PerformanceTest-size_500/channels.cfg \
  --create_free_tags \
  --empty_tids_percentage 99 \
  --referer_func random \
  --referer_size 15 \
  --click_rate 0 \
  --action_rate 0 \
  --optout_rate 50 \
  ${test_config_root}/PerformanceTest-size_500/Config.xml
  [ $? -ne 0 ] && echo "command '$2' return error status code $1" >&2 &&  exit 3
  echo "Configuring performance size_500...ok"
}


function db_clean_size_1000 ()
{
echo "Cleaning db for size_1000..."
db_clean.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --namespace 'PT' \
  --test 'size_1000'
check_error $? "db_clean.pl"
echo "Cleaning db for size_1000...ok"

}

function make_config_size_1000 ()
{

rm -rf ${test_config_root}/PerformanceTest-size_1000/*
mkdir -p ${test_config_root}/PerformanceTest-size_1000
  echo "Configuring performance size_1000..."
  make_config.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --prefix size_1000 \
  --server adsc-nbadsat1:10180 \
  --xsd ${server_root}/xsd/tests/PerformanceTests/AdServerTest/AdServerTestConfig.xsd \
  --campaign_config_file ${config_root}/TestConfig/PerformanceTest-size_1000/campaigns.cfg \
  --channel_config_file ${config_root}/TestConfig/PerformanceTest-size_1000/channels.cfg \
  --create_free_tags \
  --empty_tids_percentage 99 \
  --referer_func random \
  --referer_size 15 \
  --click_rate 0 \
  --action_rate 0 \
  --optout_rate 50 \
  ${test_config_root}/PerformanceTest-size_1000/Config.xml
  [ $? -ne 0 ] && echo "command '$2' return error status code $1" >&2 &&  exit 3
  echo "Configuring performance size_1000...ok"
}


function db_clean_history_channels ()
{
echo "Cleaning db for history_channels..."
db_clean.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --namespace 'PT' \
  --test 'history_channels'
check_error $? "db_clean.pl"
echo "Cleaning db for history_channels...ok"

}

function make_config_history_channels ()
{

rm -rf ${test_config_root}/PerformanceTest-history_channels/*
mkdir -p ${test_config_root}/PerformanceTest-history_channels
  echo "Configuring performance history_channels..."
  make_config.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --prefix history_channels \
  --server adsc-nbadsat1:10180 \
  --xsd ${server_root}/xsd/tests/PerformanceTests/AdServerTest/AdServerTestConfig.xsd \
  --campaign_config_file ${config_root}/TestConfig/PerformanceTest-history_channels/campaigns.cfg \
  --channel_config_file ${config_root}/TestConfig/PerformanceTest-history_channels/channels.cfg \
  --create_free_tags \
  --empty_tids_percentage 99 \
  --referer_func random \
  --referer_size 15 \
  --click_rate 0 \
  --action_rate 0 \
  --optout_rate 50 \
  ${test_config_root}/PerformanceTest-history_channels/Config.xml
  [ $? -ne 0 ] && echo "command '$2' return error status code $1" >&2 &&  exit 3
  echo "Configuring performance history_channels...ok"
}


function db_clean_idle ()
{
echo "Cleaning db for idle..."
db_clean.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --namespace 'PT' \
  --test 'idle'
check_error $? "db_clean.pl"
echo "Cleaning db for idle...ok"

}

function make_config_idle ()
{

rm -rf ${test_config_root}/PerformanceTest-idle/*
mkdir -p ${test_config_root}/PerformanceTest-idle
  echo "Configuring performance idle..."
  make_config.pl \
  --db $db_schema \
  --user $db_user \
  --password $db_password \
  --prefix idle \
  --server adsc-nbadsat1:10180 \
  --xsd ${server_root}/xsd/tests/PerformanceTests/AdServerTest/AdServerTestConfig.xsd \
  --campaign_config_file ${config_root}/TestConfig/PerformanceTest-idle/campaigns.cfg \
  --channel_config_file ${config_root}/TestConfig/PerformanceTest-idle/channels.cfg \
  --create_free_tags \
  --empty_tids_percentage 100 \
  --referer_func random \
  --referer_size 0 \
  --click_rate 0 \
  --action_rate 0 \
  --optout_rate 50 \
  ${test_config_root}/PerformanceTest-idle/Config.xml
  [ $? -ne 0 ] && echo "command '$2' return error status code $1" >&2 &&  exit 3
  echo "Configuring performance idle...ok"
}

