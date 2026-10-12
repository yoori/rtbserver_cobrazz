#!/usr/bin/env python

from CORBATest import *
from FunTest import tlog
from OrbTestSuite import main

from SingleThread import SingleThread
from SyncObj import ActiveSync
from Condition import Condition


import os.path, threading, time
import CORBACommons

class ClusterTest(CORBAFunTest):
  'Cluster'

  def setUp( self ):
    self.LOGLEVEL = 100
    self.setUpServers(CampaignServer, #
                        CampaignManager, #
                        UserInfoManager, #
                        UserInfoManagerController, #
                        ChannelServer, #
                        ChannelController, #
                        ExpressionMatcher, #
                        RequestInfoManager, #
                        LogGeneralizer, #
                        SyncLogs
                        ) #
    self.startProc()

  def tearDown( self ):
    self.tearDownServers()


  def run( self ):
    res = self.test(1, self.testRun)

  def testRun( self ):
    'simple run test'
    self.assert_(self.CampaignServer.is_alive(), "CampaignServer must start")
    obj1 = self.CampaignServer.getObject("ProcessControl", CORBACommons.IProcessControl)
    self.assert_(obj1.is_alive(), "CampaignServer must start")
    #
    self.assert_(self.CampaignManager.is_alive(), "CampaignManager must start")
    obj2 = self.CampaignManager.getObject("ProcessControl", CORBACommons.IProcessControl)
    self.assert_(obj2.is_alive(), "CampaignManager must start")
    #
    self.assert_(self.ChannelServer.is_alive(), "ChannelServer must start")
    self.ChannelServer.grpc('check_configuration', {})
    #
    self.assert_(self.ChannelController.is_alive(), "ChannelController must start")
    #
    self.assert_(self.UserInfoManager.is_alive(), "UserInfoManager must start")
    obj6 = self.UserInfoManager.getObject("ProcessControl", CORBACommons.IProcessControl)
    self.assert_(obj6.is_alive(), "UserInfoManager must start")
    #
    self.assert_(self.UserInfoManagerController.is_alive(), "UserInfoManagerController must start")
    obj7 = self.UserInfoManagerController.getObject("ProcessControl", CORBACommons.IProcessControl)
    self.assert_(obj7.is_alive(), "UserInfoManagerController must start")
    #
    self.assert_(self.ExpressionMatcher.is_alive(), "ExpressionMatcher must start")
    obj8 = self.ExpressionMatcher.getObject("ProcessControl", CORBACommons.IProcessControl)
    self.assert_(obj8.is_alive(), "ExpressionMatcher must start")
    #
    self.assert_(self.RequestInfoManager.is_alive(), "RequestInfoManager must start")
    obj9 = self.RequestInfoManager.getObject("ProcessControl", CORBACommons.IProcessControl)
    self.assert_(obj9.is_alive(), "RequestInfoManager must start")
    #
    #
    self.assert_(self.LogGeneralizer.is_alive(), "LogGeneralizer must start")
    #
    self.assert_(self.SyncLogs.is_alive(), "SyncLogs must start")
    #

if __name__ == '__main__':
 main()
