#ifndef ___CompositeMetricsProviderRIM
#define ___CompositeMetricsProviderRIM

#include <Generics/CompositeMetricsProvider.hpp>

class CompositeMetricsProviderRIM: public Generics::CompositeMetricsProvider
{
    
public:


//    void set_child_processors(const std::string &className, size_t sz)
//    {
//      std::map<std::string,std::string> m;
//      m["class"]=className;
//      set_value_prometheus("child_processors",m, sz);
//    }
//    void set_cmp_channels(const std::string &className, size_t sz)
//    {
//      std::map<std::string,std::string> m;
//      m["class"]=className;
//      set_value_prometheus("cmp_channels",m, sz);
//    }

    void add_container(const std::string &className,const std::string &containerName, size_t sz)
    {
      std::map<std::string,std::string> m;
      m["class"]=className;

      add_value_prometheus(containerName,m, sz);
    }
    void sub_container(const std::string &className,const std::string &containerName, size_t sz)
    {
      std::map<std::string,std::string> m;
      m["class"]=className;

      add_value_prometheus(containerName,m, - sz);
    }

};
typedef ReferenceCounting::SmartPtr<CompositeMetricsProviderRIM> CompositeMetricsProviderRIM_var;


#endif
