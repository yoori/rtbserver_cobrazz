#include <signal.h>
#include "UnitManager.hpp"

class SignalHandler
{
  DECLARE_EXCEPTION(SignalException, eh::DescriptiveException);

public:
  /**
   * Constructor.
   * @param application
   */
  SignalHandler();
  
  /**
   * Destructor.
   */
  ~SignalHandler();
  
  /**
   * Sets exit signal to true.
   * @param _ignored Not used, but required by prototype to match required handler.
   */
  static void exitSignalHandler(int _ignored);

  /**
   * Set up the signal handlers for UnitManager.
   */
  void
  setupSignalHandlers();

};

SignalHandler::SignalHandler()
{ }

SignalHandler::~SignalHandler()
{ }

void SignalHandler::exitSignalHandler(int /*_ignored*/)
{
  AutoTest::Shutdown::instance().set();
}

void
SignalHandler::setupSignalHandlers()
{
  if (signal((int) SIGINT, SignalHandler::exitSignalHandler) == SIG_ERR)
  {
    throw SignalException("Error setting up signal handlers!");
  }
}
  

int
main(int argc, const char* argv[])
{
  int result = 2;
  
  try
  {
    SignalHandler signalHandler;
    signalHandler.setupSignalHandlers();

    UnitManager manager;
    manager.run(argc, argv);

    result = manager.succeed() ? 0 : 1;
  }
  catch(const UnitManager::InvalidArgument& e)
  {
    std::cerr << e.what() << std::endl;
  }
  catch(const UnitManager::Exception& e)
  {
    std::cerr << "main: UnitManager::Exception exception caught. "
      ":" << std::endl << e.what() << std::endl;
  }
  catch(const eh::Exception& e)
  {
    std::cerr << "main: eh::Exception exception caught. "
      ":" << std::endl << e.what() << std::endl;    
  }

  return result;
}
