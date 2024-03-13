#ifndef ___CompositeMetricsProviderRIM
#define ___CompositeMetricsProviderRIM

//#include <Generics/CompositeMetricsProvider.hpp>

class CompositeMetricsProviderRIM: public ReferenceCounting::AtomicImpl //: public Generics::CompositeMetricsProvider
{
};

typedef ReferenceCounting::SmartPtr<CompositeMetricsProviderRIM> CompositeMetricsProviderRIM_var;


#endif
