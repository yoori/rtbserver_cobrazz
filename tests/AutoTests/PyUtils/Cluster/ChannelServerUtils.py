
def createChannelTriggerWord(type, word, negative=False):
  type_value = {'P': '\x04', 'S':  '\x06', 'U': '\x02' }.get(type,'\x00');
  return type_value + '\x00' + word  + '\x00';

