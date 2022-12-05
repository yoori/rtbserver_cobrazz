// @file LogGeneratorMain.hpp
#ifndef LOG_GENERATOR_MAIN_HPP
#define LOG_GENERATOR_MAIN_HPP

#include <vector>
#include <set>
#include <string>
#include <iostream>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef std::vector<std::size_t> Schedule;
  Schedule test_schedule;

  template <typename ArrayType, size_t Count>
  const ArrayType*
  endof(const ArrayType (&array)[Count])
  {
    return &array[0] + Count;
  }

  class Path : public std::string
  {
  public:
    Path() : std::string("./"), installed_(false)
    {}
    void
    set(const String::SubString& new_path)
    {
      if (!new_path.empty() && new_path != "." && new_path != "./")
      {
        installed_ = true;
        new_path.assign_to(*this);
      }
    }
    bool
    installed() const
    {
      return installed_;
    }
  private:
    bool installed_;
  } root_path;

  struct TimingResult
  {
    bool
    operator >(const TimingResult& right) const
    {
      return time > right.time ||
        (time == right.time && strcmp(log_name, right.log_name) > 0);
    }
    void
    operator ()(const TimingResult& item)
    {
      total_time += item.time;
      total_time_abs += item.time_abs;
      std::cout << prefix << std::setfill('.')
        << std::setw(40) << std::left << (std::string(item.log_name) + "':")
        << item.time << ", full time: " << item.time_abs << std::endl;
    }

    const char* log_name;
    Generics::Time time;
    Generics::Time time_abs;
    static const char* prefix;
    static Generics::Time total_time;
    static Generics::Time total_time_abs;
  };

  typedef std::set<TimingResult, std::greater<TimingResult> > Results;

/**
 * Unified interface to store objects in array and call any set of tests
 */
struct Prober : public ReferenceCounting::AtomicImpl
{
  virtual void
  init() = 0;

  static void
  s_init(std::size_t i);

  virtual void
  load_log(const char* name) /*throw(eh::Exception)*/ = 0;

  virtual int
  probe_log(bool gen_req) noexcept = 0;

  virtual const char*
  name() const noexcept = 0;

  // CPUTimer
  Generics::Time fill_time;
  Generics::Time save_time;
  Generics::Time distrib_save_time;
  Generics::Time load_time;
  // simple Timer
  Generics::Time fill_time_abs;
  Generics::Time save_time_abs;
  Generics::Time distrib_save_time_abs;
  Generics::Time load_time_abs;
protected:
  virtual
  ~Prober() noexcept {};
};

typedef ReferenceCounting::SmartPtr<Prober> Prober_var;

#endif // LOG_GENERATOR_MAIN_HPP

