#include <xsd/ChannelSvcs/DictionaryProviderConfig.hpp>
#include <Commons/Algs.hpp>

#include "DictionaryContainer.hpp"
#include "DictionaryProviderImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  const char* DictionaryProviderImpl::ASPECT = " DictionaryProvider";
  const size_t DictionaryProviderImpl::MAX_LOAD_SIZE = 1024 * 128;

  DictionaryProviderImpl::DictionaryProviderImpl(
    Logging::Logger* logger,
    const DictionaryProviderConfig* config)
    /*throw(Exception, eh::Exception)*/
    : callback_(new Logging::ActiveObjectCallbackImpl(
        logger, "DictionaryProvider", ASPECT)),
      task_runner_(new Generics::TaskRunner(callback_, config->threads())),
      task_runned_(0)
  {
    static const char* FN= "DictionaryProviderImpl::DictionaryProviderImpl";
    try
    {
      add_child_object(task_runner_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FN << ": CompositeActiveObject::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
    try
    {
      for(xsd::AdServer::Configuration::DictionaryProviderConfigType::
          Dictionary_sequence::const_iterator it = config->Dictionary().begin();
          it != config->Dictionary().end(); ++it)
      {
        Task_var msg = new LoadTask(
          this, task_runner_, it->file().c_str(), it->lang().c_str());
        task_runner_->enqueue_task(msg);
        __gnu_cxx::__atomic_add(&task_runned_, 1);
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err <<  FN << ": Exception : " << e.what();
      throw Exception(err);
    }
  }

  void DictionaryProviderImpl::load_dictionary_(
    const char* lang,
    const char* file_name) noexcept
  {
    static const char* FN = "DictionaryProviderImpl::load_dictionary_";
    try
    {
      logger()->sstream(Logging::Logger::DEBUG, ASPECT)
        << FN << ": try to load file: '" << file_name << "'";
      std::ifstream ifs(file_name);
      if (ifs.fail())
      {
        logger()->sstream(Logging::Logger::ERROR, ASPECT, "ADS-IMPL-1303")
          << FN << ": can't open file: '" << file_name << "'";
      }
      else
      {
        cont_.load_dictionary(lang, ifs);
      }
    }
    catch(const eh::Exception& e)
    {
      logger()->sstream(Logging::Logger::ERROR, ASPECT, "ADS-IMPL-1301")
        << FN << ": eh::Exception: " << e.what();
    }
    try
    {
      __gnu_cxx::__atomic_add(&task_runned_, -1);
    }
    catch(const eh::Exception& e)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-1302")
        << FN << ": Exception: " << e.what();
    }
    if(logger()->log_level() >= Logging::Logger::DEBUG && task_runned_ == 0)
    {
      std::ostringstream ostr;
      cont_.get_statistic(ostr);
      logger()->log(ostr.str(), Logging::Logger::DEBUG, ASPECT);
    }
  }
  
  ::AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq*
  DictionaryProviderImpl::get_lexemes(
    const char* language, const ::CORBACommons::StringSeq& words)
  /*throw(ChannelSvcs::NotReady, ChannelSvcs::ImplementationException)*/
  {
    if(task_runned_)
    {
      Stream::Error err;
      err << "Not ready :" << task_runned_ << " tasks remained";
      CORBACommons::throw_desc<ChannelSvcs::NotReady>(err.str());
    }
    try
    {
      LexemeItemVector item_vector(words.length());
      for(size_t i = 0; i < item_vector.size(); i++)
      {
        item_vector[i].word = words[i];
      }
      cont_.find_lexemes(language, item_vector);
      AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq_var ret_value =
        new ::AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq;
      ret_value->length(item_vector.size());
      for (size_t i = 0; i < item_vector.size(); i++)
      {
        if (!item_vector[i].forms.empty())
        {
          (*ret_value)[i].forms.length( item_vector[i].forms.size() );
          size_t j = 0;
	  
          for(Forms::const_iterator it =
              item_vector[i].forms.begin();
              it != item_vector[i].forms.end(); ++it, j++)
          {
            (*ret_value)[i].forms[j] << *it;
          }
	  
          if(logger()->log_level() >= Logging::Logger::TRACE)
          {
            const Forms& forms = item_vector[i].forms;
            Algs::print(logger()->sstream(Logging::Logger::TRACE, ASPECT) << "Matched lexeme: ", forms.begin(), forms.end());
          }
        }
      }
      return ret_value._retn();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err << "ImplementationException: " << e.what();
      CORBACommons::throw_desc<ChannelSvcs::ImplementationException>(err.str());
      return 0;
    }
  }

}
}
