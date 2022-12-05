
from CORBATestObj import CORBATestObj

from MockCampaignServer import CampaignServerTestMixin
from MockChannelProxy import ChannelProxyTestMixin

import AdServer__POA.ChannelSvcs

ObjectKey = 'ChannelManagerController'

class ChannelControllerObj(CORBATestObj):

  def __init__( self, test):
    CORBATestObj.__init__( self, ObjectKey,
                           AdServer__POA.ChannelSvcs.ChannelManagerController,
                           test)

class ChannelManagerControllerTestMixin:

  def setUp( self ):
    self.activateChannelController()

  def activateChannelController( self ):
    self.bindObject(ObjectKey, ChannelControllerObj(self))

  def ChannelManagerController_get_load_session( self ):
    raise AdServer__POA.ChannelSvcs.ChannelManagerController.ImplementationException("not implement yet")

  def ChannelManagerController_get_channel_session( self ):
    raise AdServer__POA.ChannelSvcs.ChannelManagerController.ImplementationException("not implement yet")

   
