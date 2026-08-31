<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:dyn="http://exslt.org/dynamic"
                xmlns:colo="http://www.foros.com/cms/colocation"
                xmlns:cfg="http://www.adintelligence.net/xsd/AI/Configuration"
                exclude-result-prefixes="dyn colo cfg">
  <xsl:output method="text" encoding="utf-8"/>
  <xsl:param name="XPATH"/>
  <xsl:param name="PACKAGE_NAME"/>
  <xsl:param name="BUILD_ROOT"/>
  <xsl:param name="VERSION"/>
  <xsl:param name="RELEASE"/>
  <xsl:param name="COLOCATION_NAME"/>
  <xsl:variable name="application" select="dyn:evaluate($XPATH)"/>
  <xsl:variable name="app-env" select="$application/configuration/cfg:environment"/>
  <xsl:variable name="ai-agent-config" select="$application/serviceGroup[@descriptor = 'AICluster']/service[@descriptor = 'AICluster/AIAgent']/configuration/cfg:aiAgent"/>

  <xsl:template match="/">
    <xsl:variable name="user"><xsl:choose>
      <xsl:when test="$app-env/cfg:forosZoneManagement/@user_name"><xsl:value-of select="$app-env/cfg:forosZoneManagement/@user_name"/></xsl:when>
      <xsl:otherwise>aduser</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="group"><xsl:choose>
      <xsl:when test="$app-env/cfg:forosZoneManagement/@user_group"><xsl:value-of select="$app-env/cfg:forosZoneManagement/@user_group"/></xsl:when>
      <xsl:otherwise>adgroup</xsl:otherwise>
    </xsl:choose></xsl:variable>
    <xsl:variable name="model-package"><xsl:choose>
      <xsl:when test="$ai-agent-config/@model_package"><xsl:value-of select="$ai-agent-config/@model_package"/></xsl:when>
      <xsl:otherwise>foros-ollama-model-qwen3</xsl:otherwise>
    </xsl:choose></xsl:variable>
Name: <xsl:value-of select="$PACKAGE_NAME"/>
Version: <xsl:value-of select="$VERSION"/>
Release: <xsl:value-of select="$RELEASE"/>
Summary: AI server configuration for <xsl:value-of select="$COLOCATION_NAME"/>
License: Commercial
Group: System Environment/Daemons
BuildArch: noarch
BuildRoot: <xsl:value-of select="$BUILD_ROOT"/>
AutoReq: no

Requires: foros-server-ai &gt;= %{version}
Requires: <xsl:value-of select="$model-package"/> &gt;= %{version}
Requires: <xsl:value-of select="$PACKAGE_NAME"/>-local-mgr = %{version}-%{release}

%description
AI server runtime configuration.

%package -n <xsl:value-of select="$PACKAGE_NAME"/>-mgr
Summary: AI server manager configuration
License: Commercial
Group: System Environment/Daemons
BuildArch: noarch
Requires: foros-manager &gt;= 3.5.0
Requires: <xsl:value-of select="$PACKAGE_NAME"/>-local-mgr = %{version}-%{release}

%description -n <xsl:value-of select="$PACKAGE_NAME"/>-mgr
AI server remote manager configuration.

%package -n <xsl:value-of select="$PACKAGE_NAME"/>-local-mgr
Summary: AI server local manager configuration
License: Commercial
Group: System Environment/Daemons
BuildArch: noarch
Requires: foros-manager &gt;= 3.5.0

%description -n <xsl:value-of select="$PACKAGE_NAME"/>-local-mgr
AI server local manager configuration.

%install
pushd <xsl:value-of select="$BUILD_ROOT"/>
if [[ `pwd` != %{buildroot} ]]; then
  mv -T <xsl:value-of select="$BUILD_ROOT"/>/ %{buildroot}/
fi
popd

%pre
%_sbindir/groupadd -r -f <xsl:value-of select="$group"/> &gt;/dev/null 2&gt;&amp;1 ||:
%_sbindir/useradd -r -m -c 'AI services account' -g <xsl:value-of select="$group"/><xsl:text> </xsl:text><xsl:value-of select="$user"/> &gt;/dev/null 2&gt;&amp;1 ||:

%files
%defattr(-, <xsl:value-of select="$user"/>, <xsl:value-of select="$group"/>)
/u01/foros/server-ai/var
%defattr(-, root, root)
/opt/foros/server-ai/etc/<xsl:value-of select="$COLOCATION_NAME"/>

%files -n <xsl:value-of select="$PACKAGE_NAME"/>-mgr
%defattr(-, root, root)
/opt/foros/server-ai/manager/<xsl:value-of select="$COLOCATION_NAME"/>-<xsl:value-of select="$VERSION"/>-<xsl:value-of select="$RELEASE"/>

%files -n <xsl:value-of select="$PACKAGE_NAME"/>-local-mgr
%defattr(-, root, root)
/opt/foros/manager/etc/config.d/server-<xsl:value-of select="$COLOCATION_NAME"/>-<xsl:value-of select="$VERSION"/>-<xsl:value-of select="$RELEASE"/>.xml
%defattr(-, <xsl:value-of select="$user"/>, <xsl:value-of select="$group"/>)
/opt/foros/manager/var/state/server-<xsl:value-of select="$COLOCATION_NAME"/>

%clean
rm -rf %{buildroot}

%changelog
  </xsl:template>
</xsl:stylesheet>
