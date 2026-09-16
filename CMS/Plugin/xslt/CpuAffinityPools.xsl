<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:dyn="http://exslt.org/dynamic"
  xmlns:exsl="http://exslt.org/common"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="cfg colo dyn exsl">

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:include href="Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>

<!--
  The points are rounded p75 per-host runnable shares per thread. They were measured on targetrtb
  during the 2026-09-15 10-minute sampling window. Thread counts controlled by CMS are emitted
  separately below; this list contains only fixed-size pools.
-->
<xsl:variable name="fixed-pools">
  <services>
    <service name="CampaignManager">
      <pool name="bs-grpc" threads="4" points="13"/>
      <pool name="bs-grpc-c" threads="1" points="34"/>
      <pool name="event_engine" threads="16" points="21"/>
      <pool name="fast-scheduler" threads="1" points="1"/>
      <pool name="grpc_global" threads="1" points="1"/>
      <pool name="grpcpp_sync" threads="1" points="1"/>
      <pool name="http-server" threads="4" points="1"/>
      <pool name="jemalloc_bg" threads="4" points="1"/>
      <pool name="lifeguard" threads="1" points="1"/>
      <pool name="task-runner" threads="5" points="16"/>
    </service>
    <service name="ChannelServer">
      <pool name="event_engine" threads="16" points="16"/>
      <pool name="fast-scheduler" threads="1" points="1"/>
      <pool name="grpc_global" threads="1" points="1"/>
      <pool name="grpcpp_sync" threads="2" points="1"/>
      <pool name="http-server" threads="4" points="1"/>
      <pool name="jemalloc_bg" threads="4" points="1"/>
      <pool name="lifeguard" threads="1" points="1"/>
      <pool name="task-runner" threads="2" points="1"/>
    </service>
    <service name="FCGIRtbServer">
      <pool name="event_engine" threads="16" points="41"/>
      <pool name="grpc-common" threads="16" points="53"/>
      <pool name="grpc_global" threads="1" points="1"/>
      <pool name="http-server" threads="4" points="1"/>
      <pool name="jemalloc_bg" threads="4" points="2"/>
      <pool name="lifeguard" threads="1" points="1"/>
      <pool name="task-runner" threads="9" points="2"/>
    </service>
    <service name="UserBindServer">
      <pool name="event_engine" threads="16" points="29"/>
      <pool name="fast-scheduler" threads="1" points="52"/>
      <pool name="grpc_global" threads="1" points="1"/>
      <pool name="http-server" threads="4" points="1"/>
      <pool name="jemalloc_bg" threads="4" points="1"/>
      <pool name="lifeguard" threads="1" points="1"/>
      <pool name="task-runner" threads="13" points="1"/>
    </service>
    <service name="UserInfoManager">
      <pool name="event_engine" threads="16" points="22"/>
      <pool name="fast-scheduler" threads="1" points="31"/>
      <pool name="grpc_global" threads="1" points="1"/>
      <pool name="grpcpp_sync" threads="1" points="1"/>
      <pool name="http-server" threads="4" points="1"/>
      <pool name="jemalloc_bg" threads="4" points="1"/>
      <pool name="lifeguard" threads="1" points="1"/>
      <pool name="task-runner" threads="13" points="1"/>
    </service>
  </services>
</xsl:variable>

<xsl:template name="value-or-default">
  <xsl:param name="value"/>
  <xsl:param name="default"/>
  <xsl:choose>
    <xsl:when test="count($value) &gt; 0">
      <xsl:value-of select="$value"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$default"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="max-value">
  <xsl:param name="left"/>
  <xsl:param name="right"/>
  <xsl:choose>
    <xsl:when test="number($left) &gt; number($right)">
      <xsl:value-of select="$left"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$right"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="emit-pool">
  <xsl:param name="numa-node"/>
  <xsl:param name="service"/>
  <xsl:param name="pool"/>
  <xsl:param name="threads"/>
  <xsl:param name="points"/>
  <xsl:value-of select="$numa-node"/>
  <xsl:text>&#9;</xsl:text>
  <xsl:value-of select="concat($service, '/', $pool)"/>
  <xsl:text>&#9;</xsl:text>
  <xsl:value-of select="$threads"/>
  <xsl:text>&#9;</xsl:text>
  <xsl:value-of select="$points"/>
  <xsl:text>&#10;</xsl:text>
</xsl:template>

<xsl:template name="emit-fixed-pools">
  <xsl:param name="numa-node"/>
  <xsl:param name="service"/>
  <xsl:for-each
    select="exsl:node-set($fixed-pools)/services/service[@name = $service]/pool">
    <xsl:call-template name="emit-pool">
      <xsl:with-param name="numa-node" select="$numa-node"/>
      <xsl:with-param name="service" select="$service"/>
      <xsl:with-param name="pool" select="@name"/>
      <xsl:with-param name="threads" select="@threads"/>
      <xsl:with-param name="points" select="@points"/>
    </xsl:call-template>
  </xsl:for-each>
</xsl:template>

<xsl:template name="emit-grpc-service-pools">
  <xsl:param name="config"/>
  <xsl:param name="service"/>
  <xsl:param name="process-pool"/>
  <xsl:param name="process-points"/>
  <xsl:param name="server-points"/>
  <xsl:param name="default-process-threads"/>
  <xsl:param name="default-cq-threads"/>
  <xsl:param name="batching-threads"/>
  <xsl:param name="batching-points"/>

  <xsl:variable name="numa-node">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/@numa_node"/>
      <xsl:with-param name="default" select="0"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="process-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:networkParams/@grpc_process_threads"/>
      <xsl:with-param name="default" select="$default-process-threads"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="cq-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:networkParams/@grpc_cq_threads"/>
      <xsl:with-param name="default" select="$default-cq-threads"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="threads-per-cq">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:networkParams/@grpc_threads_per_cq"/>
      <xsl:with-param name="default" select="1"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="grpc-server-threads"
    select="number($cq-threads) * number($threads-per-cq)"/>

  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="$service"/>
    <xsl:with-param name="pool" select="$process-pool"/>
    <xsl:with-param name="threads" select="$process-threads"/>
    <xsl:with-param name="points" select="$process-points"/>
  </xsl:call-template>
  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="$service"/>
    <xsl:with-param name="pool" select="'grpc-server'"/>
    <xsl:with-param name="threads" select="$grpc-server-threads"/>
    <xsl:with-param name="points" select="$server-points"/>
  </xsl:call-template>
  <xsl:if test="string-length($batching-threads) &gt; 0">
    <xsl:call-template name="emit-pool">
      <xsl:with-param name="numa-node" select="$numa-node"/>
      <xsl:with-param name="service" select="$service"/>
      <xsl:with-param name="pool" select="'rdb-batch'"/>
      <xsl:with-param name="threads" select="$batching-threads"/>
      <xsl:with-param name="points" select="$batching-points"/>
    </xsl:call-template>
  </xsl:if>
  <xsl:call-template name="emit-fixed-pools">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="$service"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="emit-campaign-manager">
  <xsl:param name="config"/>
  <xsl:variable name="numa-node">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/@numa_node"/>
      <xsl:with-param name="default" select="0"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="logging-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:statLogging/@threads"/>
      <xsl:with-param name="default" select="8"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="emit-grpc-service-pools">
    <xsl:with-param name="config" select="$config"/>
    <xsl:with-param name="service" select="'CampaignManager'"/>
    <xsl:with-param name="process-pool" select="'cm-grpc-p'"/>
    <xsl:with-param name="process-points" select="41"/>
    <xsl:with-param name="server-points" select="16"/>
    <xsl:with-param name="default-process-threads" select="32"/>
    <xsl:with-param name="default-cq-threads" select="4"/>
  </xsl:call-template>
  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'CampaignManager'"/>
    <xsl:with-param name="pool" select="'cm-logging'"/>
    <xsl:with-param name="threads" select="$logging-threads"/>
    <xsl:with-param name="points" select="56"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="emit-frontend">
  <xsl:param name="config"/>
  <xsl:variable name="numa-node">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:rtbConfig/@numa_node"/>
      <xsl:with-param name="default" select="0"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="action-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:actionModule/@threads"/>
      <xsl:with-param name="default" select="32"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="click-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:clickModule/@threads"/>
      <xsl:with-param name="default" select="32"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="impression-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:impressionModule/@threads"/>
      <xsl:with-param name="default" select="32"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="track-threads">
    <xsl:call-template name="max-value">
      <xsl:with-param name="left" select="$click-threads"/>
      <xsl:with-param name="right" select="$impression-threads"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="track-request-threads">
    <xsl:call-template name="max-value">
      <xsl:with-param name="left" select="$track-threads"/>
      <xsl:with-param name="right" select="32"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="action-request-threads">
    <xsl:call-template name="max-value">
      <xsl:with-param name="left" select="$action-threads"/>
      <xsl:with-param name="right" select="32"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="ad-request-threads-1">
    <xsl:call-template name="max-value">
      <xsl:with-param name="left" select="$action-request-threads"/>
      <xsl:with-param name="right" select="$track-request-threads"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="ad-request-threads">
    <xsl:call-template name="max-value">
      <xsl:with-param name="left" select="$ad-request-threads-1"/>
      <xsl:with-param name="right" select="128"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="user-bind-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:userBindModule/@threads"/>
      <xsl:with-param name="default" select="32"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="rtb-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:biddingModule/@threads"/>
      <xsl:with-param name="default" select="32"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="rtb-interrupt-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:biddingModule/@interrupt_threads"/>
      <xsl:with-param name="default" select="10"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="grpc-coalesce-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:rtbConfig/@grpc_coalesce_threads"/>
      <xsl:with-param name="default" select="16"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="accept-threads">
    <xsl:call-template name="value-or-default">
      <xsl:with-param name="value" select="$config/cfg:rtbFCGI1NetworkParams/@accept_threads"/>
      <xsl:with-param name="default" select="10"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'FCGIActionServer'"/>
    <xsl:with-param name="pool" select="'bid-request'"/>
    <xsl:with-param name="threads" select="$action-request-threads"/>
    <xsl:with-param name="points" select="1"/>
  </xsl:call-template>
  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'FCGIAdServer'"/>
    <xsl:with-param name="pool" select="'bid-request'"/>
    <xsl:with-param name="threads" select="$ad-request-threads"/>
    <xsl:with-param name="points" select="1"/>
  </xsl:call-template>
  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'FCGIRtbServer'"/>
    <xsl:with-param name="pool" select="'bid-request'"/>
    <xsl:with-param name="threads" select="$rtb-threads"/>
    <xsl:with-param name="points" select="82"/>
  </xsl:call-template>
  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'FCGIRtbServer'"/>
    <xsl:with-param name="pool" select="'fast-scheduler'"/>
    <xsl:with-param name="threads" select="$rtb-interrupt-threads"/>
    <xsl:with-param name="points" select="61"/>
  </xsl:call-template>
  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'FCGIRtbServer'"/>
    <xsl:with-param name="pool" select="'fcgi-accept'"/>
    <xsl:with-param name="threads" select="number($accept-threads) * 4"/>
    <xsl:with-param name="points" select="77"/>
  </xsl:call-template>
  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'FCGIRtbServer'"/>
    <xsl:with-param name="pool" select="'grpc-asio-p'"/>
    <xsl:with-param name="threads" select="$grpc-coalesce-threads"/>
    <xsl:with-param name="points" select="53"/>
  </xsl:call-template>
  <xsl:call-template name="emit-fixed-pools">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'FCGIRtbServer'"/>
  </xsl:call-template>
  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'FCGITrackServer'"/>
    <xsl:with-param name="pool" select="'bid-request'"/>
    <xsl:with-param name="threads" select="$track-request-threads"/>
    <xsl:with-param name="points" select="1"/>
  </xsl:call-template>
  <xsl:call-template name="emit-pool">
    <xsl:with-param name="numa-node" select="$numa-node"/>
    <xsl:with-param name="service" select="'FCGIUserBindServer'"/>
    <xsl:with-param name="pool" select="'bid-request'"/>
    <xsl:with-param name="threads" select="$user-bind-threads"/>
    <xsl:with-param name="points" select="5"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="/">
  <xsl:text>numa_node&#9;pool&#9;threads&#9;points&#10;</xsl:text>

  <xsl:for-each select="$xpath/service">
    <xsl:variable name="hosts">
      <xsl:call-template name="GetHosts">
        <xsl:with-param name="hosts" select="@host"/>
        <xsl:with-param name="error-prefix" select="'CPU affinity generation'"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="exsl:node-set($hosts)/host = $HOST">
      <xsl:choose>
        <xsl:when test="@descriptor = $campaign-manager-descriptor">
          <xsl:variable name="service-config" select="configuration/cfg:campaignManager"/>
          <xsl:variable name="group-config" select="../configuration/cfg:campaignManager"/>
          <xsl:call-template name="emit-campaign-manager">
            <xsl:with-param name="config"
              select="$service-config | $group-config[count($service-config) = 0]"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="@descriptor = $channel-server-descriptor">
          <xsl:variable name="service-config" select="configuration/cfg:channelServer"/>
          <xsl:variable name="group-config"
            select="../configuration/cfg:frontendCluster/cfg:channelServer"/>
          <xsl:call-template name="emit-grpc-service-pools">
            <xsl:with-param name="config"
              select="$service-config | $group-config[count($service-config) = 0]"/>
            <xsl:with-param name="service" select="'ChannelServer'"/>
            <xsl:with-param name="process-pool" select="'cs-grpc-p'"/>
            <xsl:with-param name="process-points" select="54"/>
            <xsl:with-param name="server-points" select="13"/>
            <xsl:with-param name="default-process-threads" select="32"/>
            <xsl:with-param name="default-cq-threads" select="4"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="@descriptor = $http-frontend-descriptor">
          <xsl:call-template name="emit-frontend">
            <xsl:with-param name="config" select="configuration/cfg:frontend"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="@descriptor = $user-bind-server-descriptor">
          <xsl:variable name="config" select="configuration/cfg:userBindServer"/>
          <xsl:variable name="batching-threads">
            <xsl:call-template name="value-or-default">
              <xsl:with-param name="value" select="$config/@rocksdb_batching_threads"/>
              <xsl:with-param name="default" select="16"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:call-template name="emit-grpc-service-pools">
            <xsl:with-param name="config" select="$config"/>
            <xsl:with-param name="service" select="'UserBindServer'"/>
            <xsl:with-param name="process-pool" select="'ub-grpc-p'"/>
            <xsl:with-param name="process-points" select="80"/>
            <xsl:with-param name="server-points" select="19"/>
            <xsl:with-param name="default-process-threads" select="32"/>
            <xsl:with-param name="default-cq-threads" select="4"/>
            <xsl:with-param name="batching-threads" select="$batching-threads"/>
            <xsl:with-param name="batching-points" select="81"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="@descriptor = $user-info-manager-descriptor">
          <xsl:variable name="config" select="configuration/cfg:userInfoManager"/>
          <xsl:variable name="batching-threads">
            <xsl:call-template name="value-or-default">
              <xsl:with-param name="value"
                select="$config/cfg:matchParams/@rocksdb_batching_threads"/>
              <xsl:with-param name="default" select="16"/>
            </xsl:call-template>
          </xsl:variable>
          <xsl:call-template name="emit-grpc-service-pools">
            <xsl:with-param name="config" select="$config"/>
            <xsl:with-param name="service" select="'UserInfoManager'"/>
            <xsl:with-param name="process-pool" select="'uim-grpc-p'"/>
            <xsl:with-param name="process-points" select="62"/>
            <xsl:with-param name="server-points" select="11"/>
            <xsl:with-param name="default-process-threads" select="16"/>
            <xsl:with-param name="default-cq-threads" select="4"/>
            <xsl:with-param name="batching-threads" select="$batching-threads"/>
            <xsl:with-param name="batching-points" select="50"/>
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>
    </xsl:if>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
