#ifndef __AUTOTESTS_COMMONS_DEBUGINFO_HPP
#define __AUTOTESTS_COMMONS_DEBUGINFO_HPP

#include <map>
#include <list>
#include <iostream>
#include <string>
#include <eh/Exception.hpp>
#include <tests/AutoTests/Commons/Utils.hpp>

namespace AutoTest
{
  namespace DebugInfo
  {

    /**
     * @class Value
     * @brief Interface for AdServer debug info values types.
     *
     * Contains base functions required to implement
     * debug info parsing functionality.
     */
    class Value
    {
    public:

      /**
       * @brief Default destructor.
       */
      virtual ~Value () {}

      /**
       * @brief Clear value.
       *
       * Makes current value empty.
       */
      virtual void clear() = 0;

      /**
       * @brief Parse and initialize value.
       *
       * Extracts exact value from initial string.
       * Expects no leading whitespace, and should eat all
       * trailing whitespaces.
       * @param pos initial string.
       * @param stoplist string containing stop characters.
       * Parsing continue till a first stop character.
       * @return whether parse operation is successful.
       */
      virtual bool parse (const char *&pos, const char *stoplist) = 0;

      /**
       * @brief Dump value into ostream.
       *
       * Makes indents if needed. One indent is equal to 2 whitespaces.
       * @param out stream to output value.
       * @param level number of indents.
       * @return out stream.
       */
      virtual std::ostream&  dump (std::ostream& out, int level = 0) const  = 0;

      /**
       * @brief Check whether value is empty.
       * @return true if value is empty, false otherwise.
       */
      virtual bool empty () const = 0;

      /**
       * @brief Make indents in ostream.
       * @param out ostream.
       * @param level number of indents.
       * @return out stream.
       */
      static std::ostream& indent (std::ostream& out, int level = 0);
    };

    /**
     * @class SimpleValue
     * @brief Simple value of some debug-info aspect.
     *
     * Debug-info consists of name-value pairs (aspects).
     * Value of such pair can represents single value or list
     * of attributes, which are name-value pairs too.
     * This class represents single value of some debug-info aspect
     * or single value of its attribute.
     */
    class SimpleValue
      :public Value
    {
    public:

      /**
       * @brief Get value.
       * @return object value
       */
      const std::string& value () const { return value_; }

      /**
       * @brief Get value.
       *
       * Allows to get value using '()' operator.
       * @return object value
       */
      operator const std::string& () const { return value();}

      /**
       * @brief Clear value.
       *
       * See Value::clear() for details.
       */
      void clear();

      /**
       * @brief Parse and initialize value.
       *
       * See Value::parse() for details.
       */
      bool parse (const char *&pos, const char *stoplist);

      /**
       * @brief Dump value into ostream.
       *
       * See Value::dump() for details.
       */
      std::ostream& dump (std::ostream& out, int level = 0) const;

      /**
       * @brief Check whether value is empty.
       *
       * See Value::empty() for details.
       */
      bool empty () const;

      /**
       * @brief Comparison on equality.
       *
       * Checks equality of two values.
       * @param val value to compare with SimpleValue.
       * @return whether values are equal.
       */
      bool operator== (const std::string& val) const { return value_ == val;}

      /**
       * @brief Comparison on inequality.
       *
       * Checks inequality of two values.
       * @param val value to compare with SimpleValue.
       * @return whether values are equal.
       */
      bool operator!= (const std::string& val) const { return value_ != val;}

    protected:
      std::string value_;
    };

    /**
     * @brief 'Put to stream' operator.
     * Puts SimpleValue value to ostream.
     * @param out output stream.
     * @param svalue SimpleValue value.
     * @return out stream;
     */
    inline
    std::ostream& operator<< (std::ostream& out, const SimpleValue& svalue)
    {
      out << svalue.value();
      return out;
    }

    /**
     * @brief Comparison operator (equality).
     *
     * Compares STL-string with SimpleValue object
     * and checks whether their values are equal.
     * @param val STL-string.
     * @param svalue SimpleValue object.
     * @return true if STL-string and SimpleValue are equal, false overwise.
     */
    inline
    bool
    operator== ( const std::string& val, const SimpleValue& svalue )
    {
      return svalue == val;
    }

    /**
     * @brief Comparison operator (inequality).
     *
     * Compares STL-string with SimpleValue object
     * and checks whether their values are unequal.
     * @param val STL-string.
     * @param svalue SimpleValue object.
     * @return true if STL-string and SimpleValue are unequal, false otherwise.
     */
    inline
    bool
    operator!= ( const std::string& val, const SimpleValue& svalue )
    {
      return svalue != val;
    }

    /**
     * @class ParenthesesValue
     * @brief Parenthesized value.
     *
     * Represents simple Parenthesized value.
     */
    class ParenthesesValue
      :public SimpleValue
    {
    public:

      /**
       * @brief Parse and initialize value.
       *
       * Extract exact value from initial string.
       * @param pos initial string.
       * @param stoplist string containing stop characters.
       * @return whether parse operation is successful.
       */
      bool parse (const char *&pos, const char *stoplist);
    };

    /**
     * @class StringListValue
     * @brief Represents list of simple string values.
     *
     * It parses initial string with values divided by commas
     * and save it in internal structures so you can fetch
     * one of values and use it as a single value.
     * Also you can check whether some value presents
     * in list.
     */
    class StringListValue
      :public Value
    {
    public:
      typedef std::list<std::string> string_list;
      typedef string_list::const_iterator const_iterator;
      typedef string_list::iterator iterator;
      typedef string_list::value_type value_type;
      typedef string_list::size_type  size_type;

      /**
       * @brief Get start const iterator of the StringListValue.
       *
       * Returns const_iterator for the first value,
       * so you can't modify it using result iterator.
       * You can use result value only for read purposes.
       * @return const_iterator for the first element of the list of values.
       */
      const_iterator begin () const { return values_.begin ();}

      /**
       * @brief Get end const iterator of the StringListValue.
       *
       * Returns const_iterator for the position after the last element
       * of the list, so you can't use it for write purposes.
       * @return const_iterator for the position after the last element
       * of the list of values.
       */
      const_iterator end   () const { return values_.end ();}

      /**
       * @brief Get StringListValue size.
       *
       * @return Number of elements of StringListValue.
       */
      size_type      size  () const { return values_.size ();}

      /**
       * @brief Clear StringListValue.
       *
       * Clears all elements of StringListValue and makes it empty.
       */
      void clear();

      /**
       * @brief Parse and initialize StringListValue.
       *
       * Parses initial string of values divided by commas
       * ans save it in internal structures (string list).
       * @param pos initial string
       * @param stoplist string containing stop characters.
       * @return whether parse operation is successful.
       */
      bool parse (const char *&pos, const char *stoplist);

      /**
       * @brief Print StringListValue values.
       *
       * Dumps all values of StringListValue
       * into out stream dividing it with commas.
       * @param out stream to out list of values.
       * @param level number of indents.
       * @return reference to out stream.
       */
      std::ostream& dump (std::ostream& out, int level = 0) const;

      /**
       * @brief Check whether StringListValue is empty.
       *
       * @return whether list of values is empty.
       */
      bool empty () const;

      /**
       * @brief Check appearance of some value.
       *
       * Checks whether indicated value appear in the list of values.
       * @param value value to search in the list of values.
       * @return whether list of values contains indicated value.
       */
      bool has   (const std::string& value) const;
    protected:

      /// List of values.
      string_list values_;
    };

    /**
     * @brief Concatenation operator.
     *
     * Allows to concatenate two StringListValue
     * objects into one using '+' operator.
     * @param left left side of concatenation.
     * @param right right side of concatenation.
     * @return result StringListValue object composed of two initial objects.
     */
    inline
    StringListValue::string_list
    operator+ (const StringListValue& left, const StringListValue& right)
    {
      StringListValue::string_list ret(right.begin(), right.end());
      ret.insert(ret.begin(), left.begin(), left.end());
      return ret;
    }

    /**
     * @class StructValue
     * @brief Compound value with list of attributes.
     *
     * Represents value that is list of attributes which are name-value pairs.
     * Attribute's value, in turn, can be StructValue object too or any
     * other object with DebugInfo::Value interface.
     */
    class StructValue
      :public Value
    {
    public:

      /**
       * @brief Clear StructValue object.
       *
       * Makes all attribute's values empty.
       * Calls clear() function for all attributes.
       */
      void clear();

      /**
       * @brief Parse and initialize StructValue.
       *
       * Parses initial string and extracts from it
       * attribute's values by name.
       * Then saves it in internal structure,
       * so you can use each attribute's value as a single value.
       * @param pos initial string.
       * @param stoplist string containing stop characters.
       * @return whether parse operation is successful.
       */
      bool parse (const char *&pos, const char *stoplist);

      /**
       * @brief Print StructValue object.
       *
       * Dumps all attribute's names and its values into ostream.
       * Semicolon character divides attributes, and equal symbol
       * separates name from value of attribute. The StructValue object
       * enclosed in brackets. Makes indents if needed.
       * @param out stream to out StructValue object.
       * @param level number of indents.
       * @return reference to out stream.
       */
      std::ostream& dump (std::ostream& out, int level = 0) const;

      /**
       * @brief Check whether StructValue object is empty.
       * @return false. StructValue object can't be empty.
       */
      bool empty () const;

    protected:
      /*
        TODO: re-implement this as a static lookup table.
      */
      typedef std::map<std::string, Value *> ValueMap;

      /**
       * @brief Construct name-value map of attributes.
       *
       * This function is designed to juxtapose attribute name
       * with corresponding object member that is responsible
       * for such attribute. This method is very flexible and it allows
       * to add new fields without great modifications
       * in parsing methods and classes structure.
       * @note It's a virtual function. Called in child.
       * @param map output parameter. Represents name-value map of attributes.
       * Keys of the map are attributes names, and values of the map
       * are references to the StructValue object members that are responsible
       * for corresponding attributes.
       */
      virtual void construct(ValueMap &map) = 0;
    };

    /**
     * @class TimeValue
     * @brief Time metering value of some debug-info aspect.
     *
     * Represents simple time metering value.
     */
    class TimeValue
        :public SimpleValue
    {
    public:

      /**
       * @brief Parse and initialize value.
       *
       * Extract exact value from initial string.
       * @param pos initial string.
       * @param stoplist string containing stop characters.
       * @return whether parse operation is successful.
       */
      bool parse (const char *&pos, const char *stoplist);
    };

    /**
     * @class ValuesList
     * @brief Represents list of values of some type.
     */
    template<class T>
    class ValuesList
      :public Value
    {
    public:
      typedef std::list<T> values_list;
      typedef typename values_list::const_iterator const_iterator;
      typedef typename values_list::iterator iterator;
      typedef typename values_list::value_type value_type;
      typedef typename values_list::size_type  size_type;

      /**
       * @brief Get first element of the list.
       * @return const reference to the first element of the list,
       * so you can use result only for read purposes.
       */
      const value_type& first () const { return *values_.begin ();}

      /**
       * @brief Get last element of the list.
       * @return const reference to the last element of the list,
       * so you can use result only for read purposes.
       */
      const value_type& last () const { return values_.back ();}

      /**
       * @brief Get start const_iterator of the list.
       * @return const_iterator for the position
       * of the first element of the values list.
       */
      const_iterator begin () const { return values_.begin ();}

      /**
       * @brief Get end const_iterator of the list.
       * @return const_iterator for the position
       * after the last element of the values list.
       */
      const_iterator end   () const { return values_.end ();}

      /**
       * @brief Get size of values list.
       * @return actual number of elements in the values list.
       */
      size_type      size  () const { return values_.size ();}

      /**
       * @brief Remove all elements of the values list.
       */
      void clear();

      /**
       * @brief Parse initial string and initialize values list.
       *
       * Parses initial string, fetches values from it
       * and constructs values list. Each value is bracketed
       * in initial string. If initial string contains
       * only one value, it may not be bracketed.
       * @param pos initial string.
       * @param stoplist list of stop characters.
       * @return true if parsing is successful.
       */
      bool parse (const char *&pos, const char *stoplist);

      /**
       * @brief Print values list.
       *
       * Dumps all elements of the values list into ostream
       * dividing it with commas. Makes indents if needed.
       * @param out stream to out list.
       * @param level number of indents.
       * @return reference to out stream.
       */
      std::ostream &dump (std::ostream& out, int level = 0) const;

      /**
       * @brief Check values list emptiness.
       * @return whether values list if empty.
       */
      bool empty () const
      {
        return values_.empty ();
      }
    protected:

      /// Values list
      values_list values_;
    };

    /**
     * @class TriggersList
     * @brief This class provides structured information about
     * triggers matched by AdServer.
     *
     * AdServer triggers contains information about matched triggers
     * and this class helps to get this information in structured form
     * that allows to use it more efficiently and analyse it
     * with external components.
     * Based on Value class.
     */
    class TriggersList :public Value
    {

      DECLARE_EXCEPTION(TriggerNotFound, eh::DescriptiveException);

    public:
      typedef std::list<std::string> values_list;
      typedef std::map<std::string, SimpleValue> values_map;
      typedef values_map::const_iterator const_iterator;
      typedef values_map::iterator iterator;
      typedef values_map::size_type size_type;

      /**
       * @brief Get start const_iterator of the map.
       * @return const_iterator for the position
       * of the first element of the values map.
       */
      const_iterator begin () const { return values_.begin ();}

      /**
       * @brief Get end const_iterator of the map.
       * @return const_iterator for the position
       * after the last element of the values map.
       */
      const_iterator end   () const { return values_.end ();}

      /**
       * @brief Get size of values map.
       * @return actual number of elements in the values map.
       */
      size_type      size  () const { return values_.size ();}

      /**
       * @brief Clear values map.
       */
      void clear();

      /**
       * @brief Parse initial string and initialize values list.
       *
       * Parses initial string, fetches values from it
       * and constructs values list. Each value is bracketed
       * in initial string. If initial string contains
       * only one value, it may not be bracketed.
       * @param pos initial string.
       * @param stoplist list of stop characters.
       * @return true if parsing is successful.
       */
      bool parse (const char *&pos, const char *stoplist);


      /**
       * @brief Dump triggers value into ostream.
       *
       * Makes indents if needed. One indent is equal to 2 whitespaces.
       * @param out stream to output value.
       * @param level number of indents.
       * @return out stream.
       */
      std::ostream&  dump (std::ostream& out, int level = 0) const;

      /**
       * @brief Map assignment operator.
       * @param trigger.
       * @return channels for trigger.
       */
      const SimpleValue&
      operator[] (const std::string& trigger) const
        /*throw(TriggerNotFound)*/;

      /**
       * @brief Check values map emptiness.
       * @return whether values map if empty.
       */
      bool empty () const
      {
        return values_.empty ();
      }

      /**
       * @brief Get triggers as list (without channels).
       * @return trigger list.
       */
      const values_list& list() const
      {
        return triggers_;
      }

    protected:
      values_map values_;    // values map
      values_list triggers_; // trigger list
    };


    /**
     * @class SelectedCreative
     * @brief This class provides structured information about
     * selected by AdServer creatives.
     *
     * AdServer debug-info contains information about selected creatives
     * and this class helps to get this information in structured form
     * that allows to use it more efficiently and analyze it
     * with external components. Each field-member keeps some attribute's value.
     * Based on StructValue class.
     */
    class SelectedCreative
      :public StructValue
    {
    public:

      /**
       * @brief To keep camapign creative id attribute's value.
       */
      SimpleValue  ccid;

      /**
       * @brief To keep campaign id attribute's value.
       */
      SimpleValue  cmp_id;

      /**
       * @brief To keep ecpm attribute's value.
       */
      SimpleValue  ecpm;

      /**
       * @brief To keep ecpm_bid attribute's value.
       */
      SimpleValue  ecpm_bid;

      /**
       * @brief To keep triggered_expression attribute's value.
       */
      ParenthesesValue  triggered_expression;

      /**
       * @brief To keep click_url attribute's value.
       */
      SimpleValue  click_url;

      /**
       * @brief To keep html_url attribute's value.
       */
      SimpleValue  html_url;

      /**
       * @brief To keep action_adv_url attribute's value.
       */
      SimpleValue  action_adv_url;

      /**
       * @brief To keep revenue attribute's value.
       */
      SimpleValue  revenue;

      /**
       * @brief To keep imp_revenue attribute's value.
       */
      SimpleValue  imp_revenue;

      /**
       * @brief To keep click_revenue attribute's value.
       */
      SimpleValue  click_revenue;

      /**
       * @brief To keep action_revenue attribute's value.
       */
      SimpleValue  action_revenue;

      /**
       * @brief To keep actual_cpc attribute's value.
       */
      Money        actual_cpc;

      /**
       * @brief To keep imp_revenue value.
       */
      Money        imp_revenue_value;

      /**
       * @brief To keep ecpm_bid value.
       */
      Money        ecpm_bid_value;

      /**
       * @brief To keep ecpm value.
       */
      Money        ecpm_value;

      /**
       * @brief To keep margin_rule_id attribute's value.
       */
      SimpleValue margin_rule_id;

      /**
       * @brief To keep request_id attribute's value.
       */
      SimpleValue request_id;


      /**
       * @brief Parse initial string and initialize SelectedCreative object.
       *
       * Parses initial string, fetches information about selected creatives
       * and saves its actual attributes values in corresponding field-members.
       * @sa StructValue::parse().
       * @param pos initial string.
       * @param stoplist string containing stop characters
       * necessary for parsing.
       * @return whether parse operation is successful.
       */
      bool parse (const char *&pos, const char *stoplist);

    protected:

      /**
       * @brief Construct correspondence map between attribute's name
       * and reference to the class field.
       *
       * For more information see StructValue::construct().
       * Specified for selected_creatives attributes.
       */
      void construct(ValueMap &map);
    };

    /**
     * @class SelectedCreativesList
     * @brief List of selected creatives. Based on ValuesList and SelectedCreative.
     */
    class SelectedCreativesList
      :public ValuesList<SelectedCreative>
    {
    public:
      typedef ValuesList<SelectedCreative> Base;

      /**
       * @brief List assignment operator.
       * @param creative id.
       * @return creative by creative id.
       */
      const SelectedCreative& operator[](const std::string& ccid) const;

      /**
       * @brief Find creative in list by id
       * @param creative id.
       * @return list iterator.
       */
      const_iterator find(const std::string& ccid) const;
    };


    /**
     * @class DebugInfo
     * @brief Base class for parsing AdServer Debug-Info.
     *
     * AdServer provides its debug information
     * through special HTTP header - 'Debug-Info'.
     * This class designed to parse this header,
     * fetch necessary information and save it in structured form.
     * It makes this information more useful and analyzable.
     * Each field-member of this class represents corresponding field
     * in debug-info string and has the same name.
     * Based on StructValue class.
     */
    class DebugInfo
      :public StructValue
    {
    public:
      // ns-lookup Debug-Info
      TriggersList                 triggers;
      SimpleValue                  last_request_time;
      SimpleValue                  request_id;
      SimpleValue                  location;
      SimpleValue                  session_start;
      SimpleValue                  action_revenue;
      SimpleValue                  fraud_request;
      SimpleValue                  ccid;
      SimpleValue                  click_revenue;
      SimpleValue                  cmpid;
      SimpleValue                  colo_id;
      StringListValue              context_words;
      SimpleValue                  country;
      SimpleValue                  creative_size;
      StringListValue              day_ccg_appear;
      StringListValue              history_channels;
      SimpleValue                  imp_revenue;
      SimpleValue                  ip;
      SimpleValue                  merge_uid;
      SimpleValue                  optout;
      SimpleValue                  referer;
      SimpleValue                  revenue;
      SimpleValue                  search_phrase;
      StringListValue              geo_channels;
      StringListValue              device_channels;
      SelectedCreativesList        selected_creatives;
      SimpleValue                  site_id;
      SimpleValue                  site_rate_id;
      StringListValue              special_channels_effects;
      SimpleValue                  tag_id;
      SimpleValue                  test_request;
      SimpleValue                  tid;
      SimpleValue                  cpm_threshold;
      Money                        cpm_threshold_value;
      StringListValue              total_ccg_appear;
      StringListValue              trigger_channels;
      SimpleValue                  triggered_expression;
      SimpleValue                  uid;

      TimeValue                    trigger_match_time;
      TimeValue                    request_fill_time;
      TimeValue                    history_match_time;
      TimeValue                    creative_selection_time;
      // unique for short format
      SimpleValue                  ccids;
      SimpleValue                  trigger_channels_count;
      SimpleValue                  history_channels_count;
      //Is not own:
      SimpleValue                  click_url;
      SimpleValue                  html_url;
      SimpleValue                  track_pixel_url;
      SimpleValue                  action_adv_url;
      SimpleValue                  creative_request_id;

      /**
       * @brief Parse initial debug-info string
       * and initialize DebugInfo object.
       *
       * Initial string is usually fetched from AdServer response header.
       * This function parses it and saves results in internal structure.
       * @param buf initial string.
       */
      void
      parse(const std::string &buf)
      {
        const char *pos = buf.c_str();
        parse(pos, "");
      }

    protected:

      /**
       * @brief See StructValue::parse().
       *
       * It also calls fetch_exterior_() function.
       */
      bool parse (const char *&pos, const char *stoplist);

      /**
       * @brief See StructValue::construct().
       */
      void construct(ValueMap &map);

      /**
       * @brief Move some fields attributes to exterior.
       *
       * Extracts some filed's attributes
       * and makes it visible in DebugInfo class,
       * i.e. saves it as class field-members.
       */
      void fetch_exterior_();
    };
  }//namespace DebugInfo

  /**
   * @struct AssertionTraits
   * @brief Defines comparison operations for DebugInfo::SimpleValue class.
   *
   * Allows to compare two SimpleValue objects,
   * SimpleValue object with C-string or with STL-string.
   * Also allows to convert SimpleValue object to STL-string.
   */
  template<>
  struct TypeTraits<DebugInfo::SimpleValue>
  {
    /**
     * @brief Compare two SimpleValue objects.
     * @param x first compared object.
     * @param y second compared object.
     * @return whether objects are equal.
     */
    static bool equal( const DebugInfo::SimpleValue& x, const DebugInfo::SimpleValue& y )
    {
      return x.value() == y.value();
    }

    /**
     * @brief Compare SimpleValue object with STL-string.
     * @param x compared SimpleValue object.
     * @param y compared STL-string.
     * @return whether SimpleValue object is equal to STL-string.
     */
    static bool equal( const DebugInfo::SimpleValue& x, const std::string& y )
    {
      return x.value() == y;
    }

    static bool equal( const std::string& y, const DebugInfo::SimpleValue& x )
    {
      return x.value() == y;
    }

    /**
     * @brief Compare SimpleValue object with C-string.
     * @param x compared SimpleValue object.
     * @param y compared C-string.
     * @return whether SimpleValue object is equal to C-string.
     */
    static bool equal( const DebugInfo::SimpleValue& x, const char* y )
    {
      return x.value() == y;
    }

    static bool equal( const char* y, const DebugInfo::SimpleValue& x )
    {
      return x.value() == y;
    }

    /**
     * @brief Convert SimpleValue object to STL-string.
     * @param x convertible SimpleValue object.
     * @return result STL-string converted from SimpleValue object.
     */
    static const std::string& to_string( const DebugInfo::SimpleValue& x )
    {
      return x.value();
    }
  };
}//namespace AutoTest

#endif//__AUTOTESTS_COMMONS_DEBUGINFO_HPP
