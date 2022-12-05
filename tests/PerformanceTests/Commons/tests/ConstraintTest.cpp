

#include <assert.h>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include "../Constraint.hpp"

// ConstraintElementClass

class MyConstraint : public ConstraintElement
{
public:
  MyConstraint(const unsigned long sampling_size,
               const unsigned long threshold_value) :
    ConstraintElement("Constraint", "Testing constraint",
                      sampling_size, threshold_value)
  {}

  void push(unsigned long numerator,
            unsigned long denominator) /*throw(ConstraintElement::InvalidSequence)*/
  {
    // clear error before new element pushing
    error_ = "";
    error_detected_ = false;
    ConstraintElement::push(numerator, denominator);
  }
};

  
void check_sequence(ConstraintElement& constraint,
                    unsigned long numerator,
                    unsigned long denominator)
{
  try
    {
      constraint.push(numerator, denominator);
      assert(false);
    }
  catch (ConstraintElement::InvalidSequence& e)
    {
      // it is ok
    }
}

int main()
{
  MyConstraint constraint(10, 50);
  // simple rule check - ok
  constraint.push(1, 2);
  constraint.push(2, 4);
  assert(constraint.check());

  // simple rule check - failed
  constraint.push(5, 7);
  assert(!constraint.check());

  // aggregative rule check - ok
  constraint.push(7, 15); // 5 / 11 < 0.5
  constraint.check();
  assert(constraint.check());
  
  // aggregative rule check - failed
  constraint.push(8, 16); // 6 / 12 = 0.5
  assert(!constraint.check());

  // array size = sampling size
  // only last <sampling size> records stored in array
  constraint.push(9, 17);
  constraint.push(10, 20);
  constraint.push(11, 27);
  constraint.push(12, 30);
  constraint.push(13, 31);
  constraint.push(14, 32);
  constraint.push(15, 33);
  // aggregative rule check - ok  
  assert(constraint.check()); // 5 / 13 < 0.5

  // aggregative rule check - failed  
  constraint.push(16, 34);
  constraint.push(18, 36); 
  assert(!constraint.check()); // 8 / 16 = 0.5

  // wrong sequence testing
  check_sequence(constraint, 17, 36); // wrong sequence
  check_sequence(constraint, 19, 35); // wrong sequence
  constraint.push(18, 36); // valid sequence

  // denominator not changed
  for (int i = 0; i < 10; i++)
    {
      constraint.push(18 + (i / 2), 36);
    }
  assert(constraint.check()); // 4 / 10 < 0.5

  constraint.push(23, 36);

  assert(!constraint.check()); // 5 / 10 < 0.5
  
  std::cout << "Test was successfully done!" << std::endl;

  return 0;  
}
