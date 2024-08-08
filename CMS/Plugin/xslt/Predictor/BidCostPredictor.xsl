<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:exsl="http://exslt.org/common"
  xmlns:dyn="http://exslt.org/dynamic"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="dyn exsl">

<xsl:output method="text" indent="yes" omit-xml-declaration="yes" encoding="utf-8"/>

<xsl:include href="../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>

<xsl:template name="BidCostPredictorConfigGenerator">
  <xsl:param name="env-config"/>
  <xsl:param name="predictor-config"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root[1]"/>
    <xsl:if test="count($env-config/@workspace_root[1]) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>

  <xsl:variable name="version" select="$bidcost-predictor-version"/>
  <xsl:variable name="description" select="$bidcost-predictor-description"/>
  <xsl:variable name="log_path" select="concat($workspace-root, $bidcost-predictor-log-path)"/>
  <xsl:variable name="pid_path" select="concat($workspace-root, $bidcost-predictor-pid-path)"/>
  <xsl:variable name="model_input_directory" select="concat($workspace-root, $bidcost-predictor-model-input-path)"/>
  <xsl:variable name="model_period" select="$bidcost-predictor-model-period"/>
  <xsl:variable name="model_bid_cost_file_name" select="$bidcost-predictor-model-bid-cost-file-name"/>
  <xsl:variable name="model_bid_cost_output_directory" select="concat($workspace-root, $bidcost-predictor-model-bid-cost-output-directory)"/>
  <xsl:variable name="model_bid_cost_temp_directory" select="concat($workspace-root, $bidcost-predictor-model-bid-cost-temp-directory)"/>
  <xsl:variable name="model_ctr_file_name" select="$bidcost-predictor-model-ctr-file-name"/>
  <xsl:variable name="model_ctr_output_directory" select="concat($workspace-root, $bidcost-predictor-model-ctr-output-directory)"/>
  <xsl:variable name="model_ctr_temp_directory" select="concat($workspace-root, $bidcost-predictor-model-ctr-temp-directory)"/>
  <xsl:variable name="model_ctr_max_imps" select="$bidcost-predictor-model-ctr-max-imps"/>
  <xsl:variable name="model_ctr_tag_imps" select="$bidcost-predictor-model-ctr-tag-imps"/>
  <xsl:variable name="model_ctr_trust_imps" select="$bidcost-predictor-model-ctr-trust-imps"/>
  <xsl:variable name="aggregator_input_directory" select="concat($workspace-root, $bidcost-predictor-aggregator-input-directory)"/>
  <xsl:variable name="aggregator_output_directory" select="concat($workspace-root, $bidcost-predictor-aggregator-output-directory)"/>
  <xsl:variable name="aggregator_period" select="$bidcost-predictor-aggregator-period"/>
  <xsl:variable name="aggregator_dump_max_size" select="$bidcost-predictor-aggregator-dump-max-size"/>
  <xsl:variable name="aggregator_max_process_files" select="$bidcost-predictor-aggregator-max-process-files"/>
  <xsl:variable name="reaggregator_period" select="$bidcost-predictor-reaggregator-period"/>
  <xsl:variable name="reaggregator_input_directory" select="concat($workspace-root, $bidcost-predictor-reaggregator-input-directory)"/>
  <xsl:variable name="reaggregator_output_directory" select="concat($workspace-root, $bidcost-predictor-reaggregator-output-directory)"/>
  <xsl:variable name="pg_host" select="$bidcost-predictor-pg-connection-host"/>
  <xsl:variable name="pg_port" select="$bidcost-predictor-pg-connection-port"/>
  <xsl:variable name="pg_dbname" select="$bidcost-predictor-pg-connection-dbname"/>
  <xsl:variable name="pg_user" select="$bidcost-predictor-pg-connection-user"/>
  <xsl:variable name="pg_password" select="$bidcost-predictor-pg-connection-password"/>

  <xsl:text>{</xsl:text>

  <xsl:text>
  "version" : "</xsl:text>
  <xsl:value-of select="$version"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "description" : "</xsl:text>
  <xsl:value-of select="$description"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
  "config" : {</xsl:text>

  <xsl:text>
    "log_path" : "</xsl:text>
  <xsl:value-of select="$log_path"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
    "pid_path" : "</xsl:text>
  <xsl:value-of select="$pid_path"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
    "pg_connection" : {</xsl:text>

  <xsl:text>
      "host" : "</xsl:text>
  <xsl:value-of select="$pg_host"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "port" : "</xsl:text>
  <xsl:value-of select="$pg_port"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "dbname" : "</xsl:text>
  <xsl:value-of select="$pg_dbname"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "user" : "</xsl:text>
  <xsl:value-of select="$pg_user"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "password" : "</xsl:text>
  <xsl:value-of select="$pg_password"/>
  <xsl:text>"</xsl:text>

  <xsl:text>
    },</xsl:text>

  <xsl:text>
    "model" : {</xsl:text>

  <xsl:text>
      "period" : "</xsl:text>
  <xsl:value-of select="$model_period"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "input_directory" : "</xsl:text>
  <xsl:value-of select="$model_input_directory"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "bid_cost" : {</xsl:text>

  <xsl:text>
        "file_name" : "</xsl:text>
  <xsl:value-of select="$model_bid_cost_file_name"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
        "output_directory" : "</xsl:text>
  <xsl:value-of select="$model_bid_cost_output_directory"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
        "temp_directory" : "</xsl:text>
  <xsl:value-of select="$model_bid_cost_temp_directory"/>
  <xsl:text>"</xsl:text>
  <xsl:text>
      },</xsl:text>

  <xsl:text>
      "ctr" : {</xsl:text>

  <xsl:text>
        "file_name" : "</xsl:text>
  <xsl:value-of select="$model_ctr_file_name"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
        "output_directory":"</xsl:text>
  <xsl:value-of select="$model_ctr_output_directory"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
        "temp_directory" : "</xsl:text>
  <xsl:value-of select="$model_ctr_temp_directory"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
        "max_imps" : "</xsl:text>
  <xsl:value-of select="$model_ctr_max_imps"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
        "tag_imps" : "</xsl:text>
  <xsl:value-of select="$model_ctr_tag_imps"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
        "trust_imps" : "</xsl:text>
  <xsl:value-of select="$model_ctr_trust_imps"/>
  <xsl:text>"</xsl:text>

  <xsl:text>
      }</xsl:text>

  <xsl:text>
    },</xsl:text>

  <xsl:text>
    "aggregator" : {</xsl:text>

  <xsl:text>
      "period" : "</xsl:text>
  <xsl:value-of select="$aggregator_period"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "input_directory":"</xsl:text>
  <xsl:value-of select="$aggregator_input_directory"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "output_directory" : "</xsl:text>
  <xsl:value-of select="$aggregator_output_directory"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "dump_max_size" : "</xsl:text>
  <xsl:value-of select="$aggregator_dump_max_size"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "max_process_files" : "</xsl:text>
  <xsl:value-of select="$aggregator_max_process_files"/>
  <xsl:text>"</xsl:text>

  <xsl:text>
    },</xsl:text>

  <xsl:text>
    "reaggregator" : {</xsl:text>

  <xsl:text>
      "period" : "</xsl:text>
  <xsl:value-of select="$reaggregator_period"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "input_directory":"</xsl:text>
  <xsl:value-of select="$reaggregator_input_directory"/>
  <xsl:text>",</xsl:text>

  <xsl:text>
      "output_directory" : "</xsl:text>
  <xsl:value-of select="$reaggregator_output_directory"/>
  <xsl:text>"</xsl:text>

  <xsl:text>
    }</xsl:text>

  <xsl:text>
  }</xsl:text>

  <xsl:text>&#xa;}</xsl:text>

</xsl:template>


<xsl:template match="/">
  <xsl:variable name="cluster-path" select="$xpath/../.."/>
  <xsl:variable name="bidcost-predictor-path" select="$xpath"/>
  <xsl:variable name="colo-config" select="$cluster-path/configuration/cfg:cluster"/>
  <xsl:variable name="env-config" select="$colo-config/cfg:environment"/>
  <xsl:variable name="bidcost-predictor-config" select="$bidcost-predictor-path/configuration/cfg:predictor"/>

  <xsl:call-template name="BidCostPredictorConfigGenerator">
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="predictor-config" select="$bidcost-predictor-config"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
