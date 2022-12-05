
#include <Stream/MemoryStream.hpp>
#include "ListSelectorPolicy.hpp"
#include <sys/types.h>
#include <dirent.h>


// Utils

const unsigned long MAX_PARAMVALUE_LENGTH = 540;

std::string make_filepath(const char* filename,
                          const char* basepath)
{
  if (!basepath)
    {
      return filename;
    }
  std::string path(basepath);
  std::size_t lastslash = path.rfind('/');
  if (lastslash == std::string::npos)
    {
      return filename;
    }
  std::string filepath = path.substr(0, lastslash+1);
  filepath+= filename;
  return filepath;
}

bool is_rel_path(const char* filename)
{
  if (!strlen(filename))
    {
      return true;
    }
  if (*filename == '/')
    {
      return false;
    }
  return true;
}

bool need_set(unsigned long request_index,
              unsigned long percentage)
{
  unsigned long last_index = percentage > 100? 100: percentage;
  unsigned long current_index = (request_index + 1) % 100;
  return (current_index < last_index);
}

// ConfigFileList class

ConfigFileList::ConfigFileList(const char* filepath,
                               const char* configpath) /*throw(InvalidList)*/
{
  std::fstream file;
  // May be we got relative config list file path
  std::string path(filepath);
  if (is_rel_path(filepath))
    {
      // Config list file path relative to XML-config path
      path = make_filepath(filepath, configpath);
    }
  file.open(path.c_str(), std::ios::in);
  
  if(!file.is_open())
  {
    Stream::Error ostr;
    ostr << "Error: can't open file " << path;
    throw InvalidList(ostr);
  }
  
  while(true)
  {
    std::string line;
    std::getline(file, line);
    if(line.empty()) break;
    list_.push_back(line);
  }
  file.close();
}

// ConfigDirFilesList

ConfigDirFilesList::ConfigDirFilesList(const char* filespath,
                                       const char* fileext,
                                       const char* configpath) /*throw(InvalidList)*/
{
  // Prepare files path
  std::string path(filespath);
  if (is_rel_path(filespath))
  {
    path = make_filepath(filespath, configpath);
  }

  // Directory listing
  DIR *dp;
  struct dirent *ep;     
  dp = opendir (path.c_str());

  if (dp != NULL)
  {
    while ( ( ep = readdir (dp) ) )
    {
      std::string f_path = path + "/" + ep->d_name;
      // Process file
      if (f_path.substr(f_path.find_last_of(".") + 1) == fileext)
      {
        std::fstream file;
        file.open(f_path.c_str(), std::ios::in);
        if(!file.is_open())
        {
          Stream::Error ostr;
          ostr << "Error: can't open file " << f_path;
          throw InvalidList(ostr);
        }

        // Get text
        std::ostringstream text;
        while (file.good())     
        {
          char c = file.get();
          if (file.good()) text << c;
        }

        list_.push_back(text.str());
        
        file.close();
        
      }

    }
    
    (void) closedir (dp);
  }
  else
  {
    Stream::Error ostr;
    ostr << "Couldn't open the directory" << path;
    throw InvalidList(ostr);
  }

}


// ConfigXmlList class

ConfigXmlList::ConfigXmlList(const container& container)
{
  unsigned long size = container.size();
  for (unsigned long i=0; i < size; i++)
    {
      list_.push_back(container[i]);
    }
}


// DefaultSelectorPolicy class
DefaultSelectorPolicy::DefaultSelectorPolicy(
  const String::SubString& entity_name, unsigned long empty_prc,
  const String::SubString& defs)
  : SelectorPolicy(entity_name, empty_prc), defs_(defs.str())
{
}

DefaultSelectorPolicy::~DefaultSelectorPolicy() noexcept
{
}

void DefaultSelectorPolicy::get_(std::string& value, unsigned short)
{
  value = defs_;
}


// RandomSelectorPolicy class
RandomSelectorPolicy::RandomSelectorPolicy(
  const String::SubString& entity_name, unsigned long empty_prc,
  const ConfigList_var& list)
  : SelectorPolicy(entity_name, empty_prc), list_(list)
{
}

RandomSelectorPolicy::~RandomSelectorPolicy() noexcept
{
}
  
void RandomSelectorPolicy::get_(std::string& value,
                                unsigned short flags)
{
  if (!list_->size()) return;
  unsigned long index = Generics::safe_rand(list_->size());
  list_->get(index, value);
  if ( !(flags & PO_NO_NEED_ENCODE) )
  {
    String::StringManip::mime_url_encode(value, value);
  }
}

// RandomSetSelectorPolicy class
RandomSetSelectorPolicy::RandomSetSelectorPolicy(
  const String::SubString& entity_name, unsigned long empty_prc,
  const ConfigList_var& list, unsigned short max_set_size, bool random_size)
  : RandomSelectorPolicy(entity_name,  empty_prc, list),
    max_set_size_(max_set_size? max_set_size : DEFAULT_SIZE),
    random_size_(random_size)
{
}

RandomSetSelectorPolicy::~RandomSetSelectorPolicy() noexcept
{
}
  
void RandomSetSelectorPolicy::get_(std::string& value,
                                   unsigned short flags)
{

  unsigned short size = max_set_size_;
  
  if (random_size_) size = Generics::safe_rand(1, max_set_size_);
  value = "";
  unsigned long value_len = 0;
  for (unsigned short i=0; i<size; i++)
  {
    std::string i_value;
    RandomSelectorPolicy::get_(i_value, flags);
    value_len+=i_value.length();
    // Do not break parameter value length constraint
    if (value_len + 1 >= MAX_PARAMVALUE_LENGTH) break;
    if (i)
    {
      value+= ",";
      value_len++;
    }
    value+=i_value;
  }
}



// SelectorPolicy class
const String::SubString SelectorPolicy::RANDOM_FUNC("random");
const String::SubString SelectorPolicy::SEQ_FUNC("sequence");


SelectorPolicy::SelectorPolicy(const String::SubString& entity_name,
  unsigned long empty_prc)
  : entity_name(entity_name.str()), empty_prc_(empty_prc), request_count_(0)
{
}
    
SelectorPolicy::~SelectorPolicy() noexcept
{
}

void SelectorPolicy::get(std::string& value,
                         unsigned short flags)
{
  if ( !(flags & PO_ALWAYS_SET) && need_set(request_count_++, empty_prc_)) return;
  get_(value, flags);
}


SelectorPolicy*
SelectorPolicy::make_policy(
  const String::SubString& entity_name,
  const String::SubString& value,
  const RequestLists& lists, unsigned long empty_prc, bool is_cookie)
  /*throw(InvalidConfigRequestData)*/
{
  String::RegEx::Result result;
  String::RegEx re(String::SubString("#([\\w]+):?([\\w|\\-]*)=?([\\d]*)#"));
  String::SubString list_name(entity_name);
  String::SubString policy_name(entity_name);
  unsigned short seq_size(0);
  String::SubString function_name;
  if (re.search(result, value))
  {
    if (result.size() != 4)
    {
      Stream::Error ostr;
      ostr << "'" << entity_name << "' have invalid format: "<< value;
      throw InvalidConfigRequestData(ostr);
    }
    function_name = result[1];
    list_name = result[2];
    if (!String::StringManip::str_to_int(result[3], seq_size))
    {
      seq_size = 0;
    }
    if (result[2].empty())
    {
      function_name = RANDOM_FUNC;
      list_name = result[1];
    }
    if (function_name != RANDOM_FUNC && function_name != SEQ_FUNC )
    {
      Stream::Error ostr;
      ostr << "'" << entity_name << "' using invalid function: "<< function_name;
      throw InvalidConfigRequestData(ostr);
    }
  }
  if (is_cookie)
  {
    policy_name = list_name;
  }
  if (value.empty() || !function_name.empty())
  {
    RequestLists::const_iterator list_it = lists.find(list_name.str());
    if (list_it == lists.end())
    {
      Stream::Error ostr;
      ostr << "'" << entity_name << "' list '" << list_name << "' not found in config";
      throw InvalidConfigRequestData(ostr);
    }
    if (value.empty() || seq_size > 0)
    {
      if (function_name == RANDOM_FUNC)
      {
        return new RandomSetSelectorPolicy(policy_name, empty_prc,
          list_it->second, seq_size);
      }
      else
      {
        return new RandomSetSelectorPolicy(policy_name, empty_prc,
          list_it->second, seq_size, false);
      }
    }
    return new RandomSelectorPolicy(policy_name, empty_prc, list_it->second);
  }
  return new DefaultSelectorPolicy(policy_name, empty_prc, value);
}


// Cookie utils

void make_cookie_policy_list(SelectorPolicyList& cookie_policies,
                             const String::SubString& value,
                             const RequestLists& lists)
  /*throw(SelectorPolicy::InvalidConfigRequestData)*/
{
  String::RegEx::Result cookies;
  String::RegEx cookie_re(String::SubString("(#[\\w|:]+#)"));
  cookie_re.gsearch(cookies, value);
  unsigned int cookie_index = 0;
  for (String::RegEx::Result::const_iterator it = cookies.begin();
    it != cookies.end(); ++it)
  {
    std::ostringstream ostr;
    ostr << "cookie#" << cookie_index;
    SelectorPolicy_var cookie_policy_var =
      SelectorPolicy_var(SelectorPolicy::make_policy(
        ostr.str(), *it, lists, true));
    cookie_policies.push_back(cookie_policy_var);
    cookie_index++;
  }
}
