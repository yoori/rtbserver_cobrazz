
from AdServer.ChannelSvcs import ChannelServerControl
from  CORBACommons import *

def createProxySource( channel_proxy_ior,
                       campaign_srv_iors=[],
                       count_chunks=0,
                       colo=1,
                       check_sum=43,
                       version="1.2.3" ):
  def ior2CorbaObjectRefDef(ior):
    return CorbaObjectRefDef(ior,
                             ConnectionDef(CT_NON_SECURE, SecureConnectionDef("", "", "", "")))
  return ChannelServerControl.ProxySourceInfo([], #local descriptor
                                              [ior2CorbaObjectRefDef(channel_proxy_ior)],
                                              map(ior2CorbaObjectRefDef, campaign_srv_iors),
                                              count_chunks, colo, check_sum, 
                                              version)
  
def createChannelTriggerWord(type, word, negative=False):
  type_value = {'P': '\x04', 'S':  '\x06', 'U': '\x02' }.get(type,'\x00');
  return type_value + '\x00' + word  + '\x00';
    

