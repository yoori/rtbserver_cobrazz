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
        <develParams
          management_lib_root="$SERVER_ROOT/CMS/Plugin/lib"
          management_bin_root="$SERVER_ROOT/CMS/Plugin/exec/bin"
          autorestart_state_root="$WORKSPACE_ROOT/state"/>
      </environment>
    </configuration>

    <serviceGroup descriptor="AdCluster" name="AdClusterDevel$NAME_POSTFIX">

      <configuration>
        <cluster xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
          <environment
            workspace_root="$WORKSPACE_ROOT"
            config_root="$CONFIG_ROOT"
            cache_root="$WORKSPACE_ROOT/cache"
            server_root="$SERVER_ROOT"
            unixcommons_root="$UNIXCOMMONS_ROOT"
            data_root="$WORKSPACE_ROOT/www"
            server_bin_root="$SERVER_ROOT/build"
            mib_root="$SERVER_ROOT/CMS/Plugin/data/mibs"
            unixcommons_bin_root="$UNIXCOMMONS_ROOT/build"/>
          <secureParams ca="ca.pem"/>
          <coloParams
            colo_id="1"
            segmentor="Polyglot"
            ad_request_profiling="ad-request profiling and stats collection enabled">
            <virtualServer
              port="[% PORT_BASE + 80 %]"
              https_port="[% PORT_BASE + 81 %]"
              internal_port="[% PORT_BASE + 80 %]"
              internal_https_port="[% PORT_BASE + 81 %]">
            <httpsParams>
              <cert>
                <content>
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 1 (0x1)
        Signature Algorithm: sha1WithRSAEncryption
        Issuer: C=RU, ST=Moscow, L=Moscow, O=Phorm Inc, OU=Phorm Moscow, CN=*.ocslab.com/emailAddress=server@ocslab.com
        Validity
            Not Before: Jun 23 08:25:15 2010 GMT
            Not After : Jun 20 08:25:15 2020 GMT
        Subject: C=RU, ST=Moscow, L=Moscow, O=Phorm Inc, OU=Phorm Moscow, CN=*.ocslab.com/emailAddress=server@ocslab.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
            RSA Public Key: (1024 bit)
                Modulus (1024 bit):
                    00:96:e1:78:f7:64:2f:4b:55:85:94:db:1c:05:d9:
                    46:aa:5e:ea:13:62:ec:c2:db:7e:c5:a2:2e:a8:00:
                    52:16:d0:a0:5b:1b:d1:a9:b5:cf:32:ce:08:05:2b:
                    75:ef:ab:f9:5d:07:db:b3:1a:e3:23:82:f2:67:89:
                    d9:1b:9f:42:3b:3d:c8:9a:81:bc:59:5b:a6:bf:0b:
                    6e:9b:f9:87:8e:a1:59:f7:8d:45:f1:41:17:f1:9d:
                    79:7a:85:4d:6d:56:95:8f:06:e5:92:54:8d:21:8e:
                    4b:bd:eb:24:52:55:d7:54:83:65:d6:ab:77:2e:da:
                    57:5c:9d:95:24:56:ce:66:25
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Basic Constraints:
                CA:TRUE
            Netscape Comment:
                OpenSSL Generated Certificate
            X509v3 Subject Key Identifier:
                E3:53:75:9B:4C:C5:11:1E:56:D1:26:DC:51:1B:BD:86:24:F2:90:27
            X509v3 Authority Key Identifier:
                keyid:1E:9E:ED:43:8E:23:07:3B:FC:2E:62:8A:1A:3E:46:A1:CA:21:E4:6B

    Signature Algorithm: sha1WithRSAEncryption
        58:1b:d6:5f:8b:d1:b6:da:a9:ce:46:12:84:ec:cf:10:1a:40:
        b3:ab:5b:d0:17:73:47:ea:24:01:06:85:8a:33:f1:04:a7:df:
        20:e3:2d:2a:e4:63:aa:29:c3:7e:29:be:f4:b5:ee:74:d5:56:
        29:8d:88:cd:44:2d:e3:9b:27:41:09:f7:44:38:f4:7e:b8:8d:
        d2:3d:89:36:83:ab:b6:f9:70:6e:99:8f:b5:7e:96:f9:a1:0e:
        5c:0d:e8:d4:2d:72:a1:87:e9:8d:cd:60:3a:14:c9:11:f3:1d:
        05:e1:e5:36:23:0f:e9:57:8c:59:ee:fb:e3:ee:2f:f8:8f:0d:
        87:9d
-----BEGIN CERTIFICATE-----
MIIDHDCCAoWgAwIBAgIBATANBgkqhkiG9w0BAQUFADCBkzELMAkGA1UEBhMCUlUx
DzANBgNVBAgTBk1vc2NvdzEPMA0GA1UEBxMGTW9zY293MRIwEAYDVQQKEwlQaG9y
bSBJbmMxFTATBgNVBAsTDFBob3JtIE1vc2NvdzEVMBMGA1UEAwwMKi5vY3NsYWIu
Y29tMSAwHgYJKoZIhvcNAQkBFhFzZXJ2ZXJAb2NzbGFiLmNvbTAeFw0xMDA2MjMw
ODI1MTVaFw0yMDA2MjAwODI1MTVaMIGTMQswCQYDVQQGEwJSVTEPMA0GA1UECBMG
TW9zY293MQ8wDQYDVQQHEwZNb3Njb3cxEjAQBgNVBAoTCVBob3JtIEluYzEVMBMG
A1UECxMMUGhvcm0gTW9zY293MRUwEwYDVQQDDAwqLm9jc2xhYi5jb20xIDAeBgkq
hkiG9w0BCQEWEXNlcnZlckBvY3NsYWIuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GN
ADCBiQKBgQCW4Xj3ZC9LVYWU2xwF2UaqXuoTYuzC237Foi6oAFIW0KBbG9Gptc8y
zggFK3Xvq/ldB9uzGuMjgvJnidkbn0I7PciagbxZW6a/C26b+YeOoVn3jUXxQRfx
nXl6hU1tVpWPBuWSVI0hjku96yRSVddUg2XWq3cu2ldcnZUkVs5mJQIDAQABo34w
fDAMBgNVHRMEBTADAQH/MCwGCWCGSAGG+EIBDQQfFh1PcGVuU1NMIEdlbmVyYXRl
ZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQU41N1m0zFER5W0SbcURu9hiTykCcwHwYD
VR0jBBgwFoAUHp7tQ44jBzv8LmKKGj5Gocoh5GswDQYJKoZIhvcNAQEFBQADgYEA
WBvWX4vRttqpzkYShOzPEBpAs6tb0BdzR+okAQaFijPxBKffIOMtKuRjqinDfim+
9LXudNVWKY2IzUQt45snQQn3RDj0friN0j2JNoOrtvlwbpmPtX6W+aEOXA3o1C1y
oYfpjc1gOhTJEfMdBeHlNiMP6VeMWe774+4v+I8Nh50=
-----END CERTIFICATE-----
                </content>
              </cert>
              <key>
                <content>
-----BEGIN RSA PRIVATE KEY-----
MIICXAIBAAKBgQCW4Xj3ZC9LVYWU2xwF2UaqXuoTYuzC237Foi6oAFIW0KBbG9Gp
tc8yzggFK3Xvq/ldB9uzGuMjgvJnidkbn0I7PciagbxZW6a/C26b+YeOoVn3jUXx
QRfxnXl6hU1tVpWPBuWSVI0hjku96yRSVddUg2XWq3cu2ldcnZUkVs5mJQIDAQAB
AoGAE4cHAu3CeTuOWF+rVs5yKOaz4OJyxh1mYOnGpBO2jCFgP6pwDkjrkiW8P/8J
+oUFdxbnRlz6fMQb326l3L9V8OYC6PDVX9EGMX90pJLWaaGKuEsB+sdxsH50s/Px
Kx9eYUdWfpDiheTAmJJgTfdGLCqOsODEekJuFpfNPlEhYxkCQQDH24m9gTDsL8hZ
rN4yKnAU38zMEikeIkGlN2PCAzp+VHkTf7IqdMwKLWb8DfADFsZ5Rtm8du8j5vuI
LgquuzKfAkEAwUPWw+t1gdCWThfReQsC51+cun5lpC7aZpDZt11NNQwgtDRheyE+
iiPaYB8BdAIQ0wtELa1mUVH8V0OK8mQUuwJAR6bl0xMmPwdChSP85W9hj5pNCjzY
kP0nG9yn3z7ZEcGnRt5ZOe9115A/g85bZkAcGA7WsULSqcR+GWyoV8y1cQJAO2+c
jfZM1haLEws6LaqYQwXhkm0q3xpVqnNjsYOtKeJH8IOncxGfRSaPkz4V2VKFUBJH
7nBEZj/7he7MvYzP3wJBAIwf4JzW9Lby4kRR1WscLhzJhi3Hot5wUvC4X/BegLsK
e4snuz4RmM6E0zm5syW+4rDmKekhavfIpiGvjxN8t58=
-----END RSA PRIVATE KEY-----
                </content>
              </key>
              <ca>
                <content>
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 0 (0x0)
        Signature Algorithm: sha1WithRSAEncryption
        Issuer: C=RU, ST=Moscow, L=Moscow, O=Phorm Inc, OU=Phorm Moscow, CN=*.ocslab.com/emailAddress=server@ocslab.com
        Validity
            Not Before: Jun 23 08:24:49 2010 GMT
            Not After : Jun 18 08:24:49 2030 GMT
        Subject: C=RU, ST=Moscow, L=Moscow, O=Phorm Inc, OU=Phorm Moscow, CN=*.ocslab.com/emailAddress=server@ocslab.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
            RSA Public Key: (1024 bit)
                Modulus (1024 bit):
                    00:bc:9a:50:1e:46:e5:87:1d:4a:74:f2:39:b0:c0:
                    f2:dc:4b:32:5d:50:b0:40:9f:55:86:af:f4:d7:64:
                    cc:44:8a:10:d6:70:0c:a8:0a:03:0f:13:49:8e:ca:
                    93:51:07:16:e4:c2:be:21:a7:0d:77:da:52:ef:4e:
                    44:d2:91:9f:98:3d:e6:1c:09:ad:2a:d4:69:e6:e7:
                    db:87:d6:d7:fe:ba:fb:f3:fa:8d:92:eb:18:b9:3e:
                    0c:db:ec:65:d6:42:87:f0:ad:ab:32:fc:c3:f7:8d:
                    94:d9:81:30:d5:33:7c:1a:d0:4e:25:b8:8c:f1:51:
                    5d:0b:94:fc:9a:5e:47:68:31
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Basic Constraints:
                CA:TRUE
            Netscape Comment:
                OpenSSL Generated Certificate
            X509v3 Subject Key Identifier:
                1E:9E:ED:43:8E:23:07:3B:FC:2E:62:8A:1A:3E:46:A1:CA:21:E4:6B
            X509v3 Authority Key Identifier:
                keyid:1E:9E:ED:43:8E:23:07:3B:FC:2E:62:8A:1A:3E:46:A1:CA:21:E4:6B

    Signature Algorithm: sha1WithRSAEncryption
        b4:49:a8:86:7f:a5:87:bd:b6:81:58:18:70:55:ee:a2:00:e0:
        8c:10:55:1d:4f:9d:c2:88:fb:7e:6e:30:98:fd:87:b1:b8:70:
        96:f8:ad:61:2d:7c:d9:80:24:89:de:c6:ee:0f:01:77:16:ae:
        62:83:62:fe:44:14:61:20:48:12:a2:d6:45:58:08:7c:2d:bf:
        e3:7b:39:6f:4e:8b:47:dd:a7:4a:77:97:3b:17:56:76:71:b0:
        86:f4:3f:bf:4b:40:05:c4:6c:d3:75:5d:7a:6d:3e:d3:ef:07:
        4e:d6:65:37:35:b9:57:fa:77:d0:87:7a:1f:f5:83:51:34:a5:
        bb:f2
-----BEGIN CERTIFICATE-----
MIIDHDCCAoWgAwIBAgIBADANBgkqhkiG9w0BAQUFADCBkzELMAkGA1UEBhMCUlUx
DzANBgNVBAgTBk1vc2NvdzEPMA0GA1UEBxMGTW9zY293MRIwEAYDVQQKEwlQaG9y
bSBJbmMxFTATBgNVBAsTDFBob3JtIE1vc2NvdzEVMBMGA1UEAwwMKi5vY3NsYWIu
Y29tMSAwHgYJKoZIhvcNAQkBFhFzZXJ2ZXJAb2NzbGFiLmNvbTAeFw0xMDA2MjMw
ODI0NDlaFw0zMDA2MTgwODI0NDlaMIGTMQswCQYDVQQGEwJSVTEPMA0GA1UECBMG
TW9zY293MQ8wDQYDVQQHEwZNb3Njb3cxEjAQBgNVBAoTCVBob3JtIEluYzEVMBMG
A1UECxMMUGhvcm0gTW9zY293MRUwEwYDVQQDDAwqLm9jc2xhYi5jb20xIDAeBgkq
hkiG9w0BCQEWEXNlcnZlckBvY3NsYWIuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GN
ADCBiQKBgQC8mlAeRuWHHUp08jmwwPLcSzJdULBAn1WGr/TXZMxEihDWcAyoCgMP
E0mOypNRBxbkwr4hpw132lLvTkTSkZ+YPeYcCa0q1Gnm59uH1tf+uvvz+o2S6xi5
Pgzb7GXWQofwrasy/MP3jZTZgTDVM3wa0E4luIzxUV0LlPyaXkdoMQIDAQABo34w
fDAMBgNVHRMEBTADAQH/MCwGCWCGSAGG+EIBDQQfFh1PcGVuU1NMIEdlbmVyYXRl
ZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQUHp7tQ44jBzv8LmKKGj5Gocoh5GswHwYD
VR0jBBgwFoAUHp7tQ44jBzv8LmKKGj5Gocoh5GswDQYJKoZIhvcNAQEFBQADgYEA
tEmohn+lh722gVgYcFXuogDgjBBVHU+dwoj7fm4wmP2HsbhwlvitYS182YAkid7G
7g8BdxauYoNi/kQUYSBIEqLWRVgIfC2/43s5b06LR92nSneXOxdWdnGwhvQ/v0tA
BcRs03Vdem0+0+8HTtZlNzW5V/p30Id6H/WDUTSlu/I=
-----END CERTIFICATE-----
                </content>
              </ca>
            </httpsParams>
            <adservingDomain name="$HOST"/>
            </virtualServer>
            <virtualServer
              type="nginx"
              port="[% PORT_BASE + 82 %]"
              https_port="[% PORT_BASE + 83 %]"
              internal_port="[% PORT_BASE + 82 %]"
              internal_https_port="[% PORT_BASE + 83 %]">
              <httpsParams>
                <cert>
                  <content>
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 1 (0x1)
        Signature Algorithm: sha1WithRSAEncryption
        Issuer: C=RU, ST=Moscow, L=Moscow, O=Phorm Inc, OU=Phorm Moscow, CN=*.ocslab.com/emailAddress=server@ocslab.com
        Validity
            Not Before: Jun 23 08:25:15 2010 GMT
            Not After : Jun 20 08:25:15 2020 GMT
        Subject: C=RU, ST=Moscow, L=Moscow, O=Phorm Inc, OU=Phorm Moscow, CN=*.ocslab.com/emailAddress=server@ocslab.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
            RSA Public Key: (1024 bit)
                Modulus (1024 bit):
                    00:96:e1:78:f7:64:2f:4b:55:85:94:db:1c:05:d9:
                    46:aa:5e:ea:13:62:ec:c2:db:7e:c5:a2:2e:a8:00:
                    52:16:d0:a0:5b:1b:d1:a9:b5:cf:32:ce:08:05:2b:
                    75:ef:ab:f9:5d:07:db:b3:1a:e3:23:82:f2:67:89:
                    d9:1b:9f:42:3b:3d:c8:9a:81:bc:59:5b:a6:bf:0b:
                    6e:9b:f9:87:8e:a1:59:f7:8d:45:f1:41:17:f1:9d:
                    79:7a:85:4d:6d:56:95:8f:06:e5:92:54:8d:21:8e:
                    4b:bd:eb:24:52:55:d7:54:83:65:d6:ab:77:2e:da:
                    57:5c:9d:95:24:56:ce:66:25
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Basic Constraints:
                CA:TRUE
            Netscape Comment:
                OpenSSL Generated Certificate
            X509v3 Subject Key Identifier:
                E3:53:75:9B:4C:C5:11:1E:56:D1:26:DC:51:1B:BD:86:24:F2:90:27
            X509v3 Authority Key Identifier:
                keyid:1E:9E:ED:43:8E:23:07:3B:FC:2E:62:8A:1A:3E:46:A1:CA:21:E4:6B

    Signature Algorithm: sha1WithRSAEncryption
        58:1b:d6:5f:8b:d1:b6:da:a9:ce:46:12:84:ec:cf:10:1a:40:
        b3:ab:5b:d0:17:73:47:ea:24:01:06:85:8a:33:f1:04:a7:df:
        20:e3:2d:2a:e4:63:aa:29:c3:7e:29:be:f4:b5:ee:74:d5:56:
        29:8d:88:cd:44:2d:e3:9b:27:41:09:f7:44:38:f4:7e:b8:8d:
        d2:3d:89:36:83:ab:b6:f9:70:6e:99:8f:b5:7e:96:f9:a1:0e:
        5c:0d:e8:d4:2d:72:a1:87:e9:8d:cd:60:3a:14:c9:11:f3:1d:
        05:e1:e5:36:23:0f:e9:57:8c:59:ee:fb:e3:ee:2f:f8:8f:0d:
        87:9d
-----BEGIN CERTIFICATE-----
MIIDHDCCAoWgAwIBAgIBATANBgkqhkiG9w0BAQUFADCBkzELMAkGA1UEBhMCUlUx
DzANBgNVBAgTBk1vc2NvdzEPMA0GA1UEBxMGTW9zY293MRIwEAYDVQQKEwlQaG9y
bSBJbmMxFTATBgNVBAsTDFBob3JtIE1vc2NvdzEVMBMGA1UEAwwMKi5vY3NsYWIu
Y29tMSAwHgYJKoZIhvcNAQkBFhFzZXJ2ZXJAb2NzbGFiLmNvbTAeFw0xMDA2MjMw
ODI1MTVaFw0yMDA2MjAwODI1MTVaMIGTMQswCQYDVQQGEwJSVTEPMA0GA1UECBMG
TW9zY293MQ8wDQYDVQQHEwZNb3Njb3cxEjAQBgNVBAoTCVBob3JtIEluYzEVMBMG
A1UECxMMUGhvcm0gTW9zY293MRUwEwYDVQQDDAwqLm9jc2xhYi5jb20xIDAeBgkq
hkiG9w0BCQEWEXNlcnZlckBvY3NsYWIuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GN
ADCBiQKBgQCW4Xj3ZC9LVYWU2xwF2UaqXuoTYuzC237Foi6oAFIW0KBbG9Gptc8y
zggFK3Xvq/ldB9uzGuMjgvJnidkbn0I7PciagbxZW6a/C26b+YeOoVn3jUXxQRfx
nXl6hU1tVpWPBuWSVI0hjku96yRSVddUg2XWq3cu2ldcnZUkVs5mJQIDAQABo34w
fDAMBgNVHRMEBTADAQH/MCwGCWCGSAGG+EIBDQQfFh1PcGVuU1NMIEdlbmVyYXRl
ZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQU41N1m0zFER5W0SbcURu9hiTykCcwHwYD
VR0jBBgwFoAUHp7tQ44jBzv8LmKKGj5Gocoh5GswDQYJKoZIhvcNAQEFBQADgYEA
WBvWX4vRttqpzkYShOzPEBpAs6tb0BdzR+okAQaFijPxBKffIOMtKuRjqinDfim+
9LXudNVWKY2IzUQt45snQQn3RDj0friN0j2JNoOrtvlwbpmPtX6W+aEOXA3o1C1y
oYfpjc1gOhTJEfMdBeHlNiMP6VeMWe774+4v+I8Nh50=
-----END CERTIFICATE-----
                </content>
              </cert>
              <key>
                <content>
-----BEGIN RSA PRIVATE KEY-----
MIICXAIBAAKBgQCW4Xj3ZC9LVYWU2xwF2UaqXuoTYuzC237Foi6oAFIW0KBbG9Gp
tc8yzggFK3Xvq/ldB9uzGuMjgvJnidkbn0I7PciagbxZW6a/C26b+YeOoVn3jUXx
QRfxnXl6hU1tVpWPBuWSVI0hjku96yRSVddUg2XWq3cu2ldcnZUkVs5mJQIDAQAB
AoGAE4cHAu3CeTuOWF+rVs5yKOaz4OJyxh1mYOnGpBO2jCFgP6pwDkjrkiW8P/8J
+oUFdxbnRlz6fMQb326l3L9V8OYC6PDVX9EGMX90pJLWaaGKuEsB+sdxsH50s/Px
Kx9eYUdWfpDiheTAmJJgTfdGLCqOsODEekJuFpfNPlEhYxkCQQDH24m9gTDsL8hZ
rN4yKnAU38zMEikeIkGlN2PCAzp+VHkTf7IqdMwKLWb8DfADFsZ5Rtm8du8j5vuI
LgquuzKfAkEAwUPWw+t1gdCWThfReQsC51+cun5lpC7aZpDZt11NNQwgtDRheyE+
iiPaYB8BdAIQ0wtELa1mUVH8V0OK8mQUuwJAR6bl0xMmPwdChSP85W9hj5pNCjzY
kP0nG9yn3z7ZEcGnRt5ZOe9115A/g85bZkAcGA7WsULSqcR+GWyoV8y1cQJAO2+c
jfZM1haLEws6LaqYQwXhkm0q3xpVqnNjsYOtKeJH8IOncxGfRSaPkz4V2VKFUBJH
7nBEZj/7he7MvYzP3wJBAIwf4JzW9Lby4kRR1WscLhzJhi3Hot5wUvC4X/BegLsK
e4snuz4RmM6E0zm5syW+4rDmKekhavfIpiGvjxN8t58=
-----END RSA PRIVATE KEY-----
                </content>
              </key>
              <ca>
                <content>
Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 0 (0x0)
        Signature Algorithm: sha1WithRSAEncryption
        Issuer: C=RU, ST=Moscow, L=Moscow, O=Phorm Inc, OU=Phorm Moscow, CN=*.ocslab.com/emailAddress=server@ocslab.com
        Validity
            Not Before: Jun 23 08:24:49 2010 GMT
            Not After : Jun 18 08:24:49 2030 GMT
        Subject: C=RU, ST=Moscow, L=Moscow, O=Phorm Inc, OU=Phorm Moscow, CN=*.ocslab.com/emailAddress=server@ocslab.com
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
            RSA Public Key: (1024 bit)
                Modulus (1024 bit):
                    00:bc:9a:50:1e:46:e5:87:1d:4a:74:f2:39:b0:c0:
                    f2:dc:4b:32:5d:50:b0:40:9f:55:86:af:f4:d7:64:
                    cc:44:8a:10:d6:70:0c:a8:0a:03:0f:13:49:8e:ca:
                    93:51:07:16:e4:c2:be:21:a7:0d:77:da:52:ef:4e:
                    44:d2:91:9f:98:3d:e6:1c:09:ad:2a:d4:69:e6:e7:
                    db:87:d6:d7:fe:ba:fb:f3:fa:8d:92:eb:18:b9:3e:
                    0c:db:ec:65:d6:42:87:f0:ad:ab:32:fc:c3:f7:8d:
                    94:d9:81:30:d5:33:7c:1a:d0:4e:25:b8:8c:f1:51:
                    5d:0b:94:fc:9a:5e:47:68:31
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Basic Constraints:
                CA:TRUE
            Netscape Comment:
                OpenSSL Generated Certificate
            X509v3 Subject Key Identifier:
                1E:9E:ED:43:8E:23:07:3B:FC:2E:62:8A:1A:3E:46:A1:CA:21:E4:6B
            X509v3 Authority Key Identifier:
                keyid:1E:9E:ED:43:8E:23:07:3B:FC:2E:62:8A:1A:3E:46:A1:CA:21:E4:6B

    Signature Algorithm: sha1WithRSAEncryption
        b4:49:a8:86:7f:a5:87:bd:b6:81:58:18:70:55:ee:a2:00:e0:
        8c:10:55:1d:4f:9d:c2:88:fb:7e:6e:30:98:fd:87:b1:b8:70:
        96:f8:ad:61:2d:7c:d9:80:24:89:de:c6:ee:0f:01:77:16:ae:
        62:83:62:fe:44:14:61:20:48:12:a2:d6:45:58:08:7c:2d:bf:
        e3:7b:39:6f:4e:8b:47:dd:a7:4a:77:97:3b:17:56:76:71:b0:
        86:f4:3f:bf:4b:40:05:c4:6c:d3:75:5d:7a:6d:3e:d3:ef:07:
        4e:d6:65:37:35:b9:57:fa:77:d0:87:7a:1f:f5:83:51:34:a5:
        bb:f2
-----BEGIN CERTIFICATE-----
MIIDHDCCAoWgAwIBAgIBADANBgkqhkiG9w0BAQUFADCBkzELMAkGA1UEBhMCUlUx
DzANBgNVBAgTBk1vc2NvdzEPMA0GA1UEBxMGTW9zY293MRIwEAYDVQQKEwlQaG9y
bSBJbmMxFTATBgNVBAsTDFBob3JtIE1vc2NvdzEVMBMGA1UEAwwMKi5vY3NsYWIu
Y29tMSAwHgYJKoZIhvcNAQkBFhFzZXJ2ZXJAb2NzbGFiLmNvbTAeFw0xMDA2MjMw
ODI0NDlaFw0zMDA2MTgwODI0NDlaMIGTMQswCQYDVQQGEwJSVTEPMA0GA1UECBMG
TW9zY293MQ8wDQYDVQQHEwZNb3Njb3cxEjAQBgNVBAoTCVBob3JtIEluYzEVMBMG
A1UECxMMUGhvcm0gTW9zY293MRUwEwYDVQQDDAwqLm9jc2xhYi5jb20xIDAeBgkq
hkiG9w0BCQEWEXNlcnZlckBvY3NsYWIuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GN
ADCBiQKBgQC8mlAeRuWHHUp08jmwwPLcSzJdULBAn1WGr/TXZMxEihDWcAyoCgMP
E0mOypNRBxbkwr4hpw132lLvTkTSkZ+YPeYcCa0q1Gnm59uH1tf+uvvz+o2S6xi5
Pgzb7GXWQofwrasy/MP3jZTZgTDVM3wa0E4luIzxUV0LlPyaXkdoMQIDAQABo34w
fDAMBgNVHRMEBTADAQH/MCwGCWCGSAGG+EIBDQQfFh1PcGVuU1NMIEdlbmVyYXRl
ZCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQUHp7tQ44jBzv8LmKKGj5Gocoh5GswHwYD
VR0jBBgwFoAUHp7tQ44jBzv8LmKKGj5Gocoh5GswDQYJKoZIhvcNAQEFBQADgYEA
tEmohn+lh722gVgYcFXuogDgjBBVHU+dwoj7fm4wmP2HsbhwlvitYS182YAkid7G
7g8BdxauYoNi/kQUYSBIEqLWRVgIfC2/43s5b06LR92nSneXOxdWdnGwhvQ/v0tA
BcRs03Vdem0+0+8HTtZlNzW5V/p30Id6H/WDUTSlu/I=
-----END CERTIFICATE-----
                </content>
              </ca>
              </httpsParams>
              <thirdPartyContentDomain name="$HOST"/>
              <biddingDomain name="$HOST"/>
            </virtualServer>
            
            <securityToken 
              key0="MFBOeuplH3LlQqfGSvNiew=="
              key1="gYi+K845gfiUkvx7Se1zKA=="
              key2="mVa0GF8y1uvou3mktkNxqw=="
              key3="mCFIshSwVu4VxRTi73Rf+Q=="
              key4="fCW/qXof04/U/qRTggbJLw=="
              key5="K2UPkfYBFNA+MxHKkV5OUA=="
              key6="emcqWycuiztgJVng4az0Xw=="/>
            <RTB yandex_key="04bb79a7741de975e1800764b30c41ef">
              <source id="url" instantiate_type="url" request_type="openrtb with click url" enable_notice="false"/>
              <source id="script-url" instantiate_type="script with url" request_type="openrtb with click url" enable_notice="false"/>
              <source id="body" instantiate_type="body" request_type="openrtb with click url" enable_notice="false"/>
              <source id="url-no-click" instantiate_type="url" request_type="openrtb" enable_notice="false"/>
              <source id="body-no-click" instantiate_type="body" request_type="openrtb" enable_notice="false"/>
              <source id="iframe-url" instantiate_type="iframe with url" request_type="openrtb" enable_notice="false"/>
              <source id="iframe-url-no-click" instantiate_type="iframe with url" request_type="openrtb" enable_notice="false"/>
              <source id="body-openx" instantiate_type="body" request_type="openx" enable_notice="false"/>
              <source id="iframe-url-openx" instantiate_type="iframe with url" request_type="openx" enable_notice="false"/>
              <source id="body-liverail" instantiate_type="body" request_type="liverail" enable_notice="false"/>
              <source id="url-liverail" instantiate_type="url" request_type="liverail" enable_notice="false"/>
              <source id="body-blank" instantiate_type="body" enable_notice="false"/>
              <source id="body-openrtb-notice" instantiate_type="body" request_type="openrtb with click url" enable_notice="true"/>
              <source id="iframe-url-adriver" instantiate_type="iframe with url" request_type="adriver" enable_notice="true"/>
              <source id="anx" instantiate_type="url parameters"  request_type="appnexus" enable_notice="false" appnexus_member_id="1"/>
              <source id="ipw" 
                      instantiate_type="body" 
                      request_type="openrtb" 
                      seat="94" 
                      enable_notice="true" 
                      vast_instantiate_type="video url" 
                      vast_enable_notice="false" 
                      ipw_extension="true"/>
              <source id="google" instantiate_type="iframe with url" request_type="google" vast_instantiate_type="video url" vast_enable_notice="false"/>
              <source id="yandex" 
                      instantiate_type="data parameter value" 
                      request_type="yandex" 
                      enable_notice="false" 
                      truncate_domain="false" 
                      vast_instantiate_type="data parameter value" 
                      vast_enable_notice="false" 
                      ipw_extension="false">
                 <userBind redirect="//an.yandex.ru/setud/imarker2/##UNSIGNEDSSPUID=##?sign=##YANDEXSIGN##"  
                           passback="true"/>
              </source>
            <source id="native_js" 
                    instantiate_type="body" 
                    request_type="openrtb with click url" 
                    enable_notice="false" 
                    fill_adid="true"
                    native_instantiate_type="adm"
                    native_impression_tracker_type="js"/>
            </RTB>
          </coloParams>
          <inventoryStats simplifying="100"/>
          <snmpStats enable="false"/>
          <userProfiling repeat_trigger_timeout="0">
            <userInfoExchangerRef host="$SOURCE_HOST" port="[% SOURCE_PORT_BASE + 60 %]">
[% IF SECURE_CONNECTIONS %]
              <secureParams key="ckey.pem" certificate="ccert.pem" password="adserver"/>
[% END %]
            </userInfoExchangerRef>
          </userProfiling>
          <remote>
            <campaignServerRef host="$SOURCE_HOST" port="[% SOURCE_PORT_BASE + 56 %]">
[% IF SECURE_CONNECTIONS %]
              <secureParams key="ckey.pem" certificate="ccert.pem" password="adserver"/>
[% END %]
            </campaignServerRef>
            <campaignServerRef host="$SOURCE_HOST" port="111">
[% IF SECURE_CONNECTIONS %]
              <secureParams key="ckey.pem" certificate="ccert.pem" password="adserver"/>
[% END %]
            </campaignServerRef>
            <channelProxyRef host="$SOURCE_HOST" port="[% SOURCE_PORT_BASE + 55 %]">
[% IF SECURE_CONNECTIONS %]
              <secureParams key="ckey.pem" certificate="ccert.pem" password="adserver"/>
[% END %]
            </channelProxyRef>
            <sTunnelRef host="$SOURCE_HOST" port="[% SOURCE_PORT_BASE + 54 %]"/>
          </remote>
        </cluster>
      </configuration>

      <!-- lp, ui, be -->
      <serviceGroup
        descriptor="AdCluster/BackendSubCluster"
        name="BackendSubCluster$NAME_POSTFIX">

        <serviceGroup
          descriptor="AdCluster/BackendSubCluster/LogProcessing"
          name="LogProcessing$NAME_POSTFIX">
          <configuration>
            <logProcessing xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <syncLogs>
                <networkParams port="[% PORT_BASE + 12 %]"/>
                <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
                <fileTransferring file_check_period="10" host_check_period="10"/>
              </syncLogs>

              <syncLogsServer>
                <networkParams port="[% PORT_BASE + 14 %]"/>
                <logging sys_log="false" log_level="$LOG_LEVEL"/>
              </syncLogsServer>
            </logProcessing>
          </configuration>
        </serviceGroup>

        <serviceGroup
          descriptor="AdCluster/BackendSubCluster/LocalProxy"
          name="LocalProxyDevel$NAME_POSTFIX">

          <service
            descriptor="AdCluster/BackendSubCluster/LocalProxy/ChannelProxy"
            name="ChannelProxyDevel$NAME_POSTFIX"
            host="$HOST">
            <configuration>
              <channelProxy xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
                <networkParams port="[% PORT_BASE + 20 %]"/>
                <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              </channelProxy>
            </configuration>
          </service>

          <service
            descriptor="AdCluster/BackendSubCluster/LocalProxy/STunnelClient"
            name="STunnelClientDevel$NAME_POSTFIX"
            host="$HOST">
            <configuration>
              <sTunnelClient xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
                <networkParams port="[% PORT_BASE + 21 %]"/>
              </sTunnelClient>
            </configuration>
          </service>

        </serviceGroup>

        <service
          descriptor="AdCluster/BackendSubCluster/CampaignServer"
          name="CampaignServer$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <campaignServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 6 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              <updateParams update_period="30" ecpm_update_period="30"/>
            </campaignServer>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/BackendSubCluster/LogGeneralizer"
          name="LogGeneralizer$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <logGeneralizer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 11 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              <statLogging file_check_period="10" flush_period="10" flush_size="10000"/>
            </logGeneralizer>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/BackendSubCluster/ExpressionMatcher"
          name="ExpressionMatcher$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <expressionMatcher xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 13 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              <updateParams period="30"/>
              <statLogging file_check_period="10" flush_period="10" activity_flush_period="10"/>
            </expressionMatcher>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/BackendSubCluster/RequestInfoManager"
          name="RequestInfoManager$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <requestInfoManager xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 16 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              <statLogging file_check_period="10" flush_period="30"/>
            </requestInfoManager>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/BackendSubCluster/StatsCollector"
          name="StatsCollector$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <statsCollector xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration" period="3">
              <networkParams port="[% PORT_BASE + 18 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
            </statsCollector>
          </configuration>
        </service>

      <!-- eat to much memory, uncoment 
        <service
          descriptor="AdCluster/BackendSubCluster/DictionaryProvider"
          name="DictionaryProvider$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <dictionaryProvider xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 10 %]"/>
              <logging log_level="$LOG_LEVEL" sys_log="$SYS_LOG"/>
            </dictionaryProvider>
          </configuration>
        </service> -->

        <service
          descriptor="AdCluster/BackendSubCluster/UserOperationGenerator"
          name="UserOperationGeneratorDevel"
          host="$HOST">
          <configuration>
            <userOperationGenerator xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 30 %]"/>
              <logging log_level="7" sys_log="$SYS_LOG"/>
            </userOperationGenerator>
          </configuration>
        </service>

      </serviceGroup>

      <!-- fe -->
      <serviceGroup
        descriptor="AdCluster/FrontendSubCluster"
        name="FrontendSubCluster$NAME_POSTFIX">
        <configuration>
          <frontendCluster xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <channelServing>
              <scaleParams chunks_count="10"/>
            </channelServing>

          </frontendCluster>
        </configuration>

        <service
          descriptor="AdCluster/FrontendSubCluster/UserInfoManager"
          name="UserInfoManager$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <userInfoManager xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 1 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              <updateParams update_period="30"/>
              <profilesCleanupParams life_time="3600" clean_time="00:01"/>
              <matchParams repeat_trigger_timeout="0"/>
            </userInfoManager>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/FrontendSubCluster/UserInfoManagerController"
          name="UserInfoManagerController$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <userInfoManagerController xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 2 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              <controlParams status_check_period="10"/>
            </userInfoManagerController>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/FrontendSubCluster/ChannelServer"
          name="ChannelServer$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <channelServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 3 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              <updateParams period="30"/>
            </channelServer>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/FrontendSubCluster/ChannelController"
          name="ChannelController$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <channelController xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 4 %]"/>
              <logging log_level="$LOG_LEVEL" sys_log="$SYS_LOG"/>
            </channelController>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/FrontendSubCluster/ChannelSearchService"
          name="ChannelSearchService$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <channelSearchService xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 15 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
            </channelSearchService>
          </configuration>
        </service>

        <service 
          descriptor="AdCluster/FrontendSubCluster/UserBindServer" 
          name="UserBindServer$NAME_POSTFIX" 
          host="$HOST">
          <configuration>
            <userBindServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 28 %]"/>
              <logging log_level="$LOG_LEVEL" sys_log="$SYS_LOG"/> 
            </userBindServer>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/FrontendSubCluster/Frontend"
          name="Frontend$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <frontend xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <webServerParams
                timeout="300"
                max_clients="100"/>
              <debugInfo use_acl="false" show_history_profile="true"/>
              <requestModule>
                <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
                <session timeout="30"/>
              </requestModule>
              <impressionModule>
                <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              </impressionModule>
              <clickModule>
                <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              </clickModule>
              <actionModule>
                <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              </actionModule>
              <optoutModule>
                <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL" log_ip="true"/>
              </optoutModule>
              <pageSenseModule/>
              <biddingModule max_bid_time="50"/>
              <userBindModule/>
            </frontend>
          </configuration>
        </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/NginxFrontend"
        name="Frontend$NAME_POSTFIX"
        host="$HOST">
        <configuration>
          <frontend xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
            <webServerParams
              timeout="300"
              max_clients="100"/>
            <debugInfo use_acl="false" show_history_profile="true"/>
            <requestModule>
              <logging log_level="$LOG_LEVEL"/>
              <session timeout="30"/>
            </requestModule>
            <impressionModule>
              <logging log_level="$LOG_LEVEL"/>
            </impressionModule>
            <clickModule>
              <logging log_level="$LOG_LEVEL"/>
            </clickModule>
            <actionModule>
              <logging log_level="$LOG_LEVEL"/>
            </actionModule>
            <optoutModule>
              <logging log_level="$LOG_LEVEL" log_ip="true"/>
            </optoutModule>
            <pageSenseModule/>
            <biddingModule max_bid_time="50">
              <source id="url" instantiate_type="url" request_type="openrtb with click url" enable_notice="false" fill_adid="true"/>
              <source id="script-url" instantiate_type="script with url" request_type="openrtb with click url" enable_notice="false" fill_adid="true"/>
              <source id="body" instantiate_type="body" request_type="openrtb with click url" enable_notice="false" fill_adid="true"/>
              <source id="url-no-click" instantiate_type="url" request_type="openrtb" enable_notice="false" fill_adid="true"/>
              <source id="body-no-click" instantiate_type="body" request_type="openrtb" enable_notice="false" fill_adid="true"/>
              <source id="iframe-url" instantiate_type="iframe with url" request_type="openrtb" enable_notice="false" fill_adid="true"/>
              <source id="iframe-url-no-click" instantiate_type="iframe with url" request_type="openrtb" enable_notice="false" fill_adid="true"/>
              <source id="body-openx" instantiate_type="body" request_type="openx" enable_notice="false" fill_adid="true"/>
              <source id="iframe-url-openx" instantiate_type="iframe with url" request_type="openx" enable_notice="false" fill_adid="true"/>
              <source id="body-liverail" instantiate_type="body" request_type="liverail" enable_notice="false" fill_adid="true"/>
              <source id="url-liverail" instantiate_type="url" request_type="liverail" enable_notice="false" fill_adid="true"/>
              <source id="body-blank" instantiate_type="body" enable_notice="false"/>
              <source id="body-openrtb-notice" instantiate_type="body" request_type="openrtb with click url" enable_notice="true" fill_adid="true"/>
              <source id="iframe-url-adriver" instantiate_type="iframe with url" request_type="adriver" enable_notice="true" fill_adid="true"/>
              <source id="anx" instantiate_type="url parameters"  request_type="appnexus" enable_notice="false" appnexus_member_id="1"/>
              <source id="ipw" instantiate_type="body" request_type="openrtb" seat="94" enable_notice="true" vast_instantiate_type="video url" vast_enable_notice="false" ipw_extension="true" fill_adid="true"/>
              <source id="google" instantiate_type="iframe with url" request_type="google" vast_instantiate_type="video url" vast_enable_notice="false"/>
              <source id="yandex" instantiate_type="data parameter value" request_type="yandex" enable_notice="false" truncate_domain="false" 
                 vast_instantiate_type="data parameter value" vast_enable_notice="false" ipw_extension="false"/>
            </biddingModule>
            <userBindModule yandex_key="04bb79a7741de975e1800764b30c41ef">
               <source id="yandex" redirect="//an.yandex.ru/setud/imarker2/##UNSIGNEDSSPUID##?sign=##YANDEXSIGN##"/>
            </userBindModule>
          </frontend>
        </configuration>
      </service>

        <service
          descriptor="AdCluster/FrontendSubCluster/BillingServer"
          name="BillingServer$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <billingServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 31 %]"/>
              <logging log_level="7" sys_log="false"/>
            </billingServer>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/FrontendSubCluster/CampaignManager"
          name="CampaignManager$NAME_POSTFIX"
          host="$HOST">
          <configuration>
            <campaignManager xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration">
              <networkParams port="[% PORT_BASE + 7 %]"/>
              <logging sys_log="$SYS_LOG" log_level="$LOG_LEVEL"/>
              <updateParams
                update_period="20"
                ecpm_update_period="10"
                campaign_types="all"/>
              <statLogging flush_period="10"/>
            </campaignManager>
          </configuration>
        </service>

        <service
          descriptor="AdCluster/FrontendSubCluster/ZmqProfilingBalancer"
          name="ZmqProfilingBalancer #1"
          host="$HOST">
          <configuration>
            <zmqProfilingBalancer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration" zmq_io_threads="1">
              <networkParams port="[% PORT_BASE + 74 %]"/>
              <logging log_level="$LOG_LEVEL" sys_log="$SYS_LOG"/>
              <profilingInfoBindSocket hwm="1">
                <address port="[% PORT_BASE + 88 %]"/>
              </profilingInfoBindSocket>
              <anonymousStatsBindSocket hwm="1">
                <address port="[% PORT_BASE + 89 %]"/>
              </anonymousStatsBindSocket>
            </zmqProfilingBalancer>
          </configuration>
        </service>
  
        <service
          descriptor="AdCluster/FrontendSubCluster/ProfilingServer"
          name="ProfilingServer"
          host="$HOST">
        <configuration>
          <profilingServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration" work_threads="5">
            <networkParams port="[% PORT_BASE + 75 %]" profiling_info_port="[% PORT_BASE + 86 %]" anonymous_stats_port="[% PORT_BASE + 87 %]"/>
            <logging log_level="$LOG_LEVEL" sys_log="$SYS_LOG"/>
          </profilingServer>
        </configuration>
      </service>

      <service
        descriptor="AdCluster/FrontendSubCluster/FCGIServer"
        name="FCGIServer"
        host="$HOST">
        <configuration>
          <FCGIServer xmlns="http://www.adintelligence.net/xsd/AdServer/Configuration" work_threads="5">
            <networkParams port="[% PORT_BASE + 76 %]"/>
            <logging log_level="$LOG_LEVEL" sys_log="$SYS_LOG"/>
          </FCGIServer>
        </configuration>
      </service>
      </serviceGroup>

    </serviceGroup>

  </application>

</colo:colocation>
