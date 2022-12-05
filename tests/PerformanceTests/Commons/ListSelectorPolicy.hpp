
#ifndef __LISTSELECTORPOLICY_HPP
#define __LISTSELECTORPOLICY_HPP

#include <vector>
#include <iostream>
#include <fstream>
#include <list>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <xsd/tests/PerformanceTests/Commons/AdServerTestCommonConfig.hpp>
#include <String/RegEx.hpp>
#include <String/StringManip.hpp>

#include <Generics/Rand.hpp>
#include <Sync/SyncPolicy.hpp>

using namespace xsd::tests::PerformanceTests;

/**
 * @class ConfigListErrors
 * @brief Exception using for warn about errors in configuration lists .
 */
class ConfigListErrors
{
public:
  DECLARE_EXCEPTION(InvalidList, eh::DescriptiveException);
};

enum PARAM_OPTIONS
{
    PO_NO_NEED_ENCODE = 0x01,
    PO_ALWAYS_SET  = 0x02
};

/**
 * @class ConfigList
 * @brief Base class for configuration lists.
 *
 * Give parameter value for AdServer HTTP requests.
 * For more details see requests classes in the Request.hpp
 */
template <class T> class ConfigList :
  public ConfigListErrors,
  public virtual ReferenceCounting::Interface,
  public virtual ReferenceCounting::DefaultImpl<>
{

public:

  /**
    * @brief Constructor.
    */
  ConfigList() {}


  /**
   * @brief Destructor.
   */
  virtual ~ConfigList() noexcept {}

  /**
   * @brief Get parameter value by index.
   *
   * @param index [in]  parameter index.
   * @param value [out]  parameter value.
   */
  virtual void get(unsigned long index, T& value)
  {
    ReadGuard_ guard(lock_);
    value = list_[index];
  }

  /**
   * @brief Get parameter list size.
   *
   * @return parameter list size.
   */
  virtual size_t size()
  {
    ReadGuard_ guard(lock_);
    return list_.size();
  }



protected:
  typedef Sync::PosixRWLock Mutex_;
  typedef Sync::PosixRGuard ReadGuard_;

  std::vector <std::string> list_;
  mutable Mutex_ lock_;
};


/**
 * @class ConfigFileList
 * @brief Parameter values reading from text files.
 *
 * Give value for following parameters: referer_kw(w), context_kw(w), referer.
 * Values for this parameters stored in simply text files (every line is a some value).
 */
class ConfigFileList : public ConfigList <std::string>
{

public:
  /**
   * @brief Constructor.
   *
   * @param filepath text file with parameter values path.
   * @param configpath configuration XML file path.
   */
  ConfigFileList(const char* filepath,
                 const char* configpath = 0) /*throw(InvalidList)*/;
};

/**
 * @class ConfigDirFilesList
 * @brief Parameter values reading from text files.
 *
 * Give value for following parameters: ft
 * Values for this parameters stored in files.
 */
class ConfigDirFilesList : public ConfigList <std::string>
{

public:
  /**
   * @brief Constructor.
   *
   * @param filepath path to parameters files.
   * @param extension files extension.
   * @param configpath configuration XML file path.
   */
  ConfigDirFilesList(const char* filespath,
                     const char* fileext,
                     const char* configpath = 0) /*throw(InvalidList)*/;
};

/**
 * @class ConfigXmlList
 * @brief Parameter values reading from configuration XML-file string lists.
 *
 * Give value for parameters from "set" tags of XML config.
 */
class ConfigXmlList : public ConfigList <std::string>
{
  typedef ::xml_schema::string type;
  typedef ::xsd::cxx::tree::sequence< type > container;
  typedef container::iterator iterator;
  typedef container::const_iterator const_iterator;
public:
  /**
   * @brief Constructor.
   *
   * @param container reference to "set" tag type, given as Xerces-parsing result.
   */
  ConfigXmlList(const container& container);

};


typedef ConfigList<std::string> list;
typedef ReferenceCounting::SmartPtr<list> ConfigList_var;
typedef std::map <std::string, ConfigList_var> RequestLists;

/**
 * @class SelectorPolicy
 * @brief Base class for parameter values selection policy.
 *
 * Abstract.
 */
class SelectorPolicy :
  public virtual ReferenceCounting::Interface,
  public virtual ReferenceCounting::DefaultImpl<>
{
public:
  DECLARE_EXCEPTION(InvalidConfigRequestData, eh::DescriptiveException);

public:

  /**
   * @brief Constructor.
   * @param entity_name_ parameter name.
   * @param empty_prc request with absent parameter percentage.
   */
  SelectorPolicy(const String::SubString& entity_name,
                 unsigned long empty_prc);

  /**
   * @brief Get parameter value interface.
   *
   * @param value [out] parameter value.
   * @param flags [in] parameter flags.
   */
  void get(std::string& value,
           unsigned short flags = 0);


  /**
   * @brief Making selection  policy
   *
   * @param entity_name parameter name.
   * @param value text presentation of policy from XML configuration..
   * @param lists configurations map (set name -> reference to "set" tag type).
   * @param empty_prc request with absent parameter percentage.
   * @param is_cookie cookie flag, if true parameter must send as cookie.
   */
  static SelectorPolicy* make_policy(const String::SubString& entity_name,
                                     const String::SubString& value,
                                     const RequestLists& lists,
                                     unsigned long empty_prc = 0,
                                     bool  is_cookie = false) /*throw(InvalidConfigRequestData)*/;
  const std::string entity_name;

protected:
  /**
   * @brief Destructor.
   */
  virtual ~SelectorPolicy() noexcept;

  virtual void get_(std::string& value, unsigned short flags) = 0;
private:
  unsigned long empty_prc_;
  unsigned long request_count_;
  static const String::SubString RANDOM_FUNC;
  static const String::SubString SEQ_FUNC;
};

/**
 * @class DefaultSelectorPolicy
 * @brief Selection policy which always set default value for parameter.
 *
 * Give default parameter value.
 */
class DefaultSelectorPolicy : public SelectorPolicy
{
public:
  /**
   * @brief Constructor.
   *
   * @param entity_name_ parameter name.
   * @param defs default value for parameter.
   */
  DefaultSelectorPolicy(const String::SubString& entity_name,
                        unsigned long empty_prc,
                        const String::SubString& defs);

protected:
  /**
   * @brief Destructor.
   */
  ~DefaultSelectorPolicy() noexcept;

  virtual void get_(std::string& value, unsigned short flags);

private:
  std::string defs_;
};

/**
 * @class RandomSelectorPolicy
 * @brief Selection policy which set random value for parameter.
 *
 * Give random parameter value.
 */
class RandomSelectorPolicy : public SelectorPolicy
{
public:
  /**
   * @brief Constructor.
   *
   * @param entity_name_ parameter name.
   * @param empty_prc request with absent parameter percentage.
   * @param list configuration list reference (list for random selection).
   */
  RandomSelectorPolicy(const String::SubString& entity_name,
                       unsigned long empty_prc,
                       const ConfigList_var& list);

protected:
  /**
   * @brief Destructor.
   */
  ~RandomSelectorPolicy() noexcept;

  virtual void get_(std::string& value, unsigned short flags);

private:
  const ConfigList_var& list_;

};


/**
 * @class RandomSetSelectorPolicy
 * @brief Selection policy which set random values list (set) for parameter.
 *
 * Give random parameter values set.
 * Set of values example: referer_kw=foros,ocslab,test
 * (referer_kw - parameter name; foros,ocslab,test - values set)
 */
class RandomSetSelectorPolicy : public RandomSelectorPolicy
{
public:

  static const unsigned short DEFAULT_SIZE = 10;

  /**
   * @brief Constructor.
   *
   * @param entity_name_ parameter name.
   * @param empty_prc request with absent parameter percentage.
   * @param list configuration list reference (list for random selection).
   * @param max_set_size max values in the values list (set).
   * @param random_size random size flag, if true set have random size, else
   *                    set size = max_set_size
   */
  RandomSetSelectorPolicy(const String::SubString& entity_name,
                          unsigned long empty_prc,
                          const ConfigList_var& list,
                          unsigned short max_set_size,
                          bool random_size = true);

protected:
  /**
   * @brief Destructor.
   */
  ~RandomSetSelectorPolicy() noexcept;

  virtual void get_(std::string& value, unsigned short flags);

private:
  unsigned short max_set_size_;
  bool random_size_;

};


typedef ReferenceCounting::SmartPtr<SelectorPolicy> SelectorPolicy_var;
typedef std::vector<SelectorPolicy_var> SelectorPolicyList;

/**
 * @brief making cookie selection policy list
 *
 * @param cookies [out] selection policies for cookies
 * @param value [in] text presentation of policy from XML configuration.
 * @param lists [in] configurations map (set name -> reference to "set" tag type).
 */
void make_cookie_policy_list(SelectorPolicyList& cookies,
                             const String::SubString& value,
                             const RequestLists& lists)
  /*throw(SelectorPolicy::InvalidConfigRequestData)*/;


#endif  // __LISTSELECTORPOLICY_HPP
