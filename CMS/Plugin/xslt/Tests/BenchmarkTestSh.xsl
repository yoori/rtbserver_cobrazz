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
<xsl:variable name="type" select="$TEST_TYPE"/>

<xsl:template match="/">

#!/bin/bash

. TestConfig/envparams.sh
. TestConfig/make-benchmark-config.sh

function check_error ()
{
 [ $1 -ne 0 ] &amp;&amp; echo "command '$2' return error status code $1" &gt;&amp;2 &amp;&amp;  exit 3
}

 <xsl:choose>
    <xsl:when test="count($xpath) = 0">
      <xsl:message terminate="yes">BenchmarkTest:BenchmarkTestSh: Can't find benchmark test config.</xsl:message>
    </xsl:when>
  </xsl:choose>

name=BenchmarkTest
log_path=$workspace_root/log/$name
mkdir -p $log_path

mkdir -p ${test_config_root}

# Clear old logs
rm -f $log_path/*.out
rm -f $log_path/*.err

<xsl:for-each select="$xpath">
db_clean_<xsl:value-of select="@name"/>
</xsl:for-each>

<xsl:for-each select="$xpath">
make_config_<xsl:value-of select="@name"/>
</xsl:for-each>

read -p "Restart adserver and press any key to continue..." -t 600

<xsl:for-each select="$xpath">

echo "Run tests <xsl:value-of select="@name"/>..."
AdServerBenchmark 6 \
  ${test_config_root}/$name-<xsl:value-of select="@name"/>/Benchmark-<xsl:value-of select="$type"/>.xml \
  1&gt;$log_path/$name-<xsl:value-of select="@name"/>.out \
  2&gt;$log_path/$name-<xsl:value-of select="@name"/>.err
exit_code=$?
check_error $exit_code "Run tests <xsl:value-of select="@name"/>"
echo "Run tests <xsl:value-of select="@name"/>...status=$exit_code"
echo "Check test results:"
echo "  $log_path/$name-<xsl:value-of select="@name"/>.out"
echo "  $log_path/$name-<xsl:value-of select="@name"/>.err"
</xsl:for-each>

</xsl:template>

</xsl:stylesheet>
