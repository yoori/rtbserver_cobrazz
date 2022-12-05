#ifndef _GRANULARCONTAINER_HPP_
#define _GRANULARCONTAINER_HPP_

#include <map>
#include <algorithm>
#include <Generics/Time.hpp>

namespace AdServer
{
namespace Commons
{
  typedef Generics::Time TimestampValue;

  struct DeletedIdDef
  {
    DeletedIdDef(): timestamp(0), db_version(0) {}

    DeletedIdDef(
      const TimestampValue& ts,
      const TimestampValue& db_ver = Generics::Time::ZERO)
      : timestamp(ts),
        db_version(db_ver)
    {}

    TimestampValue timestamp;
    TimestampValue db_version;
  };

  template<typename ElementType>
  struct DefaultTimestampOp
  {
    TimestampValue operator()(const ElementType& el) const
    {
      return el.timestamp;
    }
  };

  template<typename ElementType>
  struct PointerTimestampOp
  {
    TimestampValue operator()(const ElementType& el) const
    {
      return el->timestamp;
    }
  };

  /** SimpleGranularContainer
   * container for keep instances with simple versioning,
   * without internal state changes
   */
  template<
    typename KeyType,
    typename ElementType,
    typename ElementTimestampOpType = DefaultTimestampOp<ElementType>,
    template<typename, typename> class Map = std::map>
  class SimpleGranularContainer
  {
  public:
    SimpleGranularContainer() = default;
    SimpleGranularContainer(SimpleGranularContainer&) = default;

    struct SimpleDeletedIdDef
    {
      SimpleDeletedIdDef(): timestamp(0) {}
      SimpleDeletedIdDef(unsigned long ts): timestamp(ts) {}

      TimestampValue timestamp;
    };

    typedef Map<KeyType, ElementType> ActiveMap;
    typedef Map<KeyType, SimpleDeletedIdDef> InactiveMap;

    bool activate(const KeyType& id, const ElementType& val)
    {
      ElementType copy(val);
      return activate(id, copy);
    }

    bool activate(const KeyType& id, ElementType& val)
    {
      bool appear = true;
      typename InactiveMap::iterator it = inactive_instances_.find(id);
      if(it != inactive_instances_.end())
      {
        appear = false;
        inactive_instances_.erase(it);
      }

      std::pair<typename ActiveMap::iterator, bool> act_ins =
        active_instances_.insert(typename ActiveMap::value_type(id, val));
      if(!act_ins.second)
      {
        appear = false;
        act_ins.first->second = val;
      }
      return appear;
    }

    void deactivate(const KeyType& id, const TimestampValue& val)
    {
      inactive_instances_[id].timestamp = val;

      typename ActiveMap::iterator it = active_instances_.find(id);
      if(it != active_instances_.end())
      {
        active_instances_.erase(it);
      }
    }

    TimestampValue max_stamp() const
    {
      ElementTimestampOpType ts_op;
      TimestampValue stamp;

      for (typename ActiveMap::const_iterator it = active_instances_.begin();
             it != active_instances_.end(); ++it)
      {
        stamp = std::max(stamp, ts_op(it->second));
      }

      for (typename InactiveMap::const_iterator it = inactive_instances_.begin();
             it != inactive_instances_.end(); ++it)
      {
        stamp = std::max(stamp, it->second.timestamp);
      }

      return stamp;
    }

    ActiveMap& active()
    {
      return active_instances_;
    }

    const ActiveMap& active() const
    {
      return active_instances_;
    }

    const InactiveMap& inactive() const
    {
      return inactive_instances_;
    }

  protected:
    ActiveMap active_instances_;
    InactiveMap inactive_instances_;
  };

  /** GranularContainer
   * container for keep instances with internal versioning
   * change timestamp to sysdate if changed status (inactive, active) or
   *   db_version changed
   */
  template<typename ElementType>
  struct DefaultTimestampOps
  {
    TimestampValue timestamp(const ElementType& el) const
    {
      return el.timestamp;
    }

    TimestampValue db_version(const ElementType& el) const
    {
      return el.db_version;
    }

    void set_timestamp(ElementType& el, const TimestampValue& ts) const
    {
      el.timestamp = ts;
    }
  };

  template<typename ElementType>
  struct PointerTimestampOps
  {
    TimestampValue timestamp(const ElementType& el) const
    {
      return el->timestamp;
    }

    TimestampValue db_version(const ElementType& el) const
    {
      return el->db_version;
    }

    void set_timestamp(ElementType& el, const TimestampValue& ts) const
    {
      el->timestamp = ts;
    }
  };

  template<
    typename KeyType,
    typename ElementType,
    typename ElementTimestampOpsType = DefaultTimestampOps<ElementType>,
    template<typename, typename> class Map = std::map>
  class GranularContainer
  {
  public:
    typedef std::map<KeyType, ElementType> ActiveMap;
    typedef std::map<KeyType, DeletedIdDef> InactiveMap;

    void activate(const KeyType& id, const ElementType& val)
    {
      active_instances_[id] = val;
      typename InactiveMap::iterator it = inactive_instances_.find(id);
      if(it != inactive_instances_.end())
      {
        inactive_instances_.erase(it);
      }
    }

    void activate(
      const KeyType& id,
      const ElementType& val,
      const TimestampValue& sysdate,
      const GranularContainer* old_container = 0)
    {
      ElementTimestampOpsType ts_ops;
      bool changed = true;

      if (old_container != 0)
      {
        typename ActiveMap::const_iterator a_it =
          old_container->active().find(id);
        if (a_it != old_container->active().end())
        {
          if (ts_ops.db_version(val) ==
              ts_ops.db_version(a_it->second) && (
                ts_ops.timestamp(val) == Generics::Time::ZERO ||
                ts_ops.timestamp(val) == ts_ops.timestamp(a_it->second)))
          {
            active_instances_[id] = a_it->second;
            changed = false;
          }
        }
      }

      if(changed)
      {
        TimestampValue timestamp = sysdate;
        ElementType& el = active_instances_[id];
        el = val;
        ts_ops.set_timestamp(el, timestamp);
      }

      typename InactiveMap::iterator it = inactive_instances_.find(id);
      if(it != inactive_instances_.end())
      {
        inactive_instances_.erase(it);
      }
    }

    void deactivate(const KeyType& id, const TimestampValue& val)
    {
      DeletedIdDef& del_el = inactive_instances_[id];
      del_el.timestamp = val;
      del_el.db_version = 0;

      typename ActiveMap::iterator it = active_instances_.find(id);
      if(it != active_instances_.end())
      {
        active_instances_.erase(it);
      }
    }

    void deactivate(
      const KeyType& id,
      const TimestampValue& sysdate,
      const TimestampValue& db_version,
      const GranularContainer* old_container = 0)
    {
      TimestampValue timestamp = sysdate;
      TimestampValue db_vers = db_version;

      if (old_container != 0)
      {
        typename InactiveMap::const_iterator i_it =
          old_container->inactive().find(id);
        if (i_it != old_container->inactive().end())
        {
          timestamp = i_it->second.timestamp;
          db_vers = i_it->second.db_version;
        }
      }

      DeletedIdDef& del_el = inactive_instances_[id];
      del_el.timestamp = timestamp;
      del_el.db_version = db_vers;

      typename ActiveMap::iterator it = active_instances_.find(id);
      if(it != active_instances_.end())
      {
        active_instances_.erase(it);
      }
    }

    const ActiveMap& active() const
    {
      return active_instances_;
    }

    ActiveMap& active()
    {
      return active_instances_;
    }

    const InactiveMap& inactive() const
    {
      return inactive_instances_;
    }

    TimestampValue max_stamp() const
    {
      ElementTimestampOpsType ts_ops;
      TimestampValue stamp;

      for (typename ActiveMap::const_iterator it = active_instances_.begin();
             it != active_instances_.end(); ++it)
      {
        stamp = std::max(stamp, ts_ops.timestamp(it->second));
      }

      for (typename InactiveMap::const_iterator it = inactive_instances_.begin();
             it != inactive_instances_.end(); ++it)
      {
        stamp = std::max(stamp, it->second.timestamp);
      }

      return stamp;
    }

  protected:
    ActiveMap active_instances_;
    InactiveMap inactive_instances_;
  };

  /** NoCopyGranularContainer
   * uses ElementType::operator==() for assign timestamp
   */
  template<typename KeyType,
    typename ElementType,
    typename ElementTimestampOpsType = PointerTimestampOps<ElementType>,
    typename ActiveContainerType = std::map<KeyType, ElementType>,
    typename InactiveContainerType = std::map<KeyType, DeletedIdDef> >
  class NoCopyGranularContainer
  {
  public:
    NoCopyGranularContainer() = default;
    NoCopyGranularContainer(NoCopyGranularContainer&) = default;

    typedef ActiveContainerType ActiveMap;
    typedef InactiveContainerType InactiveMap;

    void activate(const KeyType& id, const ElementType& val)
    {
      ElementType copy(val);
      activate(id, copy);
    }

    void activate(const KeyType& id, ElementType& val);

    bool activate(
      const KeyType& id,
      const ElementType& val,
      const TimestampValue& sysdate,
      const NoCopyGranularContainer<
        KeyType, ElementType, ElementTimestampOpsType,
        ActiveContainerType, InactiveContainerType>* old_container = 0)
    {
      ElementType copy(val);
      return activate(id, copy, sysdate, old_container);
    }

    bool activate(
      const KeyType& id,
      ElementType& val,
      const TimestampValue& sysdate,
      NoCopyGranularContainer<
        KeyType, ElementType, ElementTimestampOpsType,
        ActiveContainerType, InactiveContainerType>* old_container);

    /* if previous object (from old_container) is equal and have equal state
     * will be used it (with old timestamp value) */
    bool activate(
      const KeyType& id,
      ElementType& val,
      const TimestampValue& sysdate,
      const NoCopyGranularContainer<
        KeyType, ElementType, ElementTimestampOpsType,
        ActiveContainerType, InactiveContainerType>* old_container = 0);

    void deactivate(
      const KeyType& id,
      const TimestampValue& sysdate,
      const NoCopyGranularContainer<
        KeyType, ElementType, ElementTimestampOpsType,
        ActiveContainerType, InactiveContainerType>* old_container = 0);

    unsigned long deactivate_nonactive(
      const NoCopyGranularContainer<
        KeyType, ElementType, ElementTimestampOpsType,
        ActiveContainerType, InactiveContainerType>& old_container,
      const TimestampValue& sysdate);

    void clean_deactivated(const TimestampValue& timestamp);

    const ActiveMap& active() const;

    ActiveMap& active();

    const InactiveMap& inactive() const;

    InactiveMap& inactive();

    TimestampValue max_stamp() const;

  protected:
    ActiveMap active_instances_;
    InactiveMap inactive_instances_;
  };
}
}

namespace AdServer
{
namespace Commons
{
  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  void
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  activate(
    const KeyType& id, ElementType& val)
  {
    active_instances_[id] = val;
    typename InactiveMap::iterator it = inactive_instances_.find(id);
    if(it != inactive_instances_.end())
    {
      inactive_instances_.erase(it);
    }
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  bool
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  activate(
    const KeyType& id,
    ElementType& val,
    const TimestampValue& sysdate,
    NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
      ActiveContainerType, InactiveContainerType>* old_container)
  {
    ElementTimestampOpsType timestamp_ops;
    bool changed = true;

    if (old_container != 0)
    {
      typename ActiveMap::iterator a_it =
        old_container->active().find(id);
      if (a_it != old_container->active().end())
      {
        if (*val == *a_it->second)
        {
          active_instances_[id] = a_it->second;
          changed = false;
        }
      }
    }

    bool result = false;

    if(changed)
    {
      typename ActiveMap::const_iterator a_it =
        active_instances_.find(id);

      if(a_it == active_instances_.end() ||
         !(*val == *a_it->second))
      {
        ElementType& el = active_instances_[id];
        el = val;
        timestamp_ops.set_timestamp(el, sysdate);
        result = true;
      }
    }

    typename InactiveMap::iterator it = inactive_instances_.find(id);
    if(it != inactive_instances_.end())
    {
      inactive_instances_.erase(it);
    }

    return result;
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  bool
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  activate(
    const KeyType& id,
    ElementType& val,
    const TimestampValue& sysdate,
    const NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
      ActiveContainerType, InactiveContainerType>* old_container)
  {
    ElementTimestampOpsType timestamp_ops;
    bool changed = true;

    if (old_container != 0)
    {
      typename ActiveMap::const_iterator a_it =
        old_container->active().find(id);
      if (a_it != old_container->active().end())
      {
        if (*val == *a_it->second)
        {
          active_instances_[id] = a_it->second;
          changed = false;
        }
      }
    }

    bool result = false;

    if(changed)
    {
      typename ActiveMap::const_iterator a_it =
        active_instances_.find(id);

      if(a_it == active_instances_.end() ||
         !(*val == *a_it->second))
      {
        ElementType& el = active_instances_[id];
        el = val;
        timestamp_ops.set_timestamp(el, sysdate);
        result = true;
      }
    }

    typename InactiveMap::iterator it = inactive_instances_.find(id);
    if(it != inactive_instances_.end())
    {
      inactive_instances_.erase(it);
    }

    return result;
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  void NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  deactivate(
    const KeyType& id,
    const TimestampValue& sysdate,
    const NoCopyGranularContainer<
      KeyType, ElementType, ElementTimestampOpsType,
      ActiveContainerType, InactiveContainerType>* old_container)
  {
    TimestampValue timestamp = sysdate;

    if (old_container != 0)
    {
      typename InactiveMap::const_iterator i_it =
        old_container->inactive().find(id);
      if (i_it != old_container->inactive().end())
      {
        timestamp = i_it->second.timestamp;
      }
    }

    inactive_instances_[id] = timestamp;

    typename ActiveMap::iterator it = active_instances_.find(id);
    if(it != active_instances_.end())
    {
      active_instances_.erase(it);
    }
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  unsigned long
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  deactivate_nonactive(
    const NoCopyGranularContainer<
      KeyType, ElementType, ElementTimestampOpsType,
      ActiveContainerType, InactiveContainerType>& old_container,
    const TimestampValue& sysdate)
  {
    unsigned long deactivated_count = 0;

    for(typename ActiveMap::const_iterator oit = old_container.active().begin();
        oit != old_container.active().end(); ++oit)
    {
      typename ActiveMap::iterator it = active_instances_.find(oit->first);
      if(it == active_instances_.end())
      {
        typename InactiveMap::const_iterator i_it =
          inactive_instances_.find(oit->first);
        if (i_it == inactive_instances_.end())
        {
          inactive_instances_.insert(std::make_pair(oit->first, sysdate));
          ++deactivated_count;
        }
      }
    }

    for(typename InactiveMap::const_iterator oit = old_container.inactive().begin();
        oit != old_container.inactive().end(); ++oit)
    {
      typename ActiveMap::iterator it = active_instances_.find(oit->first);
      if(it == active_instances_.end())
      {
        typename InactiveMap::const_iterator i_it =
          inactive_instances_.find(oit->first);
        if (i_it == inactive_instances_.end())
        {
          inactive_instances_.insert(*oit);
        }
      }
    }

    return deactivated_count;
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  void
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  clean_deactivated(const TimestampValue& timestamp)
  {
    // don't erase elements inplace, because InactiveContainerType can be unordered_map
    // copy container elements only when first element for erase found
    bool erase_found = false;
    InactiveMap new_inactive_map;

    typename InactiveMap::const_iterator inactive_begin_it =
      inactive_instances_.begin();

    for(typename InactiveMap::const_iterator oit = inactive_begin_it;
        oit != inactive_instances_.end(); ++oit)
    {
      if(oit->second.timestamp < timestamp)
      {
        if(!erase_found)
        {
          std::copy(
            inactive_begin_it,
            oit,
            std::inserter(new_inactive_map, new_inactive_map.begin()));

          erase_found = true;
        }
      }
      else
      {
        if(erase_found)
        {
          new_inactive_map.insert(*oit);
        }
      }
    }

    if(erase_found)
    {
      new_inactive_map.swap(inactive_instances_);
    }
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  const typename NoCopyGranularContainer<
    KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::ActiveMap&
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  active() const
  {
    return active_instances_;
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  typename NoCopyGranularContainer<
    KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::ActiveMap&
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  active()
  {
    return active_instances_;
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  const typename NoCopyGranularContainer<
    KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::InactiveMap&
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  inactive() const
  {
    return inactive_instances_;
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  typename NoCopyGranularContainer<
    KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::InactiveMap&
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  inactive()
  {
    return inactive_instances_;
  }

  template<typename KeyType, typename ElementType, typename ElementTimestampOpsType,
    typename ActiveContainerType, typename InactiveContainerType>
  TimestampValue
  NoCopyGranularContainer<KeyType, ElementType, ElementTimestampOpsType,
    ActiveContainerType, InactiveContainerType>::
  max_stamp() const
  {
    TimestampValue stamp;
    ElementTimestampOpsType timestamp_ops;

    for (typename ActiveMap::const_iterator it = active_instances_.begin();
         it != active_instances_.end(); ++it)
    {
      stamp = std::max(stamp, timestamp_ops.timestamp(it->second));
    }

    for (typename InactiveMap::const_iterator it = inactive_instances_.begin();
         it != inactive_instances_.end(); ++it)
    {
      stamp = std::max(stamp, it->second.timestamp);
    }

    return stamp;
  }
}
}

#endif /*_GRANULARCONTAINER_HPP_*/
