<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:colo="http://www.foros.com/cms/colocation"
                xmlns:cfg="http://www.adintelligence.net/xsd/AI/Configuration"
                exclude-result-prefixes="dyn colo cfg">
  <xsl:output method="text" encoding="utf-8"/>
  <xsl:param name="XPATH"/>
  <xsl:param name="HOST"/>
  <xsl:variable name="service" select="dyn:evaluate($XPATH)"/>
  <xsl:variable name="config" select="$service/configuration/cfg:aiAgent"/>

  <xsl:template match="/">
AI_MODEL=<xsl:choose><xsl:when test="$config/@model"><xsl:value-of select="$config/@model"/></xsl:when><xsl:otherwise>qwen3</xsl:otherwise></xsl:choose>
AI_MODEL_DIR=<xsl:choose><xsl:when test="$config/@model_dir"><xsl:value-of select="$config/@model_dir"/></xsl:when><xsl:otherwise>/u01/foros/server-ai/models/ollama/qwen3</xsl:otherwise></xsl:choose>
AI_OLLAMA_BIN=<xsl:choose><xsl:when test="$config/@ollama_binary"><xsl:value-of select="$config/@ollama_binary"/></xsl:when><xsl:otherwise>/opt/foros/ollama/bin/ollama</xsl:otherwise></xsl:choose>
AI_LISTEN_HOST=<xsl:choose><xsl:when test="$config/@listen_host"><xsl:value-of select="$config/@listen_host"/></xsl:when><xsl:otherwise>0.0.0.0</xsl:otherwise></xsl:choose>
AI_PORT=<xsl:choose><xsl:when test="$config/@port"><xsl:value-of select="$config/@port"/></xsl:when><xsl:otherwise>11434</xsl:otherwise></xsl:choose>
AI_AGENT_LISTEN_HOST=<xsl:choose><xsl:when test="$config/@agent_listen_host"><xsl:value-of select="$config/@agent_listen_host"/></xsl:when><xsl:otherwise>0.0.0.0</xsl:otherwise></xsl:choose>
AI_AGENT_PORT=<xsl:choose><xsl:when test="$config/@agent_port"><xsl:value-of select="$config/@agent_port"/></xsl:when><xsl:otherwise>11435</xsl:otherwise></xsl:choose>
AI_CLUSTER_AGENT_LISTEN_HOST=<xsl:choose><xsl:when test="$config/@cluster_agent_listen_host"><xsl:value-of select="$config/@cluster_agent_listen_host"/></xsl:when><xsl:otherwise>127.0.0.1</xsl:otherwise></xsl:choose>
AI_CLUSTER_AGENT_PORT=<xsl:choose><xsl:when test="$config/@cluster_agent_port"><xsl:value-of select="$config/@cluster_agent_port"/></xsl:when><xsl:otherwise>11436</xsl:otherwise></xsl:choose>
AI_MCP_COMMAND=<xsl:choose><xsl:when test="$config/@mcp_command"><xsl:value-of select="$config/@mcp_command"/></xsl:when><xsl:otherwise>/opt/foros/playwright-mcp/bin/playwright-mcp</xsl:otherwise></xsl:choose>
AI_MCP_ALLOWED_TOOLS=<xsl:choose><xsl:when test="$config/@mcp_allowed_tools"><xsl:value-of select="$config/@mcp_allowed_tools"/></xsl:when><xsl:otherwise>browser_click,browser_close,browser_find,browser_hover,browser_navigate,browser_navigate_back,browser_snapshot,browser_tabs,browser_wait_for,web_fetch,web_search</xsl:otherwise></xsl:choose>
AI_BROWSER_ALLOWED_DOMAINS=<xsl:value-of select="$config/@browser_allowed_domains"/>
AI_BROWSER_PROXY_SERVER=<xsl:value-of select="$config/@browser_proxy_server"/>
AI_BROWSER_ACTION_TIMEOUT_MS=<xsl:choose><xsl:when test="$config/@browser_action_timeout_ms"><xsl:value-of select="$config/@browser_action_timeout_ms"/></xsl:when><xsl:otherwise>5000</xsl:otherwise></xsl:choose>
AI_BROWSER_NAVIGATION_TIMEOUT_MS=<xsl:choose><xsl:when test="$config/@browser_navigation_timeout_ms"><xsl:value-of select="$config/@browser_navigation_timeout_ms"/></xsl:when><xsl:otherwise>30000</xsl:otherwise></xsl:choose>
AI_WEB_MCP_COMMAND=<xsl:choose><xsl:when test="$config/@web_mcp_command"><xsl:value-of select="$config/@web_mcp_command"/></xsl:when><xsl:otherwise>/opt/foros/server-ai/bin/WebToolsMcp.py</xsl:otherwise></xsl:choose>
AI_WEB_ALLOWED_DOMAINS=<xsl:value-of select="$config/@web_allowed_domains"/>
AI_WEB_PROXY_SERVER=<xsl:value-of select="$config/@web_proxy_server"/>
AI_WEB_REQUEST_TIMEOUT_MS=<xsl:choose><xsl:when test="$config/@web_request_timeout_ms"><xsl:value-of select="$config/@web_request_timeout_ms"/></xsl:when><xsl:otherwise>30000</xsl:otherwise></xsl:choose>
AI_WEB_MAX_DOWNLOAD_BYTES=<xsl:choose><xsl:when test="$config/@web_max_download_bytes"><xsl:value-of select="$config/@web_max_download_bytes"/></xsl:when><xsl:otherwise>2097152</xsl:otherwise></xsl:choose>
AI_WEB_MAX_CONTENT_CHARS=<xsl:choose><xsl:when test="$config/@web_max_content_chars"><xsl:value-of select="$config/@web_max_content_chars"/></xsl:when><xsl:otherwise>100000</xsl:otherwise></xsl:choose>
AI_CLUSTER_MCP_COMMAND=<xsl:choose><xsl:when test="$config/@cluster_mcp_command"><xsl:value-of select="$config/@cluster_mcp_command"/></xsl:when><xsl:otherwise>/opt/foros/server-ai/bin/ClusterDataMcp.py</xsl:otherwise></xsl:choose>
AI_CLUSTER_MCP_ALLOWED_TOOLS=<xsl:choose><xsl:when test="$config/@cluster_mcp_allowed_tools"><xsl:value-of select="$config/@cluster_mcp_allowed_tools"/></xsl:when><xsl:otherwise>clickhouse_describe_relation,clickhouse_list_relations,clickhouse_select,postgres_describe_relation,postgres_list_relations,postgres_select</xsl:otherwise></xsl:choose>
AI_POSTGRES_CONNECTION_STRING='<xsl:value-of select="$config/@postgres_connection_string"/>'
AI_CLICKHOUSE_URL=<xsl:value-of select="$config/@clickhouse_url"/>
AI_CLICKHOUSE_USER=<xsl:choose><xsl:when test="$config/@clickhouse_user"><xsl:value-of select="$config/@clickhouse_user"/></xsl:when><xsl:otherwise>default</xsl:otherwise></xsl:choose>
AI_CLICKHOUSE_PASSWORD='<xsl:value-of select="$config/@clickhouse_password"/>'
AI_CLUSTER_QUERY_TIMEOUT_MS=<xsl:choose><xsl:when test="$config/@cluster_query_timeout_ms"><xsl:value-of select="$config/@cluster_query_timeout_ms"/></xsl:when><xsl:otherwise>5000</xsl:otherwise></xsl:choose>
AI_CLUSTER_MAX_ROWS=<xsl:choose><xsl:when test="$config/@cluster_max_rows"><xsl:value-of select="$config/@cluster_max_rows"/></xsl:when><xsl:otherwise>1000</xsl:otherwise></xsl:choose>
AI_CLUSTER_MAX_RESULT_BYTES=<xsl:choose><xsl:when test="$config/@cluster_max_result_bytes"><xsl:value-of select="$config/@cluster_max_result_bytes"/></xsl:when><xsl:otherwise>1048576</xsl:otherwise></xsl:choose>
AI_CLUSTER_MAX_TOOL_CALLS=<xsl:choose><xsl:when test="$config/@cluster_max_tool_calls"><xsl:value-of select="$config/@cluster_max_tool_calls"/></xsl:when><xsl:otherwise>32</xsl:otherwise></xsl:choose>
AI_MAX_TOOL_CALLS=<xsl:choose><xsl:when test="$config/@max_tool_calls"><xsl:value-of select="$config/@max_tool_calls"/></xsl:when><xsl:otherwise>64</xsl:otherwise></xsl:choose>
AI_MAX_TOOL_RESULT_BYTES=<xsl:choose><xsl:when test="$config/@max_tool_result_bytes"><xsl:value-of select="$config/@max_tool_result_bytes"/></xsl:when><xsl:otherwise>1048576</xsl:otherwise></xsl:choose>
export AI_MODEL AI_MODEL_DIR AI_OLLAMA_BIN AI_LISTEN_HOST AI_PORT
export AI_AGENT_LISTEN_HOST AI_AGENT_PORT AI_MCP_COMMAND AI_MCP_ALLOWED_TOOLS
export AI_CLUSTER_AGENT_LISTEN_HOST AI_CLUSTER_AGENT_PORT
export AI_BROWSER_ALLOWED_DOMAINS AI_BROWSER_PROXY_SERVER AI_BROWSER_ACTION_TIMEOUT_MS
export AI_BROWSER_NAVIGATION_TIMEOUT_MS AI_MAX_TOOL_CALLS AI_MAX_TOOL_RESULT_BYTES
export AI_WEB_MCP_COMMAND AI_WEB_ALLOWED_DOMAINS AI_WEB_PROXY_SERVER
export AI_WEB_REQUEST_TIMEOUT_MS AI_WEB_MAX_DOWNLOAD_BYTES AI_WEB_MAX_CONTENT_CHARS
export AI_CLUSTER_MCP_COMMAND AI_CLUSTER_MCP_ALLOWED_TOOLS AI_POSTGRES_CONNECTION_STRING
export AI_CLICKHOUSE_URL AI_CLICKHOUSE_USER AI_CLICKHOUSE_PASSWORD
export AI_CLUSTER_QUERY_TIMEOUT_MS AI_CLUSTER_MAX_ROWS AI_CLUSTER_MAX_RESULT_BYTES
export AI_CLUSTER_MAX_TOOL_CALLS
  </xsl:template>
</xsl:stylesheet>
