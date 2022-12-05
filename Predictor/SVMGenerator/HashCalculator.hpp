#ifndef HASHCALCULATOR_HPP_
#define HASHCALCULATOR_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/Hash.hpp>

/*
  class HashCalculator
  class HashCalculatorFinalImpl
  class HashCalculatorFloatFinalImpl
  class HashCalculatorDelegateImpl
*/

typedef std::vector<std::pair<uint32_t, double> > HashArray;

struct Murmur32v3Adapter: public Generics::Murmur32v3Hasher
{
  Murmur32v3Adapter(std::size_t hash_seed)
    : Generics::Murmur32v3Hasher(hash_seed)
  {}

  template <typename Value>
  void
  add(const Value& value) noexcept
  {
    Generics::Murmur32v3Hasher::add(&value, sizeof(value));
  }

  void
  add(const std::string& value) noexcept
  {
    Generics::Murmur32v3Hasher::add(value.data(), value.size());
  }
};

struct HashCalculateParams
{
  typedef std::vector<std::string> Value;
  typedef std::vector<Value> ValueArray;

  ValueArray values;
};

typedef std::map<unsigned long, std::string>
  HashDictionary;

class HashCalculator: public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  HashCalculator(unsigned long shifter, const char* name)
    : shifter_(shifter),
      name_(name)
  {}

  virtual void
  eval_hashes(
    HashArray& result_hashes,
    Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params)
    /*throw(eh::Exception)*/ = 0;

  virtual void
  fill_dictionary(
    HashDictionary& dictionary,
    Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params,
    std::vector<String::SubString>& value_path)
    noexcept = 0;

protected:
  virtual
  ~HashCalculator() noexcept = default;

  bool
  hash_index_(uint32_t& index, uint32_t hash) const
  {
    // shifter > 0 => index > 1
    index = (hash >> shifter_) + 1;
    return true;
  }

  void
  add_dictionary_value_(
    HashDictionary& dictionary,
    uint32_t index,
    const std::vector<String::SubString>& value_path,
    const String::SubString* add_value)
  {
    auto dict_it = dictionary.find(index);
    if(dict_it == dictionary.end())
    {
      std::string key;
      for(auto path_it = value_path.begin(); path_it != value_path.end(); ++path_it)
      {
        if(path_it != value_path.begin())
        {
          key += '/';
        }

        key += path_it->str();
      }

      if(add_value)
      {
        if(!value_path.empty())
        {
          key += '/';
        }

        key += add_value->str();

        dictionary.insert(std::make_pair(index, name_ + ":" + key));
      }
    }
  }

protected:
  const unsigned long shifter_;
  const std::string name_;
};

typedef ReferenceCounting::SmartPtr<HashCalculator>
  HashCalculator_var;

// HashCalculatorFinalImpl
class HashCalculatorFinalImpl: public HashCalculator
{
public:
  HashCalculatorFinalImpl(
    unsigned long shifter,
    const char* name,
    unsigned long feature_index)
    noexcept
    : HashCalculator(shifter, name),
      feature_index_(feature_index)
  {}

  virtual void
  eval_hashes(
    HashArray& result_hashes,
    Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params)
    /*throw(eh::Exception)*/
  {
    HashCalculateParams::Value value;
    if(feature_index_ != 0xFFFFFFFF)
    {
      value = params.values[feature_index_];
    }

    if(!value.empty())
    {
      for(auto it = value.begin(); it != value.end(); ++it)
      {
        // need local hasher
        Murmur32v3Adapter hash_adapter_copy(hash_adapter);
        hash_adapter_copy.add(*it);

        uint32_t index;
        if(hash_index_(index, hash_adapter_copy.finalize()))
        {
          result_hashes.push_back(std::make_pair(index, 1));
        }
      }
    }
    else
    {
      hash_adapter.add(static_cast<uint32_t>(0));

      uint32_t index;
      if(hash_index_(index, hash_adapter.finalize()))
      {
        result_hashes.push_back(std::make_pair(index, 1));
      }
    }
  }

  void
  fill_dictionary(
    HashDictionary& dictionary,
    Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params,
    std::vector<String::SubString>& value_path)
    noexcept
  {
    HashCalculateParams::Value value;
    if(feature_index_ != 0xFFFFFFFF)
    {
      value = params.values[feature_index_];
    }

    if(!value.empty())
    {
      for(auto it = value.begin(); it != value.end(); ++it)
      {
        // need local hasher
        Murmur32v3Adapter hash_adapter_copy(hash_adapter);
        hash_adapter_copy.add(*it);

        uint32_t index;
        if(hash_index_(index, hash_adapter_copy.finalize()))
        {
          String::SubString val(*it);

          add_dictionary_value_(
            dictionary,
            index,
            value_path,
            &val);
        }
      }
    }
    else
    {
      hash_adapter.add(static_cast<uint32_t>(0));

      uint32_t index;
      if(hash_index_(index, hash_adapter.finalize()))
      {
        String::SubString empty_ss;
        add_dictionary_value_(
          dictionary,
          index,
          value_path,
          &empty_ss);
      }
    }
  }

protected:
  virtual
  ~HashCalculatorFinalImpl() noexcept = default;

protected:
  const unsigned long feature_index_;
};

// HashCalculatorFloatFinalImpl
class HashCalculatorFloatFinalImpl: public HashCalculator
{
public:
  HashCalculatorFloatFinalImpl(
    unsigned long shifter,
    const char* name,
    unsigned long feature_index)
    noexcept
    : HashCalculator(shifter, name),
      feature_index_(feature_index)
  {}

  virtual void
  eval_hashes(
    HashArray& result_hashes,
    Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params)
    /*throw(eh::Exception)*/
  {
    HashCalculateParams::Value value;
    if(feature_index_ != 0xFFFFFFFF)
    {
      value = params.values[feature_index_];
    }

    if(!value.empty())
    {
      for(auto it = value.begin(); it != value.end(); ++it)
      {
        // need local hasher
        Murmur32v3Adapter hash_adapter_copy(hash_adapter);

        uint32_t index;
        if(hash_index_(index, hash_adapter_copy.finalize()))
        {
          std::istringstream istr(it->c_str());
          double value;
          istr >> value;
          if(istr.fail() || !istr.eof())
          {
            //return; // temporary

            Stream::Error ostr;
            ostr << "Invalid float value: '" << *it << "'";
            throw Exception(ostr);
          }
          result_hashes.push_back(std::make_pair(index, value));
        }
      }
    }
    /*
    else
    {
      assert(0);
    }
    */
  }

  void
  fill_dictionary(
    HashDictionary& dictionary,
    Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params,
    std::vector<String::SubString>& value_path)
    noexcept
  {
    HashCalculateParams::Value value;
    if(feature_index_ != 0xFFFFFFFF)
    {
      value = params.values[feature_index_];
    }

    if(!value.empty())
    {
      for(auto it = value.begin(); it != value.end(); ++it)
      {
        // need local hasher
        Murmur32v3Adapter hash_adapter_copy(hash_adapter);

        uint32_t index;
        if(hash_index_(index, hash_adapter_copy.finalize()))
        {
          String::SubString val;

          add_dictionary_value_(
            dictionary,
            index,
            value_path,
            &val);
        }
      }
    }
    /*
    else
    {
      hash_adapter.add(static_cast<uint32_t>(0));

      uint32_t index;
      if(hash_index_(index, hash_adapter.finalize()))
      {
        String::SubString empty_ss;
        add_dictionary_value_(
          dictionary,
          index,
          value_path,
          &empty_ss);
      }
    }
    */
  }

protected:
  virtual
  ~HashCalculatorFloatFinalImpl() noexcept = default;

protected:
  const unsigned long feature_index_;
};

// HashCalculatorDelegateImpl
class HashCalculatorDelegateImpl: public HashCalculator
{
public:
  HashCalculatorDelegateImpl(
    unsigned long shifter,
    HashCalculator* next_calculator,
    unsigned long feature_index)
    noexcept
    : HashCalculator(shifter, ""),
      next_calculator_(ReferenceCounting::add_ref(next_calculator)),
      feature_index_(feature_index)
  {}

  virtual void
  eval_hashes(
    HashArray& result_hashes,
    Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params)
    /*throw(eh::Exception)*/
  {
    HashCalculateParams::Value value;
    if(feature_index_ != 0xFFFFFFFF)
    {
      value = params.values[feature_index_];
    }

    if(!value.empty())
    {
      for(auto it = value.begin(); it != value.end(); ++it)
      {
        Murmur32v3Adapter hash_adapter_copy(hash_adapter);
        hash_adapter_copy.add(*it);
        next_calculator_->eval_hashes(
          result_hashes,
          hash_adapter_copy,
          params);
      }
    }
    else
    {
      hash_adapter.add(static_cast<uint32_t>(0));
      next_calculator_->eval_hashes(
        result_hashes,
        hash_adapter,
        params);
    }
  }

  void
  fill_dictionary(
    HashDictionary& dictionary,
    Murmur32v3Adapter& hash_adapter,
    const HashCalculateParams& params,
    std::vector<String::SubString>& value_path)
    noexcept
  {
    HashCalculateParams::Value value;
    if(feature_index_ != 0xFFFFFFFF)
    {
      value = params.values[feature_index_];
    }

    if(!value.empty())
    {
      for(auto it = value.begin(); it != value.end(); ++it)
      {
        Murmur32v3Adapter hash_adapter_copy(hash_adapter);
        hash_adapter_copy.add(*it);
        value_path.push_back(*it);
        next_calculator_->fill_dictionary(
          dictionary,
          hash_adapter_copy,
          params,
          value_path);
        value_path.pop_back();
      }
    }
    else
    {
      hash_adapter.add(static_cast<uint32_t>(0));
      value_path.push_back(String::SubString());
      next_calculator_->fill_dictionary(
        dictionary,
        hash_adapter,
        params,
        value_path);
      value_path.pop_back();
    }
  }
  
protected:
  virtual
  ~HashCalculatorDelegateImpl() noexcept = default;

protected:
  const HashCalculator_var next_calculator_;
  const unsigned long feature_index_;
};

#endif /*HASHCALCULATOR_HPP_*/
