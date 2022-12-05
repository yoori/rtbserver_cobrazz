#ifndef AD_SERVER_LOG_PROCESSING_GENERIC_LOG_IO_IMPL_TPP
#define AD_SERVER_LOG_PROCESSING_GENERIC_LOG_IO_IMPL_TPP

#include <LogCommons/StatCollector.hpp>

namespace AdServer {
namespace LogProcessing {

  class DistribHashHelper
  {
  public:
    typedef unsigned long hash_type;

    /**
     * This function returns:
     * If class T has method with signture:
     *   unsigned long distrib_hash() const
     * this method will call T::distrib_hash().
     * Othewise this method will call T::hash().
     */
    template<typename T>
    static inline
    hash_type
    get_distrib_hash(const T& t)
    {
      return __get_hash(t, std::integral_constant<bool, HasDistribHashHelper<T>::value>());
    }

  private:


    template<typename T>
    class HasDistribHashHelper: public std::tr1::__sfinae_types
    {
      template<typename U, hash_type (U::*)() const>
      struct _Wrap_type
      {
      };

      template<typename U>
      static __one __test(_Wrap_type<U, &U::distrib_hash>*);

      template<typename U>
      static __two __test(...);

    public:
      static const bool value = sizeof(__test<T>(0)) == 1;
    };

    template<typename T>
    static inline
    hash_type
    __get_hash(const T& t, std::true_type)
    {
      return t.distrib_hash();
    }

    template<typename T>
    static inline
    hash_type
    __get_hash(const T& t, std::false_type)
    {
     return t.hash();
    }
  };

  class ValueTypeDistribHashHelper
  {
  public:
    typedef DistribHashHelper::hash_type hash_type;

    template <class T>
    static inline
    hash_type
    get_distrib_hash(const T& t)
    {
      return DistribHashHelper::get_distrib_hash(t);
    }

    template <class T1, class T2>
    static inline
    hash_type
    get_distrib_hash(const std::pair<const T1, T2>& t)
    {
      return DistribHashHelper::get_distrib_hash(t.first);
    }
  };

  template <class LOG_TYPE_TRAITS_>
  struct DefaultSaveStrategy
  {
    typedef typename LOG_TYPE_TRAITS_::CollectorType CollectorT;

    void
    save(std::ostream& o, const CollectorT& collector) const
    {
      o << collector;
    }
  };

  template <class LOG_TYPE_TRAITS_>
  struct PackedSaveStrategy
  {
    typedef typename LOG_TYPE_TRAITS_::CollectorType CollectorT;

    void
    save(std::ostream& o, const CollectorT& collector) const
    {
      for (typename CollectorT::const_iterator it = collector.begin();
        it != collector.end(); ++it)
      {
        if (it != collector.begin())
        {
          o << '\n';
        }

        o << it->first << '\n' << it->second;
      }
    }
  };

  template <class LOG_TYPE_TRAITS_>
  struct DefaultDistributeStrategy
  {
    typedef typename LOG_TYPE_TRAITS_::CollectorType CollectorT;
    typedef Generics::ArrayAutoPtr<CollectorT> DistribData;

    void
    distribute(
      const CollectorT& collector,
      unsigned long distrib_count,
      DistribData& distrib_data) const
    {
      for (typename CollectorT::const_iterator it = collector.begin();
        it != collector.end(); ++it)
      {
        unsigned long distrib_index = ValueTypeDistribHashHelper::get_distrib_hash(*it) % distrib_count;
        distrib_data[distrib_index].add(*it);
      }
    }
  };

  template <class LOG_TYPE_TRAITS_>
  struct PackedDistributeStrategy
  {
    typedef typename LOG_TYPE_TRAITS_::CollectorType CollectorT;
    typedef Generics::ArrayAutoPtr<CollectorT> DistribData;

    void
    distribute(
      const CollectorT& collector,
      unsigned long distrib_count,
      DistribData& distrib_data) const
    {
      for (typename CollectorT::const_iterator it = collector.begin();
        it != collector.end(); ++it)
      {
        for (typename CollectorT::DataT::const_iterator data_it = it->second.begin();
          data_it != it->second.end(); ++data_it)
        {
          unsigned long distrib_index = ValueTypeDistribHashHelper::get_distrib_hash(*data_it) % distrib_count;
          distrib_data[distrib_index].add(it->first, typename CollectorT::DataT().add(*data_it));
        }
      }
    }
  };

  template <class LOG_TYPE_TRAITS_, class SaveStrategy>
  StringPair
  GenericLogSaverImplBase<LOG_TYPE_TRAITS_, SaveStrategy>::save_file(
    const CollectorT& collector,
    unsigned long distrib_index,
    bool rename) const
  {      
    StringPair filenames;

    try
    {
      LogFileNameInfo name_info(LOG_TYPE_TRAITS_::log_base_name());
      if (distrib_count_)
      {
        name_info.distrib_count = distrib_count_;
        name_info.distrib_index = distrib_index;
      }

      filenames = make_log_file_name_pair(name_info, path_);
      std::ofstream ofs(filenames.second.c_str());

      if (!ofs)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": Error: "
           << "Failed to open file '" << filenames.second << '\'';
        throw Exception(es);
      }

      typename LOG_TYPE_TRAITS_::HeaderType log_header(LOG_TYPE_TRAITS_::current_version());
      ofs << log_header;
      if (!ofs)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": Error: Failed to write log header";
        throw Exception(es);
      }

      SaveStrategy().save(ofs, collector);
      ofs.close();

      if (!ofs)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": Error: Failed to write log data "
           << "to file '" << filenames.second << '\'';
        throw Exception(es);
      }

      if (rename && std::rename(filenames.second.c_str(), filenames.first.c_str()))
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": Error: "
           << "Failed to rename temporary file '" << filenames.second
           << "to file '" << filenames.first << '\'';
        throw Exception(es);
      }
    }
    catch (const Exception&)
    {
      unlink(filenames.second.c_str());

      throw;
    }
    catch (const eh::Exception &ex)
    {
      unlink(filenames.second.c_str());

      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
         << ": " << ex.what();

      throw Exception(es);
    }
    catch (...)
    {
      unlink(filenames.second.c_str());

      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Caught unknown exception.";

      throw Exception(es);
    }

    return filenames;
  }

  template <class LOG_TYPE_TRAITS_, class SaveStrategy>
  void
  GenericLogSaverImpl<LOG_TYPE_TRAITS_, SaveStrategy, false>::save()
  {
    if (collector_.empty())
    {
      return;
    }

    Base::save_file(collector_);
    collector_.clear();
  }

  template <class LogTraits, class SaveStrategy>
  void
  GenericLogSaverImpl<LogTraits, SaveStrategy, true>::save(
    const typename Base1::Spillover_var& data)
  {
    Base1::collector_filter_->filter(data->collector);

    if (data->collector.empty())
    {
      return;
    }

    Base2::save_file(data->collector);
  }

  template <class LOG_TYPE_TRAITS_, class SaveStrategy, class DistributeStrategy>
  void
  DistribLogSaverImpl<LOG_TYPE_TRAITS_, SaveStrategy, DistributeStrategy, false>::save()
  {
    if (collector_.empty() || !Base::distrib_count_)
    {
      return;
    }

    //Distribute to auxiliary storage.
    Generics::ArrayAutoPtr<typename Base::CollectorT> distrib_data(Base::distrib_count_);
    DistributeStrategy().distribute(collector_, Base::distrib_count_, distrib_data);

    //Clear main storage.
    collector_.clear();

    try
    {
      //Process data.
      for (unsigned long distrib_index = 0; distrib_index < Base::distrib_count_; ++distrib_index)
      {
        if (!distrib_data[distrib_index].empty())
        {
          Base::save_file(distrib_data[distrib_index], distrib_index);

          //Remove data from auxiliary storage.
          distrib_data[distrib_index].clear();
        }
      }
    }
    catch (...)
    {
      //Copy data from auxiliary storage to main storage.
      for (unsigned long distrib_index = 0; distrib_index < Base::distrib_count_; ++distrib_index)
      {
        if (!distrib_data[distrib_index].empty())
        {
          collector_.merge(distrib_data[distrib_index]);
          distrib_data[distrib_index].clear();
        }
      }

      throw;
    }
  }

  template <class LOG_TYPE_TRAITS_, class SaveStrategy, class DistributeStrategy>
  void
  DistribLogSaverImpl<LOG_TYPE_TRAITS_, SaveStrategy, DistributeStrategy, true>::save(
    const typename Base1::Spillover_var& data)
  {
    if (!Base2::distrib_count_)
    {
      return;
    }

    Base1::collector_filter_->filter(data->collector);

    if (data->collector.empty())
    {
      return;
    }

    //Distribute to auxiliary storage.
    Generics::ArrayAutoPtr<typename LOG_TYPE_TRAITS_::CollectorType> distrib_data(Base2::distrib_count_);
    DistributeStrategy().distribute(data->collector, Base2::distrib_count_, distrib_data);

    //Clear main storage.
    data->collector.clear();

    typedef std::vector<StringPair> LogFilenames;
    LogFilenames log_file_names;

    //Process data.
    for (unsigned long distrib_index = 0; distrib_index < Base2::distrib_count_; ++distrib_index)
    {
      if (!distrib_data[distrib_index].empty())
      {
        const StringPair filenames = Base2::save_file(distrib_data[distrib_index], distrib_index, false);
        log_file_names.push_back(filenames);

        //Remove data from auxiliary storage.
        distrib_data[distrib_index].clear();
      }
    }

    //Rename files.
    for (LogFilenames::iterator it = log_file_names.begin(); it != log_file_names.end(); ++it)
    {
      if (std::rename(it->second.c_str(), it->first.c_str()))
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": Error: "
           << "Failed to rename temporary file '" << it->second
           << "to file '" << it->first << '\'';
        throw typename LogSaver::Exception(es);
      }
    }
  }

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_GENERIC_LOG_IO_IMPL_TPP */
