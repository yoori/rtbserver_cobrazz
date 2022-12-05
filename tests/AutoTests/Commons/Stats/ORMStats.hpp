#ifndef __AUTOTESTS_COMMONS_STATS_ORMSTATS_HPP
#define __AUTOTESTS_COMMONS_STATS_ORMSTATS_HPP

#include <Generics/Time.hpp>
#include <iostream>
#include <limits>

#include <tests/AutoTests/Commons/Utils.hpp>
#include <tests/AutoTests/Commons/ORM/ORMLogic.hpp>
#include <tests/AutoTests/Commons/Request/BaseRequest.hpp>

namespace AutoTest
{
  namespace ORM
  {
    namespace StatsDB = ::AutoTest::DBC;

    /// Defines type of statistical value.
    typedef double       stats_value_type;

    /// Defines type of difference of two stats values.
    typedef double       base_stats_diff_type;

    /**
     * @brief Min difference between two statistical values.
     *
     * Two values are considered equal if their difference
     * doesn't exceed this number.
     */
    const base_stats_diff_type min_stats_diff = 0.000001;

    /**
     * @brief Represents any difference of two stats values.
     *
     * Its value is used when it's no matter is there any difference
     * between two values or not.
     */
    const base_stats_diff_type any_stats_diff = -std::numeric_limits<double>::max();


    /**
     * @brief NULL date.
     *
     */
    const Generics::ExtendedTime null_date = Generics::Time::ZERO.get_gm_time();


    /**
     * @brief Force 'statprocessing.proceed' job
     * 
     * Util is usefull in tests, which wait
     * CTR or eCPM changing.
     */
    bool calc_ctr(StatsDB::IConn& connection);

    /**
     * @brief Delete specific adserver statistics
     *
     * Deletes all statistics related to specified entity id
     */
    template<typename T, size_t Count>
    bool clear_stats(StatsDB::IConn& connection,
      const char* column_name,
      const T (&column_values)[Count]);

    template<typename T>
    bool clear_stats(StatsDB::IConn& connection,
      const char* column_name,
      const T& column_value);

    /**
     * @brief Update channel status
     *
     * Util is usefull in tests, which wait
     * channel status changes (W <-> A)
     */
    bool
    update_channel_display_status(
      StatsDB::IConn& conn,
      int channel_id);
    
    /**
     * @brief Update account status
     *
     * Util is usefull in tests, which wait
     * account status changes (I <-> A)
     */
    bool
    update_display_status(
      BaseUnit* test,
      const char* object_name,
      int account_id);

    /**
     * @brief Declare UnknownTimezone exception.
     *
     * Exception raised when DB return empty rowset for timezone
     */
    DECLARE_EXCEPTION(UnknownTimezone, eh::DescriptiveException);

    /**
     * @brief Get timezone ofset
     *
     * @param DB connection
     * @param timezone name
     * @return timezone ofset
     */
    Generics::Time get_tz_ofset(
      BaseUnit* test,
      const std::string& tzname) /*throw(eh::Exception)*/;

    /**
     * @brief Truncate date to hourly precision
     *
     * @param date to truncate
     * @return truncated date
     */    
    Generics::ExtendedTime
    trunc_hourly(const Generics::ExtendedTime& date);

    /**
     * @class stats_diff_type
     * @brief Represents value of difference between two statistical values.
     *
     * If difference is less than min_stats_diff
     * it's considered as a null difference.
     * Base on precisely_number class.
     */
    class stats_diff_type
      :public precisely_number
    {
    public:

      /**
       * @brief Default constructor.
       *
       * Creates stats_diff_type object and initializes it
       * with default (null) values.
       */
      stats_diff_type ()
        :precisely_number(0.0, min_stats_diff)
      {}

      /**
       * @brief Constructor.
       *
       * Creates stats_diff_type object and initializes it
       * with indicated values.
       * @param v difference's value.
       * @param precisely precision of the diff's value.
       */
      stats_diff_type (double v, double  precisely = min_stats_diff)
        :precisely_number(v, precisely)
      {}

      stats_diff_type (int v, double  precisely = min_stats_diff)
        :precisely_number(static_cast<double>(v), precisely)
      {}

      stats_diff_type (unsigned long v, double  precisely = min_stats_diff)
        :precisely_number(static_cast<double>(v), precisely)
      {}

      stats_diff_type (unsigned int v, double  precisely = min_stats_diff)
        :precisely_number(static_cast<double>(v), precisely)
      {}
      
      stats_diff_type (const char* v, double  precisely = min_stats_diff)
        :precisely_number(v, precisely)
      {}
      
      stats_diff_type (const std::string v, double precisely = min_stats_diff)
        :precisely_number(v, precisely)
      {}

      /**
       * @brief Copy constructor.
       * Creates exact copy of the indicated stats_diff_type object.
       * @param v copied object.
       */
      //stats_diff_type (const stats_diff_type& v)
      //  :precisely_number(v)
      //{}

      /**
       * @brief Constructor.
       *
       * Creates stats_diff_type object based on indicated precisely_number.
       * @param v source precisely_number value for new object.
       */
      stats_diff_type (const precisely_number& v)
        :precisely_number(v)
      {}

      /**
       * @brief Assignment operator.
       * @param other stats diff.
       */
      stats_diff_type& operator= (const stats_diff_type& v)
      {
        value_ = v.value_;
        precisely_ = v.precisely_;
        return *this;
      }

      /**
       * @brief Adding operator.
       *
       * Allows to add some value to current stats_diff_type object
       * using '+=' operator.
       * @param v added value.
       */
      stats_diff_type& operator+= (unsigned int v) { value_ += v; return *this;}

      /**
       * @brief Subtract operator.
       *
       * Allows to subtract some value from current stats_diff_type object
       * using '-=' operator.
       * @param v subtracted value.
       */
      stats_diff_type& operator-= (unsigned int v) { value_ -= v; return *this;}
      stats_diff_type& operator+= (signed   int v) { value_ += v; return *this;}
      stats_diff_type& operator-= (signed   int v) { value_ -= v; return *this;}
      stats_diff_type& operator+= (double v) { value_ += v; return *this;}
      stats_diff_type& operator-= (double v) { value_ -= v; return *this;}
      stats_diff_type& operator+= (const stats_diff_type& v)
      { value_ = value_ == ORM::any_stats_diff || v.value_ == ORM::any_stats_diff ?
          ORM::any_stats_diff: value_ + v.value_;
        return *this;}
      stats_diff_type& operator-= (const stats_diff_type& v)
      { value_ = value_ == ORM::any_stats_diff || v.value_ == ORM::any_stats_diff ?
          ORM::any_stats_diff: value_ - v.value_;
        return *this;}

      template<typename A, typename B>
      bool equal(const A& a, const B& b) const { return *this == (a - b);}
    };

    template<typename A, typename B>
    bool equal(const stats_diff_type& diff, const A& a, const B& b)
    {
      return diff.equal(a, b);
    }

    template<typename Diff, typename A, typename B>
    bool equal(const Diff& diff, const A& a, const B& b)
    {
      return diff == a- b;
    }

    /**
     * @brief check difference: initial + diff == real, 
     */
    inline
    int check_difference(const stats_value_type& real,
      const stats_value_type& initial,
      const stats_diff_type& diff)
    {
      if (diff == any_stats_diff)
        return -1;
      if (!diff.equal(real, initial))
        return 0;
      return -1;
    }

    /**
     * @brief check difference: initial[i] + diff == real[i], 
     * 
     * within iterators access
     */
    template<typename Iterator1, typename Iterator2>
    int check_difference(Iterator1 real_begin,
      Iterator1 real_end,
      Iterator2 initial_begin,
      Iterator2 initial_end,
      const stats_diff_type& diff)
    {
      int count = 0;
      for(; (real_begin != real_end) 
            && (initial_begin != initial_end);
          ++real_begin, ++initial_begin, ++count)
        if (-1 != check_difference(*real_begin, *initial_begin, diff))
          return count;
      return -1;
    }

    /**
     * @brief check difference: initial[i] + diff[i] == real[i], 
     * 
     * within iterators access
     */
    template<typename Iterator1, typename Iterator2, typename DiffIterator>
    int check_difference(Iterator1 real_begin,
      Iterator1 real_end,
      Iterator2 initial_begin,
      Iterator2 initial_end,
      DiffIterator diffs_begin,
      DiffIterator diffs_end)
    {
      int count = 0;
      for(; (real_begin != real_end)
            && (initial_begin != initial_end)
            && (diffs_begin != diffs_end);
          ++real_begin, ++initial_begin, ++diffs_begin, ++count)
      {
        if (-1 != check_difference(*real_begin, *initial_begin, *diffs_begin))
          return count;
      }
      return -1;
    }

    /**
     * @brief check difference
     * 
     * within template objects
     */
    template<typename Object1, typename Object2>
    int check_difference(const Object1& real,
      const Object2& initial,
      const stats_diff_type& diff)
    {
      return check_difference(beginof(real), endof(real),
        beginof(initial), endof(initial),
        diff);
    }

    /**
     * @brief check difference
     * 
     * within template objects
     */    
    template<typename Object1, typename Object2>
    int check_difference(const Object1& real,
      const Object2& initial,
      int diff)
    {
      return check_difference(beginof(real), endof(real),
        beginof(initial), endof(initial),
        diff);
    }
    
    /**
     * @brief check difference
     * 
     * within template objects
     */
    template<typename Object1, typename Object2, typename Diffs>
    int check_difference(const Object1& real,
      const Object2& initial,
      const Diffs& diffs)
    {
      return check_difference(beginof(real), endof(real),
        beginof(initial), endof(initial),
        beginof(diffs), endof(diffs));
    }

    /**
     * @brief check each difference: initial[i] + diff == real[i], 
     * 
     * within iterators access
     */
    template<typename Iterator1, typename Iterator2, typename Diffs>
    int check_each_difference(Iterator1 real_begin,
      Iterator1 real_end,
      Iterator2 initial_begin,
      Iterator2 initial_end,
      const Diffs& diffs)
    {
      int count = 0;

      for(; (real_begin != real_end)
            && (initial_begin != initial_end);
          ++real_begin, ++initial_begin, ++count)
      {
        if (-1 != check_difference(*real_begin, *initial_begin, diffs))
        {
          return count;
        }
      }
      return -1;
    }
    
    /**
     * @brief check each difference
     * 
     * within template objects
     */
    template<typename Object1, typename Object2, typename Diffs>
    int check_each_difference(const Object1& real,
      const Object2& initial,
      const Diffs& diffs)
    {
      return check_each_difference(beginof(real), endof(real),
        beginof(initial), endof(initial), diffs);
    }


    /**
     * @class Selectable
     * @brief Abstract class for any selectable from db entyties.
     *
     * This class provides interface for entyties that can be selected form DB.
     */
    class Selectable
    {
    protected:

      /**
       * @brief Executes SELECT query and extracts DB object.
       * @param connection specify DB connection where query is sent.
       * @return whether query is successful.
       */
      virtual bool query_select_ (StatsDB::IConn& connection) = 0;
    public:

      /**
       * @brief Default constructor.
       */
      Selectable();

      /**
       * @brief Destructor.
       */
      virtual ~Selectable();

      /**
       * @brief Select object from DB.
       */
      bool
      select(
        StatsDB::IConn& connection, bool initial = true);

    protected:
      bool initial_;
    };

    /**
     * @class OneRowObject
     * @brief Abstract class for selected from DB object,
     * which represents only one row.
     */
    class OneRowObject:
      public virtual Selectable
    {
    public:

      /**
       * @brief Get query result.
       */
      virtual void get(StatsDB::Result& result) = 0;

      /**
       * @brief Perform query and get result.
       */
      bool ask(StatsDB::Query& query);

      /**
       * @brief Destructor.
       */
      virtual ~OneRowObject();
    };

    /**
     * @brief only throw exception with string representation of object
     */
    template<class T>
    void throw_not_select_object(const T& object)
    {
      Stream::Error error;
      error << "Can't select: ";
      object.print_idname(error);
      throw Exception(error);
    }

    extern
    Generics::ExtendedTime
    default_date();

    /**
     * @class BasicStats
     * @brief Basic class for all stats
     * implemented as array, provides checking features
     */
    template<class T, int Count>
    class BasicStats:
      public virtual OneRowObject
    {
    protected:
      std::string description_;
    public:
      static const int SelfCount = Count;
      typedef T Self;
      typedef stats_value_type values_type[Count];
      typedef stats_diff_type  diffs_type [Count];
      typedef const char*      names_type[Count];
      typedef const stats_value_type* const_iterator;

      values_type             values;
      static names_type const field_names;

      BasicStats (const char* description)
        : description_(description)
      {
        memset(values, 0, sizeof(values));
      }
      BasicStats& operator= (const BasicStats& from)
      { 
        memmove(values, from.values, sizeof(values));
        description_ = from.description_;
        return *this;
      }
      BasicStats (const BasicStats& from):
        Selectable(),
        OneRowObject(),
        description_(from.description_)
      { 
        memmove(values, from.values, sizeof(values));
      }
      void get (StatsDB::Result& result)
      {
        for(int i = 0;  i < Count; ++i)
          result.get(values[i]);
      }
      void description(const char* description)
      {
        description_ = description;
      }
      void description(const std::string& description)
      {
        description_ = description;
      }
      stats_value_type& operator[] (int i) { return values[i];}
      const stats_value_type& operator[] (int i) const { return values[i];}
      bool operator== (const values_type& stats) const
      {
        return std::equal(values, values + Count, stats);
      }
      bool operator!= (const values_type& stats) const
      {
        return !operator==(stats);
      }
      bool operator== (const Self& stats) const
      {
        return operator==(stats.values);
      }
      bool operator!= (const Self& stats) const
      {
        return !operator==(stats.values);
      }
      const stats_value_type* begin() const
      {
        return static_cast<const stats_value_type*>(values);
      }
      const stats_value_type* end() const
      {
        return static_cast<const stats_value_type*>(values + Count);
      }
      static void print_diff(
        std::ostream& out,
        const stats_diff_type& value)
      {
        if(value == any_stats_diff)
        {
          out << '-';
        }
        else
        {
          out << value;
        }
      }

      template<class Diff>
      static void print_diff(
        std::ostream& out,
        const Diff (&values)[Count])
      {
        for(int i = 0; i < Count-1; ++i)
        {
          print_diff(out, values[i]);
          out << ", ";
        }
        print_diff(out, values[Count-1]);
      }

      template<typename Diff>
      void print_diff  (std::ostream& out, 
                        const Diff& diff,
                        const BasicStats<T, Count>& real) const
      {
        static_cast<const T*>(this)->print_idname (out);
        for(int i = 0;  i < Count; ++i)
        {
          out << "  " << field_names[i] << " = " << values[i] << " + ";
          print_diff(out, diff);
          out << " -> " << real[i] << ';';
          if(diff != any_stats_diff && !equal(diff, real[i], values[i]))
          {
            out << " *** ";
          }
          out << std::endl;
        }
      }
      template<typename Diff>
      void print_diff  (std::ostream& out, 
                        const Diff (&diff)[Count],
                        const BasicStats<T, Count>& real) const
      {
        std::ostringstream diff_out;
        bool not_expected = false;
        for(int i = 0;  i < Count; ++i)
        {
          diff_out << "  " << field_names[i] << " = " << values[i] << " + ";
          print_diff(diff_out, diff[i]);
          diff_out << " -> " << real[i] << ';';
          if(diff[i] != any_stats_diff && !equal(diff[i], real[i], values[i]))
          {
            not_expected = true;
            diff_out << " *** ";
          }
          diff_out << std::endl;
        }
        if (not_expected) out << "!FAILED! ";
        static_cast<const T*>(this)->print_idname (out);
        out << diff_out.str();
      }
    };
    template<class T, int Count>
    std::ostream& operator<< (std::ostream& out, const BasicStats<T, Count>& stats)
    {
      static_cast<const T&>(stats).print_idname (out);
      for(int i = 0;  i < Count; ++i)
      {
        out << "  " << stats.field_names[i] << " = " << stats.values[i] << ";" << std::endl;
      }
      return out;
    }

  }//namespace ORM
}//namespace AutoTest

#include "ORMStats.ipp"

#endif // __AUTOTESTS_COMMONS_STATS_ORMSTATS_HPP
