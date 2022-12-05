#ifndef SEGMENTUTIL_HPP_
#define SEGMENTUTIL_HPP_

#include <map>
#include <set>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>
#include <Generics/Time.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  Application_() noexcept;

  virtual ~Application_() noexcept;

  void main(int& argc, char** argv) /*throw(eh::Exception)*/;

protected:
  typedef std::map<std::string, std::vector<uint32_t> > Dictionary;

  struct SegmentRule
  {
    Generics::Time time_to;
    unsigned long min_visits;
  };

  typedef std::vector<SegmentRule> SegmentRuleArray;

  typedef std::map<uint32_t, std::string> HashNameDictionary;

  struct HashDescr
  {
    HashDescr(unsigned long base_hash_val, const SegmentRule* rule_val)
      : base_hash(base_hash_val),
        rule(rule_val)
    {}

    unsigned long base_hash;
    const SegmentRule* rule;
  };

protected:
  void
  print_profile_(
    const char* user_id,
    const char* filename)
    /*throw(eh::Exception)*/;

  void
  print_profile_from_buf_(
    const char* user_id,
    const Generics::MemBuf& buf)
    noexcept;

  void
  make_profile_(
    std::istream& in,
    const char* filename,
    const char* columns,
    unsigned long level0_size)
    /*throw(eh::Exception)*/;

  void
  request_profile_(
    std::istream& in,
    const char* filename,
    const char* columns,
    const SegmentRuleArray& segment_config,
    HashNameDictionary* res_hash_dictionary,
    const HashNameDictionary* base_hash_dictionary,
    bool svm_format,
    std::unordered_set<unsigned long>* filter_features,
    unsigned long dimension)
    /*throw(eh::Exception)*/;

  void
  parse_hashes_(
    std::vector<uint32_t>& res,
    const String::SubString& str);

  void
  load_hash_dictionary_(
    std::map<std::string, std::vector<uint32_t> >& dict,
    const char* file);

  void
  save_hash_name_dictionary_(
    const char* file_name,
    const HashNameDictionary& dict);

  void
  load_hash_name_dictionary_(
    HashNameDictionary& dict,
    const char* file);
};

typedef Generics::Singleton<Application_> Application;

#endif /*SEGMENTUTIL_HPP_*/
