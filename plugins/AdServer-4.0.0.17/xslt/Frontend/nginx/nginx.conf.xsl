<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:exsl="http://exslt.org/common"
  xmlns:dyn="http://exslt.org/dynamic"
  extension-element-prefixes="exsl"
  exclude-result-prefixes="exsl dyn">

<xsl:output method="text" indent="no" encoding="utf-8"/>

<xsl:key name="domainKey" match="cfg:biddingDomain | cfg:thirdPartyContentDomain" use="@name"/>

<xsl:include href="../../Functions.xsl"/>

<xsl:variable name="xpath" select="dyn:evaluate($XPATH)"/>
<xsl:variable name="out-dir" select="$OUT_DIR"/>
<xsl:variable name="host" select="$HOST"/>
<xsl:variable name="conf-type" select="$CONF_TYPE"/>

<xsl:template name="AddEndPoint">
  <xsl:param name="port"/>
# Listen <xsl:value-of select="$port"/>
</xsl:template>

<!-- httpd config generate function -->
<xsl:template name="httpdConfigGenerator">
  <xsl:param name="app-config"/>
  <xsl:param name="env-config"/>
  <xsl:param name="colo-config"/>
  <xsl:param name="frontend-config"/>

  <xsl:variable name="separate_isp_zone" select="
    $app-config/cfg:ispZoneManagement/@separate_isp_zone |
    $xsd-isp-zone-management-type/xsd:attribute[@name='separate_isp_zone']/@default"/>
  <xsl:variable name="zone_condition"
    select="$separate_isp_zone = '1' or $separate_isp_zone = 'true'"/>

  <xsl:variable name="app-zone-config"
    select="$app-config/cfg:ispZoneManagement[$zone_condition] |
      $app-config/cfg:forosZoneManagement[not($zone_condition)]"/>

  <xsl:variable name="workspace-root"><xsl:value-of select="$env-config/@workspace_root"/>
    <xsl:if test="count($env-config/@workspace_root) = 0"><xsl:value-of select="$def-workspace-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="config-root-base"><xsl:value-of select="$env-config/@config_root"/>
    <xsl:if test="count($env-config/@config_root) = 0"><xsl:value-of select="$def-config-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="config-root" select="concat($config-root-base, '/', $out-dir)"/>
  <xsl:variable name="server-root"><xsl:value-of select="$env-config/@server_root"/>
    <xsl:if test="count($env-config/@server_root) = 0"><xsl:value-of select="$def-server-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="server-bin-root"><xsl:value-of select="$env-config/@server_bin_root"/>
    <xsl:if test="count($env-config/@server_bin_root) = 0"><xsl:value-of select="$server-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="lib-root" select="concat($server-bin-root, '/lib')"/>
  <xsl:variable name="data-root"><xsl:value-of select="$env-config/@data_root"/>
    <xsl:if test="count($env-config/@data_root) = 0"><xsl:value-of select="$def-data-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="user-name">
     <xsl:call-template name="GetUserName">
       <xsl:with-param name="app-env" select="$app-zone-config"/>
     </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="user-group">
    <xsl:call-template name="GetAttr">
      <xsl:with-param name="node" select="$app-zone-config"/>
      <xsl:with-param name="name" select="'user_group'"/>
      <xsl:with-param name="type" select="$xsd-zone-management-type"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="unixcommons-bin-root"><xsl:value-of select="$env-config/@unixcommons_bin_root"/>
    <xsl:if test="count($env-config/@unixcommons_bin_root) = 0"><xsl:value-of select="$server-bin-root"/></xsl:if>
  </xsl:variable>
  <xsl:variable name="unixcommons-bin" select="concat($unixcommons-bin-root, '/bin')"/>
  <xsl:variable name="unixcommons-lib-root" select="concat($unixcommons-bin-root, '/lib')"/>

  <xsl:variable name="adfrontend-port">
    <xsl:value-of select="$frontend-config/cfg:networkParams/@port"/>
    <xsl:if test="count($frontend-config/cfg:networkParams/@port) = 0">
      <xsl:value-of select="$def-frontend-port"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="adfrontend-secure-port">
    <xsl:if test="count($frontend-config/cfg:networkParams/@secure_port) = 0">
      <xsl:value-of select="$def-secure-frontend-port"/>
    </xsl:if>
    <xsl:value-of select="$frontend-config/cfg:networkParams/@secure_port"/>
  </xsl:variable>

  <xsl:variable name="httpd-timeout"><xsl:value-of select="$frontend-config/cfg:webServerParams/@timeout"/>
    <xsl:if test="count($frontend-config/cfg:webServerParams/@timeout) = 0">
      <xsl:value-of select="$web-server-timeout"/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="virtual-servers-raw">
    <xsl:for-each select="$colo-config/cfg:coloParams/cfg:virtualServer[count(@enable)= 0 or @enable='true']">
       <xsl:copy-of select="."/>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="virtual-servers" select="exsl:node-set($virtual-servers-raw)/cfg:virtualServer"/>

  <xsl:variable name="secure-virtual-servers-raw">
    <xsl:for-each select="$colo-config/cfg:coloParams/cfg:secureVirtualServer[count(@enable)= 0 or @enable='true']">
       <xsl:copy-of select="."/>
    </xsl:for-each>
  </xsl:variable>

  <xsl:variable name="secure-virtual-servers" 
     select="exsl:node-set($secure-virtual-servers-raw)/cfg:secureVirtualServer"/>

  <xsl:variable name="ps-res-data-root" select="concat($data-root, '/PageSense')"/>

# For more information on configuration, see:
#   * Official English Documentation: http://nginx.org/en/docs/
#   * Official Russian Documentation: http://nginx.org/ru/docs/

error_log error.log;

# user <xsl:value-of select="concat($user-name,' ',$user-group)"/>;
worker_processes <xsl:call-template name="GetAttr">
  <xsl:with-param name="node" select="$frontend-config/cfg:webServerParams"/>
  <xsl:with-param name="name" select="'server_limit'"/>
  <xsl:with-param name="type" select="$xsd-webserver-params-type"/>
</xsl:call-template>;

#error_log  /var/log/nginx/error.log  notice;
#error_log  /var/log/nginx/error.log  info;

pid "<xsl:value-of select="$workspace-root"/>/run/nginx.pid";

events {
    worker_connections <xsl:call-template name="GetAttr">
  <xsl:with-param name="node" select="$frontend-config/cfg:webServerParams"/>
  <xsl:with-param name="name" select="'max_clients'"/>
  <xsl:with-param name="type" select="$xsd-webserver-params-type"/>
</xsl:call-template>;
}

<xsl:variable name="ps-data-root" select="concat($config-root-base, '/www/PageSense')"/>

http {
    error_log error.log;

    include       /etc/nginx/mime.types;
    default_type  text/plain;

    log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
                      '$status $body_bytes_sent "$http_referer" '
                      '"$http_user_agent" "$http_x_forwarded_for"';

    #access_log <xsl:value-of select="$workspace-root"/>/log/nginx-access.log;
    access_log off;

    client_body_temp_path <xsl:value-of select="$workspace-root"/>/tmp/client_body;
    proxy_temp_path <xsl:value-of select="$workspace-root"/>/tmp/proxy_temp;
    fastcgi_temp_path <xsl:value-of select="$workspace-root"/>/tmp/fastcgi_temp;
    uwsgi_temp_path <xsl:value-of select="$workspace-root"/>/tmp/uwsgi_temp;
    scgi_temp_path <xsl:value-of select="$workspace-root"/>/tmp/scgi_temp;

    sendfile        on;
    #tcp_nopush     on;

    #keepalive_timeout  0;
    client_max_body_size 30720;

    #gzip  on;

    upstream fastcgi_rtbbackend {
      <xsl:variable name="rtbbackend_socket_arr">
        <i>1</i><i>2</i><i>3</i><i>4</i>
      </xsl:variable>
      <xsl:for-each select="exsl:node-set($rtbbackend_socket_arr)/i">
      server unix:<xsl:value-of select="concat($workspace-root,'/run/fcgi_rtbserver',
        ., '.sock')"/> max_fails=0;
      </xsl:for-each>

      keepalive <xsl:value-of select="$frontend-config/cfg:rtbFCGINetworkParams/@keep_connections"/><xsl:if
        test="count($frontend-config/cfg:rtbFCGINetworkParams/@keep_connections) = 0">800</xsl:if>;
    }

    upstream fastcgi_adbackend {
      <xsl:variable name="adbackend_socket_arr">
        <i>1</i><i>2</i><i>3</i><i>4</i>
      </xsl:variable>
      <xsl:for-each select="exsl:node-set($adbackend_socket_arr)/i">
      server unix:<xsl:value-of select="concat($workspace-root,'/run/fcgi_adserver',
       ., '.sock')"/> max_fails=0;
      </xsl:for-each>

      keepalive <xsl:value-of select="$frontend-config/cfg:adFCGINetworkParams/@keep_connections"/><xsl:if
        test="count($frontend-config/cfg:adFCGINetworkParams/@keep_connections) = 0">800</xsl:if>;
    }

    upstream fastcgi_userbindbackend {
      server unix:<xsl:value-of select="concat($workspace-root,'/run/fcgi_userbindserver.sock')"/> max_fails=0;
      keepalive <xsl:value-of select="$frontend-config/cfg:userBindFCGINetworkParams/@keep_connections"/><xsl:if
        test="count($frontend-config/cfg:userBindFCGINetworkParams/@keep_connections) = 0">800</xsl:if>;
    }

    upstream fastcgi_userbindintbackend {
      server unix:<xsl:value-of select="concat($workspace-root,'/run/fcgi_userbindintserver.sock')"/> max_fails=0;
      keepalive <xsl:value-of select="$frontend-config/cfg:userBindIntFCGINetworkParams/@keep_connections"/><xsl:if
        test="count($frontend-config/cfg:userBindIntFCGINetworkParams/@keep_connections) = 0">800</xsl:if>;
    }

    upstream fastcgi_userbindaddbackend {
      server unix:<xsl:value-of select="concat($workspace-root,'/run/fcgi_userbindaddserver.sock')"/> max_fails=0;
      keepalive <xsl:value-of select="$frontend-config/cfg:userBindAddFCGINetworkParams/@keep_connections"/><xsl:if
        test="count($frontend-config/cfg:userBindAddFCGINetworkParams/@keep_connections) = 0">800</xsl:if>;
    }

    <xsl:variable name="cors">
        if ($http_origin)
        {
          add_header Access-Control-Allow-Origin $http_origin;
          add_header Access-Control-Allow-Credentials true;
          add_header Vary Origin;
        }
    </xsl:variable>

    <xsl:variable name="locations">
      <xsl:variable name="tags-data-root" select="concat($data-root, '/tags')"/>

      # Static
      location =/ready {
        root <xsl:value-of select="$config-root"/>/http/htdocs;
      }

      location =/robots.txt {
        root <xsl:value-of select="$config-root"/>/http/htdocs;
      }

      location =/favicon.ico {
        root <xsl:value-of select="$config-root"/>/http/htdocs;
        expires 365d;
      }

      #location =/crossdomain.xml {
      #  alias <xsl:value-of select="$config-root"/>/conf/crossdomain.xml;
      #}

      # Status
      location =/status {
        stub_status;
      }

      location /.well-known/ {
        alias /var/www/html/.well-known/;
      }

      location /log/ {
         alias <xsl:value-of select="$ps-data-root"/>/log/PageSense/;
         gzip on;
         gzip_types text/plain text/xml application/x-javascript;
         expires +4h;
         etag on;
         try_files $uri $uri; 
         location /log/ {
           rewrite /log/([^/]+)/([^?]+) /log/$2 last;
         }
      }

      # Toolbar
      location /toolbar/ {
         alias <xsl:value-of select="$ps-res-data-root"/>/http/toolbar/;
         gzip on;
         gzip_types text/plain text/xml application/x-javascript;
         expires +4h;
         etag on;
         disable_symlinks on;
         <xsl:value-of select="$cors"/>
      }

      # Creatives
      location /creatives/
      {
        alias <xsl:value-of select="$data-root"/>/Creatives/;
        disable_symlinks on from=<xsl:value-of select="$data-root"/>/Creatives/;
        <xsl:value-of select="$cors"/>
      }

      # Templates   
      location /templates/ {
        alias  <xsl:value-of select="$data-root"/>/Templates/;
        <xsl:value-of select="$cors"/>
      }

      location /publ/ {     
         alias <xsl:value-of select="$data-root"/>/Publ/;
         <xsl:value-of select="$cors"/>
      }

    </xsl:variable>

    <xsl:for-each select="$virtual-servers">
      <xsl:variable name="port"><xsl:value-of select="@internal_port"/><xsl:if
        test="count(@internal_port) = 0"><xsl:value-of select="$def-frontend-port"/></xsl:if></xsl:variable>
 
      <xsl:variable name="max-keep-alive-requests"><xsl:choose><xsl:when test="count(@keep_alive)= 0 or @keep_alive = '1'
        or @keep_alive = 'true'"><xsl:value-of select="@max_keep_alive_requests"/><xsl:if
        test="count(@max_keep_alive_requests) = 0">100</xsl:if></xsl:when><xsl:otherwise>0</xsl:otherwise></xsl:choose></xsl:variable>

      <xsl:variable name="keep-alive-timeout"><xsl:choose><xsl:when test="count(@keep_alive)=0 or @keep_alive = '1'
        or @keep_alive = 'true'"><xsl:value-of select="@keep_alive_timeout"/><xsl:if
          test="count(@keep_alive_timeout) = 0">5</xsl:if></xsl:when><xsl:otherwise>0</xsl:otherwise></xsl:choose></xsl:variable>

      <xsl:variable name="all-domains">
        <xsl:for-each select="(cfg:adservingDomain | cfg:redirectDomain | cfg:biddingDomain |
            cfg:videoDomain | cfg:clickDomain | cfg:profilingDomain |
            cfg:thirdPartyContentDomain)/@name">
          <xsl:text> </xsl:text>
          <xsl:value-of select="."/>
        </xsl:for-each>
      </xsl:variable>

      <xsl:variable name="nginx-domains">
        <xsl:for-each select="(cfg:adservingDomain | cfg:redirectDomain | cfg:biddingDomain |
            cfg:videoDomain | cfg:clickDomain | cfg:profilingDomain |
            cfg:thirdPartyContentDomain)
            [not(preceding-sibling::*/@name = @name)]/@name">
          <xsl:text> </xsl:text>
          <xsl:value-of select="."/>
        </xsl:for-each>
      </xsl:variable>

    server {
      <xsl:choose>
        <xsl:when test="@proxy_protocol= 'true' or @proxy_protocol = '1'">
           listen <xsl:value-of select="$port"/> proxy_protocol;
           set_real_ip_from 0.0.0.0/24;
           real_ip_header proxy_protocol;
        </xsl:when>
        <xsl:otherwise>
           listen <xsl:value-of select="$port"/> default_server;
        </xsl:otherwise>
      </xsl:choose>

      <xsl:if test="count(@mon_port) != 0">
           listen <xsl:value-of select="@mon_port"/> default_server;
      </xsl:if>

      server_name <xsl:value-of select="$nginx-domains"/>;

      keepalive_requests <xsl:value-of select="$max-keep-alive-requests"/>;
      keepalive_timeout <xsl:value-of select="$keep-alive-timeout"/>;

      location = / {
        error_page 420 = @rootbind;

        if ( $args ~ "id=" ) { return 420; }
      }

      location @rootbind {
        fastcgi_pass fastcgi_userbindaddbackend;
        fastcgi_keep_conn on;

        fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;        
      }

      <xsl:value-of select="$locations"/>

      # PS content
      location /cs/ {
         alias <xsl:value-of select="$ps-res-data-root"/>/http/cs;
         gzip on;
         gzip_types text/plain text/xml application/x-javascript;
         expires +4h;
         etag on;
         <xsl:value-of select="$cors"/>
      }     

      location /tag/ {
         alias <xsl:value-of select="$ps-res-data-root"/>/http/tag/;
         <xsl:value-of select="$cors"/>
      }

      # Bidding
      location ~ ^/(bid|tanx|baidu|google|openrtb|appnexus)$ {
        fastcgi_pass fastcgi_rtbbackend;
        fastcgi_keep_conn on;

        fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      # UserBind
      location ~ ^/(userbind|userbind[.]gif|userbind[.]png)$ {
        return 301 https://$host$request_uri;

        #fastcgi_pass fastcgi_userbindbackend;
        #fastcgi_keep_conn on;

        #fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        #include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      location ~ ^/(userbindint|userbindint[.]gif|userbindint[.]png)$ {
        return 301 https://$host$request_uri;

        #fastcgi_pass fastcgi_userbindintbackend;
        #fastcgi_keep_conn on;

        #fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        #include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      location ~ ^/(userbindadd|userbindadd[.]gif|userbindadd[.]png)$ {
        return 301 https://$host$request_uri;

        #fastcgi_pass fastcgi_userbindaddbackend;
        #fastcgi_keep_conn on;

        #fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        #include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      # Directory, PubPixel, Content
      location / {
        fastcgi_pass fastcgi_adbackend;
        fastcgi_keep_conn on;

        fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }
     
      error_page   500 502 503 504  /50x.html;
      location = /50x.html {
        root   /usr/share/nginx/html;
      }
    }
    </xsl:for-each>

    <xsl:for-each select="$secure-virtual-servers">

    <xsl:variable name="port"><xsl:value-of select="@internal_port"/><xsl:if
      test="count(@internal_port) = 0"><xsl:value-of select="$def-secure-frontend-port"/></xsl:if></xsl:variable>
 
    <xsl:variable name="max-keep-alive-requests"><xsl:choose><xsl:when test="count(@keep_alive)= 0 or @keep_alive = '1'
        or @keep_alive = 'true'"><xsl:value-of select="@max_keep_alive_requests"/><xsl:if
        test="count(@max_keep_alive_requests) = 0">100</xsl:if></xsl:when><xsl:otherwise>0</xsl:otherwise></xsl:choose></xsl:variable>

    <xsl:variable name="keep-alive-timeout"><xsl:choose><xsl:when test="count(@keep_alive)=0 or @keep_alive = '1'
      or @keep_alive = 'true'"><xsl:value-of select="@keep_alive_timeout"/><xsl:if
        test="count(@keep_alive_timeout) = 0">5</xsl:if></xsl:when><xsl:otherwise>0</xsl:otherwise></xsl:choose></xsl:variable>

    <xsl:variable name="certificate_key"><xsl:value-of
      select="$config-root-base"/>/<xsl:value-of
      select="$out-dir"/>/cert/apkey-<xsl:value-of select="$port"/>.pem</xsl:variable>

    <xsl:variable name="certificate_and_ca"><xsl:value-of
      select="$config-root-base"/>/<xsl:value-of
      select="$out-dir"/>/cert/apcertca-<xsl:value-of select="$port"/>.pem</xsl:variable>

    <xsl:variable name="nginx-domains">
      <xsl:for-each select="(cfg:adservingDomain | cfg:redirectDomain | cfg:biddingDomain |
        cfg:videoDomain | cfg:clickDomain | cfg:profilingDomain |
        cfg:thirdPartyContentDomain)
        [not(preceding-sibling::*/@name = @name)]/@name">
        <xsl:text> </xsl:text>
        <xsl:value-of select="."/>
      </xsl:for-each>
    </xsl:variable>

    server {
      <xsl:choose>
        <xsl:when test="@proxy_protocol= 'true' or @proxy_protocol = '1'">
           listen <xsl:value-of select="$port"/> ssl proxy_protocol;
           set_real_ip_from 0.0.0.0/24;
           real_ip_header proxy_protocol;
        </xsl:when>
        <xsl:otherwise>
           listen <xsl:value-of select="$port"/> ssl;
        </xsl:otherwise>
      </xsl:choose>

      <xsl:if test="count(@mon_port) != 0">
           listen <xsl:value-of select="@mon_port"/> default_server;
      </xsl:if>

      server_name <xsl:value-of select="$nginx-domains"/>;

      ssl_certificate <xsl:value-of select="$certificate_and_ca"/>;
      ssl_certificate_key <xsl:value-of select="$certificate_key"/>;

      ssl_session_timeout 5m;

      ssl_verify_depth 1;
      ssl_protocols SSLv3 TLSv1 TLSv1.1 TLSv1.2;
      ssl_ciphers "HIGH:!aNULL:!MD5 or HIGH:!aNULL:!MD5:!3DES";
      ssl_prefer_server_ciphers on;

      keepalive_requests <xsl:value-of select="$max-keep-alive-requests"/>;
      keepalive_timeout <xsl:value-of select="$keep-alive-timeout"/>;

      location = / {
        error_page 420 = @rootbind;

        if ( $args ~ "id=" ) { return 420; }
      }

      location @rootbind {
        fastcgi_pass fastcgi_userbindaddbackend;
        fastcgi_keep_conn on;

        fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      <xsl:value-of select="$locations"/>

      # PS content
      location /cs/ {
         alias <xsl:value-of select="$ps-res-data-root"/>/https/cs;
         gzip on;
         gzip_types text/plain text/xml application/x-javascript;
         expires +4h;
         etag on;
         <xsl:value-of select="$cors"/>
      }     

      location /tag/ {
         alias <xsl:value-of select="$ps-res-data-root"/>/https/tag/;
         <xsl:value-of select="$cors"/>
      }

      # Bidding
      location ~ ^/(bid|tanx|baidu|google|openrtb|appnexus)$ {
        fastcgi_pass fastcgi_rtbbackend;
        fastcgi_keep_conn on;

        fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      # UserBind
      location ~ ^/(userbind|userbind[.]gif|userbind[.]png)$ {
        fastcgi_pass fastcgi_userbindbackend;
        fastcgi_keep_conn on;

        fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      location ~ ^/(userbindint|userbindint[.]gif|userbindint[.]png)$ {
        fastcgi_pass fastcgi_userbindintbackend;
        fastcgi_keep_conn on;

        fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      location ~ ^/(userbindadd|userbindadd[.]gif|userbindadd[.]png)$ {
        fastcgi_pass fastcgi_userbindaddbackend;
        fastcgi_keep_conn on;

        fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      # Directory, PubPixel, Content
      location / {
        fastcgi_pass fastcgi_adbackend;
        fastcgi_keep_conn on;

        fastcgi_param REMOTE_ADDR <xsl:choose><xsl:when test="@proxy_protocol=
          'true' or @proxy_protocol = '1'">$proxy_protocol_addr;</xsl:when><xsl:otherwise
          >$remote_addr;</xsl:otherwise></xsl:choose>
        include <xsl:value-of select="$config-root"/>/conf/fastcgi_params;
      }

      error_page   500 502 503 504  /50x.html;
      location = /50x.html {
        root   /usr/share/nginx/html;
      }
    }
    </xsl:for-each>
}

<xsl:variable name="log-filename" select="NginxFrontend.log"/>
<xsl:variable name="time-span" select="
  $frontend-config/cfg:logging/@rotate_time |
  $xsd-frontend-logging-params-type/xsd:attribute[@name='rotate_time']/@default"/>
<xsl:variable name="size-span" select="
  $frontend-config/cfg:logging/@rotate_size |
  $xsd-frontend-logging-params-type/xsd:attribute[@name='rotate_size']/@default"/>
# filter apache errors appeared in client request processing context (file not found, invalid method, ...)
# for exclude private information logging and ability to fill up logs by client requests
# ErrorLog "|<xsl:value-of select="$unixcommons-bin"/>/RotateLog --size <xsl:value-of
  select="$size-span"/> --time <xsl:value-of
  select="$time-span"/> --cron 00:00 <xsl:value-of select="$workspace-root"/>/log/<xsl:value-of select="$log-filename"/>"
#ErrorLog  "|/bin/sed -u -r \'s/\\\\[client [0-9.]*\\\\] /[] /\' | /usr/sbin/rotatelogs <xsl:value-of select="$workspace-root"/>/log/error_log.frontend.%Y%m%d 86400"

#ServerTokens Prod
#ServerSignature Off
#TraceEnable Off
#ExtendedStatus On


# RequestReadTimeout header=<xsl:value-of
  select="$frontend-config/cfg:webServerParams/@request_headers_reading_timeout"/><xsl:if
  test="count($frontend-config/cfg:webServerParams/@request_headers_reading_timeout) = 0">
  <xsl:value-of select="$web-server-request-headers-reading-timeout"/>
</xsl:if> body=<xsl:value-of
  select="$frontend-config/cfg:webServerParams/@request_body_reading_timeout"/><xsl:if
  test="count($frontend-config/cfg:webServerParams/@request_body_reading_timeout) = 0">
  <xsl:value-of select="$web-server-request-body-reading-timeout"/>
</xsl:if>

# Include conf/ad_virtual_server.conf

# SetOutputFilter DEFLATE
# AddOutputFilterByType DEFLATE text/html
# AddOutputFilterByType DEFLATE text/css
# AddOutputFilterByType DEFLATE text/plain
# AddOutputFilterByType DEFLATE text/xml
# AddOutputFilterByType DEFLATE text/javascript
# AddOutputFilterByType DEFLATE application/javascript
# AddOutputFilterByType DEFLATE application/x-javascript

</xsl:template>

<!-- -->
<xsl:template match="/">
  <!-- find pathes -->
  <xsl:variable name="app-path" select="$xpath/../../.."/>
  <xsl:variable name="full-cluster-path" select="$xpath/../.."/>
  <xsl:variable name="fe-cluster-path" select="$xpath/.."/>
  <xsl:variable name="frontend-path" select="$fe-cluster-path/service[@descriptor = $http-frontend-descriptor]"/>

  <xsl:choose>
    <!-- check pathes -->
    <xsl:when test="count($xpath) = 0">
      <xsl:message terminate="yes"> ngnix: Can't find XPATH element </xsl:message>
    </xsl:when>

    <xsl:when test="count($full-cluster-path) = 0">
      <xsl:message terminate="yes"> ngnix: Can't find full cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($fe-cluster-path) = 0">
      <xsl:message terminate="yes"> ngnix: Can't find fe-cluster group </xsl:message>
    </xsl:when>

    <xsl:when test="count($frontend-path) = 0">
      <xsl:message terminate="yes"> ngnix: Can't find NginxFrontend services </xsl:message>
    </xsl:when>

  </xsl:choose>

  <!-- find config sections -->

  <xsl:variable
    name="frontend-config" select="$frontend-path/configuration/cfg:frontend"/>

  <xsl:variable
    name="service-config" select="$frontend-path/configuration/cfg:frontend"/>

  <xsl:variable
    name="service-group-config" select="$frontend-path/../configuration/cfg:frontend"/>

  <!-- ???  xsl:variable
    name="frontend-config"
    select="$service-config[count($service-config) > 0] |
            $service-group-config[count($service-config) = 0]"/ -->

  <xsl:variable
    name="colo-config"
    select="$full-cluster-path/configuration/cfg:cluster"/>

  <xsl:variable
    name="fe-cluster-config"
    select="$fe-cluster-path/configuration/cfg:frontendCluster"/>

  <xsl:variable
    name="env-config"
    select="$fe-cluster-config/cfg:environment | $colo-config/cfg:environment"/>

  <!-- check config sections -->
  <xsl:choose>
    <xsl:when test="count($colo-config) = 0">
      <xsl:message terminate="yes"> AdVirtualServer: Can't find full cluster config </xsl:message>
    </xsl:when>
    <xsl:when test="count($frontend-config) = 0">
      <xsl:message terminate="yes"> httpd: Can't find frontend config </xsl:message>
    </xsl:when>
  </xsl:choose>

  <xsl:call-template name="httpdConfigGenerator">
    <xsl:with-param name="app-config" select="$app-path/configuration/cfg:environment"/>
    <xsl:with-param name="env-config" select="$env-config"/>
    <xsl:with-param name="colo-config" select="$colo-config"/>
    <xsl:with-param name="frontend-config" select="$frontend-config"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
