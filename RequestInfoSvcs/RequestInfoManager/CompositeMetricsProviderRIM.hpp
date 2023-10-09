#ifndef ___CompositeMetricsProviderRIM
#define ___CompositeMetricsProviderRIM

#include <Generics/CompositeMetricsProvider.hpp>

class CompositeMetricsProviderRIM: public CompositeMetricsProvider
{
    
public:
    typedef ReferenceCounting::SmartPtr<CompositeMetricsProviderRIM> CompositeMetricsProviderRIM_var;
    void set_child_processors(const std::string &className, size_t sz)
    {
      std::map<std::string,std::string> m;
      m["class"]=className;
      cmp->set_value_prometheus("child_processors",m, sz);
    }

};


#endif