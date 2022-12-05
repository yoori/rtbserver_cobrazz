/**
 * @file PlainStorage/RecordLayerUtils.hpp
 * Util functions
 * for operate posed block sequences
 */

#ifndef _RECORD_LAYER_UTILS_HPP_
#define _RECORD_LAYER_UTILS_HPP_

#include "BaseLayer.hpp"

namespace PlainStorage
{
  /**
   * read_block_seq
   *   read content of block sequences into buffer
   *   throw BaseException if read_size != sum size of blocks
   * @param read_blocks The data blocks to read
   * @param buffer Allocated storage to get read data, read destination.
   * @param read_size Size of allocated storage and size of read data portion
   */
  template <typename PosedBlockListType>
  void
  read_block_seq(
    PosedBlockListType& read_blocks,
    void* buffer,
    SizeType read_size)
    /*throw(BaseException, CorruptedRecord)*/;

  /**
   * read_block_seq_part
   *   read part of block sequence content
   *   from pos with size = read_size
   * @param read_blocks The data blocks container to read
   * @param pos The position of the original piece of memory from which to read data
   * @param buffer Allocated storage to get read data, read destination.
   * @param read_size Size of allocated storage and size of read data portion
   */
  template <typename PosedBlockListType, typename LessOpType>
  void
  read_block_seq_part(
    PosedBlockListType& read_blocks,
    SizeType pos,
    void* buffer,
    SizeType read_size)
    /*throw(BaseException, CorruptedRecord)*/;

  /**
   * write_block_seq():
   *   fill WriteBlock sequence by buffer content
   *   write_blocks must be allocated for write_size
   *   if its sum size != write_size method throw BaseException
   * @param write_blocks The sequence of blocks, destination of writing
   * @param buffer The pointer to the recording data
   * @param write_size The size of recording data
   */
  template <typename PosedBlockListType>
  void
  write_block_seq(
    PosedBlockListType& write_blocks,
    const void* buffer,
    SizeType write_size)
    /*throw(BaseException, CorruptedRecord)*/;

  /**
   * write_block_seq_part
   *   rewrite part of block sequence content
   *   in position = pos with size = write_size by buffer content
   * @param write_blocks The data blocks container for writing
   * @param pos The position of the original piece of memory where to write data
   * @param buffer Allocated storage to get write data, write source.
   * @param write_size Size of allocated storage and size of write data portion
   */
  template <typename PosedBlockListType, typename LessOpType>
  void
  write_block_seq_part(
    PosedBlockListType& write_blocks,
    SizeType pos,
    const void* buffer,
    SizeType write_size)
    /*throw(BaseException, CorruptedRecord)*/;

}

#include "RecordLayerUtils.tpp"

#endif /*_RECORD_LAYER_UTILS_HPP_*/
