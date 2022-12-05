
from Util import currentTime, time2str
from OrbUtil import time2orb, orb2time
from FunTest import tlog
from AdServer.ChannelSvcs import ChannelUpdateBase_v33, NotConfigured
from TestComparison import ComparisonMixin
from CORBATestObj import CORBATestObj

ChannelUpdateCurrent = ChannelUpdateBase_v33

# Aliases for some types
ChannelVersion = ChannelUpdateCurrent.ChannelVersion
TriggerVersion = ChannelUpdateCurrent.TriggerVersion
CheckData      = ChannelUpdateCurrent.CheckData
CCGResult      = ChannelUpdateCurrent.PosCCGResult
PosCCGResult   = ChannelUpdateCurrent.PosCCGResult
ChannelProxy   = ChannelUpdateCurrent
CheckQuery     = ChannelUpdateCurrent.CheckQuery
ChannelById    = ChannelUpdateCurrent.ChannelById
UpdateData     = ChannelUpdateCurrent.UpdateData
TriggerInfo    = ChannelUpdateCurrent.TriggerInfo


import AdServer__POA.ChannelSvcs

ObjectKey = 'ChannelProxy_v33'

class ChannelProxyObj(CORBATestObj):

  def __init__( self, test):
    CORBATestObj.__init__( self, 'ChannelProxy',
                           AdServer__POA.ChannelSvcs.ChannelProxy_v33,
                           test)

class ChannelProxyTestMixin(ComparisonMixin):

  SourceID = 1

  def setUp( self ):
    self.specialTrack = False
    self.specialAdv = False
    self.channelById = []
    self.activateChannelProxy()
    self.first_stamp = currentTime()

  def activateChannelProxy( self ):
    channelProxy = ChannelProxyObj(self)
    self.MockChannelProxyIOR = self.bindObject(ObjectKey, channelProxy)
    tlog(10, "ChannelProxy '%s' activated" % ObjectKey)

  def getChannelProxy( self ):
    return self.getObject( ObjectKey, AdServer__POA.ChannelSvcs.ChannelProxy_v33)

  def ChannelProxy_check(self, query):
    tlog(10, "ChannelProxy.check(colo=%d, version='%s', timestamp='%s', new_ids=%s)" % \
         (query.colo_id,
          query.version,
          time2str(orb2time(query.master_stamp)),
          query.new_ids))
    def channel2version(channel):
      return ChannelVersion(
        channel.channel_id,
        reduce(lambda x, y: x+y, map(lambda x: len(x.trigger), channel.words), 0) - \
          (len(channel.words) * 3),
        channel.stamp)
    return CheckData(
      time2orb(self.first_stamp), # first_stamp
      time2orb(currentTime()),  # master_stamp
      # versions (trigger versions sequence)
      map(channel2version, self.channelById),
      self.specialTrack,        # special_track
      self.specialAdv,          # special_adv
      self.SourceID,            # source_id
      time2orb(currentTime())   # max_time
      )

  def ChannelProxy_update_triggers(self, ids):
    tlog(10, "ChannelProxy.update_triggers(ids=%s)" % ids)
    def channelFilter(channel):
      return channel.channel_id in ids
    channels = filter(channelFilter, self.channelById)
    tlog(10, "ChannelProxy.update_triggers(source_id=%d, channels=%s)" % \
         (self.SourceID, channels))
    return UpdateData(self.SourceID, channels)

    
  def ChannelProxy_update_all_ccg(self, query):
    tlog(10, "ChannelProxy.update_all_ccg(timestamp='%s', start=%d, limit=%d)" % \
         (time2str(orb2time(query.master_stamp)),
          query.start, query.limit))
    return PosCCGResult(query.start, [], [], 1)



