
#ifndef __CONSTRAINT_HPP
#define __CONSTRAINT_HPP

#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <sstream>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>

class BaseConstraint :
  public virtual ReferenceCounting::Interface,
  public virtual ReferenceCounting::DefaultImpl<>
{
public:
  BaseConstraint(const char* name_,
                 const char* description_);

  virtual ~BaseConstraint() noexcept;

  virtual bool check() = 0;

  virtual const std::string& error() const;

const std::string name;
const std::string description;

protected:
  std::string error_;
};

class ConstraintElement : public BaseConstraint
{
 
  struct PercentagePair
  {
    unsigned long numerator;
    unsigned long denominator;
  };

public:
  
  DECLARE_EXCEPTION(InvalidSequence, eh::DescriptiveException);
  
public:
  ConstraintElement(const char* name_,
                    const char* description_,
                    const unsigned long sampling_size,
                    const unsigned long threshold_value);
  
  virtual ~ConstraintElement() noexcept;
  
  virtual void push(unsigned long numerator,
                    unsigned long denominator) /*throw(InvalidSequence)*/;
  
  bool check();
  
private:
  unsigned long sampling_size_;
  unsigned long threshold_value_;
  unsigned long sampling_begin_idx_;
  unsigned long sampling_end_idx_;
  unsigned long current_denominator_;
  std::vector <PercentagePair> stats_;

  void detect_error();
  unsigned long _dec_index(unsigned long index);
  unsigned long _incr_index(unsigned long index);

protected:
  bool error_detected_;
};


typedef ReferenceCounting::SmartPtr <BaseConstraint> Constraint_var;
typedef std::list <Constraint_var> ConstraintsList;

class ConstraintsContainer : public BaseConstraint
{
   
public:
  ConstraintsContainer(const char* name_,
                       const char* description_);

  ~ConstraintsContainer() noexcept;

  void register_constraint(Constraint_var& constraint);

  bool check();
    
private:
  ConstraintsList constraints_;
};



#endif  // __CONSTRAINT_HPP
