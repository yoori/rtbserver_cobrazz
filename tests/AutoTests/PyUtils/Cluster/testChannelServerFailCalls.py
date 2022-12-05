#!/usr/bin/env python

from CORBATest import *
from FunTest import tlog
from OrbTestSuite import main
from Util import currentTime, time2str
from OrbUtil import time2orb, orb2time
from MockCampaignServer import CampaignServerTestMixin, CampaignServer, SimpleChannelKey, \
     BehavParameter, BehavParamInfo
from MockChannelProxy import ChannelProxyTestMixin, ChannelVersion, ChannelById, \
     ChannelProxy, CheckData, TriggerInfo
from MockChannelController import ChannelManagerControllerTestMixin
from FakeCallWrapper import ExceptionCallWrapper
from omniORB import CORBA
from MTValue import MTFlag

import ChannelServerUtils,  AdServer.ChannelSvcs

class ChannelServersFailCallsTest(CORBAFunTest,  CampaignServerTestMixin,
                                  ChannelProxyTestMixin, ChannelManagerControllerTestMixin):
  'ChannelServer fail calls'

  def setUp( self ):
    self.LOGLEVEL = 100
    self.CONFIG_UPDATE_PERIOD = 1
    self.CAMPAIGNSRV_PORT  = self.orbPort
    self.CHANNELPROXY_PORT = self.orbPort
    self.CHANNELPROXY_USED = True
    self.checkFlag = MTFlag()
    CampaignServerTestMixin.setUp( self )
    ChannelProxyTestMixin.setUp( self )
    self.__prepareConfig()
    # CampaignServer.simple_channels wrapper
    self.simpleChannelWrapper = ExceptionCallWrapper(self,
                                                     'CampaignServer_chsv_simple_channels',
                                                     [(CORBA.UNKNOWN, (), 6),
                                                     (CampaignServer.ImplementationException,
                                                      ("invalid implementation",), 6),
                                                      (CampaignServer.NotReady,
                                                       ("server is not ready",), 6)])
    # ChannelProxy.check wrapper
    self.checkWrapper = ExceptionCallWrapper(self,
                                             'ChannelProxy_check',
                                             [(CORBA.UNKNOWN, (), 6),
                                              (AdServer.ChannelSvcs.ImplementationException,
                                               ("invalid implementation", ), 6),
                                              (AdServer.ChannelSvcs.NotConfigured,
                                               ("server is not configured",), 6)])
    # ChannelProxy.update_all_ccg wrapper
    self.updateAllCcgWrapper = ExceptionCallWrapper(self,
                                                    'ChannelProxy_update_all_ccg',
                                               [(CORBA.UNKNOWN, (), 6),
                                                (AdServer.ChannelSvcs.ImplementationException,
                                                 ("invalid implementation", ), 6),
                                                (AdServer.ChannelSvcs.NotConfigured,
                                                 ("server is not configured",), 6)])
    # ChannelProxy.update_triggers wrapper
    self.updateTriggersWrapper = ExceptionCallWrapper(self,
                                                      'ChannelProxy_update_triggers',
                                                      [(CORBA.UNKNOWN, (), 6),
                                                       (AdServer.ChannelSvcs.ImplementationException,
                                                        ("invalid implementation", ), 6),
                                                       (AdServer.ChannelSvcs.NotConfigured,
                                                        ("server is not configured",), 6)])
    
    self.setUpServers(ChannelServer)
    self.startProc()
                                               
  def __prepareConfig( self ):
    self.simple_channels = [SimpleChannelKey(
                                             11,                       # channel_id
                                             "GN",                     # country_code
                                             "",                       # language
                                             "A",                      # status
                                             1,                        # behav_param_list_id
                                             "",                       # str_behav_param_list_id
                                             [],                       # categories 
                                             0,                        # threshold
                                             0,                        # discover
                                             [],                       # page_triggers
                                             [],                       # search_triggers
                                             [],                       # url_triggers
                                             [],                       # url_keyword_triggers
                                             time2orb(currentTime())   # timestamp
                                             )]
    self.behav_params = [BehavParamInfo(1, 0, time2orb(currentTime()),
                                        [BehavParameter(1, 0, 0, 1, "P")])]
    self.channelById = [ChannelById(11,                     # channel_id
                                    # page_words
                                    [TriggerInfo(1, ChannelServerUtils.createChannelTriggerWord('P',
                                                                                                "word")) ],
                                    time2orb(currentTime()) # stamp        
                                    )]


  def __synced_ChannelProxy_check(self, query):
    self.checkFlag.wait()
    return ChannelProxyTestMixin.ChannelProxy_check(self, query)

  def tearDown( self ):
    self.tearDownServers()

  def run( self ):
    res = self.test(1, self.testCampaignServerSimpleChannelsFail)
    res = self.test(1, self.testChannelProxyCheckFail, res)
    res = self.test(1, self.testChannelProxyUpdateAllCcgFail, res)
    res = self.test(1, self.testChannelProxyUpdateTriggersFail, res)
    res = self.test(1, self.testNormalReconnect, res)

  def testCampaignServerSimpleChannelsFail( self ):
    'CampaignServer simple_channels fail'
    obj = self.ChannelServer.getObject("ChannelServerControl",
                                       AdServer.ChannelSvcs.ChannelServerControl)
    source = ChannelServerUtils.createProxySource(self.MockChannelProxyIOR,
                                                  [self.MockCampaignServerIOR],
                                                  count_chunks = 4)
    obj.set_proxy_sources(source, [0,1,2,3])
    self.simpleChannelWrapper.wait()

  def testChannelProxyCheckFail( self ):
    'ChannelProxy check fail'
    self.checkWrapper.wait(skipCalls = ['CampaignServer_chsv_simple_channels'])

  def testChannelProxyUpdateAllCcgFail( self ):
    'ChannelProxy update_all_ccg fail'
    self.updateAllCcgWrapper.wait(skipCalls = ['CampaignServer_chsv_simple_channels',
                                               'ChannelProxy_check'])
    self.ChannelProxy_check = self.__synced_ChannelProxy_check



  def testChannelProxyUpdateTriggersFail( self ):
    'ChannelProxy update_triggers fail'
    self.updateTriggersWrapper.wait(skipCalls = ['CampaignServer_chsv_simple_channels',
                                                 'ChannelProxy_check',
                                                 'ChannelProxy_update_all_ccg'])

  def testNormalReconnect( self ):
    'normal reconnect'
    self.channelById[0].stamp = time2orb(currentTime())
    self.checkFlag.set()
    self.checkCallSequence(expCalls = [CallSequence('CampaignServer_chsv_simple_channels', 20),
                                       'ChannelProxy_check',
                                       'ChannelProxy_update_all_ccg',
                                       'ChannelProxy_update_triggers'],
                           timeout = 6)


if __name__ == '__main__':
 main()

  
