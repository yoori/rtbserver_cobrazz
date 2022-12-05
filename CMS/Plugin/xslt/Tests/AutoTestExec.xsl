<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet 
  version="1.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:exsl="http://exslt.org/common"
  xmlns:colo="http://www.foros.com/cms/colocation"
  exclude-result-prefixes="dyn exsl">


<xsl:output method="text" indent="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<xsl:template match="/">
#!/bin/bash

# AutoTests runner script
# CMS generated file

. TestConfig/envparams.sh
. TestConfig/testsCommons.sh

if [ "x$1" != "x" ]
then
export vg_report_suffix=$1
fi

<xsl:variable name="auto-test-config" select="$xpath/configuration/cfg:autoTest"/>

mkdir -p $auto_tests_config_root || { echo "Can't create test_config_root=$auto_tests_config_root" &amp;&amp; exit 1 ; }

# Prepare DB data
./prepareDB.sh
[ $? -ne 0 ] &amp;&amp;  exit $?

# Wait data actualization or server restart
read -p "Restart adserver and press any key to continue..." -t 600

# Run tests
stage=0
./runTests.sh "$colocation_name-stage-$stage-simple"

<xsl:variable name="zero-downtime">
<xsl:choose>
  <xsl:when test="count($auto-test-config/@zero-downtime) > 0">
    <xsl:value-of select="$auto-test-config/@zero-downtime"/>
  </xsl:when>
  <xsl:otherwise>false</xsl:otherwise>
</xsl:choose>
</xsl:variable>

<xsl:if test="$zero-downtime = 'true'">
# Start zerodowntime 

# stage#1 - stop services
stop_service ui
stop_service be
stop_service lp
stop_service fe1


# run tests stage#1 - server stopped for upgrade
let "stage += 1"
run_tests "$colocation_name-stage-$stage-zerodowntime-1" \
  --config=$config_root/TestConfig/AutoTests/AutoTestsConfig.xml  \
  --params=$auto_tests_config_root/LocalParams.xml \
  --categories="GranularUpdate"

# stage#2 - start some service
start_service be
start_service fe1

sleep 300 # TODO: remove this sleep after server fixed

# run tests stage#2 - server partially started
let "stage += 1"
run_tests "$colocation_name-stage-$stage-zerodowntime-2" \
           --config=$config_root/TestConfig/AutoTests/AutoTestsConfig-zerodowntime.xml  \
           --params=$auto_tests_config_root/LocalParams.xml \
           --categories="GranularUpdate"

# stage#3 - start all services
start_service ui
start_service lp

# run tests stage#3 - server was started after upgrade
let "stage += 1"
run_tests "$colocation_name-stage-$stage-zerodowntime-3" \
  --config=$config_root/TestConfig/AutoTests/AutoTestsConfig.xml  \
  --params=$auto_tests_config_root/LocalParams.xml \
  --categories="GranularUpdate"
</xsl:if>

<xsl:if test="count($auto-test-config/cfg:StoppedServices/cfg:AdService) > 0">
  <xsl:variable name="stopped-services">
    <xsl:for-each select="$auto-test-config/cfg:StoppedServices/cfg:AdService">
      <xsl:variable name="current-service">
        <xsl:value-of select="@name"/>,<xsl:value-of select="@host"/>
      </xsl:variable>
      <xsl:value-of select="concat($current-service, ' ')"/>
    </xsl:for-each>
  </xsl:variable>
stopped_services="<xsl:value-of select="normalize-space($stopped-services)"/>"

# Stop services cycle
for service in $stopped_services
do
 srv=(`echo $service | tr ',' ' '`)
 echo "Stop service ${srv[0]} on ${srv[1]}..."
 ssh -i $adserver_ssh_identity -o StrictHostKeyChecking=no ${srv[1]} \
        "(cd $config_root; ./adserver.sh ${srv[1]}:${srv[0]} stop)"
 check_error $? "${srv[0]} stop "
 echo "Stop service ${srv[0]} on ${srv[1]}...ok"
done
# run tests - after services was stopped
let "stage += 1"
run_tests "$colocation_name-stage-$stage-after-service-stopped" \
  --config=$config_root/TestConfig/AutoTests/AutoTestsConfig-granular.xml  \
  --params=$auto_tests_config_root/LocalParams.xml \
  --exclude_categories="TriggerMatching,UserProfiling,UserProfilesExchange"

# Start services cycle
for service in $stopped_services
do
 srv=(`echo $service | tr ',' ' '`)
 echo "Start service ${srv[0]} on ${srv[1]}..."
 ssh -i $adserver_ssh_identity -o StrictHostKeyChecking=no ${srv[1]} \
        "(cd $config_root; ./adserver.sh ${srv[1]}:${srv[0]} start)"
 check_error $? "${srv[0]} start "
 echo "Start service ${srv[0]} on ${srv[1]}...ok"
 
done
</xsl:if>

</xsl:template>

</xsl:stylesheet>
