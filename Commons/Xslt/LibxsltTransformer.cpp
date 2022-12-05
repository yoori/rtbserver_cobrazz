// @file Xslt/LibxsltTransformer.cpp

#include <fstream>

#include <libxslt/transform.h>
#include <ReferenceCounting/DefaultImpl.hpp>

#include "XslTransformer.hpp"
#include "LibxsltTransformer.hpp"
#include "LibxsltExFunctions.hpp"

namespace AdServer
{

  XslFunctionRegistrar::Mutex XslFunctionRegistrar::mutex;

  /**
   * This class will handle XML and XSLT parsing errors events. We cannot raise
   * exceptions while libxslt processing error callbacks, this
   * action leads to memory leaks in C code. And decision is storing
   * errors descriptions, check state after completion of libxslt code.
   * Class created because xsltSetGenericErrorFunc doesn't support threads
   * (xmlSetGenericErrorFunc is thread-safe, if libxml is compiled
   * with LIBXML_THREAD_ENABLED)
   */
  class ErrorListener
  {
  public:
    /**
     * Called back while error events
     * @param message Trouble description should be stored into
     *   listener
     */
    void
    event_error(const char* message, int n) /*throw(eh::Exception)*/;

    /**
     * @param result Here will be returned the description of last troubles.
     * @return True if result not empty.
     * Clear stored state before return
     */
    const char*
    get_last_error() noexcept;

    /**
     * xsltGenericErrorDefaultFunc:
     * @param context   an error context
     * @param message   the message to display/transmit
     * @param ...       extra parameters for the message display
     *
     * Default handler for out of context error messages.
     */
    static void
    xslt_generic_error(void* context, const char* message, ...) noexcept;

    /**
     * Remove error place for current calling thread from global
     * container
     */
    struct ThreadGuard : Generics::Uncopyable
    {
      /**
       * Construct errors place for calling thread
       */
      ThreadGuard() /*throw(eh::Exception)*/;
      /**
       * Remove errors place for calling thread from caching container
       */
      ~ThreadGuard() noexcept;
    };

  private:

    void
    clear_thread_last_error_() noexcept;

    struct String : public std::string,
      ReferenceCounting::DefaultImpl<>
    {
    private:
      virtual
      ~String() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<String> String_var;

    /// Contain troubles description, empty if don't exist any thread,
    /// working with XslTransformer at current moment
    typedef std::map<pthread_t, String_var> ThreadsErrors;
    ThreadsErrors threads_last_errors_;

    typedef Sync::PosixRWLock Lock;
    typedef Sync::PosixRGuard RGuard;
    typedef Sync::PosixWGuard WGuard;
    Lock lock_;
  };

  namespace
  {
    /// Here accumulate errors descriptions from libxslt calls
    /// Access point to thread specific errors description
    ErrorListener error_listener;

    struct LibxsltTransformContextDestroyer
    {
      void
      operator()(const xsltTransformContextPtr context) noexcept;
    };

    typedef std::unique_ptr<
      xsltTransformContext,
      LibxsltTransformContextDestroyer> XsltTransformContextPtr;

    struct LibxsltXmlDocDestroyer
    {
      void
      operator ()(const xmlDocPtr doc) noexcept
      {
        if(doc)
        {
          doc->URL = 0;
          xmlFreeDoc(doc);
        }
      }
    };

    typedef std::unique_ptr<
      xmlDoc,
      LibxsltXmlDocDestroyer> XmlDocPtr;

    struct LibxsltXmlOutputBufferDestroyer
    {
      void
      operator ()(const xmlOutputBufferPtr buffer) noexcept
      {
        xmlOutputBufferClose(buffer);
      }
    };

    typedef std::unique_ptr<
      xmlOutputBuffer,
      LibxsltXmlOutputBufferDestroyer> XmlOutputBufferPtr;
  }

  //
  // LibxslTransformer::ErrorListener class
  //

  void
  ErrorListener::event_error(const char* message, int n) /*throw(eh::Exception)*/
  {
    pthread_t tid = pthread_self();
    std::string* all_msg;

    {
      WGuard guard(lock_);
      String_var& str = threads_last_errors_[tid];
      if (!str)
      {
        str = new String;
      }
      all_msg = str;
    }
    all_msg->append(message, n);
    return;
  }

  void
  ErrorListener::xslt_generic_error(
    void* /*context*/,
    const char* message,
    ...)
    noexcept
  {
    char* text = 0;
    va_list args;
    va_start(args, message);
    int n = vasprintf(&text, message, args);
    va_end(args);

    try
    {
      if (n != -1)
      {
        error_listener.event_error(text, n);
        free(text);
      }
      else
      {
        static const char MSG[] = "XSLT error, cannot form description";
        error_listener.event_error(MSG, sizeof(MSG));
      }
    }
    catch (eh::Exception&)
    {}
  }

  const char*
  ErrorListener::get_last_error() noexcept
  {
    pthread_t tid = pthread_self();
    const char* msg = 0;

    {
      RGuard guard(lock_);
      ThreadsErrors::iterator it = threads_last_errors_.find(tid);
      if (it != threads_last_errors_.end())
      {
        msg = it->second->c_str();
      }
    }
    return msg;
  }

  /**
   * Remove errors place for calling thread from caching container
   */
  void
  ErrorListener::clear_thread_last_error_() noexcept
  {
    if (get_last_error())
    {
      pthread_t tid = pthread_self();
      WGuard guard(lock_);
      threads_last_errors_.erase(tid);
    }
  }

  ErrorListener::ThreadGuard::ThreadGuard() /*throw(eh::Exception)*/
  {
    xmlSetGenericErrorFunc(0, ErrorListener::xslt_generic_error);
  }

  ErrorListener::ThreadGuard::~ThreadGuard() noexcept
  {
    error_listener.clear_thread_last_error_();
  }

  //
  // LibxsltHolder
  //
  LibxsltHolder::Mutex LibxsltHolder::lock_;
  LibxsltHolder_var LibxsltHolder::holder_;

  LibxsltHolder::LibxsltHolder() noexcept
  {
    initialize_i_();
  }

  LibxsltHolder::~LibxsltHolder() noexcept
  {
    deinitialize_();
  }

  void
  LibxsltHolder::register_external_fun(
    const char* fun_namespace,
    const char* fun_name,
    XslFunction* fun)
    /*throw(XslTransformerBase::Exception)*/
  {
    static const char* FUN = "LibxslTransformer::register_external_fun()";

    try
    {
      std::string full_name = fun_namespace;
      full_name += ":";
      full_name += fun_name;

      Guard lock(ext_functions_lock_);

      XslFunctionMap::const_iterator fun_it = ext_functions_.find(full_name);
      if(fun_it == ext_functions_.end())
      {
        fun->registrate(fun_namespace, fun_name);
        ext_functions_.insert(std::make_pair(
          full_name,
          ReferenceCounting::add_ref(fun)));
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw XslTransformerBase::Exception(ostr);
    }
  }

  LibxsltHolder*
  LibxsltHolder::instance() /*throw(eh::Exception)*/
  {
    Guard lock(lock_);

    if (!holder_.in())
    {
      holder_ = new LibxsltHolder;
    }

    return ReferenceCounting::add_ref(holder_);
  }

  void
  LibxsltHolder::initialize_i_() noexcept
  {
    xmlInitParser();
    // return the last value for substitution.
    // 0 for no substitution, 1 for substitution
    xmlSubstituteEntitiesDefault(1); // noexcept
    // global variable
    xmlLoadExtDtdDefaultValue = 1;

    xsltSetGenericErrorFunc(0, ErrorListener::xslt_generic_error);
  }

  void
  LibxsltHolder::deinitialize_() noexcept
  {
    Guard lock(lock_);

    xsltSetGenericErrorFunc(0, 0);
    xsltCleanupGlobals();
    initGenericErrorDefaultFunc(0);
    xmlCleanupParser();
  }

  //
  // LibxslTransformer
  //

  LibxslTransformer::LibxslTransformer() noexcept
  {
  }

  LibxslTransformer::LibxslTransformer(std::istream& xslt_istr,
    const char* base_path)
    /*throw(Exception)*/
    : base_path_(base_path)
  {
    init_(xslt_istr);
  }

  LibxslTransformer::LibxslTransformer(const char* xsl_file,
    const char* base_path)
    /*throw(FileNotExists, Exception)*/
  {
    open(xsl_file, base_path);
  }

  LibxslTransformer::~LibxslTransformer() noexcept
  {
  }

  void
  LibxslTransformer::open(const char* xsl_file, const char* base_path)
    /*throw(FileNotExists, Exception)*/
  {
    static const char* FUN = "LibxslTransformer::open()";
    std::ifstream fstr(xsl_file, std::ios::in);
    if (!fstr.is_open())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open file '" << xsl_file << "'.";
      throw FileNotExists(ostr);
    }
    base_path_ = base_path ? base_path : xsl_file;

    init_(fstr);
  }

  /**
   * Load xml based documents into xmlDocPtr
   * @param istr The stream on XML or XSLT file
   */
  LibxslTransformer::ParserContextPtr
  LibxslTransformer::parse_xml_stream_(
    std::istream& istr)
    /*throw(Exception)*/
  {
    static const char* FUN = "LibxslTransformer::parse_xml_stream_()";

    char encoding_detection_chunk[4];
    istr.read(encoding_detection_chunk, sizeof(encoding_detection_chunk));
    if (!istr)
    {
      Stream::Error ostr;
      ostr << FUN << ": failed to read XML data.";
      throw Exception(ostr);
    }

    /// Auto pointer to standard libxslt context
    // Parser context using one-time!
    ParserContextPtr parser_context_ptr(
      xmlCreatePushParserCtxt(0, 0,
        encoding_detection_chunk,
        sizeof(encoding_detection_chunk), 0));

    // set options used in xsltproc tool (XML_PARSE_NOCDATA in particular)
    xmlCtxtUseOptions(
      parser_context_ptr.get(),
      XSLT_PARSE_OPTIONS);

    // For some reason this seems to completely break if node names
    // are interned.
    parser_context_ptr->dictNames = 0;

    const std::size_t XML_PARSER_CHUNK_SIZE = 4096;
    char buffer[XML_PARSER_CHUNK_SIZE];

    if (!istr || istr.eof() ||
      istr.peek() == std::istream::traits_type::eof())
    {
      Stream::Error ostr;
      ostr << FUN << ": cannot read xml file";
      throw Exception(ostr);
    }

    int ret_val;
    while (istr.read(buffer, XML_PARSER_CHUNK_SIZE) || istr.gcount())
    {
      buffer[istr.gcount()] = 0;
      ret_val = xmlParseChunk(parser_context_ptr.get(),
        buffer, istr.gcount(), 0);
      if (ret_val != XML_ERR_OK)
      {
        xmlFreeDoc(parser_context_ptr->myDoc);
        Stream::Error ostr;
        ostr << FUN <<
          ": fail parsing xml file, parser error code=" <<
          ret_val;
        if (const char* error = error_listener.get_last_error())
        {
          ostr << ", parser last error=" << error;
        }
        throw Exception(ostr);
      }
    }

    // Finalize parsing
    ret_val = xmlParseChunk(parser_context_ptr.get(), NULL, 0, 1);

    if (ret_val != XML_ERR_OK ||
        !parser_context_ptr->myDoc ||
        (!istr && !istr.eof()))
    {
      xmlFreeDoc(parser_context_ptr->myDoc);
      Stream::Error ostr;
      ostr << FUN << ": Failed parsing XML source";
      if (ret_val != XML_ERR_OK)
      {
        ostr << ", error code=" << ret_val;
      }
      if (!parser_context_ptr->myDoc)
      {
        ostr << ", document is not created";
      }
      if (const char* error = error_listener.get_last_error())
      {
        ostr << ", parser last error=" << error;
      }
      throw Exception(ostr);
    }

    // Suppose that parser_context_ptr->myDoc->URL identically equal 0,
    // because parser processed on memory buffer without URL's
    // And we can simply set value to URL, and clear pointer,
    // before xmlDocFree call.
    if (!base_path_.empty())
    {
      parser_context_ptr->myDoc->URL =
        reinterpret_cast<const xmlChar*>(base_path_.c_str());
    }

    return parser_context_ptr;
  }

  void
  LibxslTransformer::init_(std::istream& xslt_istr)
    /*throw(Exception)*/
  {
    static const char* FUN = "LibxslTransformer::init_()";

    try
    {
      libxslt_holder_ = LibxsltHolder::instance();
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw Exception(ostr);
    }

    ErrorListener::ThreadGuard thread_cleaner;

    // Parse stream and put context in smart pointer

    ParserContextPtr xslt_parser_context(
      parse_xml_stream_(xslt_istr));
    // make available compiled stylesheet for object lifetime
    // parser context will be freed here by auto pointer.
    stylesheet_.reset(
        xsltParseStylesheetDoc(xslt_parser_context->myDoc));

    const char* error = error_listener.get_last_error();
    if (error || !stylesheet_.get())
    {
      // If stylesheet_ has been created, the xml document
      // is freed by smart pointer, but in this case
      // we should release document explicitly
      if (!stylesheet_.get())
      {
        xslt_parser_context->myDoc->URL = 0;
        xmlFreeDoc(xslt_parser_context->myDoc);
      }
      Stream::Error ostr;
      ostr << FUN << ": fail parsing XSL stylesheet";
      if (error)
      {
        ostr << ", parser last error=" << error;
      }
      throw Exception(ostr);
    }

    // And now unique_ptr will free parser context, but compiled
    // stylesheets alive
  }

  /**
   * xmlOutputWriteCallback:
   * @context:  an Output context
   * @buffer:  the buffer of data to write
   * @len:  the length of the buffer in bytes
   *
   * Callback used in the I/O Output API to write to the resource
   *
   * Returns the number of bytes written or -1 in case of error
   */
  int
  transform_result_write_callback(void* context,
    const char* buffer,
    int len) noexcept
  {
    std::ostream& output = *static_cast<std::ostream*>(context);
    output.write(buffer, len);
    return len;
  }

  /**
   * xmlOutputCloseCallback:
   * @context:  an Output context
   *
   * Callback used in the I/O Output API to close the resource
   *
   * Returns 0 or -1 in case of error
   */
  int
  transform_result_write_close_callback(void* /*context*/) noexcept
  {
//    std::ostream& output = *static_cast<std::ostream*>(context);
//    output << std::flush;
    return 0;
  }

  void
  LibxslTransformer::transform(
    std::istream& input,
    std::ostream& output,
    const XslParameters* parameters) /*throw(Exception)*/
  {
    static const char* FUN = "LibxslTransformer::transform()";

    if (!libxslt_holder_.in())
    {
      Stream::Error ostr;
      ostr << FUN << ": LibxslTransformer isn't inited.";
      throw Exception(ostr);
    }

    ErrorListener::ThreadGuard thread_cleaner;
    ParserContextPtr xml_parser_context(parse_xml_stream_(input));
    XmlDocPtr xml_guard(xml_parser_context->myDoc);

    Generics::ArrayAutoPtr<const char*> params;
    // fill params
    if (parameters)
    {
      const XslParameters& params_container = *parameters;
      params.reset(params_container.size() * 2 + 1);
      std::size_t i = 0;
      for (XslParameters::const_iterator cit = params_container.begin();
        cit != params_container.end(); ++cit)
      {
        params[i++] = cit->first.c_str();
        params[i++] = cit->second.c_str();
      }
      params[i] = 0;
    }
    else
    {
      params.reset(1);
      params[0] = 0;
    }

    XmlDocPtr result;
    {
      XsltTransformContextPtr ctxt(
        xsltNewTransformContext(stylesheet_.get(), xml_parser_context->myDoc));
      if (!ctxt.get())
      {
        Stream::Error ostr;
        ostr << FUN << ": failed to create XSL transformation context";
        throw Exception(ostr);
      }
      static const int OPTIONS = XSLT_PARSE_OPTIONS;
      xsltSetCtxtParseOptions(ctxt.get(), OPTIONS);

      result.reset(xsltApplyStylesheetUser(stylesheet_.get(),
        xml_parser_context->myDoc, params.get(), 0, 0, ctxt.get()));

      if (ctxt->state == XSLT_STATE_ERROR ||
        ctxt->state == XSLT_STATE_STOPPED || !result.get())
      {
        Stream::Error ostr;
        ostr << FUN << ": XSL transformation failed";
        if (const char* error = error_listener.get_last_error())
        {
          ostr << ", XSLT parser last error=" << error;
        }
        if (ctxt->state == XSLT_STATE_STOPPED)
        {
          ostr << " XSLT transformation was stopped";
        }
        throw Exception(ostr);
      }
    }

    xmlCharEncodingHandlerPtr handler(stylesheet_->encoding ?
      xmlFindCharEncodingHandler(
        reinterpret_cast<char*>(stylesheet_->encoding)) : 0);

    // To avoid double copying we will write result transformation
    // through callbacks directly to stream, without useless temporary
    // buffer or file
    XmlOutputBufferPtr buf(
      xmlOutputBufferCreateIO(transform_result_write_callback,
      transform_result_write_close_callback,
      &output,
      handler));

    const char* error = error_listener.get_last_error();
    if (error || !buf.get())
    {
      Stream::Error ostr;
      ostr << FUN << ": cannot create buffer for transformation results";
      if (error)
      {
        ostr << ", XSLT last error=" << error;
      }
      throw Exception(ostr);
    }

    if (xsltSaveResultTo(buf.get(), result.get(), stylesheet_.get()) == -1)
    {
      Stream::Error ostr;
      ostr << FUN << ": writing the XSL transformation results failed";
      error = error_listener.get_last_error();
      if (error)
      {
        ostr << ", XSLT last error=" << error;
      }
      throw Exception(ostr);
    }
  }

  void
  LibxslTransformer::register_external_fun(
    const char* fun_namespace,
    const char* fun_name,
    XslFunction* fun) /*throw(Exception)*/
  {
    libxslt_holder_->register_external_fun(
      fun_namespace,
      fun_name,
      fun);
  }

  inline void
  LibxsltTransformContextDestroyer::operator ()(
    const xsltTransformContextPtr context)
    noexcept
  {
    xsltFreeTransformContext(context);
  }

} // namespace AdServer
