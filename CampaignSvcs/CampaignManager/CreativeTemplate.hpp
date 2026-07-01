#pragma once

#include <list>
#include <set>
#include <map>

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/Time.hpp>
#include <Sync/SyncPolicy.hpp>
#include <String/TextTemplate.hpp>
#include <Generics/CRC.hpp>
#include <Generics/GnuHashTable.hpp>

#include "CreativeTemplateArgs.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    struct RCOptionTokenValueMap:
      public OptionTokenValueMap,
      public ReferenceCounting::AtomicImpl
    {
    protected:
      virtual ~RCOptionTokenValueMap() noexcept {}
    };

    typedef ReferenceCounting::ConstPtr<RCOptionTokenValueMap>
      COptionTokenValueMap_var;

    struct TemplateParams:
      public TokenValueMap,
      public ReferenceCounting::DefaultImpl<>
    {
    protected:
      virtual ~TemplateParams() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<TemplateParams> TemplateParams_var;

    typedef std::list<TemplateParams_var> TemplateParamsList;

    /** Template - interface */
    struct Template: public ReferenceCounting::AtomicImpl
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(FileNotExists, Exception);
      DECLARE_EXCEPTION(InvalidParams, Exception);
      DECLARE_EXCEPTION(InvalidTemplate, Exception);
      DECLARE_EXCEPTION(ImplementationException, Exception);

      virtual std::string
      instantiate(
        const TemplateParams* request_params,
        const TemplateParamsList& creative_params)
        /*throw(InvalidParams,
          InvalidTemplate,
          ImplementationException)*/ = 0;

      virtual bool
      key_used(const String::SubString& key) const
        noexcept = 0;

      static void
      get_keys(
        String::TextTemplate::Keys& keys,
        const String::SubString& text)
        noexcept;

    protected:
      virtual ~Template() noexcept {};
    };

    typedef ReferenceCounting::SmartPtr<Template> Template_var;

    /** CreativeTemplateFactory */
    class CreativeTemplateFactory
    {
    public:
      typedef Generics::Time State;

      struct Handler
      {
        enum Type
        {
          CTT_NULL,
          CTT_TEXT
        };

        Handler(): type(CTT_NULL) {}

        Handler(const char* file_val, Type type_val)
          : file(file_val), type(type_val)
        {}

        bool operator<(const Handler& right) const noexcept
        {
          return file < right.file || (file == right.file && type < right.type);
        }

        bool operator==(const Handler& right) const noexcept
        {
          return file == right.file && type == right.type;
        }

        std::string file;
        Type type;
      };

      DECLARE_EXCEPTION(InvalidTemplate, eh::DescriptiveException);
      DECLARE_EXCEPTION(ImplementationException, eh::DescriptiveException);

      Template* create(const Handler& , State&) const
        /*throw(Template::FileNotExists,
              Template::InvalidTemplate,
              ImplementationException)*/;

      bool need_update(const Handler& , const State&) const
        /*throw(Template::InvalidTemplate, ImplementationException)*/;

      Template* update(Template*, const Handler& , State&) const
        /*throw(Template::FileNotExists,
              Template::InvalidTemplate,
              ImplementationException)*/;
    };

    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    class TemplateMap
    {
    protected:
      typedef Sync::Policy::PosixThread SyncPolicy;

    public:
      typedef Generics::GnuHashTable<KeyType, ValueType> KeyMap;
      typedef Generics::GnuHashSet<KeyType> KeySet;

    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      typedef typename KeyMap::const_iterator const_iterator;
      typedef typename KeyMap::iterator iterator;

      TemplateMap() noexcept;

      void insert(
        const KeyType& key,
        const ValueType& value)
        /*throw(Exception)*/;

      bool exist(const KeyType& key) const noexcept;

      bool get_value(const KeyType& key, ValueType& val) const noexcept;

      Template* get(const KeyType& key) const
        /*throw(Template::FileNotExists, Template::InvalidTemplate, Exception)*/;

      Template* get(const KeyType& key, ValueType& val) const
        /*throw(Template::FileNotExists, Template::InvalidTemplate, Exception)*/;

      void assign(const KeySet& key_set, TemplateMap& source_map)
        /*throw(Exception)*/;

      size_t size() const
      {
        return key_map_.size();
      }

      const_iterator begin() const
      {
        return key_map_.begin();
      }

      const_iterator end() const
      {
        return key_map_.end();
      }

      iterator begin()
      {
        return key_map_.begin();
      }

      iterator end()
      {
        return key_map_.end();
      }

    protected:
      struct TemplateWithState
      {
        TemplateWithState() {};

        TemplateWithState(const TemplateWithState& init)
          : state(init.state),
            templ(init.templ)
        {}

        TemplateWithState(
          const typename FactoryType::State& state_val,
          Template* templ_val)
          : state(state_val),
            templ(ReferenceCounting::add_ref(templ_val))
        {}

        typename SyncPolicy::Mutex lock;
        typename FactoryType::State state;
        Template_var templ;
      };

      typedef std::map<ValueHandlerType, TemplateWithState> ValueMap;

    protected:
      void insert_(
        const KeyType& key,
        const ValueType& value,
        const TemplateWithState& templ_with_state)
        /*throw(Exception)*/;

      Template* get_(typename ValueMap::iterator& ) const
        /*throw(Template::FileNotExists, Template::InvalidTemplate)*/;

    protected:
      FactoryType factory_;
      KeyMap key_map_;
      mutable ValueMap value_map_;
    };

    /** CreativeTemplateKey */
    struct CreativeTemplateKey
    {
      CreativeTemplateKey(
        const char* creative_format_val,
        const char* creative_size_val,
        const char* app_format_val) noexcept;

      size_t hash() const
      {
        return hash_;
      }

      bool operator==(const CreativeTemplateKey& src) const /* noexcept */;

      std::string creative_format;
      std::string creative_size;
      std::string app_format;
      size_t hash_;
    };

    struct CreativeTemplate: public CreativeTemplateFactory::Handler
    {
      CreativeTemplate(): track_impressions(false), timestamp(0) {};

      CreativeTemplate(
        const char* file_val,
        Type type_val,
        const char* mime_format_val,
        bool track_impr,
        RCOptionTokenValueMap* tokens,
        RCOptionTokenValueMap* hidden_tokens,
        const Generics::Time& ts_val) noexcept
        : CreativeTemplateFactory::Handler(file_val, type_val),
          mime_format(mime_format_val),
          track_impressions(track_impr),
          tokens(ReferenceCounting::add_ref(tokens)),
          hidden_tokens(ReferenceCounting::add_ref(hidden_tokens)),
          timestamp(ts_val),
          status('W')
      {}

      std::string mime_format;
      bool track_impressions;
      COptionTokenValueMap_var tokens;
      COptionTokenValueMap_var hidden_tokens;
      Generics::Time timestamp;
      char status; //W or A
    };

    typedef
      TemplateMap<
        CreativeTemplateKey,
        CreativeTemplate,
        CreativeTemplateFactory::Handler,
        CreativeTemplateFactory>
      CreativeTemplateMap;
  }
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    inline
    CreativeTemplateKey::CreativeTemplateKey(
      const char* creative_format_val,
      const char* creative_size_val,
      const char* app_format_val)
      noexcept
      : creative_format(creative_format_val),
        creative_size(creative_size_val),
        app_format(app_format_val)
    {
      hash_ = Generics::CRC::quick(0, creative_format.c_str(), creative_format.size());
      hash_ = Generics::CRC::quick(hash_, creative_size.c_str(), creative_size.size());
      hash_ = Generics::CRC::quick(hash_, app_format.c_str(), app_format.size());
    }

    inline
    bool
    CreativeTemplateKey::operator==(
      const CreativeTemplateKey& right) const
      /* noexcept */
    {
      return
        creative_format == right.creative_format &&
        creative_size == right.creative_size &&
        app_format == right.app_format;
    }

    /** TemplateMap */
    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    TemplateMap<KeyType, ValueType, ValueHandlerType, FactoryType>::
      TemplateMap()
      noexcept
    {}

    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    void
    TemplateMap<KeyType, ValueType, ValueHandlerType, FactoryType>::assign(
      const KeySet& key_set,
      TemplateMap& source_map)
      /*throw(Exception)*/
    {
      for(typename KeySet::const_iterator it = key_set.begin();
          it != key_set.end(); ++it)
      {
        typename KeyMap::iterator key_it = source_map.key_map_.find(*it);
        if(key_it != source_map.key_map_.end())
        {
          typename ValueMap::iterator val_it =
            source_map.value_map_.find(key_it->second);

          assert(val_it != source_map.value_map_.end());

          insert_(key_it->first, val_it->first, val_it->second);
        }
      }
    }

    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    void
    TemplateMap<KeyType, ValueType, ValueHandlerType, FactoryType>::insert_(
      const KeyType& key,
      const ValueType& handler,
      const TemplateWithState& templ_with_state)
      /*throw(Exception)*/
    {
      key_map_[key] = handler;
      typename ValueMap::const_iterator it = value_map_.find(handler);
      if(it == value_map_.end())
      {
        value_map_.insert(std::make_pair(handler, templ_with_state));
      }
    }

    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    void
    TemplateMap<KeyType, ValueType, ValueHandlerType, FactoryType>::insert(
      const KeyType& key,
      const ValueType& handler)
      /*throw(Exception)*/
    {
      insert_(key, handler, TemplateWithState());
    }

    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    bool
    TemplateMap<KeyType, ValueType, ValueHandlerType, FactoryType>::exist(
      const KeyType& key) const
      noexcept
    {
      typename KeyMap::const_iterator it = key_map_.find(key);
      return it != key_map_. end();
    }

    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    bool
    TemplateMap<KeyType, ValueType, ValueHandlerType, FactoryType>::get_value(
      const KeyType& key,
      ValueType& val) const
      noexcept
    {
      typename KeyMap::const_iterator it = key_map_.find(key);
      if(it == key_map_.end())
      {
        return false;
      }

      val = it->second;
      return true;
    }

    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    Template*
    TemplateMap<KeyType, ValueType, ValueHandlerType, FactoryType>::get_(
      typename ValueMap::iterator& value_map_it) const
      /*throw(Template::FileNotExists, Template::InvalidTemplate)*/
    {
      if(value_map_it == value_map_.end())
      {
        typename SyncPolicy::WriteGuard lock(value_map_it->second.lock);
        typename FactoryType::State state;
        Template_var templ = factory_.create(value_map_it->first, state);
        value_map_.insert(
          std::make_pair(value_map_it->first, TemplateWithState(state, templ)));
        return ReferenceCounting::add_ref(templ);
      }
      else
      {
        {
          typename SyncPolicy::ReadGuard lock(value_map_it->second.lock);

          if(!(value_map_it->second.templ.in() == 0) &&
             !factory_.need_update(
                value_map_it->first, value_map_it->second.state))
          {
            return ReferenceCounting::add_ref(value_map_it->second.templ);
          }
        }

        typename SyncPolicy::WriteGuard lock(value_map_it->second.lock);

        if(value_map_it->second.templ.in() == 0)
        {
          value_map_it->second.templ = factory_.create(
            value_map_it->first,
            value_map_it->second.state);
        }
        else if(factory_.need_update(
                  value_map_it->first, value_map_it->second.state))
        {
          value_map_it->second.templ = factory_.update(
            value_map_it->second.templ,
            value_map_it->first,
            value_map_it->second.state);
        }

        return ReferenceCounting::add_ref(value_map_it->second.templ);
      }
    }

    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    Template*
    TemplateMap<KeyType, ValueType, ValueHandlerType, FactoryType>::get(
      const KeyType& key) const
      /*throw(Template::FileNotExists, Template::InvalidTemplate, Exception)*/
    {
      typename KeyMap::const_iterator it = key_map_.find(key);
      if(it == key_map_.end())
      {
        return 0;
      }

      typename ValueMap::iterator templ_it = value_map_.find(it->second);

      return get_(templ_it);
    }

    template<
      typename KeyType,
      typename ValueType,
      typename ValueHandlerType,
      typename FactoryType>
    Template*
    TemplateMap<KeyType, ValueType, ValueHandlerType, FactoryType>::get(
      const KeyType& key,
      ValueType& val) const
      /*throw(Template::FileNotExists, Template::InvalidTemplate, Exception)*/
    {
      typename KeyMap::const_iterator it = key_map_.find(key);
      if(it == key_map_.end())
      {
        return 0;
      }

      val = it->second;

      typename ValueMap::iterator templ_it = value_map_.find(it->second);

      return get_(templ_it);
    }

  } /* CampaignSvcs */
} /* AdServer */
