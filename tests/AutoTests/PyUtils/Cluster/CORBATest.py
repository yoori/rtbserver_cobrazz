
import sys, time, httplib, urllib, string, os.path, os, shutil, glob

import FunTest, OrbTestSuite, MockCampaignServer, MockChannelProxy, \
       MockChannelController

from TestComparison import ComparisonMixin
from Logger import log, logException

from omniORB import CORBA
from MTQueue import MTQueue, XTimedOut
import CORBACommons

import ConfigVars

VERSION = '1.12.5.0'

class CallSequence:

  def __init__(self, call, count):
    self.call = call
    self.count = count

class CORBAProcess(FunTest.Process):

  def __init__( self, info, testName, opts=[], indirs=[], outdirs=[], cachedirs=[]):
    self.processControl = None
    self.started        = False
    self.name = self.__class__.__name__
    self.indirs = indirs
    self.outdirs = outdirs
    self.cachedirs = cachedirs
    cfgFileName = self.name + ".xml"
    self.processControl = None
    self.srcDir = ConfigVars.BINDIR
    FunTest.Process.__init__(self, info, testName, self.name, self.srcDir, self.name, cfgFileName, opts)
    srvPath = os.path.join(self.srcDir, self.name)
    self.cmd = '%s %s %s' % \
               (os.path.join(info.workDir, srvPath), self.cfgFilePath, ' '.join(opts))
    log(1, self.cmd)
    self.logFile = os.path.join(self.tmpDir, self.name) + ".log"
    self.ROOT = os.path.abspath(info.tmpDir)
    self.PFX = os.path.abspath(self.tmpDirPrefix)
    self.PORT_BASE         = ConfigVars.PORT_BASE
    self.USERINFOMGR_PORT  = self.PORT_BASE + 1
    self.USERINFOCTRL_PORT = self.PORT_BASE + 2
    self.CHANNELSRV_PORT   = self.PORT_BASE + 3
    self.CHANNELCTRL_PORT  = self.PORT_BASE + 4
    self.CHANNELPROXY_PORT = self.PORT_BASE + 5
    self.CAMPAIGNSRV_PORT  = self.PORT_BASE + 6
    self.CAMPAIGNMGR_PORT  = self.PORT_BASE + 7
    self.CHANNELSEARCHSVC_PORT  = self.PORT_BASE + 9
    self.LOGGENERALIZER_PORT  = self.PORT_BASE + 11
    self.SYNCLOGS_PORT = self.PORT_BASE + 12
    self.EXPRESSIONMATCHER_PORT = self.PORT_BASE + 13
    self.REQUESTINFOMGR_PORT = self.PORT_BASE + 20
    self.AD_IMAGESRV_PORT = self.PORT_BASE + 80
    self.FE_PORT = self.PORT_BASE + 80
    self.VERSION = VERSION
    # Oracle connection
    self.ORA_DBSERVER = ConfigVars.ORA_DBSERVER
    self.ORA_DB = ConfigVars.ORA_DB
    self.ORA_DBPWD = ConfigVars.ORA_DBPWD
    # Postgres connection
    self.PQ_HOST = ConfigVars.PQ_HOST
    self.PQ_PORT = ConfigVars.PQ_PORT
    self.PQ_DB = ConfigVars.PQ_DB
    self.PQ_USER = ConfigVars.PQ_USER
    self.PQ_DBPWD = ConfigVars.PQ_DBPWD
    #
    self.THREADING_POOL = 10
    #
    self.HOME = ConfigVars.HOME
    self.USER = ConfigVars.USER
    self.HOST = ConfigVars.HOST
    self.LOGROOT = os.path.abspath(self.tmpDir)
    self.XSDDIR = ConfigVars.XSDDIR
    self.RUNDIR =  ConfigVars.RUNDIR
    self.LOGLEVEL = 8
    self.COLO_ID = 1
    #
    self.CONFIG_UPDATE_PERIOD = 10
    self.CONFIG_COUNT_CHUNKS = 10
    self.ECPM_UPDATE_PERIOD = 10
    self.STATEMENT_TIMEOUT = 600
    self.CAMPAIGN_SERVER_ID = 1
    self.CAMPAIGNSERVER_OBJECT_KEY = MockCampaignServer.ObjectKey
    self.CHANNELPROXY_OBJECT_KEY = MockChannelProxy.ObjectKey
    self.CHANNELCRTL_OBJECT_KEY = MockChannelController.ObjectKey
    self.CHANNELPROXY_USED = False

  def createDirs( self, root, dirs):
    try:
      os.mkdir(root)
      for dir in dirs:
        subs = dir.split('/')
        d = root
        for sub in subs:
          d = os.path.join(d, sub)
          if not os.path.exists(d):
            os.mkdir(d)
    except Exception, exc:
      print exc

  def prepare( self ):
    if len(self.indirs) > 0:
      ind = self.LOGROOT + '/In'
      self.createDirs(ind, self.indirs)
    if len(self.outdirs) > 0:
      outd = self.LOGROOT + '/Out'
      self.createDirs(outd, self.outdirs)
    if len(self.cachedirs) > 0:
      outd = self.LOGROOT + '/cache'
      self.createDirs(outd, self.cachedirs)

  def getObject( self, ior, iface ):
    pc_ior = "corbaloc:iiop:localhost:%i/%s" % (self.port, ior)
    obj = self.info.orb.string_to_object(pc_ior)
    obj = obj._narrow(iface)
    return obj

  def waitReady( self, timeout = 2 ):
    start = time.time()
    probeObj = self.getObject("ProcessControl",
                              CORBACommons.IProcessControl)
    def status2str(status):
      if status == CORBACommons.IProcessControl.AS_READY:
        return "READY"
      elif status == CORBACommons.IProcessControl.AS_ALIVE:
        return "ALIVE"
      elif status == CORBACommons.IProcessControl.AS_NOT_ALIVE:
        return "NOT_ALIVE"
      else:
        return "UNKNOWN"
    while time.time() - start <= timeout:
      status = probeObj.is_alive()
      FunTest.tlog(5, "%s.is_alive status = %s" % (self.name, status2str(status)))
      if status == CORBACommons.IProcessControl.AS_READY:
        return
      time.sleep(1)
    raise Exception("%s isn't ready (not configured yet)" % self.name)



  def start( self ):
    os.environ['workspace_root'] = self.tmpDir
    FunTest.Process.start(self)
    self.started = True
    time.sleep(1)
    for i in xrange(1, 300):
      try:
        if not self.started:
          return False
        self.processControl = self.getObject("ProcessControl", CORBACommons.IProcessControl)
        if None == self.processControl:
          continue
        return True
      except Exception, exc:
        log(2, 'starting %s exception "%s"' % (self.name, exc))
        time.sleep(1)
        continue
    return False

  def _finished( self ):
    FunTest.Process._finished(self)
    self.started = False

  def stop( self ):
    if self.processControl:
      self.processControl.shutdown(False)
    if self.started:
      FunTest.Process.stop(self)

  def is_alive( self ):
    return self.processControl.is_alive()


class CampaignServer(CORBAProcess):

  def __init__(  self, info, testName, opts=[] ):
    CORBAProcess.__init__(self, info, testName, opts,
                          outdirs = ['ColoUpdateStat', 'ColoUpdateStat_'])

  def writeCfgFile( self ):
    self.CONFIG_UPDATE_PERIOD = 10
    self.ECPM_UPDATE_PERIOD = 10
    self.COLO_UPDATE_STAT_FLUSH_PERIOD = 30
    self.port = self.CAMPAIGNSRV_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/CampaignSvcs/CampaignServerConfig.xsd">
 <cfg:CampaignServer log_root="%(LOGROOT)s/Out" config_update_period="%(CONFIG_UPDATE_PERIOD)i"
  ecpm_update_period="%(ECPM_UPDATE_PERIOD)i" server_id="%(CAMPAIGN_SERVER_ID)i"
  colo_id="%(COLO_ID)i" version="%(VERSION)s">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(CAMPAIGNSRV_PORT)i">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="CampaignServer" name="CampaignServer"/>
    <cfg:Object servant="CampaignServer_v330" name="CampaignServer_v330"/>
    <cfg:Object servant="CampaignServer_v340" name="CampaignServer_v340"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:Logger filename="%(LOGROOT)s/CampaignServer.log" log_level="%(LOGLEVEL)i">
    <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
    <cfg:Suffix min_log_level="5" max_log_level="7" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
  <cfg:Logging>
   <cfg:ColoUpdateStat flush_period="%(COLO_UPDATE_STAT_FLUSH_PERIOD)i"/>
  </cfg:Logging>
  <cfg:ServerMode stat_stamp_sync_period="3600">
   <cfg:PGConnection connection_string="host=%(PQ_HOST)s port=%(PQ_PORT)s dbname=%(PQ_DB)s user=%(PQ_USER)s password=%(PQ_DBPWD)s"/>
   <cfg:LogGeneralizerCorbaRef>
     <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(LOGGENERALIZER_PORT)i/LogGeneralizer"/>
   </cfg:LogGeneralizerCorbaRef>
  </cfg:ServerMode>
 </cfg:CampaignServer>
</cfg:AdConfiguration>
""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

class CampaignManager(CORBAProcess):

  def __init__(  self, info, testName, opts=[]):
    dirs = ['CreativeStat', 'ColoUsers',
            'OptOutStat', 'ChannelTriggerStat',
            'ChannelHitStat', 'KeywordStat', 'ActionRequest',
            'PublisherInventory', 'UserProperties',
            'Request', 'Impression', 'Click',
            'AdvertiserAction', 'PassbackImpression', 'RequestBasicChannels']
    CORBAProcess.__init__(self, info, testName, opts,
                          outdirs = dirs + map((lambda a: a + '_'), dirs))
    self.WWWROOT = os.path.join(self.LOGROOT, 'www')
    self.CFGROOT = self.LOGROOT

  def prepare( self ):
    CORBAProcess.prepare(self)
    self.createDirs(self.WWWROOT, ['Creatives', 'Templates', 'Templates/Passback'])
    self.__writeTestTemplate()
    self.__writeDomainConfig()
    self.__writePostInstantiateHtml()
    shutil.copy('./config/rid_private_key.der', self.CFGROOT)

  def __writePostInstantiateHtml(self):
    path = os.path.join(self.CFGROOT, 'PostInstantiateScript.html')
    f = file(path, 'w')
    try:
      f.write("""<script type="text/javascript" src="##URL##"></script>""")
    except Exception, exc:
      print >> f, exc
    f.close()

  def __writeDomainConfig(self):
    path = os.path.join(self.CFGROOT, 'DomainConfig.xml')
    f = file(path, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:DomainConfiguration
  xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  xmlns:colo="http://www.foros.com/cms/colocation"
  xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/CampaignSvcs/DomainConfig.xsd">
  <cfg:Domain name="com">
    <cfg:SubDomain name="gb"/>
    <cfg:SubDomain name="jpn"/>
    <cfg:SubDomain name="kr"/>
    <cfg:SubDomain name="ru"/>
    <cfg:SubDomain name="us"/>
    <cfg:SubDomain name="operaunite"/>
  </cfg:Domain>
</cfg:DomainConfiguration>""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

  def __writeTestTemplate( self ):
    for filename in glob.glob(os.path.join('./config/Templates', '*.*')):
      shutil.copy(filename, os.path.join(self.WWWROOT, 'Templates'))

    for filename in glob.glob(os.path.join('./config/Templates/Passback', '*.*')):
      shutil.copy(filename, os.path.join(self.WWWROOT, 'Templates/Passback'))

  def writeCfgFile( self ):
    self.port = self.CAMPAIGNMGR_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration
 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/CampaignSvcs/CampaignManagerConfig.xsd">
 <cfg:CampaignManager log_root="%(LOGROOT)s"
  config_update_period="%(CONFIG_UPDATE_PERIOD)i"
  campaigns_update_timeout="0"
  ecpm_update_period="%(ECPM_UPDATE_PERIOD)i"
  campaigns_type="all"
  colocation_id="%(COLO_ID)i"
  domain_config_path="%(CFGROOT)s/DomainConfig.xml"
  service_index="1_1"
  rid_private_key="%(CFGROOT)s/rid_private_key.der">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(CAMPAIGNMGR_PORT)i">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="CampaignManager" name="CampaignManager"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:Logger filename="%(LOGROOT)s/CampaignManager.log" log_level="%(LOGLEVEL)i">
      <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
      <cfg:Suffix min_log_level="5" max_log_level="8" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
  <!-- <cfg:Klt base="/opt/KLT/hdic/KLT2000.ini"/> -->
  <!-- <cfg:Mecab base="/usr/etc/mecabrc"/> -->
  <cfg:Creative
    creative_file_dir="%(WWWROOT)s/Creatives"
    template_file_dir="%(WWWROOT)s/Templates"
    post_instantiate_script_template_file="%(CFGROOT)s/PostInstantiateScript.html"
    post_instantiate_script_mime_format="text/html"
    post_instantiate_iframe_template_file="%(CFGROOT)s/PostInstantiateIFrame.html"
    post_instantiate_iframe_mime_format="text/html">
  <cfg:CreativeRule name="unsecure" secure="false"
    image_url="http://%(HOST)s:%(FE_PORT)i/creatives"
    publ_url="http://%(HOST)s:%(FE_PORT)i/publ"
    ad_click_url="http://%(HOST)s:%(FE_PORT)i/services/AdClickServer"
    ad_server="http://%(HOST)s:%(FE_PORT)i"
    ad_image_server="http://%(HOST)s:%(FE_PORT)i"
    track_pixel_url="http://%(HOST)s:%(FE_PORT)i/services/ImprTrack/pt.gif"
    passback_pixel_url="http://%(HOST)s:%(FE_PORT)i/services/passback"
    action_pixel_url="http://%(HOST)s:%(FE_PORT)i/services/ActionServer/SetCookie"
    passback_template_path_prefix="%(WWWROOT)s/Templates/Passback/pb."
    local_passback_prefix="http://%(HOST)s:%(FE_PORT)i/tags/"
    dynamic_creative_prefix="http://%(HOST)s:%(FE_PORT)i/services/dcreative"
    pub_pixels_optin="http://%(HOST)s:%(FE_PORT)i/pubpixels?us=in"
    pub_pixels_optout="http://%(HOST)s:%(FE_PORT)i/pubpixels?us=out"
    script_instantiate_url="http://%(HOST)s:%(FE_PORT)i/services/inst?format=js&amp;"
    iframe_instantiate_url="http://%(HOST)s:%(FE_PORT)i/services/inst?format=html&amp;"
    direct_instantiate_url="http://%(HOST)s:%(FE_PORT)i/services/inst?format=js&amp;"
  />
  <cfg:CreativeRule name="secure" secure="true"
    image_url="https://%(HOST)s:%(FE_PORT)i/creatives"
    publ_url="https://%(HOST)s:%(FE_PORT)i/publ"
    ad_click_url="https://%(HOST)s:%(FE_PORT)i/services/AdClickServer"
    ad_server="https://%(HOST)s:%(FE_PORT)i"
    ad_image_server="https://%(HOST)s:%(FE_PORT)i"
    track_pixel_url="https://%(HOST)s:%(FE_PORT)i/services/ImprTrack/pt.gif"
    passback_pixel_url="https://%(HOST)s:%(FE_PORT)i/services/passback"
    action_pixel_url="https://%(HOST)s:%(FE_PORT)i/services/ActionServer/SetCookie"
    passback_template_path_prefix="%(WWWROOT)s/Templates/Passback/pb."
    local_passback_prefix="https://%(HOST)s:%(FE_PORT)i/tags/"
    dynamic_creative_prefix="https://%(HOST)s:%(FE_PORT)i/services/dcreative"
    pub_pixels_optin="https://%(HOST)s:%(FE_PORT)i/pubpixels?us=in"
    pub_pixels_optout="https://%(HOST)s:%(FE_PORT)i/pubpixels?us=out"
    script_instantiate_url="https://%(HOST)s:%(FE_PORT)i/services/inst?format=js&amp;"
    iframe_instantiate_url="https://%(HOST)s:%(FE_PORT)i/services/inst?format=html&amp;"
    direct_instantiate_url="https://%(HOST)s:%(FE_PORT)i/services/inst?format=js&amp;"
  />
  </cfg:Creative>
  <cfg:CampaignServerCorbaRef name="CampaignServer">
   <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(CAMPAIGNSRV_PORT)i/%(CAMPAIGNSERVER_OBJECT_KEY)s"/>
  </cfg:CampaignServerCorbaRef>
  <cfg:Logging inventory_users_percentage="100" distrib_count="4" use_referrer_site_referrer_stats="empty">
   <cfg:ChannelTriggerStat flush_period="10"/>
   <cfg:ChannelHitStat flush_period="10"/>
   <cfg:RequestBasicChannels dump_channel_triggers="true" flush_period="10" adrequest_anonymize="false"/>
   <cfg:OptOutStat flush_period="10"/>
   <cfg:CreativeStat flush_period="10"/>
   <cfg:KeywordStat flush_period="10"/>
   <cfg:ActionRequest flush_period="10"/>
   <cfg:Request flush_period="10"/>
   <cfg:Impression flush_period="10"/>
   <cfg:Click flush_period="10"/>
   <cfg:AdvertiserAction flush_period="10"/>
   <cfg:PublisherInventory flush_period="10"/>
  </cfg:Logging>
 </cfg:CampaignManager>
</cfg:AdConfiguration>
""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

class UserInfoManager(CORBAProcess):
  def __init__(  self, info, testName, opts=[] ):
    CORBAProcess.__init__(self, info, testName, opts,
                          outdirs = ['CCGStat', 'CCGStat_'])
    self.USER_CACHE_ROOT = os.path.join(self.LOGROOT, 'var', 'cache', 'Users')

  def prepare( self ):
    CORBAProcess.prepare(self)
    os.makedirs(self.USER_CACHE_ROOT)

  def writeCfgFile( self ):
    self.port = self.USERINFOMGR_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/UserInfoSvcs/UserInfoManagerConfig.xsd">
 <cfg:UserInfoManagerConfig max_base_profile_waiters="8"
  max_temp_profile_waiters="8" max_pref_profile_waiters="8" max_freqcap_profile_waiters="8"
  channels_update_period="10" profile_lifetime="3600"
  temp_profile_lifetime="30" session_timeout="30" repeat_trigger_timeout="0"
  history_optimization_period="3600" root_dir="%(LOGROOT)s" colo_id="%(COLO_ID)i"
  service_index="1">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(USERINFOMGR_PORT)i">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="UserInfoManager" name="UserInfoManager"/>
    <cfg:Object servant="UserInfoManagerControl" name="UserInfoManagerControl"/>
    <cfg:Object servant="UserInfoManagerStats" name="UserInfoManagerStats"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:Logger filename="%(LOGROOT)s/UserInfoManager.log" log_level="%(LOGLEVEL)i">
    <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
    <cfg:Suffix min_log_level="5" max_log_level="7" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
  <cfg:ChunksConfig
      common_chunks_number="10" chunks_root="%(USER_CACHE_ROOT)s"
      rw_buffer_size="10485760" rwlevel_max_size="104857600"
      max_undumped_size="262144000" max_levels0="20"/>
  <cfg:CampaignServerCorbaRef name="CampaignServer">
   <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(CAMPAIGNSRV_PORT)i/CampaignServer"/>
  </cfg:CampaignServerCorbaRef>
  <cfg:UserProfilesCleanup content_cleanup_time="43200" process_portion="100" start_time="00:01"/>
  <cfg:FreqCaps confirm_timeout="1"/>
 </cfg:UserInfoManagerConfig>
</cfg:AdConfiguration>
""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

class UserInfoManagerController(CORBAProcess):
  def __init__(  self, info, testName, opts=[] ):
    CORBAProcess.__init__(self, info, testName, opts)

  def writeCfgFile( self ):
    self.port = self.USERINFOCTRL_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/UserInfoSvcs/UserInfoManagerControllerConfig.xsd">
 <cfg:UserInfoManagerControllerConfig colo_id="%(COLO_ID)i" status_check_period="10">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(USERINFOCTRL_PORT)i">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="UserInfoManagerController" name="UserInfoManagerController"/>
    <cfg:Object servant="UserInfoClusterControl" name="UserInfoClusterControl"/>
    <cfg:Object servant="UserInfoClusterStats" name="UserInfoClusterStats"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:Logger filename="%(LOGROOT)s/UserInfoManagerController.log" log_level="%(LOGLEVEL)i">
    <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
    <cfg:Suffix min_log_level="5" max_log_level="7" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
  <cfg:UserInfoManagerHost>
   <cfg:UserInfoManagerRef name="UserInfoManager" ref="corbaloc:iiop:%(HOST)s:%(USERINFOMGR_PORT)i/UserInfoManager"/>
   <cfg:UserInfoManagerControlRef name="UserInfoManagerControl" ref="corbaloc:iiop:%(HOST)s:%(USERINFOMGR_PORT)i/UserInfoManagerControl"/>
   <cfg:UserInfoManagerStatsRef name="UserInfoManagerStats" ref="corbaloc:iiop:%(HOST)s:%(USERINFOMGR_PORT)i/UserInfoManagerStats"/>
  </cfg:UserInfoManagerHost>
 </cfg:UserInfoManagerControllerConfig>
</cfg:AdConfiguration>""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

class ChannelServer(CORBAProcess):

  def __init__(  self, info, testName, opts=[] ):
    CORBAProcess.__init__(self, info, testName, opts,
                          outdirs = ['ColoUpdateStat', 'ColoUpdateStat_'])

  def writeCfgFile( self ):
    self.port = self.CHANNELSRV_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/ChannelSvcs/ChannelServerConfig.xsd">
 <cfg:ChannelServerConfig country="ru" count_chunks="32" merge_size="1"
   update_period="%(CONFIG_UPDATE_PERIOD)i" log_root="%(LOGROOT)s/Out" service_index="1">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(CHANNELSRV_PORT)s">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="ChannelServerControl" name="ChannelServerControl"/>
    <cfg:Object servant="ChannelServer" name="ChannelServer"/>
    <cfg:Object servant="ChannelUpdate" name="ChannelUpdate"/>
    <cfg:Object servant="ProcessStatsControl" name="ProcessStatsControl"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:MatchOptions nonstrict="false">
   <cfg:AllowPort>80</cfg:AllowPort>
  </cfg:MatchOptions>
  <cfg:Logger filename="%(LOGROOT)s/ChannelServer.log" log_level="%(LOGLEVEL)i">
      <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
      <cfg:Suffix min_log_level="5" max_log_level="17" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
  <cfg:UpdateStatLogger size="3" period="30" path="ColoUpdateStat"/>
  <cfg:Segmentors matching_segmentor="Polyglot">
    <cfg:Segmentor name="Polyglot" country="" base="/opt/foros/polyglot/dict/"/>
  </cfg:Segmentors>
 </cfg:ChannelServerConfig>
</cfg:AdConfiguration>""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

class ChannelController(CORBAProcess):

  def __init__(  self, info, testName, opts=[] ):
    CORBAProcess.__init__(self, info, testName, opts)

  def writeCfgFile( self ):
    self.port = self.CHANNELCTRL_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration
 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/ChannelSvcs/ChannelManagerControllerConfig.xsd">
 <cfg:ChannelControllerConfig count_chunks="%(CONFIG_COUNT_CHUNKS)i" source_id="1">
  <cfg:Primary control="true"/>
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(CHANNELCTRL_PORT)i">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="ChannelManagerController" name="ChannelManagerController"/>
    <cfg:Object servant="ChannelClusterControl" name="ChannelClusterControl"/>
    <cfg:Object servant="ProcessStatsControl" name="ProcessStatsControl"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:ColoSettings colo="%(COLO_ID)i" version="%(VERSION)s"/>""" % self.__dict__
      if self.CHANNELPROXY_USED:
        self.__writeChannelProxySource(f)
      else:
        self.__writeChannelDBSource(f)
      print >> f, """<cfg:ControlGroup>
   <cfg:ChannelHost>
     <cfg:ChannelServerControlRef name="ChannelServerControl" ref="corbaloc:iiop:%(HOST)s:%(CHANNELSRV_PORT)i/ChannelServerControl"/>
     <cfg:ChannelServerProcRef name="ChannelServerProc" ref="corbaloc:iiop:%(HOST)s:%(CHANNELSRV_PORT)i/ProcessControl"/>
     <cfg:ChannelServerRef name="ChannelServer" ref="corbaloc:iiop:%(HOST)s:%(CHANNELSRV_PORT)i/ChannelServer"/>
     <cfg:ChannelUpdateRef name="ChannelUpdate" ref="corbaloc:iiop:%(HOST)s:%(CHANNELSRV_PORT)i/ChannelUpdate"/>
     <cfg:ChannelStatRef name="ProcessStatsControl" ref="corbaloc:iiop:%(HOST)s:%(CHANNELSRV_PORT)i/ProcessStatsControl"/>
   </cfg:ChannelHost>
  </cfg:ControlGroup>
  <cfg:Logger filename="%(LOGROOT)s/ChannelController.log" log_level="%(LOGLEVEL)i">
    <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
    <cfg:Suffix min_log_level="5" max_log_level="7" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
 </cfg:ChannelControllerConfig>
</cfg:AdConfiguration>""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

  def __writeChannelDBSource( self, f ):
    print >> f, """<cfg:ChannelSource>
      <cfg:RegularSource>
       <cfg:CampaignServerCorbaRef name="CampaignServer">
         <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(CAMPAIGNSRV_PORT)i/%(CAMPAIGNSERVER_OBJECT_KEY)s"/>
       </cfg:CampaignServerCorbaRef>
       <cfg:PGConnection connection_string="host=%(PQ_HOST)s port=%(PQ_PORT)s dbname=%(PQ_DB)s user=%(PQ_USER)s password=%(PQ_DBPWD)s"/>
      </cfg:RegularSource>
    </cfg:ChannelSource>""" % self.__dict__

  def __writeChannelProxySource( self, f ):
    print >> f, """<cfg:ChannelSource>
     <cfg:ProxySource>
        <cfg:CampaignServerCorbaRef name="CampaignServer">
          <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(CAMPAIGNSRV_PORT)i/%(CAMPAIGNSERVER_OBJECT_KEY)s"/>
        </cfg:CampaignServerCorbaRef>
        <cfg:ProxyRefs name="ChannelProxy">
          <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(CHANNELPROXY_PORT)i/%(CHANNELPROXY_OBJECT_KEY)s"/>
        </cfg:ProxyRefs>
      </cfg:ProxySource>
    </cfg:ChannelSource>""" % self.__dict__


class ChannelSearchService(CORBAProcess):

  def __init__(  self, info, testName, opts=[] ):
    CORBAProcess.__init__(self, info, testName, opts)

  def writeCfgFile( self ):
    self.port = self.CHANNELSEARCHSVC_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/ChannelSearchSvcs/ChannelSearchServiceConfig.xsd">
 <cfg:ChannelSearchServiceConfig service_index="1">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(CHANNELSEARCHSVC_PORT)i">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="ChannelSearch" name="ChannelSearch"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:Logger filename="%(LOGROOT)s/ChannelSearchService.log" log_level="%(LOGLEVEL)i">
    <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
    <cfg:Suffix min_log_level="5" max_log_level="7" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
  <cfg:ChannelManagerControllerRefs name="ChannelManagerControllers">
    <cfg:Ref ref="corbaloc:iiop::%(HOST)s:%(CHANNELCTRL_PORT)i/%(CHANNELCRTL_OBJECT_KEY)s"/>
  </cfg:ChannelManagerControllerRefs>
  <cfg:CampaignServerCorbaRef name="CampaignServer">
    <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(CAMPAIGNSRV_PORT)i/%(CAMPAIGNSERVER_OBJECT_KEY)s"/>
  </cfg:CampaignServerCorbaRef>
 </cfg:ChannelSearchServiceConfig>
</cfg:AdConfiguration>
""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()


class ExpressionMatcher(CORBAProcess):
  def __init__(  self, info, testName, opts=[] ):
    CORBAProcess.__init__(self, info, testName, opts,
                          indirs = ['RequestBasicChannels', 'RequestBasicChannels/Error', 'RequestBasicChannels/Intermediate'],
                          outdirs = ['ChannelInventory', 'ChannelInventory_',
                                     'ChannelImpInventory', 'ChannelImpInventory_',
                                     'ChannelPriceRange', 'ChannelPriceRange_',
                                     'ChannelInventoryActivity', 'ChannelInventoryActivity_',
                                     'ChannelInventoryEstimationStat', 'ChannelInventoryEstimationStat_',
                                     'ChannelPerformance',  'ChannelPerformance_',
                                     'ChannelTriggerImpStat', 'ChannelTriggerImpStat_',
                                     'ColoUserStat', 'ColoUserStat_',
                                     'ChannelOverlapUserStat', 'ChannelOverlapUserStat_',
                                     'GlobalColoUserStat', 'GlobalColoUserStat_'],
                          cachedirs = ['Inventory', 'Estimation', 'UserTriggerMatch',
                                       'RequestTriggerMatch', 'HouseholdColoReach'])

  def writeCfgFile( self ):
    self.port = self.EXPRESSIONMATCHER_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/LogProcessing/ExpressionMatcherConfig.xsd">
 <cfg:ExpressionMatcherConfig log_root="%(LOGROOT)s"
  update_period="10" inventory_users_percentage="100"
  colo_id="%(COLO_ID)i" service_index="0">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(EXPRESSIONMATCHER_PORT)i">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="ExpressionMatcher" name="ExpressionMatcher"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:Logger filename="%(LOGROOT)s/ExpressionMatcher.log" log_level="%(LOGLEVEL)i">
    <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
    <cfg:Suffix min_log_level="5" max_log_level="7" time_span="86400" size_span="104857600" name=".trace"/>
   </cfg:Logger>
  <cfg:CampaignServerCorbaRef name="CampaignServer">
    <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(CAMPAIGNSRV_PORT)i/CampaignServer_v340"/>
  </cfg:CampaignServerCorbaRef>
   <cfg:ExpressionMatcherGroup distrib_count="4">
      <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(EXPRESSIONMATCHER_PORT)i/ExpressionMatcher"/>
    </cfg:ExpressionMatcherGroup>
  <cfg:UserInfoManagerControllerGroup name="UserInfoManagerControllers">
      <cfg:Ref ref="corbaloc:iiop::%(HOST)s:%(USERINFOCTRL_PORT)i/UserInfoManagerController"/>
  </cfg:UserInfoManagerControllerGroup>
  <cfg:ChunksConfig chunks_prefix="Inventory_" days_to_keep="3" max_levels0="20"
   rw_buffer_size="10485760" rwlevel_max_size="104857600" max_undumped_size="262144000"
   chunks_root="%(LOGROOT)s/cache/Inventory/" life_time="1"/>
  <cfg:EstimationChunksConfig chunks_prefix="Estimation_"  days_to_keep="3"  max_levels0="20"
   rw_buffer_size="10485760" rwlevel_max_size="104857600" max_undumped_size="262144000"
   chunks_root="%(LOGROOT)s/cache/Estimation/" life_time="1"/>
  <cfg:TriggerImpsConfig>
    <cfg:UserChunksConfig chunks_prefix="UserTriggerMatch_" temp_chunks_prefix="TempUserTriggerMatch_"
      positive_triggers_group_size="5" negative_triggers_group_size="5" chunks_root="%(LOGROOT)s/cache/UserTriggerMatch/"
      rw_buffer_size="10485760" rwlevel_max_size="104857600" max_undumped_size="262144000"
      life_time="43200" temp_life_time="60"  max_levels0="20"/>
    <cfg:RequestChunksConfig chunks_prefix="RequestTriggerMatch_"  max_levels0="20"
      rw_buffer_size="10485760" rwlevel_max_size="104857600" max_undumped_size="262144000"
      chunks_root="%(LOGROOT)s/cache/RequestTriggerMatch/" life_time="1"/>
  </cfg:TriggerImpsConfig>
  <cfg:HouseholdColoReachChunksConfig chunks_prefix="HouseholdColoReach_" life_time="86400"
    rw_buffer_size="10485760" rwlevel_max_size="104857600" max_undumped_size="262144000"
    chunks_root="%(LOGROOT)s/cache/HouseholdColoReach/"  max_levels0="20"/>
  <cfg:LogProcessing threads="1"  adrequest_anonymize="false" cache_blocks="1000">
   <cfg:InLogs log_root="%(LOGROOT)s/In/" check_logs_period="10">
    <cfg:RequestBasicChannels/>
    <cfg:ConsiderAction/>
    <cfg:ConsiderClick/>
    <cfg:ConsiderImpression/>
    <cfg:ConsiderRequest/>
   </cfg:InLogs>
   <cfg:OutLogs log_root="%(LOGROOT)s/Out/">
    <cfg:ChannelInventory period="10"/>
    <cfg:ChannelImpInventory period="10"/>
    <cfg:ChannelPriceRange period="10"/>
    <cfg:ChannelInventoryActivity period="3600"/>
    <cfg:ChannelPerformance period="10"/>
    <cfg:ChannelTriggerImpStat period="10"/>
    <cfg:GlobalColoUserStat period="10"/>
    <cfg:ColoUserStat period="10"/>
    <cfg:ChannelOverlapUserStat period="10"/>
   </cfg:OutLogs>
  </cfg:LogProcessing>
  <cfg:DailyProcessing processing_time="00:01">
    <cfg:Filter index="0" index_count="1" distrib_count="4"/>
  </cfg:DailyProcessing>
 </cfg:ExpressionMatcherConfig>
</cfg:AdConfiguration>""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

class RequestInfoManager(CORBAProcess):
  def __init__(  self, info, testName, opts=[] ):
    CORBAProcess.__init__(self, info, testName, opts,
                          indirs = ['Request', 'Impression',
                                    'Click', 'AdvertiserAction',
                                    'PassbackOpportunity','PassbackImpression',],
                          outdirs = ['CreativeStat', 'CreativeStat_',
                                     'UserProperties', 'UserProperties_',
                                     'ChannelPerformance', 'ChannelPerformance_',
                                     'SiteChannelStat','SiteChannelStat_',
                                     'ExpressionPerformance', 'ExpressionPerformance_',
                                     'CcgKeywordStat', 'CcgKeywordStat_',
                                     'KeywordStat', 'KeywordStat_',
                                     'CmpStat', 'CmpStat_',
                                     'ActionStat', 'ActionStat_',
                                     'ActionStat', 'ActionStat_',
                                     'CcgUserStat', 'CcgUserStat_',
                                     'CcUserStat', 'CcUserStat_',
                                     'CampaignUserStat', 'CampaignUserStat_',
                                     'PassbackStat', 'PassbackStat_',
                                     'AdvertiserUserStat', 'AdvertiserUserStat_',
                                     'SiteReferrerStat', 'SiteReferrerStat_',
                                     'PageLoadsDailyStat', 'PageLoadsDailyStat_',
                                     'TagPositionStat', 'TagPositionStat_',
                                     'ResearchAction', 'ResearchAction_',
                                     'ResearchBid', 'ResearchBid_',
                                     'RequestOperation', 'RequestOperation_'],
                          cachedirs = ['UserActions', 'UserCampaignReach',
                                       'UserFraudProtection', 'Passback',
                                       'UserSiteReach', 'UserTagRequestGroup',
                                       'Requests'])

  def writeCfgFile( self ):
    self.port = self.REQUESTINFOMGR_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/RequestInfoSvcs/RequestInfoManagerConfig.xsd">
 <cfg:RequestInfoManagerConfig
   colo_id="%(COLO_ID)i"
   service_index="0"
   services_count="1"
   distrib_count="1"
   action_ignore_time="30"
   use_referrer_site_referrer_stats="empty">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(REQUESTINFOMGR_PORT)i">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="RequestInfoManager" name="RequestInfoManager"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:Logger filename="%(LOGROOT)s/RequestInfoManager.log" log_level="%(LOGLEVEL)i">
    <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
    <cfg:Suffix min_log_level="5" max_log_level="7" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
  <cfg:UserActionChunksConfig chunks_prefix="UserAction_" chunks_root="%(LOGROOT)s/cache/UserActions/"/>
  <cfg:UserCampaignReachChunksConfig chunks_prefix="UserCampaignReach_" chunks_root="%(LOGROOT)s/cache/UserCampaignReach/"/>
  <cfg:UserFraudProtectionChunksConfig chunks_prefix="UserFraudProtection_" chunks_root="%(LOGROOT)s/cache/UserFraudProtection/"/>
  <cfg:PassbackChunksConfig chunks_prefix="Passback_" chunks_root="%(LOGROOT)s/cache/Passback/"/>
  <cfg:UserSiteReachChunksConfig chunks_prefix="UserSiteReach_" chunks_root="%(LOGROOT)s/cache/UserSiteReach/"/>
  <cfg:TagRequestGroupingConfig merge_time_bound="1" chunks_prefix="UserTagRequestGroup_" chunks_root="/%(LOGROOT)s/cache/UserTagRequestGroup/"/>
  <cfg:ChunksConfig chunks_prefix="Request_" chunks_root="%(LOGROOT)s/cache/Requests/"/>
  <cfg:LogProcessing cache_blocks="1000">
   <cfg:InLogs log_root="%(LOGROOT)s/In/" check_logs_period="10">
    <cfg:Request priority="2"/>
    <cfg:Impression priority="2"/>
    <cfg:Click priority="2"/>
    <cfg:AdvertiserAction priority="2"/>
    <cfg:PassbackImpression priority="1"/>
    <cfg:TagRequest priority="1"/>
    <cfg:RequestOperation priority="1"/>
   </cfg:InLogs>
   <cfg:OutLogs log_root="%(LOGROOT)s/Out/" notify_impressions="true" notify_revenue="true" distrib_count="4">
    <cfg:CreativeStat period="30"/>
    <cfg:UserProperties period="30"/>
    <cfg:ChannelPerformance period="30"/>
    <cfg:SiteChannelStat period="30"/>
    <cfg:ExpressionPerformance period="30"/>
    <cfg:CcgKeywordStat period="30"/>
    <cfg:CmpStat period="30"/>
    <cfg:ActionStat period="30"/>
    <cfg:ChannelImpInventory period="30"/>
    <cfg:CcgUserStat period="30"/>
    <cfg:CcUserStat period="30"/>
    <cfg:CampaignUserStat period="30"/>
    <cfg:AdvertiserUserStat period="30"/>
    <cfg:PassbackStat period="30"/>
    <cfg:SiteUserStat period="30"/>
    <cfg:SiteReferrerStat period="30"/>
    <cfg:PageLoadsDailyStat period="30"/>
    <cfg:TagPositionStat period="30"/>
    <cfg:ResearchAction period="30"/>
    <cfg:ResearchBid period="30"/>
    <cfg:RequestOperation chunks_count="24" period="30"/>
    <cfg:ConsiderAction period="30"/>
    <cfg:ConsiderClick period="30"/>
    <cfg:ConsiderImpression period="30"/>
    <cfg:ConsiderRequest period="30"/>
   </cfg:OutLogs>
  </cfg:LogProcessing>
  <cfg:CampaignServerCorbaRef name="CampaignServer">
    <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(CAMPAIGNSRV_PORT)i/%(CAMPAIGNSERVER_OBJECT_KEY)s"/>
  </cfg:CampaignServerCorbaRef>
  <cfg:UserInfoManagerControllerGroup name="UserInfoManagerControllers">
    <cfg:Ref ref="corbaloc:iiop:%(HOST)s:%(USERINFOCTRL_PORT)i/UserInfoManagerController"/>
  </cfg:UserInfoManagerControllerGroup>
 </cfg:RequestInfoManagerConfig>
</cfg:AdConfiguration>""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

class LogGeneralizer(CORBAProcess):
  def __init__(  self, info, testName, opts=[] ):
    dirs = ['ActionRequest', 'ActionStat', 'AdvertiserUserStat',
            'CCGKeywordStat', 'CCGStat', 'CCGUserStat', 'CCStat',
            'CCUserStat', 'CampaignStat', 'CampaignUserStat',
            'ChannelHitStat', 'ChannelImpInventory', 'ChannelInventory',
            'ChannelInventoryEstimationStat', 'ChannelPerformance',
            'ChannelPriceRange', 'ColoUpdateStat', 'ColoUserStat',
            'ColoUsers', 'ExpressionPerformance', 'GlobalColoUserStat',
            'OptOutStat', 'PassbackStat', 'PublisherInventory',
            'SiteChannelStat', 'SiteReferrerStat', 'SiteStat',
            'SiteUserStat', 'UserProperties',
            'CMPStat', 'CreativeStat', 'ChannelTriggerStat',
            'WebStat', 'ChannelTriggerImpStat', 'ChannelCountStat']
    CORBAProcess.__init__(self, info, testName, opts,
                          indirs = map(lambda a: a + '/Deferred', dirs),
                          outdirs = dirs + map(lambda a: a + '_', dirs))

  def writeCfgFile( self ):
    self.port = self.LOGGENERALIZER_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/LogProcessing/LogGeneralizerConfig.xsd">
 <cfg:LogGeneralizerConfig input_logs_dir="%(LOGROOT)s/In" output_logs_dir="%(LOGROOT)s/Out">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(LOGGENERALIZER_PORT)i">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
    <cfg:Object servant="ProcessStatsControl" name="ProcessStatsControl"/>
    <cfg:Object servant="LogGeneralizer" name="LogGeneralizer"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:Logger filename="%(LOGROOT)s/LogGeneralizer.log" log_level="%(LOGLEVEL)i">
    <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
    <cfg:Suffix min_log_level="5" max_log_level="7" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
  <cfg:DBConnection>
   <cfg:Postgres connection_string="host=%(PQ_HOST)s port=%(PQ_PORT)s dbname=%(PQ_DB)s user=%(PQ_USER)s password=%(PQ_DBPWD)s"/>
  </cfg:DBConnection>
  <cfg:XSearchStatParams url="http://%(HOST)s:28082/psp/stat"/>
  <cfg:LogProcessing>
    <cfg:CMPStat upload_type="postgres_csv" check_logs_period="10" check_deferred_logs_period="20" max_time="10" max_size="10000" max_upload_task_count="4" backup_mode="false"/>
    <cfg:CreativeStat upload_type="postgres_csv" check_logs_period="10" check_deferred_logs_period="20" max_time="10" max_size="10000" max_upload_task_count="4" backup_mode="false"/>

    <cfg:CampaignStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelCountStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ColoUsers check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:SiteStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:WebStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:AdvertiserUserStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:CCStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:CCGStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:CCGKeywordStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:CCGUserStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:CCUserStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:CampaignUserStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ColoUserStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:GlobalColoUserStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:PageLoadsDailyStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:PassbackStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:SiteUserStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ActionRequest check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ActionStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ColoUpdateStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:CCGSelectionFailureStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelHitStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelImpInventory check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelInventory check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelInventoryEstimationStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelOverlapUserStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelPerformance check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelPriceRange check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelTriggerImpStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ChannelTriggerStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:DeviceChannelCountStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:ExpressionPerformance check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:OptOutStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:PublisherInventory check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:SearchEngineStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:SiteChannelStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:SiteReferrerStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:TagAuctionStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:TagPositionStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:UserAgentStat check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
    <cfg:UserProperties check_logs_period="10" max_time="10" max_size="10000" max_upload_task_count="2" backup_mode="false"/>
  </cfg:LogProcessing>
 </cfg:LogGeneralizerConfig>
</cfg:AdConfiguration>""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()

class SyncLogs(CORBAProcess):

  def __init__(  self, info, testName, opts=[] ):
    CORBAProcess.__init__(self, info, testName, opts)

  def writeCfgFile( self ):
    self.port = self.SYNCLOGS_PORT
    f = file(self.cfgFilePath, 'w')
    try:
      print >> f, """<?xml version="1.0" encoding="utf-8"?>
<cfg:AdConfiguration
 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 xmlns:cfg="http://www.adintelligence.net/xsd/AdServer/Configuration"
 xmlns:colo="http://www.foros.com/cms/colocation"
 xsi:schemaLocation="http://www.adintelligence.net/xsd/AdServer/Configuration %(XSDDIR)s/LogProcessing/SyncLogsConfig.xsd">
 <cfg:SyncLogsConfig log_root="%(LOGROOT)s" check_logs_period="10" host_check_period="10" hostname="%(HOST)s">
  <cfg:CorbaConfig threading-pool="%(THREADING_POOL)i">
   <cfg:Endpoint host="*" port="%(SYNCLOGS_PORT)s">
    <cfg:Object servant="ProcessControl" name="ProcessControl"/>
   </cfg:Endpoint>
  </cfg:CorbaConfig>
  <cfg:Logger filename="%(LOGROOT)s/SyncLogs.log" log_level="%(LOGLEVEL)i">
    <cfg:Suffix time_span="86400" size_span="104857600" max_log_level="4" name=".error"/>
    <cfg:Suffix min_log_level="5" max_log_level="7" time_span="86400" size_span="104857600" name=".trace"/>
  </cfg:Logger>
  <cfg:ClusterConfig root_logs_dir="%(ROOT)s" definite_hash_schema="%(XSDDIR)s/AdServerCommons/HostDistributionFile.xsd">
   <cfg:FeedRouteGroup
    local_copy_command_type="rsync"
    remote_copy_command_type="rsync"
    tries_per_file="2"
    local_copy_command="/usr/bin/rsync -t -z --timeout=55 --log-format=%%f --ignore-existing ##SRC_PATH## ##DST_PATH##"
    remote_copy_command="/usr/bin/rsync -e 'ssh -i %(HOME)s/.ssh/adkey' -t -z --timeout=55 --log-format=%%f --ignore-existing ##SRC_PATH## %(USER)s@##DST_HOST##:##DST_PATH##">
    <cfg:Route type="RoundRobin">
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/CreativeStat/CreativeStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/CreativeStat"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/ColoUsers/ColoUsers.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ColoUsers"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/OptOutStat/OptOutStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/OptOutStat"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/ChannelTriggerStat/ChannelTriggerStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ChannelTriggerStat"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/ChannelHitStat/ChannelHitStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ChannelHitStat"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/SiteReferrerStat/SiteReferrerStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/SiteReferrerStat"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/KeywordStat/KeywordStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/KeywordStat"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/ActionRequest/ActionRequest.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ActionRequest"/>
     <cfg:hosts
      source="%(HOST)s" destination="%(HOST)s"/>
    </cfg:Route>
    <cfg:Route type="RoundRobin">
     <cfg:files
      source="%(PFX)s/CampaignServer/Out/ColoUpdateStat/ColoUpdateStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ColoUpdateStat"/>
     <cfg:hosts source="%(HOST)s" destination="%(HOST)s"/>
    </cfg:Route>
    <cfg:Route type="RoundRobin">
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/Request/Request.log_*"
      destination="%(PFX)s/RequestInfoManager/In/Request"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/Impression/Impression.log_*"
      destination="%(PFX)s/RequestInfoManager/In/Impression"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/Click/Click.log_*"
      destination="%(PFX)s/RequestInfoManager/In/Click"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/AdvertiserAction/AdvertiserAction.log_*"
      destination="%(PFX)s/RequestInfoManager/In/AdvertiserAction"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/PassbackOpportunity/PassbackOpportunity.log_*"
      destination="%(PFX)s/RequestInfoManager/In/PassbackOpportunity"/>
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/PassbackImpression/PassbackImpression.log_*"
      destination="%(PFX)s/RequestInfoManager/In/PassbackImpression"/>
     <cfg:hosts source="%(HOST)s" destination="%(HOST)s"/>
    </cfg:Route>
    <cfg:Route type="RoundRobin">
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/CreativeStat/CreativeStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/CreativeStat"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/UserProperties/UserProperties.log_*"
      destination="%(PFX)s/LogGeneralizer/In/UserProperties"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/ChannelPerformance/ChannelPerformance.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ChannelPerformance"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/ExpressionPerformance/ExpressionPerformance.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ExpressionPerformance"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/CCGKeywordStat/CCGKeywordStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/CCGKeywordStat"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/SiteChannelStat/SiteChannelStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/SiteChannelStat"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/CCGStat/CCGStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/CCGStat"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/CCStat/CCStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/CCStat"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/CampaignStat/CampaignStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/CampaignStat"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/ActionStat/ActionStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ActionStat"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/KeywordStat/KeywordStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/KeywordStat"/>
     <cfg:files
      source="%(PFX)s/RequestInfoManager/Out/PassbackStat/PassbackStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/PassbackStat"/>
     <cfg:hosts
      source="%(HOST)s" destination="%(HOST)s"/>
    </cfg:Route>
    <cfg:Route type="RoundRobin">
     <cfg:files
      source="%(PFX)s/ChannelServer/Out/ColoUpdateStat/ColoUpdateStat.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ColoUpdateStat"/>
     <cfg:hosts
      source="%(HOST)s" destination="%(HOST)s"/>
    </cfg:Route>
    <cfg:Route type="RoundRobin">
     <cfg:files
      source="%(PFX)s/CampaignManager/Out/RequestBasicChannels/RequestBasicChannels.log_*"
      destination="%(PFX)s/ExpressionMatcher/In/RequestBasicChannels"/>
     <cfg:hosts source="%(HOST)s" destination="%(HOST)s"/>
    </cfg:Route>
    <cfg:Route type="RoundRobin">
     <cfg:files
      source="%(PFX)s/ExpressionMatcher/Out/ChannelInventory/ChannelInventory.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ChannelInventory"/>
     <cfg:files
      source="%(PFX)s/ExpressionMatcher/Out/ChannelPriceRange/ChannelPriceRange.log_*"
      destination="%(PFX)s/LogGeneralizer/In/ChannelPriceRange"/>
     <cfg:hosts source="%(HOST)s" destination="%(HOST)s"/>
    </cfg:Route>
   </cfg:FeedRouteGroup>
   <cfg:FeedRouteGroup
    local_copy_command="/bin/echo"
    local_copy_command_type="generic"
    remote_copy_command_type="rsync"
    tries_per_file="2"
    parse_source="false"
    unlink_source="false"
    interruptible="true"
    remote_copy_command="/usr/bin/rsync -e 'ssh -i %(HOME)s/.ssh/adkey' --partial -avz -t --timeout=55 --log-format=%%f --delete-after ##DST_HOST##:##SRC_PATH## ##DST_PATH##">
    <cfg:Route type="RoundRobin">
     <cfg:files source="/opt/foros/ui/var/sync/tags/" destination="%(RUNDIR)s/var/www/tags/"/>
     <cfg:files source="/opt/foros/ui/var/sync/Creatives/" destination="%(RUNDIR)s/var/www/Creatives/"/>
     <cfg:files source="/opt/foros/ui/var/sync/Templates/" destination="%(RUNDIR)s/var/www/Templates/"/>
     <cfg:files source="/opt/foros/ui/var/sync/WebwiseDiscover/" destination="%(RUNDIR)s/var/www/WebwiseDiscover/"/>
     <cfg:hosts source="%(HOST)s" destination="%(HOST)s"/>
    </cfg:Route>
   </cfg:FeedRouteGroup>
  </cfg:ClusterConfig>
 </cfg:SyncLogsConfig>
</cfg:AdConfiguration>
""" % self.__dict__
    except Exception, exc:
      print >> f, exc
    f.close()


class CORBAFunTest(OrbTestSuite.OrbTestSuite):

  proc    = None

  def __init__( self, info, children = None ):
    OrbTestSuite.OrbTestSuite.__init__( self, info, children )
    self.calls = MTQueue()
    self.srvs = []

  def addCall( self, callName):
    self.calls.push(callName)

  def applyVars(self, proc):
    for key, value in self.__dict__.iteritems():
      if proc.__dict__.has_key(key):
        proc.__dict__[key] = value

  def setUpServers( self, *srvs):
    cdesc = self.classDesc() or self.__class__.__name__
    for srv in srvs:
      name = srv.__name__
      try:
        FunTest.tlog(10, 'create %s server' % (name))
        proc = srv(self.info, cdesc)
        self.applyVars(proc)
        setattr(self, name, proc)
        self.srvs.append(proc)
      except Exception, exc:
        print exc
        log(2, 'create server %s exception "%s"' % (name, exc))
    FunTest.tlog(10, 'create servers ok')

  def startProc( self ):
    FunTest.tlog(10, 'start servers')
    for srv in self.srvs:
      if srv.start():
        FunTest.tlog(10, '%s process port is %d' % (srv.name, srv.port))
        started = False
        for i in xrange(1, 300):
          if srv.is_alive():
            FunTest.tlog(10, '%s process started' % srv.name)
            started = True
            break
          else:
            continue
        if not started:
          FunTest.tlog(10, '%s process not started' % srv.name)
      else:
        raise Exception("%s isn't started" % srv.name)

  def tearDownServers( self ):
    for srv in self.srvs:
      if isinstance(srv, FunTest.Process):
        srv.stop()
        FunTest.tlog(10, '%s process stopped' % srv.name)
    self.deactivateObjects()

  def checkCallSequence( self,
                         expCalls = [],
                         skipCalls = [],
                         timeout = 1 ):
    gotCalls = []
    expCalls_ = []
    skipCalls_ = []
    def get_calls(iCalls, oCalls):
      for exp in iCalls:
        if isinstance(exp, CallSequence):
          for i in range(exp.count):
            oCalls.append(exp.call)
        else:
          oCalls.append(exp)
    get_calls(expCalls, expCalls_)
    get_calls(skipCalls, skipCalls_)
    for i in range(len(expCalls_)):
      try:
        gotCall = self.calls.pop(timeout)
        while gotCall in skipCalls_:
          gotCall = self.calls.pop(timeout)
        gotCalls.append(gotCall)
      except XTimedOut:
        break
    #FunTest.tlog(10, 'exp: %s, got: %s' % (expCalls_, gotCalls))
    self.assertHasOrderedItems(expCalls_, gotCalls, 'CORBA calls')

