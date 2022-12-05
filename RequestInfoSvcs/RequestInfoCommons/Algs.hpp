#ifndef REQUESTINFOCOMMONS_ALGS_HPP
#define REQUESTINFOCOMMONS_ALGS_HPP

#include <iostream>
#include <sstream>
#include <list>
#include <Generics/BitAlgs.hpp>
#include <Generics/Time.hpp>

//#define REQUESTINFOCOMMONS_ALGS_DEBUG

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    const int LAST_APPEARANCE_MAX_DIFF = 31;

    // last mask bit: 1 - mask contains day when saved total appearance, 0 - otherwise
    const int TOTAL_APPEARANCE_BIT_POS = 63;
    const int MAX_DAY_BIT_POS = 63;
    const uint64_t TOTAL_APPEARANCE_MASK = (uint64_t)1 << TOTAL_APPEARANCE_BIT_POS;
  }

  struct IdAppearance
  {
    IdAppearance(
      unsigned long id_val,
      const Generics::Time& date_val,
      const Generics::Time& last_appearance_date_val,
      long counter_val);

    bool operator==(const IdAppearance& right) const;

    unsigned long id;
    Generics::Time date;
    Generics::Time last_appearance_date;
    long counter;
  };

  typedef std::list<IdAppearance> IdAppearanceList;

  // returns true if colo_id is found in list, and save founded date in 'date'.
  //   Otherwise set 'date' = Generics::Time::ZERO and return false
  template <typename ReaderIteratorType>
  bool get_time_by_id(
    ReaderIteratorType begin,
    ReaderIteratorType end,
    unsigned int colo_id,
    Generics::Time& time)
  {
    for (ReaderIteratorType iter = begin; iter != end; ++iter)
    {
      if ((*iter).id() == colo_id)
      {
        time = Generics::Time((*iter).create_time());
        return true;
      }
    }

    time = Generics::Time::ZERO;
    return false;
  }

  inline
  Generics::Time
  calculate_isp_time(
    const Generics::Time& gmt_time,
    const Generics::Time& isp_time,
    const Generics::Time& gmt_create_time)
  {
    Generics::Time tz_delta = isp_time - gmt_time;
    return (gmt_create_time + tz_delta);
  }

  namespace
  {
    inline
    void add_normalized_appearance(
      IdAppearanceList& result,
      unsigned long appear_id,
      const Generics::Time& appearance_date,
      const Generics::Time& last_appearance_date,
      long counter)
    {
      result.push_back(IdAppearance(
        appear_id,
        appearance_date,
        std::max(
          Generics::Time(last_appearance_date),
          appearance_date - Generics::Time::ONE_DAY * LAST_APPEARANCE_MAX_DIFF),
        counter));
    }
  }

  inline
  void
  add_total_appearance(
    IdAppearanceList& result,
    unsigned long appear_id,
    const Generics::Time& appearance_time)
  {
    result.push_back(IdAppearance(
      appear_id,
      appearance_time,
      Generics::Time::ZERO,
      1));
  }

  inline
  void
  invert_appearance_list(
    IdAppearanceList& result,
    const IdAppearanceList& source)
  {
    for(IdAppearanceList::const_iterator it = source.begin();
        it != source.end(); ++it)
    {
      result.push_back(IdAppearance(
        it->id,
        it->date,
        it->last_appearance_date,
        -it->counter));
    }
  }

  template<typename CheckIteratorType, typename IdGetterType>
  bool
  collect_all_appearance(
    IdAppearanceList& result,
    CheckIteratorType check_begin_it,
    CheckIteratorType check_end_it,
    long count,
    const IdGetterType& id_getter)
  {
    for(CheckIteratorType it = check_begin_it; it != check_end_it; ++it)
    {
      Generics::Time app_date = Generics::Time((*it).last_appearance_date());
      uint64_t prev_appearances_mask = (*it).prev_appearances_mask();
      bool contains_total_app = prev_appearances_mask & TOTAL_APPEARANCE_MASK;
      prev_appearances_mask = prev_appearances_mask & ~TOTAL_APPEARANCE_MASK;

      int appearance_before_pos;

      while(prev_appearances_mask && (
              appearance_before_pos = Generics::BitAlgs::lowest_bit_64(
                prev_appearances_mask)) < MAX_DAY_BIT_POS)
      {
        Generics::Time prev_app_date =
          app_date - Generics::Time::ONE_DAY * (appearance_before_pos + 1);
        add_normalized_appearance(
          result,
          id_getter(*it),
          app_date,
          prev_app_date,
          count);
        app_date = prev_app_date;
        prev_appearances_mask = (
          prev_appearances_mask >> (appearance_before_pos + 1));
      }

      if(contains_total_app)
      {
        result.push_back(IdAppearance(
          id_getter(*it),
          app_date,
          Generics::Time::ZERO,
          count));
      }
    }

    return !result.empty();
  }

  template<typename Type>
  void
  print_appearances(
    std::ostream& out,
    const Type& app_cell)
  {
    Generics::Time last_appearance_date(app_cell.last_appearance_date());
    unsigned long prev_appearances_mask = app_cell.prev_appearances_mask();

    out <<
      "[ id = " << app_cell.id() <<
      ", last_appearance_date = " << last_appearance_date.gm_f() <<
      (prev_appearances_mask & TOTAL_APPEARANCE_MASK ? "(total)" : "") <<
      ", prev_appearances = (";

    bool one_found = false;
    for(int i = 0; i < MAX_DAY_BIT_POS;
        ++i, prev_appearances_mask >>= 1)
    {
      if(prev_appearances_mask & 1)
      {
        out << (one_found ? ", " : "") <<
          (last_appearance_date - Generics::Time::ONE_DAY * (i + 1)).
            gm_f();
        one_found = true;
      }
    }

    out << " ) ]";
  }

  template<
    typename CheckIteratorType,
    typename IdLessOpType>
  bool
  collect_appearance(
    IdAppearanceList& result,
    CheckIteratorType check_begin_it,
    CheckIteratorType check_end_it,
    unsigned long appear_id,
    const Generics::Time& appearance_time,
    const IdLessOpType& id_less)
  {
#   ifdef REQUESTINFOCOMMONS_ALGS_DEBUG
    std::cerr << "collect_appearance(" << appear_id << ", '" <<
      appearance_time.gm_f() << "')" << std::endl;
#   endif

    CheckIteratorType pos_it = std::lower_bound(
      check_begin_it, check_end_it, appear_id, id_less);

    if(pos_it == check_end_it || id_less(appear_id, *pos_it))
    {
      result.push_back(IdAppearance(
        appear_id,
        appearance_time,
        Generics::Time::ZERO,
        1));

      return true;
    }
    else
    {
      if(appearance_time.tv_sec > (*pos_it).last_appearance_date())
      {
        add_normalized_appearance(
          result,
          appear_id,
          Generics::Time(appearance_time),
          Generics::Time((*pos_it).last_appearance_date()),
          1);

        return true;
      }
      else if(appearance_time.tv_sec < (*pos_it).last_appearance_date())
      {
#       ifdef REQUESTINFOCOMMONS_ALGS_DEBUG
        std::cerr << "collect_appearance(): reappear: ";
        print_appearances(std::cerr, *pos_it);
        std::cerr << std::endl;
#       endif
        // reappear days ago :
        int app_bit_pos = ((Generics::Time(
          (*pos_it).last_appearance_date()) - appearance_time) /
          Generics::Time::ONE_DAY.tv_sec).tv_sec - 1;

        // appearing can be passed to mask and it not reported for this day
        if(app_bit_pos < MAX_DAY_BIT_POS &&
           (((*pos_it).prev_appearances_mask() >> app_bit_pos) & 1) == 0)
        {
          // appeared, find appearance before and after new appearance point
          int appearance_before_pos = app_bit_pos + Generics::BitAlgs::lowest_bit_64(
            (*pos_it).prev_appearances_mask() >> app_bit_pos) + 1;

          int appearance_after_pos = Generics::BitAlgs::highest_bit_64(
            (*pos_it).prev_appearances_mask() &
            (((uint64_t)1 << (app_bit_pos + 1)) - 1));

          Generics::Time last_appearance_date((*pos_it).last_appearance_date());

          Generics::Time appearance_after_date =
            appearance_after_pos < MAX_DAY_BIT_POS ?
            last_appearance_date - Generics::Time::ONE_DAY * (appearance_after_pos + 1) :
            last_appearance_date;

#         ifdef REQUESTINFOCOMMONS_ALGS_DEBUG
          std::cerr << "collect_appearance(): "
            "app_bit_pos = " << app_bit_pos <<
            ", appearance_before_pos = " << appearance_before_pos <<
            ", appearance_after_pos = " << appearance_after_pos <<
            ", appearance_after_date = '" << appearance_after_date.gm_f() <<
            ", total_app = " << (((*pos_it).prev_appearances_mask() & TOTAL_APPEARANCE_MASK) ? 1 : 0) <<
            std::endl;
#         endif

          if(appearance_before_pos <= MAX_DAY_BIT_POS)
          {
#           ifdef REQUESTINFOCOMMONS_ALGS_DEBUG
            std::cerr << "collect_appearance(): surrounded reappear: " <<
              "appearance_before_pos = " << appearance_before_pos <<
              std::endl;
#           endif

            Generics::Time appearance_before_date =
              last_appearance_date - Generics::Time::ONE_DAY * appearance_before_pos;

            // divide reported transition
            add_normalized_appearance(
              result,
              appear_id,
              appearance_after_date,
              appearance_before_date,
              -1);

            add_normalized_appearance(
              result,
              appear_id,
              appearance_time,
              appearance_before_date,
              1);

            add_normalized_appearance(
              result,
              appear_id,
              appearance_after_date,
              appearance_time,
              1);
          }
          else if((*pos_it).prev_appearances_mask() & TOTAL_APPEARANCE_MASK)
          {
#           ifdef REQUESTINFOCOMMONS_ALGS_DEBUG
            std::cerr << "collect_appearance(): total reappear" << std::endl;
#           endif

            // revert already reported total appearance
            result.push_back(IdAppearance(
              appear_id,
              appearance_after_date,
              Generics::Time::ZERO,
              -1));

            result.push_back(IdAppearance(
              appear_id,
              appearance_time,
              Generics::Time::ZERO,
              1));

            add_normalized_appearance(
              result,
              appear_id,
              appearance_after_date,
              appearance_time,
              1);
          }
          else if(app_bit_pos < MAX_DAY_BIT_POS - LAST_APPEARANCE_MAX_DIFF)
          {
#           ifdef REQUESTINFOCOMMONS_ALGS_DEBUG
            std::cerr << "collect_appearance(): reappear with max diff" << std::endl;
#           endif

            result.push_back(IdAppearance(
              appear_id,
              appearance_after_date,
              Generics::Time::ONE_DAY * (TOTAL_APPEARANCE_BIT_POS + 1),
              -1));
          }
          else
          {
#           ifdef REQUESTINFOCOMMONS_ALGS_DEBUG
            std::cerr << "collect_appearance(): lost reappear, " <<
              "app_bit_pos = " << app_bit_pos << std::endl;
#           endif

            /* case when:
             *   mask don't contains point before new appeared time and
             *   mask don't contains total reported point and
             *   last point possible reported with last_appearance_date !=
             *     appearance_time - LAST_APPEARANCE_MAX_DIFF days
             * can't be processed correctly - skip it
             */
            return false;
          }

          return true;
        }
      }
    }

    return false;
  }

  template<
    typename ResultContainerType,
    typename InsertRecordAdapterType,
    typename IdLessOpType>
  void
  update_appearance(
    ResultContainerType& result,
    unsigned long appear_id,
    const Generics::Time& appearance_time,
    const IdLessOpType& id_less,
    const InsertRecordAdapterType& insert_adapter)
  {
    typename ResultContainerType::iterator pos_it = result.begin();

    while(pos_it != result.end() && id_less(*pos_it, appear_id))
    {
      ++pos_it;
    }

    if(pos_it != result.end() && !id_less(appear_id, *pos_it))
    {
      // update old record
      long last_appearance_date_diff = (
        appearance_time.tv_sec -
        pos_it->last_appearance_date()) /
        Generics::Time::ONE_DAY.tv_sec;

      // update mask
      if(last_appearance_date_diff > 0)
      {
        uint64_t only_days_mask =
          pos_it->prev_appearances_mask() & ~TOTAL_APPEARANCE_MASK;

        uint64_t total_appearance_mask =
          pos_it->prev_appearances_mask() & TOTAL_APPEARANCE_MASK;

        bool eldest_day_bit_lost =
          (((only_days_mask << last_appearance_date_diff) & ~TOTAL_APPEARANCE_MASK) >>
            last_appearance_date_diff) ^ only_days_mask;

        pos_it->prev_appearances_mask() =
          ((only_days_mask << 1 | 1) << (last_appearance_date_diff - 1)) |
          (eldest_day_bit_lost ? 0 : total_appearance_mask);

        pos_it->last_appearance_date() = appearance_time.tv_sec;
      }
      else
      {
        int bit_pos = (-last_appearance_date_diff - 1);
        if(bit_pos < MAX_DAY_BIT_POS)
        {
          pos_it->prev_appearances_mask() |= (uint64_t)1 << bit_pos;
        }
      }

#   ifdef REQUESTINFOCOMMONS_ALGS_DEBUG
      std::cerr << "update_appearance(" << appearance_time.gm_f() << "): ";
      print_appearances(std::cerr, *pos_it);
      std::cerr << ", last_appearance_date_diff = " << last_appearance_date_diff <<
        std::endl;
#   endif
    }
    else
    {
      // insert new record
      result.insert(
        pos_it,
        insert_adapter(appear_id, appearance_time));
    }
  }

  template<typename ContainerType>
  void
  print_appearance_list(
    std::ostream& out,
    const ContainerType& appears_list,
    const char* space_align)
  {
    for(typename ContainerType::const_iterator it = appears_list.begin();
        it != appears_list.end(); ++it)
    {
      if(it != appears_list.begin())
      {
        out << std::endl;
      }

      out << space_align;
      print_appearances(out, *it);
    }
  }

  template<typename ContainerType>
  std::string
  stringify_appearance_list(
    const ContainerType& appears_list,
    const char* space_align)
  {
    std::ostringstream ostr;
    ostr << std::endl;
    print_appearance_list(ostr, appears_list, space_align);
    return ostr.str();
  }

  struct BaseAppearanceIdGetter
  {
    template<typename ReaderType>
    unsigned long operator()(const ReaderType& reader) const
    {
      return reader.id();
    }
  };

  struct BaseAppearanceLess
  {
    template<typename ReaderType>
    bool operator()(
      unsigned long id,
      const ReaderType& reader) const
    {
      return id < reader.id();
    }

    template<typename ReaderType>
    bool operator()(
      const ReaderType& reader,
      unsigned long id) const
    {
      return reader.id() < id;
    }
  };

  template<typename RecordType>
  struct BaseInsertAppearanceAdapter
  {
    RecordType operator()(
      unsigned long id,
      const Generics::Time& appearance_time) const
    {
      RecordType res;
      res.id() = id;
      res.last_appearance_date() = appearance_time.tv_sec;
      res.prev_appearances_mask() = TOTAL_APPEARANCE_MASK;
      return res;
    }
  };

  // same code
  typedef BaseAppearanceLess BaseIdTimeLess;

  template<typename RecordType>
  struct BaseInsertIdTimeAdapter
  {
    RecordType operator()(
      unsigned long id,
      const Generics::Time& time) const
    {
      RecordType res;
      res.id() = id;
      res.create_time() = time.tv_sec;
      return res;
    }
  };

  // update isp_colo_create_time
  //   insert new (id, isp_time) if required
  //   update records for current colo_id
  //   update full list in case
  //     request_info.create_time < profile_reader.create_time()
  template<
    typename ResultContainerType,
    typename InsertRecordAdapterType,
    typename IdLessOpType>
  void
  update_id_time_list(
    ResultContainerType& result,
    unsigned long appear_id,
    const Generics::Time& old_gmt_time,
    const Generics::Time& new_gmt_time,
    const Generics::Time& new_isp_time,
    const IdLessOpType& id_less,
    const InsertRecordAdapterType& insert_adapter)
  {
    typename ResultContainerType::iterator
      pos_for_appear_id = result.begin();

    // Update all list in case if changed gmt_create_time
    bool changed_gmt_create_time = new_gmt_time < old_gmt_time;

    for(typename ResultContainerType::iterator pos_it = result.begin();
          pos_it != result.end(); ++pos_it)
    {
      // Search position for passed appear_id ().
      if(id_less(*pos_it, appear_id))
      {
        ++pos_for_appear_id;
      }

      // Update all with delta_tz = pos_it->create_time() - old_gmt_time
      //   except current colo_id (possible TZ changed for current colo_id)
      if(changed_gmt_create_time &&
          (id_less(*pos_it, appear_id) || id_less(appear_id, *pos_it)))
      {
        pos_it->create_time() =
          calculate_isp_time(
            old_gmt_time,
            Generics::Time(pos_it->create_time()),
            new_gmt_time).tv_sec;
      }
    }

    if(pos_for_appear_id != result.end() &&
       !id_less(appear_id, *pos_for_appear_id))
    {
      // found appear_id in list
      if(new_isp_time.tv_sec < pos_for_appear_id->create_time())
      {
        pos_for_appear_id->create_time() = new_isp_time.tv_sec;
      }
    }
    else
    {
      // If added colo_id not present in list and current request newer
      //   generate isp_create_time based on old_gmt_time
      //   and delta_tz for current request.
      Generics::Time isp_create_time_for_new_colo_id =
        (changed_gmt_create_time ?
          Generics::Time(new_isp_time.tv_sec) :
           calculate_isp_time(
            new_gmt_time,
            new_isp_time,
            old_gmt_time));

      result.insert(
        pos_for_appear_id,
        insert_adapter(
          appear_id, isp_create_time_for_new_colo_id));
    }
  }

  template<typename ContainerType>
  std::string
  stringify_id_date_list(
    const ContainerType& appears_list,
    const char* space_align)
  {
    std::ostringstream ostr;
    ostr << std::endl;

    for(typename ContainerType::const_iterator it = appears_list.begin();
        it != appears_list.end(); ++it)
    {
      if(it != appears_list.begin())
      {
        ostr << std::endl;
      }

      ostr << space_align;
      ostr << "[ id: " << (*it).id() << ", time = "
           << Generics::Time((*it).create_time()).gm_f() << " ]";
    }

    return ostr.str();
  }
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  inline
  IdAppearance::IdAppearance(
    unsigned long id_val,
    const Generics::Time& date_val,
    const Generics::Time& last_appearance_date_val,
    long counter_val)
    : id(id_val),
      date(date_val),
      last_appearance_date(last_appearance_date_val),
      counter(counter_val)
  {}

  inline
  bool
  IdAppearance::operator==(const IdAppearance& right) const
  {
    return id == right.id &&
      date == right.date &&
      last_appearance_date == right.last_appearance_date &&
      counter == right.counter;
  }

}
}

inline
std::ostream&
operator<<(std::ostream& out, const AdServer::RequestInfoSvcs::IdAppearance& val)
{
  out << "[ id: " << val.id << ", date = " << val.date.gm_f() <<
    ", last_appearance_date = " << val.last_appearance_date.gm_f() <<
    ", counter = " << val.counter << " ]";

  return out;
}

#endif
