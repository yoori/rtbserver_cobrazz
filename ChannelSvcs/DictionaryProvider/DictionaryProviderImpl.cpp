#include <fstream>
#include <xsd/ChannelSvcs/DictionaryProviderConfig.hpp>
#include <Commons/Algs.hpp>

#include "DictionaryContainer.hpp"
#include "DictionaryProviderImpl.hpp"

namespace AdServer::ChannelSvcs
{
  const char* DictionaryProviderImpl::ASPECT = " DictionaryProvider";
  const size_t DictionaryProviderImpl::MAX_LOAD_SIZE = 1024 * 128;

  DictionaryProviderImpl::DictionaryProviderImpl(
    Logging::Logger* logger,
    const DictionaryProviderConfig* config)
    /*throw(Exception, eh::Exception)*/
    : callback_(new Logging::ActiveObjectCallbackImpl(logger, "DictionaryProvider", ASPECT)),
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
      for (xsd::AdServer::Configuration::DictionaryProviderConfigType::
          Dictionary_sequence::const_iterator it = config->Dictionary().begin();
          it != config->Dictionary().end(); ++it)
      {
        Task_var msg = new LoadTask(this, task_runner_, it->file().c_str(), it->lang().c_str());
        task_runner_->enqueue_task(msg);
        task_runned_.fetch_add(1, std::memory_order_relaxed);
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err <<  FN << ": Exception : " << e.what();
      throw Exception(err);
    }
  }

  void DictionaryProviderImpl::load_dictionary_(const char* lang, const char* file_name) noexcept
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
      task_runned_.fetch_sub(1, std::memory_order_acq_rel);
    }
    catch(const eh::Exception& e)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT, "ADS-IMPL-1302")
        << FN << ": Exception: " << e.what();
    }

    if (logger()->log_level() >= Logging::Logger::DEBUG &&
      task_runned_.load(std::memory_order_relaxed) == 0)
    {
      std::ostringstream ostr;
      cont_.get_statistic(ostr);
      logger()->log(ostr.str(), Logging::Logger::DEBUG, ASPECT);
    }
  }

  DictionaryProviderImpl::~DictionaryProviderImpl() noexcept = default;

  grpc::Status DictionaryProviderImpl::get_lexemes(
    grpc::ServerContext* context,
    const adserver::channel_svcs::dictionary_provider::GetLexemesRequest* request,
    adserver::channel_svcs::dictionary_provider::GetLexemesResponse* response)
  {
    const long remaining = task_runned_.load(std::memory_order_acquire);
    if (remaining)
    {
      return {grpc::StatusCode::UNAVAILABLE,
        "DictionaryProvider not ready: " + std::to_string(remaining) + " tasks remained"};
    }

    try
    {
      LexemeItemVector items(request->words_size());
      for (std::size_t i = 0; i < items.size(); ++i)
      {
        items[i].word = request->words(i).c_str();
      }

      cont_.find_lexemes(request->language().c_str(), items);
      std::size_t index = 0;
      for (const auto& item : items)
      {
        // Synchronous gRPC cancellation checks poll the completion queue.
        if (index++ % 1024 == 0 && context->IsCancelled())
        {
          return {grpc::StatusCode::CANCELLED, "Dictionary request cancelled"};
        }

        auto* lexeme = response->add_lexemes();
        for (const auto& form : item.forms)
        {
          lexeme->add_forms(form.data(), form.size());
        }
      }

      return grpc::Status::OK;
    }
    catch (const std::exception& error)
    {
      return {grpc::StatusCode::INTERNAL, error.what()};
    }
    catch (...)
    {
      return {grpc::StatusCode::INTERNAL, "Dictionary lookup failed"};
    }
  }
}
