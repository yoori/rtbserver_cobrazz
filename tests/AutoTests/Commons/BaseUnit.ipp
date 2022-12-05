
//
// UnitStat inlines
//
inline
bool
UnitStat::db_active() const noexcept
{
  return !(flags & UM_NO_DB);
}

//
// BaseUnit inlines
//
inline
Params
BaseUnit::get_global_params()
  noexcept
{
  return params_.get_global_params();
}

inline
const GlobalConfig& BaseUnit::get_config() const
  noexcept
{
  return params_.get_config();
}

inline
Locals
BaseUnit::get_local_params()
  noexcept
{
  return params_.get_local_params();
}

inline
std::string
BaseUnit::fetch_string(const char* obj_name)
{
  ::xsd::tests::AutoTests::ValueType v =
    get_object_by_name(obj_name).Value();
  if (v.base64())
  {
    std::string ret;
    String::StringManip::base64mod_decode(
      ret, v, false);
    return ret;
  }
  return v;
}

inline
std::string
BaseUnit::fetch_string(const std::string& obj_name)
{
  return fetch_string(obj_name.c_str());
}

inline
unsigned long
BaseUnit::fetch_int(const std::string& obj_name)
    /*throw(Exception)*/
{
  return fetch_int(obj_name.c_str());
}

inline
long double
BaseUnit::fetch_float(const std::string& obj_name)
  /*throw(Exception)*/
{
  return fetch_float(obj_name.c_str());
}

inline
void
BaseUnit::fetch(
  long double& v,
  const std::string& obj_name)
  /*throw(Exception)*/
{
  v = fetch_float(obj_name);
}

inline
void
BaseUnit::fetch(
  unsigned long& v,
  const std::string& obj_name)
  /*throw(Exception)*/
{
  v = fetch_int(obj_name);
}

inline
void
BaseUnit::fetch(
  std::string& v,
  const std::string& obj_name)
  /*throw(Exception)*/
{
  v = fetch_string(obj_name);  
}
