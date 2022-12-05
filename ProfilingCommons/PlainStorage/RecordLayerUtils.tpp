/**
 * Implementations of util functions
 * for operate posed block sequences
 * @file PlainStorage/RecordLayerUtils.tpp
 */

#ifndef _RECORD_LAYER_UTILS_TPP_
#define _RECORD_LAYER_UTILS_TPP_

#include "BaseLayer.hpp"
#include "Stream/MemoryStream.hpp"

namespace PlainStorage
{
  /**
   * Functor ordered by ascending
   */
  template<typename PosedBlockType>
  struct BlockWithPosLess
  {
    bool operator()(
      const PosedBlockType& left, const PosedBlockType& right) const
    {
      return left.pos < right.pos;
    }

    bool operator()(
      const PosedBlockType& left, SizeType right) const
    {
      return left.pos < right;
    }

    bool operator()(
      SizeType left, const PosedBlockType& right) const
    {
      return left < right.pos;
    }
  };

  template <typename PosedBlockListType>
  void
  write_block_seq(
    PosedBlockListType& write_blocks,
    const void* buffer,
    SizeType write_size)
    /*throw (BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "Internal write_block_seq()";

    SizeType written_size = 0;

    for(typename PosedBlockListType::iterator it = write_blocks.begin();
        it != write_blocks.end(); ++it)
    {
      SizeType sz = it->block->size();

      if (it->data_offset > sz)
      {
        Stream::Error ostr;
        ostr << FUN << "The offset to user data=" << it->data_offset
          << " more than the block size=" << sz;
        throw CorruptedRecord(ostr);
      }

      if (it->pos >= write_size && (it->pos != write_size || it->data_offset != sz))
      {
        Stream::Error ostr;
        ostr << FUN
          << ": Overflow for block borders while do writing or "
          "incorrect fulfilled last block in sequence, "
          "buffer position=" << it->pos << ", write size=" << write_size
          << ", data offset=" << it->data_offset << ", block size="
          << sz;
        throw CorruptedRecord(ostr);
      }

      if(sz != it->data_offset)
      {
        it->block->write(
          it->data_offset,
          (const char*)buffer + it->pos,
          sz - it->data_offset);

        written_size += sz - it->data_offset;
      }
    }

    if(written_size < write_size)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't finish writing of block sequence."
           << "written size < write_size ("
           << written_size << " < " << write_size << ").";
      throw BaseException(ostr);
    }
  }

  template <typename PosedBlockListType>
  void
  read_block_seq(
    PosedBlockListType& read_blocks,
    void* buffer,
    SizeType read_size)
    /*throw (BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "read_block_seq()";

    for(typename PosedBlockListType::iterator it = read_blocks.begin();
        it != read_blocks.end(); ++it)
    {
      SizeType sz = it->block->size();

      if(read_size < it->pos + (sz - it->data_offset))
      {
        Stream::Error ostr;
        ostr << FUN << ": can't finish reading of block sequence."
             << " seq size >= "
             << it->pos + (sz - it->data_offset)
             << " > read_size = "
             << read_size << "."
             << " element: pos = " << it->pos
             << ", block-size = " << sz
             << ", data-offset = " << it->data_offset;
        throw BaseException(ostr);
      }

      if (sz < it->data_offset)
      {
        Stream::Error ostr;
        ostr << FUN << "The offset to user data=" << it->data_offset
          << " more than the block size=" << sz;
        throw CorruptedRecord(ostr);
      }

      if(sz != it->data_offset)
      {
        it->block->read(
          it->data_offset,
          (char*)buffer + it->pos,
          sz - it->data_offset);
      }
    }
  }

  template <typename PosedBlockListType, typename LessOpType>
  void
  read_block_seq_part(
    PosedBlockListType& read_blocks,
    SizeType pos,
    void* buffer,
    SizeType read_size)
    /*throw (BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "read_block_seq_part()";

    if(read_blocks.empty())
    {
      Stream::Error ostr;
      ostr << FUN << "can't read from empty block sequence.";
      throw BaseException(ostr);
    }

    typename PosedBlockListType::iterator left =
      std::upper_bound(
        read_blocks.begin(),
        read_blocks.end(),
        pos,
        LessOpType());

    typename PosedBlockListType::iterator right =
      std::upper_bound(
        left,
        read_blocks.end(),
        pos + read_size,
        LessOpType());

    if(left == read_blocks.begin() || right == read_blocks.begin())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't read from pos=" << pos <<
        ", first block start in pos=" << read_blocks.begin()->pos;
      throw BaseException(ostr);
    }

    --left; --right;

    if(left == right)
    {
      left->block->read(
        pos - left->pos + left->data_offset,
        buffer,
        read_size);
    }
    else
    {
      typename PosedBlockListType::value_type::BlockType* block = left->block;

      SizeType readed_size =
        (block->size() - left->data_offset) - (pos - left->pos);

      block->read(
        pos - left->pos + left->data_offset,
        buffer,
        readed_size);

      for(typename PosedBlockListType::iterator cur = ++left;
          cur != right; ++cur)
      {
        block = cur->block;

        SizeType sz = block->size();

        if (cur->data_offset > sz)
        {
	  Stream::Error ostr;
          ostr << FUN << "The offset to user data=" << cur->data_offset <<
            " more than the block size=" << sz;
          throw CorruptedRecord(ostr);
        }

        if(sz != cur->data_offset)
        {
          block->read(
            cur->data_offset,
            static_cast<char*>(buffer) + readed_size,
            sz - cur->data_offset);

          readed_size += sz - cur->data_offset;
        }
      }

      right->block->read(
        right->data_offset,
        static_cast<char*>(buffer) + readed_size,
        read_size - readed_size);
    }
  }

  template <typename PosedBlockListType, typename LessOpType>
  void
  write_block_seq_part(
    PosedBlockListType& write_blocks,
    SizeType pos,
    const void* buffer,
    SizeType write_size)
    /*throw (BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "write_block_seq_part()";

    if(write_blocks.empty())
    {
      if(write_size == 0)
      {
        return;
      }

      Stream::Error ostr;
      ostr << FUN << ": can't write to empty block sequence.";
      throw BaseException(ostr);
    }

    typename PosedBlockListType::iterator left =
      std::upper_bound(
        write_blocks.begin(),
        write_blocks.end(),
        pos,
        LessOpType());

    typename PosedBlockListType::iterator right =
      std::upper_bound(
        left,
        write_blocks.end(),
        pos + write_size,
        LessOpType());

    if(left == write_blocks.begin() || right == write_blocks.begin())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't write to pos=" << pos
           << ", first block start in pos=" << write_blocks.begin()->pos;
      throw BaseException(ostr);
    }

    --left; --right;

    if(left == right)
    {
      left->block->write(
        pos - left->pos + left->data_offset,
        buffer,
        write_size);
    }
    else
    {
      SizeType written_size =
        (left->block->size() - left->data_offset) - (pos - left->pos);

      left->block->write(
        pos - left->pos + left->data_offset,
        buffer,
        written_size);

      for(typename PosedBlockListType::iterator cur = ++left;
          cur != right; ++cur)
      {
        SizeType sz = cur->block->size();

        if (cur->data_offset > sz)
        {
	  Stream::Error ostr;
          ostr << FUN << "The offset to user data=" << cur->data_offset
            << " more than the block size=" << sz;
          throw CorruptedRecord(ostr);
        }

        if(sz != cur->data_offset)
        {
          cur->block->write(
            cur->data_offset,
            (const char*)buffer + written_size,
            sz - cur->data_offset);

          written_size += sz - cur->data_offset;
        }
      }

      right->block->write(
        right->data_offset,
        (const char*)buffer + written_size,
        write_size - written_size);
    }
  }
}

#endif // _RECORD_LAYER_UTILS_TPP_
