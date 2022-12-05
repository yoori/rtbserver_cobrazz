#include <Commons/CorbaAlgs.hpp>
#include <ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include "UpdateContainer.hpp"
#include "DictionaryMatcher.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  const char* DictionaryMatcher::ASPECT = "DictionaryMatcher";

  DictionaryMatcher::DictionaryMatcher(
    const CORBACommons::CorbaClientAdapter& adapter,
    CORBACommons::CorbaObjectRefList& dictionary_server_refs,
    Logging::Logger* logger)
    /*throw(Exception)*/
    : c_adapter_(adapter),
      logger_(logger)
  {
    static const char* FN = "DictionaryMatcher::DictionaryMatcher";
    if(dictionary_server_refs.empty())
    {
      return;
    }
    try
    {
      DictionaryProviderPoolConfig pool_config(&c_adapter_);
      pool_config.timeout = Generics::Time(10);
      pool_config.iors_list.insert(
        pool_config.iors_list.end(),
        dictionary_server_refs.begin(),
        dictionary_server_refs.end());
      dictionary_pool_.reset(new DictionaryProviderPool(pool_config));
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FN << ": CORBA::SystemException "
        "on resolving dictionary provider : " << e ;
      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ": eh::Exception : " << e.what();
      throw Exception(ostr);
    }
  }

  static Stream::Error& print(const LexemeData& lexeme, Stream::Error& trace) noexcept
  {
    trace << " ";
    for(LexemeData::Forms::const_iterator v_it = lexeme.forms.begin();
        v_it != lexeme.forms.end(); ++v_it)
    {
      if(v_it != lexeme.forms.begin())
      {
        trace << ",";
      }
      trace << "'" <<  v_it->text().str() << "'";
    }
    trace << ".";
    return trace;
  }

  void DictionaryMatcher::get_lexemes(
    const char* lang,
    LexemeCache& lexemes)
    /*throw(Exception)*/
  {
    if(!dictionary_pool_.get())
    {
      return;
    }
    try
    {
      CORBACommons::StringSeq seq_words;
      seq_words.length(lexemes.size());
      CORBA::ULong count = 0;
      for (auto it = lexemes.begin(); it != lexemes.end(); ++it)
      {
        seq_words[count++] << String::SubString(it->first.text());
      }
      AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq_var lexemes_res =
        query_dictionary_words_(lang, seq_words);
      assert(seq_words.length() == lexemes_res->length());
      for(CORBA::ULong i = 0; i < count; ++i)
      {
        const AdServer::ChannelSvcs::DictionaryProvider::Lexeme& lex =
          lexemes_res[i];
        if(lex.forms.length())
        {
          Lexeme_var& lex_data = lexemes[String::SubString(seq_words[i])];
          lex_data = new Lexeme;
          size_t len_form = 0;
          lex_data->forms.resize(lex.forms.length());
          for(size_t j = 0; j < lex.forms.length(); ++j)
          {
            lex_data->forms[j] = String::SubString(lex.forms[j]);
            len_form += lex_data->forms[j].text().length();
          }
          lex_data->data.reserve(len_form);
          for(size_t j = 0; j < lex.forms.length(); ++j)
          {
            size_t start = lex_data->data.size();
            lex_data->data.append(
              lex_data->forms[j].text().data(),
              lex_data->forms[j].text().length());
            lex_data->forms[j] = String::SubString(
              lex_data->data.c_str() + start,
              lex_data->forms[j].text().length());
          }
        }
      }
      trace_result_(lang, lexemes);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception : " << e.what();
      throw Exception(ostr);
    }
  }

  void DictionaryMatcher::trace_result_(
    const char* lang,
    const LexemeCache& cache)
    noexcept
  {
    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      for(auto it = cache.begin(); it != cache.end(); ++it)
      {
        Stream::Error trace;
        trace << lang << ": '" << it->first << "' :";
        if (it->second)
        {
          print(*it->second, trace);
        }
        logger_->log(trace.str(), Logging::Logger::TRACE, ASPECT);
      }
    }
  }
  
  AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq*
  DictionaryMatcher::query_dictionary_words_(
    const char* lang,
    const CORBACommons::StringSeq& seq_words) const
    /*throw(Exception)*/
  {
    try
    {
      DictionaryProviderPool::ObjectHandlerType dictionary_provider =
        dictionary_pool_->get_object();
      try
      {
        return dictionary_provider->get_lexemes(lang, seq_words);
      }
      catch(const ChannelSvcs::NotReady& e)
      {
        Stream::Error ostr;
        ostr << __func__ << ": NotReady :" << e.description;
        dictionary_provider.release_bad(ostr.str()); 
        throw Exception(ostr);
      }
      catch(const ChannelSvcs::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << __func__ << ": ImplementationException :" << e.description;
        dictionary_provider.release_bad(ostr.str()); 
        throw Exception(ostr);
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << __func__ << ": CORBA::SystemException :" << ex;
        dictionary_provider.release_bad(ostr.str());
        throw Exception(ostr);
      }
    }
    catch(const DictionaryProviderPool::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception :" << e.what();
      logger_->log(
        ostr.str(), Logging::Logger::WARNING, ASPECT, "ADS-ICON-10");
      throw Exception(ostr);
    }
  }

  bool DictionaryMatcher::ready() const noexcept
  {
    if(!dictionary_pool_.get())
    {
      return true;
    }
    try
    {
      AdServer::ChannelSvcs::DictionaryProvider::LexemeSeq_var res =
        query_dictionary_words_("", CORBACommons::StringSeq());
      return true;
    }
    catch(const Exception&)
    {
      return false;
    }
  }
}
}
