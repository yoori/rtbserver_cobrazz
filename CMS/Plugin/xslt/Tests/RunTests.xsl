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

# Run autotests (devel's entry-point)
# CMS generated file

. TestConfig/envparams.sh
. TestConfig/testsCommons.sh

if [ ! -e $config_root/TestConfig/AutoTests/AutoTestsConfig.xml ]
then
  echo "$config_root/TestConfig/AutoTests/AutoTestsConfig.xml is not exists." \
      " Need run confgen.sh &amp; prepareDB.sh before!" &amp;&amp; exit 1
fi 

if [ ! -e $auto_tests_config_root/LocalParams.xml ]
then
  echo "$auto_tests_config_root/LocalParams.xml is not exists." \
      " Need run prepareDB.sh before!" &amp;&amp; exit 1
fi 


run_name=${1:-"${colocation_name}-simple"}
shift
<xsl:variable name="auto-test-config" select="$xpath/configuration/cfg:autoTest"/>

<xsl:variable name="groups">
  <xsl:for-each select="$auto-test-config/cfg:Groups/cfg:Group">
    <xsl:variable name="group">
      <xsl:value-of select="."/>
    </xsl:variable>
    <xsl:if test="position()=1">&quot;</xsl:if><xsl:value-of select="$group"/><xsl:if test="position()!=last()">,</xsl:if><xsl:if test="position()=last()">&quot;</xsl:if>
  </xsl:for-each>
</xsl:variable>
groups=${1:-<xsl:value-of select="$groups"/>}
shift

<xsl:variable name="categories">
  <xsl:for-each select="$auto-test-config/cfg:Categories/cfg:Category">
    <xsl:variable name="category">
      <xsl:value-of select="."/>
    </xsl:variable>
    <xsl:if test="position()=1">&quot;</xsl:if><xsl:value-of select="$category"/><xsl:if test="position()!=last()">,</xsl:if><xsl:if test="position()=last()">&quot;</xsl:if>
  </xsl:for-each>
</xsl:variable>

<xsl:variable name="tests">
  <xsl:for-each select="$auto-test-config/cfg:Tests/cfg:Test">
    <xsl:variable name="test">
      <xsl:value-of select="."/>
    </xsl:variable>
    <xsl:if test="position()=1">&quot;</xsl:if><xsl:value-of select="$test"/><xsl:if test="position()!=last()">,</xsl:if><xsl:if test="position()=last()">&quot;</xsl:if>
  </xsl:for-each>
</xsl:variable>

<xsl:variable name="excluded-tests">
  <xsl:for-each select="$auto-test-config/cfg:ExcludedTests/cfg:Test">
    <xsl:variable name="test">
      <xsl:value-of select="."/>
    </xsl:variable>
    <xsl:if test="position()=1">&quot;</xsl:if><xsl:value-of select="$test"/><xsl:if test="position()!=last()">,</xsl:if><xsl:if test="position()=last()">&quot;</xsl:if>
  </xsl:for-each>
</xsl:variable>

run_tests "$run_name" \
  --config=$config_root/TestConfig/AutoTests/AutoTestsConfig.xml \
  --params=$auto_tests_config_root/LocalParams.xml \
  ${groups:+--groups=${groups}}<xsl:if test="string-length($categories) > 0"> \
  --categories=<xsl:value-of select="$categories"/></xsl:if><xsl:if test="string-length($tests) > 0"> \
  --tests=<xsl:value-of select="$tests"/></xsl:if><xsl:if test="string-length($excluded-tests) > 0"> \
  --exclude=<xsl:value-of select="$excluded-tests"/></xsl:if><xsl:if test="count($auto-test-config/@nodb) > 0
        and translate($auto-test-config/@nodb, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ',  'abcdefghijklmnopqrstuvwxyz') = 'true'"> \
  --nodb</xsl:if>

</xsl:template>

</xsl:stylesheet>

