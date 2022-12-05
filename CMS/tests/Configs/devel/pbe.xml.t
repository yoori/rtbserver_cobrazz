<?xml version="1.0" encoding="UTF-8"?>

<colo:colocation
  name="adserver"
  description="Devel config"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xsi:schemaLocation="http://www.foros.com/cms/colocation platform:/resource/coloModel/src/main/model/colocation.xsd">

  <host name="$HOST" hostName="$HOST"/>

  <application descriptor="AdServer-$VERSION">
    <configuration>
      <environment xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration"
        autorestart="0">
        <forosZoneManagement
          user_name="$USER"
          user_group="$USERGROUP"
          ssh_key="$HOME/.ssh/adkey"/>
        <ispZoneManagement separate_isp_zone="true"
          user_name="$USER"
          user_group="$USERGROUP"
          ssh_key="$HOME/.ssh/adkey"/>
        <develParams autorestart_state_root="$WORKSPACE_ROOT/state"/>
      </environment>
    </configuration>

    <serviceGroup descriptor="AdProxyCluster" name="AdProxyCluster$NAME_POSTFIX">

      <configuration>
        <cluster xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
          <environment
            workspace_root="$WORKSPACE_ROOT"
            config_root="$CONFIG_ROOT"
            cache_root="$WORKSPACE_ROOT/cache"
            server_root="$SERVER_ROOT"
            unixcommons_root="$UNIXCOMMONS_ROOT"
            server_bin_root="$SERVER_ROOT/build"
            mib_root="$SERVER_ROOT/CMS/Plugin/data/mibs"
            data_root="$WORKSPACE_ROOT/www"/>
          <secureParams ca="ca.pem"/>
          <campaignServerRef host="$SOURCE_HOST" port="[% SOURCE_PORT_BASE + 6 %]"/>
          <channelControllerRef host="$SOURCE_HOST" port="[% SOURCE_PORT_BASE + 4 %]"/>
          <syncLogs xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="[% PORT_BASE + 12 %]"/>
            <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
            <fileTransferring file_check_period="10" host_check_period="10" port="[% SOURCE_PORT_BASE + 14 %]">
              <logMove host="$HOST"/>
              <dataSourceRef host="foros" path="filesender"/>
            </fileTransferring>
          </syncLogs>
          <snmpStats enable="false"/>
        </cluster>
      </configuration>

      <service
        descriptor="AdProxyCluster/ChannelProxy"
        name="ChannelProxy$NAME_POSTFIX"
        host="$HOST">
        <configuration>
          <channelProxy xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="[% PORT_BASE + 5 %]"/>
[% IF SECURE_CONNECTIONS %]
            <externalNetworkParams port="[% PORT_BASE + 55 %]" host="$HOST"/>
[% ELSE %]
            <externalNetworkParams port="[% PORT_BASE + 55 %]" host="$HOST" secure="false"/>
[% END %]
            <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
          </channelProxy>
        </configuration>
      </service>

      <service
        descriptor="AdProxyCluster/CampaignServer"
        name="CampaignServer$NAME_POSTFIX"
        host="$HOST">
        <configuration>
          <campaignServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="[% PORT_BASE + 6 %]"/>
[% IF SECURE_CONNECTIONS %]
            <externalNetworkParams port="[% PORT_BASE + 56 %]" host="$HOST"/>
[% ELSE %]
            <externalNetworkParams port="[% PORT_BASE + 56 %]" host="$HOST" secure="false"/>
[% END %]
            <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
            <updateParams update_period="30" ecpm_update_period="30"/>
          </campaignServer>
        </configuration>
      </service>

      <service
        descriptor="AdProxyCluster/UserInfoExchanger"
        name="UserInfoExchanger$NAME_POSTFIX"
        host="$HOST">
        <configuration>
          <userInfoExchanger xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="[% PORT_BASE + 10 %]"/>
[% IF SECURE_CONNECTIONS %]
            <externalNetworkParams port="[% PORT_BASE + 60 %]" host="$HOST"/>
[% ELSE %]
            <externalNetworkParams port="[% PORT_BASE + 60 %]" host="$HOST" secure="false"/>
[% END %]
            <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
          </userInfoExchanger>
        </configuration>
      </service>

      <service
        descriptor="AdProxyCluster/STunnelServer"
        name="STunnelServer$NAME_POSTFIX"
        host="$HOST">
        <configuration>
          <sTunnelServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="[% PORT_BASE + 54 %]" internal_port="[% PORT_BASE + 22 %]"/>
          </sTunnelServer>
        </configuration>
      </service>
    </serviceGroup>

  </application>

</colo:colocation>
