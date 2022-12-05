
template<typename InsertIterator>
InsertIterator
BaseUnit::fetch_int_sequence(
  InsertIterator ins_it,
  const std::string& obj_name,
  const char* separators)
  /*throw(Exception)*/
{
  std::string sval = get_object_by_name(obj_name).Value();
  const String::AsciiStringManip::CharCategory SEP(separators);
  String::StringManip::CharSplitter tokenizer(sval, SEP);
  String::SubString token;
  while(tokenizer.get_token(token))
  {
    unsigned long val;
    if(String::StringManip::str_to_int(token, val))
    {
      *ins_it++ = val;
    }
  }

  return ins_it;
}

template < 
  template <class T, class A> class container,
  template <class T> class allocator,
  class Value>
void
BaseUnit::fetch(
  std::insert_iterator< container<Value, allocator<Value> > > &iterator,
  const std::string& obj_name)
  /*throw(Exception)*/
{
  Value v;
  BaseUnit::fetch(v, obj_name);
  *iterator = v;
}

template<typename InsertIterator>
InsertIterator
BaseUnit::fetch_objects(
  InsertIterator iterator,
  const char* obj_names,
  const char* separator)
  /*throw(Exception)*/
{
  const String::AsciiStringManip::CharCategory SEP(separator);
  String::StringManip::CharSplitter tokenizer(
    String::SubString(obj_names), SEP);
  String::SubString token;
  while(tokenizer.get_token(token))
  {
    fetch(*iterator++, token.str());
  }

  return iterator;
}

template <class T>
void
BaseUnit::add_wait_checker(
  const std::string& description,
  const T& checker) /*throw(eh::Exception)*/
{
  add_checker(description,
    AutoTest::WaitChecker<T>(checker));
}

template<typename CheckerType>
void
BaseUnit::add_checker(
  const std::string& description,
  const CheckerType& checker) /*throw(eh::Exception)*/
{
  checkers_.push_back(
    DescriptiveChecker(
      description,
      AutoTest::Logger::thlog(),
      new AutoTest::Internals::CheckerHolderImpl<CheckerType>(checker)));
}
