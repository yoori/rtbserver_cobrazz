#include <fstream>
#include "PassbackTemplate.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    namespace
    {
      const Generics::Time UPDATE_PERIOD(1);

      class TextTemplate: public PassbackTemplate
      {
      public:
        TextTemplate(const char* file)
          /*throw(PassbackTemplate::FileNotExists, Exception)*/;

        virtual void
        instantiate(
          std::ostream& ostr,
          const TokenValueMap& args)
          /*throw(InvalidParams,
            InvalidTemplate,
            ImplementationException)*/;

      protected:
        String::TextTemplate::IStream text_template_;
      };

      TextTemplate::TextTemplate(const char* file)
        /*throw(PassbackTemplate::FileNotExists, Exception)*/
      {
        static const char* FUN = "TextTemplate::TextTemplate()";

        std::fstream fstr(file, std::ios::in);
        if(!fstr.is_open())
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't open file '" << file << "'";
          throw PassbackTemplate::FileNotExists(ostr);
        }

        try
        {
          text_template_.init(
            fstr,
            TokenTemplateProperties::START_TOKEN,
            TokenTemplateProperties::STOP_TOKEN);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't init template - caught eh::Exception: " <<
            ex.what();
          throw Exception(ostr);
        }
      }

      void
      TextTemplate::instantiate(
        std::ostream& ostr,
        const TokenValueMap& args)
        /*throw(InvalidParams,
          InvalidTemplate,
          ImplementationException)*/
      {
        try
        {
          String::TextTemplate::ArgsContainer<TokenValueMap,
            String::TextTemplate::ArgsContainerStringAdapter> args_cont(&args);
          String::TextTemplate::DefaultValue default_cont(&args_cont);
          String::TextTemplate::ArgsEncoder encoder(&default_cont);
          ostr << text_template_.instantiate(encoder);
        }
        catch(const String::TextTemplate::UnknownName& ex)
        {
          Stream::Error ostr;
          ostr << "Can't instantiate creative. Caught UnknownName: " <<
            ex.what();
          throw InvalidParams(ostr);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "Can't instantiate creative. Caught eh::Exception: " <<
            ex.what();
          throw ImplementationException(ostr);
        }
      }
    }

    PassbackTemplate_var
    PassbackTemplateMap::get(const Generics::StringHashAdapter& key)
      /*throw(PassbackTemplate::FileNotExists,
        PassbackTemplate::InvalidTemplate)*/
    {
      TemplateHolder_var holder;
      
      {
        SyncPolicy::WriteGuard lock(templates_lock_);
        KeyMap::iterator it = templates_.find(key);
        if(it != templates_.end())
        {
          holder = it->second;
        }
      }

      if(holder)
      {
        Generics::Time now = Generics::Time::get_time_of_day();
        SyncPolicy::WriteGuard lock(holder->lock);

        if(now - holder->last_update > UPDATE_PERIOD)
        {
          // reopen template
          holder->last_update = now;
          try
          {
            holder->passback_template = new TextTemplate(key.text().c_str());
          }
          catch(...)
          {
            // use old template
          }
        }

        return holder->passback_template;
      }

      // open template
      Generics::Time now = Generics::Time::get_time_of_day();
      TemplateHolder_var new_holder = new TemplateHolder();

      {
        OpenLockMap::WriteGuard open_lock(open_locks_.write_lock(key));
        new_holder->last_update = now;
        new_holder->passback_template = new TextTemplate(key.text().c_str());

        SyncPolicy::WriteGuard lock(templates_lock_);
        templates_.insert(std::make_pair(key, new_holder));
      }

      return new_holder->passback_template;
    }
  }
}
