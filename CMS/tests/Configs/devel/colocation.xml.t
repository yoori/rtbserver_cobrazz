<?xml version="1.0" encoding="UTF-8"?>

<colo:colocation
  name="adserver"
  description="Devel config"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xsi:schemaLocation="http://www.foros.com/cms/colocation platform:/resource/coloModel/src/main/model/colocation.xsd">
  <host name="{$HOST}" hostName="{$HOST}"/>

  <application descriptor="AdServer-{$VERSION}">
    <configuration>
      <environment xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration"
        autorestart="0">
        <forosZoneManagement
          user_name="{$USER}"
          user_group="{$USERGROUP}"
          ssh_key="{$HOME}/.ssh/adkey"/>
          <ispZoneManagement
            user_name="{$USER}"
            user_group="{$USERGROUP}"
            ssh_key="{$HOME}/.ssh/adkey"/>
        <develParams
          management_lib_root="{$SERVER_ROOT}/CMS/Plugin/lib"
          management_bin_root="{$SERVER_ROOT}/CMS/Plugin/exec/bin"
          autorestart_state_root="{$WORKSPACE_ROOT}/state"/>
      </environment>
    </configuration>

  <serviceGroup descriptor="AdCluster" name="AdCluster{$NAME_POSTFIX}">

    <configuration>
      <cluster xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
        <environment
          workspace_root="{$WORKSPACE_ROOT}"
          config_root="{$CONFIG_ROOT}"
          cache_root="{$WORKSPACE_ROOT}/cache"
          server_root="{$SERVER_ROOT}"
          unixcommons_root="{$UNIXCOMMONS_ROOT}"
          data_root="{$WORKSPACE_ROOT}/www"
          server_bin_root="{$SERVER_ROOT}/build"
          mib_root="{$SERVER_ROOT}/CMS/Plugin/data/mibs"
          unixcommons_bin_root="{$UNIXCOMMONS_ROOT}/build"/>
        <coloParams
          colo_id="1"
          segmentor="Polyglot"
          ad_request_profiling="ad-request profiling and stats collection enabled">
          <virtualServer
            port="{ $PORT_BASE + 80 }"
            internal_port="{ $PORT_BASE + 80 }">
            <adservingDomain name="{$HOST}"/>
            <thirdPartyContentDomain name="{$HOST}"/>
            <biddingDomain  name="{$HOST}"/>
          </virtualServer>
          <secureVirtualServer
            port="{ $PORT_BASE + 83 }"
            internal_port="{ $PORT_BASE + 83 }">
            <cert>
              <content>-----BEGIN CERTIFICATE-----
MIIGpzCCBY+gAwIBAgISA4KucHZ4MDLquuYfYG1ToDL/MA0GCSqGSIb3DQEBCwUA
MDIxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQswCQYDVQQD
EwJSMzAeFw0yNDAxMDUxOTIyMjJaFw0yNDA0MDQxOTIyMjFaMB8xHTAbBgNVBAMT
FG5ldy1wcm9ncmFtbWF0aWMuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB
CgKCAQEAqg4PkPG+f0Yckd0BLYGSBUdnPdVCHmr5O/sUK1tWhM2Nvjuyq2TRbDGC
WIAdUPqkVXv0OjIwCI2te1Vu2laqT/edXMv5E4BUug5Hc9upZWLxMQaSDyk7BUUW
X6y9gR2Rb8p3lVn0f4+tPXYlPbGlX8I/b+262KbiX4ZARuN0nGEeuNAkUovpU8n2
Egrf8+tVwFCDC+b8y6PnKuBjg78YsgItimhAWhhaoYLVy4b01Vv9oUPTee7JYNjy
mcVU+V3yVWaGXtRSoGieknkcZ9EGeY0WQpQvQTPaYQYhRIg+owE/5z5Q8GzQTTaO
n9KqE4eF6Ou5Ac5QkNgwKGwoQ0FaJwIDAQABo4IDyDCCA8QwDgYDVR0PAQH/BAQD
AgWgMB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjAMBgNVHRMBAf8EAjAA
MB0GA1UdDgQWBBS1pwv52BqVEvNHPkflsimsxIatDDAfBgNVHSMEGDAWgBQULrMX
t1hWy65QCUDmH6+dixTCxjBVBggrBgEFBQcBAQRJMEcwIQYIKwYBBQUHMAGGFWh0
dHA6Ly9yMy5vLmxlbmNyLm9yZzAiBggrBgEFBQcwAoYWaHR0cDovL3IzLmkubGVu
Y3Iub3JnLzCCAc8GA1UdEQSCAcYwggHCggthZC1ibGFzdC5ydYIOYWQuYWQtYmxh
c3QucnWCF2FkLm5ldy1wcm9ncmFtbWF0aWMuY29tghhhZG1hbmFnZXIuZ2VuaXVz
Z3JvdXAuY2OCF2FkbWFuYWdlci5waGFybWF0aWMucHJvgiBkZXZlbG9wbWVudC5u
ZXctcHJvZ3JhbW1hdGljLmNvbYIPZHNwLmFkLWJsYXN0LnJ1ghhkc3AubmV3LXBy
b2dyYW1tYXRpYy5jb22CEGtpY2stY2FwdGNoYS5jb22CEW1hdGNoLmFkLWJsYXN0
LnJ1ghptYXRjaC5uZXctcHJvZ3JhbW1hdGljLmNvbYIRbWVkaWEuYWQtYmxhc3Qu
cnWCGm1lZGlhLm5ldy1wcm9ncmFtbWF0aWMuY29tghRuZXctcHJvZ3JhbW1hdGlj
LmNvbYIUcGxhdGZvcm0uY29icmF6ei5jb22CDnVpLmNvYnJhenouY29tghd1aS5u
ZXctcHJvZ3JhbW1hdGljLmNvbYIadmlkZW8ubmV3LXByb2dyYW1tYXRpYy5jb22C
D3d3dy5hZC1ibGFzdC5ydYIYd3d3Lm5ldy1wcm9ncmFtbWF0aWMuY29tMBMGA1Ud
IAQMMAowCAYGZ4EMAQIBMIIBBAYKKwYBBAHWeQIEAgSB9QSB8gDwAHcAO1N3dT4t
uYBOizBbBv5AO2fYT8P0x70ADS1yb+H61BcAAAGM20qChwAABAMASDBGAiEAoNL9
KSneFufZNxHICCN+wDw3P8gna7fGcFZNwPCnmBMCIQC1p0pyTtZay4LeA+j5Q7gi
U36k4AQbRxNkx1Rbh9e0MgB1AHb/iD8KtvuVUcJhzPWHujS0pM27KdxoQgqf5mdM
Wjp0AAABjNtKg2QAAAQDAEYwRAIgHNPDfpFV97Zdv+RCpr5DBHWzlIDPr47joJgd
pW96YxECIBsEC15i1G3kY4U3+UoJjgXVheHaKcY2WLGKQMR92hk4MA0GCSqGSIb3
DQEBCwUAA4IBAQCY0oB/T+8W6oCsKOIAmcgY9Rq5/oughVZovO96mTGNmG/j551e
X0MHdA3kaTcp5k5B4IWJHkTDEa9vWN3e6Z0xiYkuASMB9aS5cafLrd69hQZIaMU3
olj/WpQOR3i89Fv95LT7a5JTz46u/vVNvl7eci7wOx4dnh8CFFoGPB4RUeNJrvEP
Y9WdPBVHSqdZw1cpJ76CtiP05sCQBuzhW60WJMHX5k77VjFUFZKTWZbXe7JqFCqV
IPZJupvDi7npOcIRR4evKk+oCk8lYoqvvKZ0WSHG8GywjEOwGxBX9pFz7d4Jnue3
Dc2Pp7Lz+dNX5IJi+OO7mGrwUNO7TRSxNMQe
-----END CERTIFICATE-----</content>
            </cert>
            <key>
              <content>-----BEGIN PRIVATE KEY-----
MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCqDg+Q8b5/RhyR
3QEtgZIFR2c91UIeavk7+xQrW1aEzY2+O7KrZNFsMYJYgB1Q+qRVe/Q6MjAIja17
VW7aVqpP951cy/kTgFS6Dkdz26llYvExBpIPKTsFRRZfrL2BHZFvyneVWfR/j609
diU9saVfwj9v7brYpuJfhkBG43ScYR640CRSi+lTyfYSCt/z61XAUIML5vzLo+cq
4GODvxiyAi2KaEBaGFqhgtXLhvTVW/2hQ9N57slg2PKZxVT5XfJVZoZe1FKgaJ6S
eRxn0QZ5jRZClC9BM9phBiFEiD6jAT/nPlDwbNBNNo6f0qoTh4Xo67kBzlCQ2DAo
bChDQVonAgMBAAECggEAaXvHVBVgIPqQhjocnOmedf/Qnp5GITAh4X7hB/PkOwR2
Q+Yr8fzbms0rGVJ+3NhcT1pQKDkxalzXm4Vm+bCAfJIP3pv3qI0WAUMSN1+uN+Gj
0tFMkHL21VIBit9PvWNl734bp1zhOK8auuEqQJaNClUnnHpOSE1cZfyUqmQ01Tq2
Oh6KLJ159HKAPuJquYKrh93+0jcU0Ik6hbK7+8GE0RS+Cd7KOlePftRwZvlFKG6g
qJ6OpnQVmrOEieW92XnQjJPr5XkQZa3mhKmXzwb2a/kKMIQi685VjPxAcUCM8yzA
5OJSPnCtpi3jEXi0QV5UjLL41bm/Uc7vbifVbcWn4QKBgQDUt4lwK29kIXJkxMWg
eRJxPhYygYuyC9k0ybqc1+OpJl0mnYqebg1jG5xfgueJYV2OExnrvFD+DcCji6NR
4OOVQDJ3e6iXCjynJ0Q/g48LM+cI21jze5gDEZ5G+KKsXGDeX8/O4Eri0CpRuvbC
jke/qy1IlVT+q1Dyg/bcFoEnZQKBgQDMqECBWd7KQP/aiz6wR00I0NwJ9VlorTbd
tkelLczFmfFBeWRrobkKEpVh1WVVBhgFdlbl67IfKaBJOkAXAAvevivEkeDojsBo
DLhmJRPtGN2Bfc/HbCDIQE5NOe4yjs8Bkn6o8199dAH6WPvO+RQ1teAOrzWnkY7K
MUWKgiyAmwKBgGgxIlPcIqq0K1kqfPbWdu8bDzpb8/YZEaJmsU6D/NJspTH19uEV
XN9Pte+eqhTDqVSfDJJIYo+eYt541l2Tv0Xq9Q1Ld8/LxtvFQwutQBKnsKHI9zAE
OPg/1/xBa0Tr1tL1sU9O0793YVM5NgckNAaxLPQhmUlI1tSDSRaSu151AoGAd3Vm
YZqBrvEN5xUMPH0B/Dldlvp9e9pPgk7ZtxLqyhS3fA5NjX3SsoxyYa7b/SsXbmjL
BF31lLcJdnTn4AUI6LV8errj2xsoOBPktsrBvM2cze8QD0mQQRM4IV/FKAGv1y0Z
fajBfkrutKCoKwG8yDUnWarv2KXgASmNe/HL5C0CgYBS0sd/8c/0XCk6zEi/DVjV
bkQC+JSRikhLw4TwQSFJGOefJDBXF5xFpFPYW0eFEYMsSV6rbr5+Sg8a+OR4ivYC
9N/JD/X4sPdN9t78cYc00ThYrt2iisrAVRt/G2sm9QqWmOzO7PVK6V/IVWCmH3Lh
aBJI59KiOrSDXlN5Q2tZzA==
-----END PRIVATE KEY-----</content>
            </key>
            <ca>
              <content>-----BEGIN CERTIFICATE-----
MIIFFjCCAv6gAwIBAgIRAJErCErPDBinU/bWLiWnX1owDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjAwOTA0MDAwMDAw
WhcNMjUwOTE1MTYwMDAwWjAyMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDELMAkGA1UEAxMCUjMwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK
AoIBAQC7AhUozPaglNMPEuyNVZLD+ILxmaZ6QoinXSaqtSu5xUyxr45r+XXIo9cP
R5QUVTVXjJ6oojkZ9YI8QqlObvU7wy7bjcCwXPNZOOftz2nwWgsbvsCUJCWH+jdx
sxPnHKzhm+/b5DtFUkWWqcFTzjTIUu61ru2P3mBw4qVUq7ZtDpelQDRrK9O8Zutm
NHz6a4uPVymZ+DAXXbpyb/uBxa3Shlg9F8fnCbvxK/eG3MHacV3URuPMrSXBiLxg
Z3Vms/EY96Jc5lP/Ooi2R6X/ExjqmAl3P51T+c8B5fWmcBcUr2Ok/5mzk53cU6cG
/kiFHaFpriV1uxPMUgP17VGhi9sVAgMBAAGjggEIMIIBBDAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0lBBYwFAYIKwYBBQUHAwIGCCsGAQUFBwMBMBIGA1UdEwEB/wQIMAYB
Af8CAQAwHQYDVR0OBBYEFBQusxe3WFbLrlAJQOYfr52LFMLGMB8GA1UdIwQYMBaA
FHm0WeZ7tuXkAXOACIjIGlj26ZtuMDIGCCsGAQUFBwEBBCYwJDAiBggrBgEFBQcw
AoYWaHR0cDovL3gxLmkubGVuY3Iub3JnLzAnBgNVHR8EIDAeMBygGqAYhhZodHRw
Oi8veDEuYy5sZW5jci5vcmcvMCIGA1UdIAQbMBkwCAYGZ4EMAQIBMA0GCysGAQQB
gt8TAQEBMA0GCSqGSIb3DQEBCwUAA4ICAQCFyk5HPqP3hUSFvNVneLKYY611TR6W
PTNlclQtgaDqw+34IL9fzLdwALduO/ZelN7kIJ+m74uyA+eitRY8kc607TkC53wl
ikfmZW4/RvTZ8M6UK+5UzhK8jCdLuMGYL6KvzXGRSgi3yLgjewQtCPkIVz6D2QQz
CkcheAmCJ8MqyJu5zlzyZMjAvnnAT45tRAxekrsu94sQ4egdRCnbWSDtY7kh+BIm
lJNXoB1lBMEKIq4QDUOXoRgffuDghje1WrG9ML+Hbisq/yFOGwXD9RiX8F6sw6W4
avAuvDszue5L3sz85K+EC4Y/wFVDNvZo4TYXao6Z0f+lQKc0t8DQYzk1OXVu8rp2
yJMC6alLbBfODALZvYH7n7do1AZls4I9d1P4jnkDrQoxB3UqQ9hVl3LEKQ73xF1O
yK5GhDDX8oVfGKF5u+decIsH4YaTw7mP3GFxJSqv3+0lUFJoi5Lc5da149p90Ids
hCExroL1+7mryIkXPeFM5TgO9r0rvZaBFOvV2z0gp35Z0+L4WPlbuEjN/lxPFin+
HlUjr8gRsI3qfJOQFy/9rKIJR0Y/8Omwt/8oTWgy1mdeHmmjk7j1nYsvC9JSQ6Zv
MldlTTKB3zhThV1+XWYp6rjd5JW1zbVWEkLNxE7GJThEUG3szgBVGP7pSWTUTsqX
nLRbwHOoq7hHwg==
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIFYDCCBEigAwIBAgIQQAF3ITfU6UK47naqPGQKtzANBgkqhkiG9w0BAQsFADA/
MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT
DkRTVCBSb290IENBIFgzMB4XDTIxMDEyMDE5MTQwM1oXDTI0MDkzMDE4MTQwM1ow
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwggIiMA0GCSqGSIb3DQEB
AQUAA4ICDwAwggIKAoICAQCt6CRz9BQ385ueK1coHIe+3LffOJCMbjzmV6B493XC
ov71am72AE8o295ohmxEk7axY/0UEmu/H9LqMZshftEzPLpI9d1537O4/xLxIZpL
wYqGcWlKZmZsj348cL+tKSIG8+TA5oCu4kuPt5l+lAOf00eXfJlII1PoOK5PCm+D
LtFJV4yAdLbaL9A4jXsDcCEbdfIwPPqPrt3aY6vrFk/CjhFLfs8L6P+1dy70sntK
4EwSJQxwjQMpoOFTJOwT2e4ZvxCzSow/iaNhUd6shweU9GNx7C7ib1uYgeGJXDR5
bHbvO5BieebbpJovJsXQEOEO3tkQjhb7t/eo98flAgeYjzYIlefiN5YNNnWe+w5y
sR2bvAP5SQXYgd0FtCrWQemsAXaVCg/Y39W9Eh81LygXbNKYwagJZHduRze6zqxZ
Xmidf3LWicUGQSk+WT7dJvUkyRGnWqNMQB9GoZm1pzpRboY7nn1ypxIFeFntPlF4
FQsDj43QLwWyPntKHEtzBRL8xurgUBN8Q5N0s8p0544fAQjQMNRbcTa0B7rBMDBc
SLeCO5imfWCKoqMpgsy6vYMEG6KDA0Gh1gXxG8K28Kh8hjtGqEgqiNx2mna/H2ql
PRmP6zjzZN7IKw0KKP/32+IVQtQi0Cdd4Xn+GOdwiK1O5tmLOsbdJ1Fu/7xk9TND
TwIDAQABo4IBRjCCAUIwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYw
SwYIKwYBBQUHAQEEPzA9MDsGCCsGAQUFBzAChi9odHRwOi8vYXBwcy5pZGVudHJ1
c3QuY29tL3Jvb3RzL2RzdHJvb3RjYXgzLnA3YzAfBgNVHSMEGDAWgBTEp7Gkeyxx
+tvhS5B1/8QVYIWJEDBUBgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEEAYLfEwEB
ATAwMC4GCCsGAQUFBwIBFiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2VuY3J5cHQu
b3JnMDwGA1UdHwQ1MDMwMaAvoC2GK2h0dHA6Ly9jcmwuaWRlbnRydXN0LmNvbS9E
U1RST09UQ0FYM0NSTC5jcmwwHQYDVR0OBBYEFHm0WeZ7tuXkAXOACIjIGlj26Ztu
MA0GCSqGSIb3DQEBCwUAA4IBAQAKcwBslm7/DlLQrt2M51oGrS+o44+/yQoDFVDC
5WxCu2+b9LRPwkSICHXM6webFGJueN7sJ7o5XPWioW5WlHAQU7G75K/QosMrAdSW
9MUgNTP52GE24HGNtLi1qoJFlcDyqSMo59ahy2cI2qBDLKobkx/J3vWraV0T9VuG
WCLKTVXkcGdtwlfFRjlBz4pYg1htmf5X6DYO8A4jqv2Il9DjXA6USbW1FzXSLr9O
he8Y4IWS6wY7bCkjCWDcRQJMEhg76fsO3txE+FiYruq9RUWhiF1myv4Q6W+CyBFC
Dfvp7OOGAN6dEOM4+qR9sdjoSYKEBpsr6GtPAQw4dy753ec5
-----END CERTIFICATE-----</content>
            </ca>
            <adservingDomain name="{$HOST}"/>
            <thirdPartyContentDomain name="{$HOST}"/>
            <biddingDomain name="{$HOST}"/>
          </secureVirtualServer>
          <RTB yandex_key="04bb79a7741de975e1800764b30c41ef">
            <source id="url" instantiate_type="url" request_type="openrtb with click url" notice="disabled"/>
            <source id="script-url" instantiate_type="script with url" request_type="openrtb with click url" notice="disabled"/>
            <source id="body" instantiate_type="body" request_type="openrtb with click url" notice="disabled"/>
            <source id="url-no-click" instantiate_type="url" request_type="openrtb" notice="disabled"/>
            <source id="body-no-click" instantiate_type="body" request_type="openrtb" notice="disabled"/>
            <source id="iframe-url" instantiate_type="iframe with url" request_type="openrtb" notice="disabled"/>
            <source id="iframe-url-no-click" instantiate_type="iframe with url" request_type="openrtb" notice="disabled"/>
            <source id="body-openx" instantiate_type="body" request_type="openx" notice="disabled"/>
            <source id="iframe-url-openx" instantiate_type="iframe with url" request_type="openx" notice="disabled"/>
            <source id="body-liverail" instantiate_type="body" request_type="liverail" notice="disabled"/>
            <source id="url-liverail" instantiate_type="url" request_type="liverail" notice="disabled"/>
            <source id="body-blank" instantiate_type="body" notice="disabled"/>
            <source id="body-openrtb-notice" instantiate_type="body" request_type="openrtb with click url" notice="nurl"/>
            <source id="iframe-url-adriver" instantiate_type="iframe with url" request_type="adriver" notice="nurl"/>
            <source id="anx" instantiate_type="url parameters"  request_type="appnexus" notice="disabled" appnexus_member_id="1"/>
            <source id="ipw" 
                    instantiate_type="body" 
                    request_type="openrtb" 
                    seat="94"
                    notice="nurl" 
                    vast_instantiate_type="video url" 
                    vast_notice="disabled" 
                    ipw_extension="true"/>
            <source id="google" instantiate_type="iframe with url" request_type="google" vast_instantiate_type="video url" vast_notice="disabled"/>
            <source id="yandex" 
                    instantiate_type="data parameter value" 
                    request_type="yandex" 
                    notice="disabled" 
                    truncate_domain="false" 
                    vast_instantiate_type="data parameter value" 
                    vast_notice="disabled" 
                    ipw_extension="false">
               <userBind redirect="//an.yandex.ru/setud/imarker2/##UNSIGNEDSSPUID=##?sign=##YANDEXSIGN##"  
                         passback="true"/>
            </source>
            <source id="native_js" 
                    instantiate_type="body" 
                    request_type="openrtb with click url" 
                    notice="disabled" 
                    fill_adid="true"
                    native_instantiate_type="adm"
                    native_impression_tracker_type="js"/>
          </RTB>
        </coloParams>
        <inventoryStats simplifying="100"/>
        <snmpStats enable="false"/>
        <userProfiling chunks_count="4" repeat_trigger_timeout="0"/>

        <central>
          <pgConnection connection_string="host=postdb00 port=5432 dbname=stat user=ro password=Q1oL6mm5hPTjnDQ"/>
          <pgConnectionForLogProcessing connection_string="host=postdb00 port=5432 dbname=stat user=ads password=3Azb0uDoR33tPpI"/>
          <statFilesReceiverRef host="stat-dev1" port="10873"/>
          <dataSourceRef host="taskbot64" port="8873"/>
          <segmentorMapping>
            <country name='' segmentor='Polyglot'/>
            <country name='cn' segmentor='Nlpir'/>
          </segmentorMapping>
        </central>
      </cluster>
    </configuration>

    <!-- lp, ui, be -->
    <serviceGroup
      descriptor="AdCluster/BackendSubCluster"
      name="BackendSubCluster{$NAME_POSTFIX}">

      <serviceGroup
        descriptor="AdCluster/BackendSubCluster/LogProcessing"
        name="LogProcessing{$NAME_POSTFIX}">
        <configuration>
          <logProcessing xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <syncLogs>
              <networkParams port="{ $PORT_BASE + 12 }"/>
              <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
              <fileTransferring file_check_period="10" host_check_period="10" logs_backup="all"/>
            </syncLogs>

            <syncLogsServer>
              <networkParams port="{ $PORT_BASE + 14 }"/>
              <logging log_level="{$LOG_LEVEL}" sys_log="false"/>
            </syncLogsServer>
          </logProcessing>
        </configuration>
      </serviceGroup>

      <service
        descriptor="AdCluster/BackendSubCluster/CampaignServer"
        name="CampaignServer{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <campaignServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 6 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
            <updateParams update_period="10" ecpm_update_period="10"/>
          </campaignServer>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/BackendSubCluster/LogGeneralizer"
        name="LogGeneralizer{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <logGeneralizer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 11 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
            <statLogging file_check_period="10" flush_period="10" flush_size="10000"/>
          </logGeneralizer>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/BackendSubCluster/ExpressionMatcher"
        name="ExpressionMatcher{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <expressionMatcher xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 13 }" mon_port="{ $PORT_BASE + 313 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
            <updateParams period="10"/>
            <statLogging file_check_period="10" flush_period="10" activity_flush_period="10" inventory_ecpm_flush_period="10"/>
          </expressionMatcher>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/BackendSubCluster/StatsCollector"
        name="StatsCollector{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <statsCollector xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration" period="3">
            <networkParams port="{ $PORT_BASE + 18 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
          </statsCollector>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/BackendSubCluster/RequestInfoManager"
        name="RequestInfoManager{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <requestInfoManager xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 20 }" mon_port="{ $PORT_BASE + 320 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
            <statLogging file_check_period="10" flush_period="30"/>
          </requestInfoManager>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/BackendSubCluster/DictionaryProvider"
        name="DictionaryProvider{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <dictionaryProvider xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 10 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
          </dictionaryProvider>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/BackendSubCluster/UserOperationGenerator"
        name="UserOperationGeneratorDevel"
        host="{$HOST}">
        <configuration>
          <userOperationGenerator xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 30 }"/>
            <logging log_level="7" sys_log="{$SYS_LOG}"/>
          </userOperationGenerator>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/BackendSubCluster/Predictor"
        name="Predictor"
        host="{$HOST}">
        <configuration>
          <predictor xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <logging log_level="7" sys_log="false"/>
            <syncServer>
              <networkParams port="{ $PORT_BASE + 68 }"/>
            </syncServer>
            <merger max_timeout="600">
              <impression from="0" to="-1"/>
              <click from="-1" to="4"/>
              <action from="-1" to="5"/>
            </merger>
          </predictor>
        </configuration>
      </service>

    </serviceGroup>

    <!-- fe -->
    <serviceGroup
      descriptor="AdCluster/FrontendSubCluster"
      name="FrontendSubCluster{$NAME_POSTFIX}">
      <configuration>
        <frontendCluster xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
          <channelServing>
            <scaleParams chunks_count="10"/>
          </channelServing>
        </frontendCluster>
      </configuration>

      <service
        descriptor="AdCluster/FrontendSubCluster/UserInfoManager"
        name="UserInfoManager{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <userInfoManager xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 1 }"
              grpc_port="{$PORT_BASE + 201}"
              mon_port="{$PORT_BASE + 301}"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
            <updateParams update_period="10"/>
            <profilesCleanupParams life_time="3600" clean_time="00:01"/>
            <matchParams repeat_trigger_timeout="0"/>
          </userInfoManager>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/UserInfoManagerController"
        name="UserInfoManagerController{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <userInfoManagerController xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 2 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
            <controlParams status_check_period="10"/>
          </userInfoManagerController>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/ChannelServer"
        name="ChannelServer{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <channelServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 3 }"
              grpc_port="{$PORT_BASE + 203}"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
            <updateParams period="30"/>
          </channelServer>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/ChannelController"
        name="ChannelController{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <channelController xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 4 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
          </channelController>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/ChannelSearchService"
        name="ChannelSearchService{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <channelSearchService xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 9 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
          </channelSearchService>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/UserBindServer"
        name="UserBindServer{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <userBindServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{$PORT_BASE + 28}"
              grpc_port="{$PORT_BASE + 228}" mon_port="{$PORT_BASE + 328}"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
          </userBindServer>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/UserBindController"
        name="UserBindController{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <userBindController xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams port="{ $PORT_BASE + 29 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
          </userBindController>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/HttpFrontend"
        name="Frontend{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <frontend xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <adHttpNetworkParams/>
            <adFCGINetworkParams port="{ $PORT_BASE + 76 }" mon_port="{ $PORT_BASE + 376 }"/>
            <rtbFCGI1NetworkParams port="{ $PORT_BASE + 77 }" mon_port="{ $PORT_BASE + 377 }"/>
            <rtbFCGI2NetworkParams port="{ $PORT_BASE + 96 }" mon_port="{ $PORT_BASE + 396 }"/>
            <rtbFCGI3NetworkParams port="{ $PORT_BASE + 97 }" mon_port="{ $PORT_BASE + 397 }"/>
            <rtbFCGI4NetworkParams port="{ $PORT_BASE + 98 }" mon_port="{ $PORT_BASE + 398 }"/>
            <rtbFCGI5NetworkParams port="{ $PORT_BASE + 100 }" mon_port="{ $PORT_BASE + 400 }"/>
            <rtbFCGI6NetworkParams port="{ $PORT_BASE + 101 }" mon_port="{ $PORT_BASE + 401 }"/>
            <rtbFCGI7NetworkParams port="{ $PORT_BASE + 102 }" mon_port="{ $PORT_BASE + 402 }"/>
            <rtbFCGI8NetworkParams port="{ $PORT_BASE + 103 }" mon_port="{ $PORT_BASE + 403 }"/>
            <rtbFCGI9NetworkParams port="{ $PORT_BASE + 104 }" mon_port="{ $PORT_BASE + 404 }"/>
            <rtbFCGI10NetworkParams port="{ $PORT_BASE + 105 }" mon_port="{ $PORT_BASE + 405 }"/>
            <rtbFCGI11NetworkParams port="{ $PORT_BASE + 106 }" mon_port="{ $PORT_BASE + 406 }"/>
            <rtbFCGI12NetworkParams port="{ $PORT_BASE + 107 }" mon_port="{ $PORT_BASE + 407 }"/>
            <rtbFCGI13NetworkParams port="{ $PORT_BASE + 108 }" mon_port="{ $PORT_BASE + 408 }"/>
            <rtbFCGI14NetworkParams port="{ $PORT_BASE + 109 }" mon_port="{ $PORT_BASE + 409 }"/>
            <rtbFCGI15NetworkParams port="{ $PORT_BASE + 110 }" mon_port="{ $PORT_BASE + 410 }"/>
            <rtbFCGI16NetworkParams port="{ $PORT_BASE + 111 }" mon_port="{ $PORT_BASE + 411 }"/>

            <trackFCGI1NetworkParams port="{ $PORT_BASE + 95 }" mon_port="{ $PORT_BASE + 395 }"/>
            <trackFCGI2NetworkParams port="{ $PORT_BASE + 99 }" mon_port="{ $PORT_BASE + 399 }"/>

            <userBindFCGI1NetworkParams port="{ $PORT_BASE + 78 }" mon_port="{ $PORT_BASE + 378 }"/>
            <userBindFCGI2NetworkParams port="{ $PORT_BASE + 94 }" mon_port="{ $PORT_BASE + 394 }"/>

            <userBindIntFCGINetworkParams port="{ $PORT_BASE + 79 }" mon_port="{ $PORT_BASE + 379 }"/>
            <userBindAddFCGINetworkParams port="{ $PORT_BASE + 93 }" mon_port="{ $PORT_BASE + 393 }"/>
            <webServerParams max_clients="1000"/>
            <debugInfo use_acl="false" show_history_profile="true"/>
            <requestModule>
              <logging log_level="{$LOG_LEVEL}"/>
              <session timeout="30"/>
            </requestModule>
            <impressionModule>
              <logging log_level="{$LOG_LEVEL}"/>
            </impressionModule>
            <clickModule>
              <logging log_level="{$LOG_LEVEL}"/>
            </clickModule>
            <actionModule>
              <logging log_level="{$LOG_LEVEL}"/>
            </actionModule>
            <optoutModule>
              <logging log_level="{$LOG_LEVEL}" log_ip="true"/>
            </optoutModule>
            <biddingModule max_bid_time="50"/>
            <userBindModule/>
          </frontend>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/BillingServer"
        name="BillingServer{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <billingServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams
              port="{ $PORT_BASE + 31 }"
              grpc_port="${ $PORT_BASE + 231}"/>
            <logging log_level="7" sys_log="false"/>
          </billingServer>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/CampaignManager"
        name="CampaignManager{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <campaignManager xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <networkParams
              port="{ $PORT_BASE + 7 }"
              grpc_port="{ $PORT_BASE + 202 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
            <updateParams
              update_period="10"
              ecpm_update_period="10"
              campaign_types="all"/>
            <statLogging flush_period="10"/>
          </campaignManager>
        </configuration>
      </service>

      <!--
      <service
        descriptor="AdCluster/FrontendSubCluster/ZmqProfilingBalancer"
        name="ZmqProfilingBalancer #1"
        host="{$HOST}">
        <configuration>
          <zmqProfilingBalancer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration" zmq_io_threads="1">
            <networkParams port="{ $PORT_BASE + 74 }"/>
            <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
            <profilingInfoBindSocket hwm="1">
              <address port="{ $PORT_BASE + 88 }"/>
            </profilingInfoBindSocket>
            <anonymousStatsBindSocket hwm="1">
              <address port="{ $PORT_BASE + 89 }"/>
            </anonymousStatsBindSocket>
            <dmpProfilingInfoBindSocket hwm="1">
              <address port="{ $PORT_BASE + 91 }"/>
            </dmpProfilingInfoBindSocket>
          </zmqProfilingBalancer>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/ProfilingServer"
        name="ProfilingServer"
        host="{$HOST}">
      <configuration>
        <profilingServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration" work_threads="5">
          <networkParams port="{ $PORT_BASE + 75 }" profiling_info_port="{ $PORT_BASE + 86 }" anonymous_stats_port="{ $PORT_BASE + 87 }"/>
          <logging log_level="{$LOG_LEVEL}" sys_log="{$SYS_LOG}"/>
        </profilingServer>
      </configuration>
    </service>
    -->

    </serviceGroup>

    <serviceGroup
      descriptor="AdCluster/Tests"
      name="Tests">
      <configuration>
        <testsCommon xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
          <pgConnection connection_string="host=stat-dev1.ocslab.com port=5432 dbname=ads_dev user=test_ads password=adserver"/>
          <!-- UNCOMMENT THIS TO PROCESS RESULT TO TASKBOT -->
          <!-- ATTENTION! taskbot source required -->
          <!-- svn+ssh://svn.ocslab.com/home/svnroot/foros/taskbot/trunk -->
          <!-- <taskbot root-path="{$HOME}/projects/foros/taskbot/trunk/"/> -->
        </testsCommon>
      </configuration>
      <service
        descriptor="AdCluster/Tests/AutoTest"
        name="AutoTest{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <autoTest xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration"
                    nodb="true">
            <!-- ENABLED GROUPS  -->
            <Groups>
              <Group>fast</Group>
            </Groups>
            <!-- THIS IS EXAMPLE OF AUTOTEST SETTINGS -->
            <!-- UNCOMMENT & CHANGE IF IT NEEDED -->
            <!-- ENABLED CATEGORIES NAMES -->
            <!-- <Categories>  -->
            <!--   <Category>Text advertising</Category>  -->
            <!-- </Categories>  -->
            <!-- INCLUDED TESTS NAMES -->
            <!-- <Tests>  -->
            <!--   <Test>CreativePresenceTest</Test>  -->
            <!--   <Test>NegativeMatchTest</Test>  -->
            <!-- </Tests>  -->
            <!-- EXCLUDED TESTS NAMES -->
            <!-- <ExcludedTests>  -->
            <!--   <Test>TextAdvertisingTest</Test>  -->
            <!--   <Test>TextDisplayAdvertisingWatershedTest</Test>  -->
            <!-- </ExcludedTests>  -->

            <!-- This test require specific adserver configuration -->
            <ExcludedTests>
               <Test>RemoteTriggersCheck</Test>
               <Test>BannedChAdReqProfDisabling</Test>
               <Test>PartyMatchTest</Test>
               <Test>AdRequestsProfiling</Test>
               <Test>NonGMTColoHistoryTargeting</Test>
            </ExcludedTests>
          </autoTest>
        </configuration>
      </service>
      <service
        descriptor="AdCluster/Tests/BenchmarkTest"
        name="BenchmarkTest{$NAME_POSTFIX}"
        host="{$HOST}">
        <configuration>
          <benchmarkTest xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <test name="simple"
                  benchmark_size="100000"
                  free_tags_size="30"
                  threads="30">
              <campaigns>
                <campaign count="20">
                 <type>FreqCapsCampaign</type>
                </campaign>
                <campaign count="20">
                 <type>FreqCapsCreative</type>
                </campaign>
                <campaign count="20">
                 <type>FreqCapsCCG</type>
                </campaign>
                <campaign count="20">
                  <type>CampaignSpecificSites</type>
                </campaign>
                <campaign count="20"/>
              </campaigns>
              <channels>
                <channel time_from="0" time_to="2592000" count="30" minimum_visits="1" channel_type="D"/>
                <channel time_from="0" time_to="2592000" count="33"/>
                <channel time_from="0" time_to="300" count="33"/>
                <channel count="34"/>
              </channels>
            </test>
          </benchmarkTest>
        </configuration>
      </service>
    </serviceGroup>

  </serviceGroup>

  </application>

</colo:colocation>
