#!/usr/bin/env python

import time
import base64
from CORBATest import *
from FunTest import tlog
from Util import currentTime, time2str
from OrbUtil import time2orb, orb2time
from OrbTestSuite import main
from MockCampaignServer import CampaignServerTestMixin, SimpleChannelKey, \
     BehavParameter, BehavParamInfo
from MockChannelProxy import ChannelProxyTestMixin, TriggerVersion, ChannelById, TriggerInfo
from SingleThread import SingleThread
import AdServer.ChannelSvcs, ChannelServerUtils


class ChannelSvcs(CORBAFunTest, CampaignServerTestMixin, ChannelProxyTestMixin):
  'ChannelSvcs'

  def setUp( self ):
    self.LOGLEVEL = 100
    self.CAMPAIGNSRV_PORT  = self.orbPort
    self.CHANNELPROXY_PORT = self.orbPort
    self.CHANNELPROXY_USED = True
    CampaignServerTestMixin.setUp( self )
    ChannelProxyTestMixin.setUp( self )
    self.__prepareConfig()
    self.setUpServers(ChannelServer)
    self.startProc()
    self.ChannelServer.configure_proxy()

  def tearDown( self ):
    self.tearDownServers()

  def __prepareConfig( self ):
    self.simple_channels = [SimpleChannelKey(
                                             10,                       # channel_id
                                            "GN",                      # country_code
                                             "",                       # language
                                             "A",                      # status
                                             11,                       # behav_param_list_id
                                             "",                       # str_behav_param_list_id
                                             [],                       # categories
                                             0,                        # threshold
                                             0,                        # discover
                                             [1],                      # page_triggers
                                             [],                       # search_triggers
                                             [],                       # url_triggers
                                             [],                       # url_keyword_triggers
                                             time2orb(currentTime())   # timestamp
                                             ),
                            SimpleChannelKey(
                                             12,                       # channel_id
                                             "GN",                     # country_code
                                             "",                       # language
                                             "A",                      # status
                                             11,                       # behav_param_list_id
                                             "",                       # str_behav_param_list_id
                                             [],                       # categories
                                             0,                        # threshold
                                             0,                        # discover
                                             [1],                      # page_triggers
                                             [],                       # search_triggers
                                             [],                       # url_triggers
                                             [],                       # url_keyword_triggers
                                             time2orb(currentTime())   # timestamp
                                             )]
    self.behav_params = [BehavParamInfo(11, 0, time2orb(currentTime()),
                                        [BehavParameter(1, 0, 0, 1, "P")])]
    self.channelById = [ChannelById(10,                    # channel_id
                                    # hard_words
                                    [TriggerInfo(1, ChannelServerUtils.createChannelTriggerWord('P', "word"))],
                                    time2orb(currentTime()) # stamp
                                    ),
                        ChannelById(12,                    # channel_id
                                    # hard_words
                                    [TriggerInfo(1, ChannelServerUtils.createChannelTriggerWord('P', "word"))],
                                    time2orb(currentTime()) # stamp
                                    )]

  def __checkMatchingResult(self, matchResult,
                            page_channels=[],
                            search_channels=[],
                            url_channels=[],
                            content_channels=[]):
    matched = matchResult.get('matched_channels', {})
    gotPageChannels = [int(c['id']) for c in matched.get('page_channels', [])]
    gotSearchChannels = [int(c['id']) for c in matched.get('search_channels', [])]
    gotUrlChannels = [int(c['id']) for c in matched.get('url_channels', [])]
    gotContentChannels = [int(c['id']) for c in matchResult.get('content_channels', [])]
    tlog(10, "Got page channels: %s" % gotPageChannels)
    tlog(10, "Got search channels: %s" % gotSearchChannels)
    tlog(10, "Got url channels: %s" % gotUrlChannels)
    tlog(10, "Got content channels: %s" % gotContentChannels)
    self.compareUnorderedList(page_channels, gotPageChannels, 'page channels')
    self.compareUnorderedList(search_channels, gotSearchChannels, 'search channels')
    self.compareUnorderedList(url_channels, gotUrlChannels, 'url channels')
    self.compareUnorderedList(content_channels, gotContentChannels, 'content channels')


  def run( self ):
    res = self.test(1, self.testConnect)
    res = self.test(1, self.testMatch, res)

  def testConnect( self ):
    "connect"
    self.checkCallSequence(expCalls = [CallSequence('CampaignServer_chsv_simple_channels', 20),
                                       'ChannelProxy_check',
                                       'ChannelProxy_update_all_ccg',
                                       'ChannelProxy_update_triggers'])
  def testMatch( self ):
    "match"
    query = {'request_id': 'req_id', 'pwords': base64.b64encode('word'),
             'statuses': base64.b64encode('AD'), 'fill_content': True}
    time.sleep(2)
    self.__checkMatchingResult(self.ChannelServer.grpc('match', query),
                               page_channels=[10, 12],
                               content_channels=[10, 12])

if __name__ == '__main__':
 main()


