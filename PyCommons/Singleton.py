
class Singleton:

  instance = None
  
  def __init__( self ):
    assert not self.instance  # already instantiated
    self.setInstance(self)

  @classmethod
  def setInstance( cls, self ):
    cls.instance = self
