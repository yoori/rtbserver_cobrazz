// @file PlainStorage/TransientPropertyLayerImpl.hpp

#ifndef _TRANSIENTPROPERTYLAYERIMPL_HPP_
#define _TRANSIENTPROPERTYLAYERIMPL_HPP_

#include "BaseLayer.hpp"

namespace PlainStorage
{
  /**
   * TransientPropertyReadLayerImpl class
   * delegate get_property call to next_layer
   */
  template<typename LocalIndexType, typename NextIndexType>
  class TransientPropertyReadLayerImpl:
    public virtual ReadBaseLayer<LocalIndexType>
  {
  public:
    TransientPropertyReadLayerImpl(ReadBaseLayer<NextIndexType>* next_layer) noexcept;

    virtual
    bool
    get_property(const char* name, PropertyValue& property)
      /*throw(BaseException)*/;

    /**
     * Accessor to next_layer_
     */
    ReadBaseLayer<NextIndexType>* next_layer() const noexcept;

  private:
    ReferenceCounting::SmartPtr<ReadBaseLayer<NextIndexType> > next_layer_;
  };

  /**
   * TransientPropertyWriteLayerImpl class
   * delegate get_property, set_property calls to next_layer
   */
  template<typename LocalIndexType, typename NextIndexType>
  class TransientPropertyWriteLayerImpl:
    public virtual WriteBaseLayer<LocalIndexType>,
    public virtual TransientPropertyReadLayerImpl<
      LocalIndexType, NextIndexType>
  {
  public:
    TransientPropertyWriteLayerImpl(
      WriteBaseLayer<NextIndexType>* next_layer) noexcept;

    virtual
    void
    set_property(const char* name, const PropertyValue& property)
      /*throw(BaseException)*/;

    /**
     * Accessor to next_layer_
     */
    WriteBaseLayer<NextIndexType>* next_layer() const noexcept;

  private:
    ReferenceCounting::SmartPtr<WriteBaseLayer<NextIndexType> > next_layer_;
  };

}

namespace PlainStorage
{
  /**
   * TransientPropertyReadLayerImpl implementation
   */
  template<typename LocalIndexType, typename NextIndexType>
  TransientPropertyReadLayerImpl<LocalIndexType, NextIndexType>::TransientPropertyReadLayerImpl(
    ReadBaseLayer<NextIndexType>* next_layer) noexcept
    : next_layer_(ReferenceCounting::add_ref(next_layer))
  {}

  template<typename LocalIndexType, typename NextIndexType>
  bool
  TransientPropertyReadLayerImpl<LocalIndexType, NextIndexType>::get_property(
    const char* name, PropertyValue& property)
    /*throw(BaseException)*/
  {
    return next_layer_->get_property(name, property);
  }

  template<typename LocalIndexType, typename NextIndexType>
  ReadBaseLayer<NextIndexType>*
  TransientPropertyReadLayerImpl<LocalIndexType, NextIndexType>::next_layer()
    const noexcept
  {
    return next_layer_;
  }

  /**
   * TransientPropertyWriteLayerImpl implementation
   */
  template<typename LocalIndexType, typename NextIndexType>
  TransientPropertyWriteLayerImpl<LocalIndexType, NextIndexType>::
  TransientPropertyWriteLayerImpl(
    WriteBaseLayer<NextIndexType>* next_layer) noexcept
    : TransientPropertyReadLayerImpl<LocalIndexType, NextIndexType>(next_layer),
      next_layer_(ReferenceCounting::add_ref(next_layer))
  {}

  template<typename LocalIndexType, typename NextIndexType>
  void
  TransientPropertyWriteLayerImpl<LocalIndexType, NextIndexType>::set_property(
    const char* name, const PropertyValue& property)
    /*throw(BaseException)*/
  {
    next_layer_->set_property(name, property);
  }

  template<typename LocalIndexType, typename NextIndexType>
  WriteBaseLayer<NextIndexType>*
  TransientPropertyWriteLayerImpl<LocalIndexType, NextIndexType>::next_layer()
    const noexcept
  {
    return next_layer_;
  }

}

#endif // _TRANSIENTPROPERTYLAYERIMPL_HPP_
