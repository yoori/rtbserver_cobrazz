
#ifndef _COMMONS_ERROR_HANDLER_HPP_
#define _COMMONS_ERROR_HANDLER_HPP_

#include <eh/Exception.hpp>
#include <xsd/cxx/xml/dom/auto-ptr.hxx>
#include <xsd/cxx/xml/error-handler.hxx>
#include <xsd/cxx/tree/parsing.hxx>

namespace Config
{
  class ErrorHandler : public virtual ::xsd::cxx::xml::error_handler< char >
  {
  public:
    ErrorHandler() /*throw(eh::Exception)*/;
    ~ErrorHandler() noexcept;
    
    virtual bool handle (
      std::string const& id,
      unsigned long line,
      unsigned long column,
      severity s,
      std::string const& message);
    
    bool has_errors() const noexcept;

    const char* text(std::string& txt) const /*throw(eh::Exception)*/;
    
  private:
    std::ostringstream errors_;
    bool               has_errors_;
  };
  
  inline
  ErrorHandler::ErrorHandler() /*throw(eh::Exception)*/
    : has_errors_(false)
  {
  }
  
  inline
  ErrorHandler::~ErrorHandler() noexcept
  {
  }

  inline  
  bool
  ErrorHandler::handle(
    std::string const& id,
    unsigned long line,
    unsigned long column,
    severity s,
    std::string const& message)
  {
    if (s == severity::error || s == severity::fatal)
    {
      has_errors_ = true;

      errors_
        << "File: " << id << std::endl
        << "Line: " << line << ", Column: " << column
        << ", Severity: " << (s == severity::error ? "error" : "fatal")
        << std::endl
        << ": \"" << message << "\"" << std::endl << std::endl;
    }
    
    return true;          
  }

  inline
  bool
  ErrorHandler::has_errors() const noexcept
  {
    return has_errors_;
  }
  
  inline
  const char*
  ErrorHandler::text(std::string& txt) const /*throw(eh::Exception)*/
  {
    if (has_errors_)
    {        
      txt = errors_.str();
    }
      
    return txt.c_str();      
  }

  DECLARE_EXCEPTION(LoadError, eh::DescriptiveException);
  
  template<typename Ret>
  inline
  Ret 
  load_helper (Ret (*loader) (const ::std::string&,
			      ::xsd::cxx::xml::error_handler<char>&,
			      ::xsd::cxx::tree::flags,
			      const ::xsd::cxx::tree::properties< char >&), 
	       const char* file_name, 
	       ::xsd::cxx::tree::flags flags = 0,
	       const ::xsd::cxx::tree::properties< char >& properties 
	       = ::xsd::cxx::tree::properties< char > ())
  {
    ErrorHandler error_handler;
    try
    {
      Ret ret = loader (file_name, error_handler, flags, properties);
      if(error_handler.has_errors())
      {
	std::string error_string;
	throw LoadError(error_handler.text(error_string));
      }
      return ret;
    }
    catch(const ::xsd::cxx::tree::exception< char >& e)
    {
      if(error_handler.has_errors())
      {
	std::string error_string;
	throw LoadError(error_handler.text(error_string));
      }
      std::ostringstream error;
      error << e;
      throw LoadError(error.str());
    }
  }
}

#endif /* _COMMONS_ERROR_HANDLER_HPP_ */
