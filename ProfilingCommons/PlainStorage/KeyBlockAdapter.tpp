// @file PlainStorage/KeyBlockAdapter.tpp

#ifndef KEYBLOCKADAPTER_TPP
#define KEYBLOCKADAPTER_TPP

#include <iostream>
#include <Commons/AtomicInt.hpp>

namespace PlainStorage
{
  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  KeyBlockAdapter(WriteBlock* write_block) noexcept
    : write_block_(ReferenceCounting::add_ref(write_block))
  {}

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  SizeType
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  size() const
    noexcept
  {
    return write_block_->size();
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  void
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  init(SizeType use_size) /*throw(BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::init()";

    try
    {
      u_int32_t buf[KBH_SIZE];
      buf[KBH_USEDSIZE] = BlockIndexAccessorType::max_size() + KBH_SIZE*sizeof(u_int32_t);
      buf[KBH_NEXTBLOCKINDEXSIZE] = 0;
      if(use_size == 0)
      {
        use_size = write_block_->available_size();
      }
      write_block_->resize(std::max(use_size, (SizeType)buf[KBH_USEDSIZE]));
      write_block_->write(0, buf, KBH_SIZE*sizeof(u_int32_t));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  bool
  KeyBlockAdapter<KeyType,
    KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  add_size_(SizeType add_use_size) /*throw (BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::add_size_()";

    try
    {
      u_int32_t buf[KBH_SIZE];
      write_block_->read(0, buf, KBH_SIZE * sizeof(u_int32_t));
      SizeType new_size = buf[KBH_USEDSIZE] + add_use_size;
      if (new_size > write_block_->available_size())
      {
        return false;
      }
      write_block_->resize(new_size);
      return true;
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  SizeType
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  find(const KeyType& key) const
    /*throw (BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::find()";

    try
    {
      KeyAccessorType key_accessor;
      Generics::ArrayAutoPtr<char> key_buf;
      KeyType check_key;
      SizeType key_buf_size = 0;

      SizeType used_size = used_size_();

      WriteBlock* write_block = write_block_;

      for(SizeType pos = KBH_SIZE*sizeof(u_int32_t) +
            BlockIndexAccessorType::max_size();
          pos < used_size; )
      {
        u_int32_t kh_buf[KH_SIZE];
        write_block->read(pos, kh_buf, KH_SIZE*sizeof(u_int32_t));
        bool deleted = (kh_buf[KH_MARK] == KeyBlockAdapter_::MARK_DELETED);
        u_int32_t key_size = kh_buf[KH_KEY_SIZE];
        u_int32_t block_index_size = kh_buf[KH_BLOCKINDEXSIZE];

        if (block_index_size > BlockIndexAccessorType::max_size())
        {
          Stream::Error ostr;
          ostr << "The size of block index=" <<
            block_index_size << " exceeds maximum allowed size for index=" <<
            BlockIndexAccessorType::max_size();
          throw BaseException(ostr);
        }

        if(!deleted)
        {
          /* read key */
          if(key_buf_size < key_size)
          {
            key_buf.reset(key_size);
            key_buf_size = key_size;
          }
          write_block->read(pos + KH_SIZE*sizeof(u_int32_t), key_buf.get(), key_size);

          key_accessor.load(key_buf.get(), key_size, check_key);

          if(check_key == key)
          {
            return pos;
          }
        }

        pos += KH_SIZE*sizeof(u_int32_t) + key_size + block_index_size;
      }

      return 0;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  void
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  copy_keys(std::list<KeyType>& keys) const
    /*throw(BaseException)*/
  {
    // FIXME: copypaste from KeyBlockAdapter::find
    static const char* FUN = "KeyBlockAdapter::copy_keys()";

    try
    {
      KeyAccessorType key_accessor;
      Generics::ArrayAutoPtr<char> key_buf;
      KeyType key;
      SizeType key_buf_size = 0;

      SizeType used_size = used_size_();

      WriteBlock* write_block = write_block_;

      for(SizeType pos = KBH_SIZE*sizeof(u_int32_t) +
            BlockIndexAccessorType::max_size();
          pos < used_size; )
      {
        u_int32_t kh_buf[KH_SIZE];
        write_block->read(pos, kh_buf, KH_SIZE*sizeof(u_int32_t));
        bool deleted = (kh_buf[KH_MARK] == KeyBlockAdapter_::MARK_DELETED);
        u_int32_t key_size = kh_buf[KH_KEY_SIZE];
        u_int32_t block_index_size = kh_buf[KH_BLOCKINDEXSIZE];

        if (block_index_size > BlockIndexAccessorType::max_size())
        {
          Stream::Error ostr;
          ostr << "The size of block index=" <<
            block_index_size << " exceeds maximum allowed size for index=" <<
            BlockIndexAccessorType::max_size();
          throw BaseException(ostr);
        }

        if(!deleted)
        {
          /* read key */
          if(key_buf_size < key_size)
          {
            key_buf.reset(key_size);
            key_buf_size = key_size;
          }
          write_block->read(pos + KH_SIZE*sizeof(u_int32_t), key_buf.get(), key_size);

          key_accessor.load(key_buf.get(), key_size, key);
          keys.push_back(key);
        }

        pos += KH_SIZE*sizeof(u_int32_t) + key_size + block_index_size;
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  SizeType
  KeyBlockAdapter<KeyType,
    KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  push_back(
    const KeyType& key,
    const BlockIndexType& block_index,
    bool allow_resize)
    /*throw (BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::push_back()";

    try
    {
      KeyAccessorType key_accessor;
      BlockIndexAccessorType block_index_accessor;
      SizeType used_size = used_size_();
      SizeType new_key_size = key_accessor.size(key);
      SizeType new_block_index_size = block_index_accessor.size(block_index);

      const SizeType new_kh_size = KH_SIZE*sizeof(u_int32_t) +
        new_key_size + new_block_index_size;

      if (used_size + new_kh_size > write_block_->size())
      {
        if (!allow_resize || !add_size_(std::max(new_kh_size,
          KeyBlockAdapter_::RESIZE_PORTION)))
        {
          return 0;
        }
      }

      Generics::ArrayAutoPtr<char> kh_buf(new_kh_size);

      u_int32_t* kh_head = reinterpret_cast<u_int32_t*>(kh_buf.get());
      kh_head[KH_KEY_SIZE] = new_key_size;
      kh_head[KH_BLOCKINDEXSIZE] = new_block_index_size;
      kh_head[KH_MARK] = KeyBlockAdapter_::MARK_VALID;

      key_accessor.save(
        key, kh_buf.get() + KH_SIZE*sizeof(u_int32_t), new_key_size);

      block_index_accessor.save(
        block_index,
        kh_buf.get() + KH_SIZE*sizeof(u_int32_t) + new_key_size,
        new_block_index_size);

      write_block_->write(used_size, kh_buf.get(), new_kh_size);
      used_size_(used_size + new_kh_size);

      return used_size;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  SizeType
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  get(
    SizeType pos,
    SizeType& key_pos,
    KeyType& result_key,
    BlockIndexType& result_block_index,
    bool loop) const
    /*throw (BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::get()";

    try
    {
      KeyAccessorType key_accessor;
      BlockIndexAccessorType block_index_accessor;

      SizeType used_size = used_size_();
      bool found = false;

      if(pos == 0)
      {
        pos = BlockIndexAccessorType::max_size() + KBH_SIZE*sizeof(u_int32_t);
      }

      while(pos < used_size && !found)
      {
        u_int32_t kh_buf[KH_SIZE];
        write_block_->read(pos, kh_buf, KH_SIZE*sizeof(u_int32_t));
        bool deleted = (kh_buf[KH_MARK] == KeyBlockAdapter_::MARK_DELETED);
        u_int32_t key_size = kh_buf[KH_KEY_SIZE];
        u_int32_t block_index_size = kh_buf[KH_BLOCKINDEXSIZE];

        if (block_index_size > BlockIndexAccessorType::max_size())
        {
          Stream::Error ostr;
          ostr << "The size of block index=" <<
            block_index_size << " exceeds maximum allowed size for index=" <<
            BlockIndexAccessorType::max_size();
          throw CorruptedRecord(ostr);
        }

        if(!deleted)
        {
          /* read key */
          Generics::ArrayAutoPtr<char> key_buf(key_size);
          Generics::ArrayAutoPtr<char> block_index_buf(block_index_size);

          write_block_->read(
            pos + KH_SIZE*sizeof(u_int32_t), key_buf.get(), key_size);
          write_block_->read(
            pos + KH_SIZE*sizeof(u_int32_t) + key_size, block_index_buf.get(), block_index_size);

          key_accessor.load(key_buf.get(), key_size, result_key);
          block_index_accessor.load(
            block_index_buf.get(), block_index_size, result_block_index);

/*
          std::cout << "DEBUG INDEX: '" << result_key << "': (" <<
            result_block_index << "), pos = " << pos << ": " <<
            (deleted ? "deleted" : "not deleted") << std::endl;
*/

          key_pos = pos;
          found = true;
        }

        if(!loop)
        {
          break;
        }

        pos += KH_SIZE*sizeof(u_int32_t) + key_size + block_index_size;
      }

      return found ? pos : 0;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  void
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  rewrite(
    SizeType pos,
    const BlockIndexType& block_index)
    /*throw(BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::rewrite()";

    try
    {
      BlockIndexAccessorType block_index_accessor;
      SizeType new_block_index_size = block_index_accessor.size(block_index);

      Generics::ArrayAutoPtr<char> kh_buf(KH_SIZE*sizeof(u_int32_t));
      write_block_->read(pos, kh_buf.get(), KH_SIZE*sizeof(u_int32_t));

      Generics::ArrayAutoPtr<char> block_index_buf(new_block_index_size);
      block_index_accessor.save(
        block_index,
        block_index_buf.get(),
        new_block_index_size);

      write_block_->write(
        pos + KH_SIZE*sizeof(u_int32_t) + kh_buf[KH_KEY_SIZE],
        block_index_buf.get(),
        new_block_index_size);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  void
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  erase(SizeType pos)
    /*throw(BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::erase()";

    try
    {
      u_int32_t marker;

      /*
      {
        Stream::Error ostr;
        ostr << "read by pos = " << (pos + KH_MARK*sizeof(u_int32_t)) <<
          " from block with size = " << write_block_->size();
        std::cout << ostr.str() << std::endl;
      }
      */

      write_block_->read(
        pos + KH_MARK*sizeof(u_int32_t),
        &marker,
        sizeof(u_int32_t));

      if(marker != KeyBlockAdapter_::MARK_VALID)
      {
        throw BaseException("key already removed.");
      }

      write_block_->write(
        pos + KH_MARK*sizeof(u_int32_t),
        &KeyBlockAdapter_::MARK_DELETED,
        sizeof(KeyBlockAdapter_::MARK_DELETED));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  SizeType
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  used_size_() const
    /*throw(BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::used_size_()";

    try
    {
      u_int32_t used_size;
      write_block_->read(
        KBH_USEDSIZE*sizeof(u_int32_t), &used_size, sizeof(u_int32_t));
      return used_size;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  void
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  used_size_(
    SizeType used_size)
    /*throw(BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::used_size_()";

    try
    {
      u_int32_t used_size_val = used_size;
      write_block_->write(
        KBH_USEDSIZE*sizeof(u_int32_t), &used_size_val, sizeof(u_int32_t));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  void
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  attach_next_block(
    const BlockIndexType& next_block_index)
    /*throw(BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::attach_next_block()";

    try
    {
      BlockIndexAccessorType block_index_accessor;
      u_int32_t next_block_index_size = block_index_accessor.size(next_block_index);
      Generics::ArrayAutoPtr<char> next_block_index_buf(next_block_index_size);

      block_index_accessor.save(
        next_block_index, next_block_index_buf.get(), next_block_index_size);

      write_block_->write(
        KBH_SIZE*sizeof(u_int32_t),
        next_block_index_buf.get(),
        next_block_index_size);

      write_block_->write(
        KBH_NEXTBLOCKINDEXSIZE*sizeof(u_int32_t),
        &next_block_index_size,
        sizeof(u_int32_t));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexAccessorType>
  bool
  KeyBlockAdapter<KeyType, KeyAccessorType, BlockIndexType, BlockIndexAccessorType>::
  next_block(
    BlockIndexType& next_block_index)
    /*throw(BaseException)*/
  {
    static const char* FUN = "KeyBlockAdapter::next_block()";

    try
    {
      BlockIndexAccessorType block_index_accessor;
      u_int32_t next_block_index_size;

      write_block_->read(
        KBH_NEXTBLOCKINDEXSIZE*sizeof(u_int32_t),
        &next_block_index_size,
        sizeof(u_int32_t));

      if(next_block_index_size == 0)
      {
        return false;
      }

      Generics::ArrayAutoPtr<char> next_block_index_buf(next_block_index_size);

      write_block_->read(
        KBH_SIZE*sizeof(u_int32_t),
        next_block_index_buf.get(),
        next_block_index_size);

      block_index_accessor.load(
        next_block_index_buf.get(), next_block_index_size, next_block_index);

      return true;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw BaseException(ostr);
    }
  }
}

#endif // KEYBLOCKADAPTER_TPP
