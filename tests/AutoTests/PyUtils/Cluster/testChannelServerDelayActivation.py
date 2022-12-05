#!/usr/bin/env python

from CORBATest import *
from FunTest import tlog
from Util import currentTime, time2str
from OrbUtil import time2orb, orb2time
from OrbTestSuite import main
from MockCampaignServer import CampaignServerTestMixin, CampaignServer, SimpleChannelKey, \
     BehavParameter, BehavParamInfo
from MockChannelProxy import ChannelProxyTestMixin, TriggerVersion, ChannelById, \
     ChannelProxy, CheckData, CheckQuery, NotConfigured, ChannelUpdateCurrent, \
     TriggerInfo
from ChannelServerUtils import createChannelTriggerWord
from FakeCallWrapper import DelayCallWrapper

import AdServer.ChannelSvcs, random, string, time

class ChannelServersDelayActivation(CORBAFunTest,
                                    CampaignServerTestMixin,
                                    ChannelProxyTestMixin):
  'ChannelServer delayed activation'

  def setUp( self ):
    self.LOGLEVEL = 100
    self.CONFIG_UPDATE_PERIOD = 1
    self.CONFIG_COUNT_CHUNKS = 1
    self.CAMPAIGNSRV_PORT  = self.orbPort
    self.CHANNELPROXY_PORT = self.orbPort
    self.CHANNELPROXY_USED = True
    CampaignServerTestMixin.setUp( self )
    ChannelProxyTestMixin.setUp( self )
    self.__prepareConfig()

    self.setUpServers(ChannelServer, 
                      ChannelController)
    self.startProc()
    self.channelServerObj = self.ChannelServer.getObject("ChannelUpdate",
                                                         ChannelUpdateCurrent)
  def __prepareConfig( self ):
    now       = currentTime()
    self.checkTimeStamp = None
    self.startTriggerId = 4
    self.simple_channels = [SimpleChannelKey(12,                       # channel_id
                                             "gn",                     # country_code
                                             "",                       # language
                                             "A",                      # status
                                             1,                        # behav_param_list_id
                                             "",                       # str_behav_param_list_id 
                                             [],                       # categories
                                             1,                        # threshold
                                             0,                        # discover
                                             [],                       # page_triggers
                                             [],                       # search_triggers
                                             [],                       # url_triggers
                                             [],                       # url_keyword_triggers
                                             time2orb(now)             # timestamp
                                             )]
    self.behav_params = [BehavParamInfo(1,                             # id
                                        0,                             # threshold
                                        time2orb(now),                 # timestamp
                                        # bp_seq
                                        [BehavParameter(1,               # min_visits
                                                        0,               # time_from
                                                        0,               # time_to
                                                        1,               # weight
                                                        "P"              # trigger_type
                                                        )])]
    self.channelById = [ChannelById(12,                                # channel_id
                                    # hard_words
                                    self.__generateHardWord(0x04), # PAGE TYPE
                                    time2orb(now)                  # stamp
                                    )]
    

  def __prepareDelay( self ):
    now       = currentTime()
    # CampaignServer.simple_channels wrapper
    self.simpleChannelWrapper = DelayCallWrapper(self,
                                                 'CampaignServer_chsv_simple_channels')
    self.simpleChannelWrapper.wait()

    # ChannelProxy.check wrapper
    self.checkWrapper = DelayCallWrapper(self, 'ChannelProxy_check')
    
    # ChannelProxy.update_triggers wrapper
    self.updateTriggersWrapper = DelayCallWrapper(self, 'ChannelProxy_update_triggers', 2)

    # Change config
    self.simple_channels +=[SimpleChannelKey(13,                       # channel_id
                                             "gn",                     # country_code
                                             "",                       # language
                                             "A",                      # status
                                             1,                        # behav_param_list_id
                                             "",                       # str_behav_param_list_id 
                                             [],                       # categories
                                             1,                        # threshold
                                             0,                        # discover
                                             [],                       # page_triggers
                                             [],                       # search_triggers
                                             [],                       # url_triggers
                                             [],                       # url_keyword_triggers
                                             time2orb(now)             # timestamp
                                             ),
                            SimpleChannelKey(14,                        # channel_id
                                             "gn",                     # country_code
                                             "",                       # language
                                             "A",                      # status
                                             1,                        # behav_param_list_id
                                             "",                       # str_behav_param_list_id 
                                             [],                       # categories
                                             1,                        # threshold
                                             0,                        # discover
                                             [],                       # page_triggers
                                             [],                       # search_triggers
                                             [],                       # url_triggers
                                             [],                       # url_keyword_triggers
                                             time2orb(now)             # timestamp
                                             )]
    self.channelById += [ChannelById(13,                            # channel_id
                                     # hard_words
                                     self.__generateHardWord(0x04), # PAGE TYPE
                                     time2orb(now)                  # stamp
                                    ),
                         ChannelById(14,                            # channel_id
                                     # hard_words
                                     self.__generateHardWord(0x04), # PAGE TYPE
                                     time2orb(now)                  # stamp
                                    )]


  def __generateHardWord( self, type, size = 1024*1024, max_word_size = 512 ):
    triggers = []
    for i in range(size/max_word_size):
      word_letters = [random.choice(string.letters) for x in xrange(max_word_size)]
      trigger = TriggerInfo(self.startTriggerId,
                            createChannelTriggerWord('P',
                                                     "".join(word_letters)))
      triggers.append(trigger)
      self.startTriggerId += 1
    return triggers
    

  def __channelServerSafeCall( self, method, *args, **kw ):
    fn = getattr(self.channelServerObj, method)
    try:
      return fn( *args, **kw)
    except NotConfigured:
      return None

  def tearDown( self ):
    self.tearDownServers()

  def checkLastCall( self, expCall, timeout ):
    gotCall = ''
    while True:
      try:
        gotCall = self.calls.pop(timeout)
      except XTimedOut:
        break
    self.assertEqual(expCall, gotCall, 'Last CORBA call')

  def run( self ):
    res = self.test(1, self.testPrepare)
    res = self.test(1, self.testDelayedSimpleChannels, res)
    res = self.test(1, self.testDelayedCheck, res)
    res = self.test(1, self.testDelayedUpdate, res)
    res = self.test(1, self.test1stTriggerActivation, res)
    res = self.test(1, self.test2ndTriggerActivation, res)

  def testPrepare( self ):
    '1st full update withot delays'
    self.checkCallSequence(expCalls = [CallSequence('CampaignServer_chsv_simple_channels', 20),
                                       'ChannelProxy_check',
                                       'ChannelProxy_update_all_ccg',
                                       'ChannelProxy_update_triggers'], timeout = 2)

    self.ChannelServer.waitReady()

    checkData = self.channelServerObj.check(CheckQuery(1, "1", time2orb(), [], False))
    self.assertNotEqual(None, orb2time(checkData.master_stamp))
    result = self.channelServerObj.update_triggers([12])
    self.assertEqual(1, len(result.channels), 'channel size')
    self.assertEqual(12, result.channels[0].channel_id, 'channel#1.id')
    self.assertNotEqual(None, orb2time(result.channels[0].stamp), 'channel#1.stamp')
    self.assertNotEqual(1, len(result.channels[0].words), 'channel#1.triggers')
    self.__prepareDelay()
    tlog(10, "Calls: %s" % self.calls.queue)
        
  def testDelayedSimpleChannels( self ):
    'CampaignServer simple_channels delayed'
    tlog(10, "Calls: %s" % self.calls.queue)
    self.checkLastCall('CampaignServer_chsv_simple_channels', timeout = 2)
    checkData = self.channelServerObj.check(CheckQuery(1, "1", time2orb(), [], False))
    self.assertNotEqual(self.checkTimeStamp, orb2time(checkData.master_stamp),
                     'CheckData.master_stamp')
    self.checkTimeStamp = orb2time(checkData.master_stamp)
    self.assertEqual(False, checkData.special_track, 'CheckData.special_track')
    self.assertEqual(False, checkData.special_adv, 'CheckData.special_adv')
    result = self.channelServerObj.update_triggers([13, 14])
    self.assertEqual(0, len(result.channels), 'channel size')
    self.simpleChannelWrapper.proceed()
    tlog(10, "Calls: %s" % self.calls.queue)

  def testDelayedCheck( self ):
    'ChannelProxy check delayed'
    self.checkWrapper.wait()
    checkData = self.channelServerObj.check(CheckQuery(1, "1", time2orb(self.checkTimeStamp), [], False))
    self.assertEqual(self.checkTimeStamp, orb2time(checkData.master_stamp),
                     'CheckData.master_stamp')
    self.assertEqual(False, checkData.special_track, 'CheckData.special_track')
    self.assertEqual(False, checkData.special_adv, 'CheckData.special_adv')
    result = self.channelServerObj.update_triggers([13, 14])
    self.assertEqual(0, len(result.channels), 'channel size')
    self.checkWrapper.proceed()
    self.checkCallSequence(expCalls = [CallSequence('CampaignServer_chsv_simple_channels', 19),
                                       'ChannelProxy_check',
                                       'ChannelProxy_update_all_ccg'])
    tlog(10, "Calls: %s" % self.calls.queue) 

  def testDelayedUpdate( self ):
    'ChannelProxy update delayed'
    self.updateTriggersWrapper.wait()
    checkData = self.channelServerObj.check(CheckQuery(1, "1", time2orb(self.checkTimeStamp), [], False))
    self.assertEqual(self.checkTimeStamp, orb2time(checkData.master_stamp))
    result = self.channelServerObj.update_triggers([13, 14])
    self.assertEqual(0, len(result.channels), 'channel size')
    self.updateTriggersWrapper.proceed()
    self.checkCallSequence(expCalls = ['ChannelProxy_update_triggers'],
                           timeout = 10)

  def test1stTriggerActivation( self ):
    '1st trigger activation'
    self.updateTriggersWrapper.wait()
    checkData = self.channelServerObj.check(CheckQuery(1, "1", time2orb(self.checkTimeStamp), [], False))
    self.assertEqual(self.checkTimeStamp, orb2time(checkData.master_stamp))
    result = self.channelServerObj.update_triggers([13, 14])
    tlog(10, "ChannelServer.update_triggers([13,14]): %s" % result)
    self.assertEqual(1, len(result.channels), 'channel size')
    self.assertEqual(13, result.channels[0].channel_id, 'channel#1.id')
    self.assertNotEqual(None, orb2time(result.channels[0].stamp), 'channel#1.stamp')
    self.assertNotEqual(1, len(result.channels[0].words), 'channel#1.triggers')
    self.updateTriggersWrapper.proceed()
    self.checkCallSequence(expCalls = ['ChannelProxy_update_triggers'],
                           timeout = 10)

  def test2ndTriggerActivation( self ):
    '2nd trigger activation'
    self.checkCallSequence(expCalls = [CallSequence('CampaignServer_chsv_simple_channels', 20)],
                           timeout = 10)
    checkData = self.channelServerObj.check(CheckQuery(1, "1", time2orb(self.checkTimeStamp), [], False))
    self.assertNotEqual(self.checkTimeStamp, orb2time(checkData.master_stamp))
    self.channelServerObj.update_triggers([13, 14])
    result = self.channelServerObj.update_triggers([13, 14])
    tlog(10, "ChannelServer.update_triggers([13,14]): %s" % result)
    self.assertEqual(2, len(result.channels), 'channel size')
    self.assertEqual(13, result.channels[0].channel_id, 'channel#1.id')
    self.assertEqual(14, result.channels[1].channel_id, 'channel#2.id')
    self.assertNotEqual(None, orb2time(result.channels[0].stamp), 'channel#1.stamp')
    self.assertNotEqual(None, orb2time(result.channels[1].stamp), 'channel#2.stamp')
    self.assertNotEqual(1, len(result.channels[0].words), 'channel#1.triggers')
    self.assertNotEqual(1, len(result.channels[1].words), 'channel#2.triggers')

if __name__ == '__main__':
 main()





  


