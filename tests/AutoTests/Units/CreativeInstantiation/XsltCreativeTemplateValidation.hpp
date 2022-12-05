#ifndef _UNITTEST__XSLTCREATIVETEMPLATEVALIDATION_
#define _UNITTEST__XSLTCREATIVETEMPLATEVALIDATION_

 
#include <tests/AutoTests/Commons/Common.hpp>
 
/**
 * @class XsltCreativeTemplateValidation
 * @brief tests xslt templates validation
 */
class XsltCreativeTemplateValidation: public BaseUnit
{
public:
 
  XsltCreativeTemplateValidation(
      UnitStat& stat_var, 
      const char* task_name, 
      XsdParams params_var):
    BaseUnit(stat_var, task_name, params_var)
  {};
 
  virtual ~XsltCreativeTemplateValidation() noexcept
  {};
 
private:
 
  virtual bool run_test();
 
};

#endif
 
