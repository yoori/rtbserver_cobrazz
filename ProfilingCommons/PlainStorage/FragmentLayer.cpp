// @file FragmentLayer.cpp

#include "FileLayer.hpp"
#include "DefaultAllocatorLayer.hpp"
#include "FragmentLayer.hpp"

/**
 * Test that the templates are instantiated
 */
void
fun()
{
  typedef
    ReferenceCounting::SmartPtr<
      PlainStorage::WriteFragmentLayer<PlainStorage::FileBlockIndex> >
    FragmentLayer_var;

  PlainStorage::WriteFileLayer_var file_layer(
    new PlainStorage::WriteFileLayer(
      "test.txt",
      Sys::File::get_page_size(),
      PlainStorage::WriteFileLayer::OT_OPEN_OR_CREATE));

  PlainStorage::DefaultAllocatorLayer_var allocator_layer(
    new PlainStorage::DefaultAllocatorLayer(file_layer));

  FragmentLayer_var
    frag_layer(
      new PlainStorage::WriteFragmentLayer<PlainStorage::FileBlockIndex>(
        allocator_layer,
        allocator_layer));
}
