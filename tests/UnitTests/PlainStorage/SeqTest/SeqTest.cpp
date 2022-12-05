// @file SeqTest/SeqTest.cpp

#include <memory>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <memory>

#include <ProfilingCommons/PlainStorage/RecordLayerUtils.hpp>

using namespace PlainStorage;

struct TestBlock
{
  enum Op
  {
    OP_READ,
    OP_WRITE
  };

  struct OpHolder
  {
    OpHolder(
      Op op_val,
      unsigned long pos_val,
      unsigned long size_val)
      : op(op_val),
        pos(pos_val),
        size(size_val)
    {}

    bool
    operator ==(const OpHolder& right) const
    {
      return op == right.op && pos == right.pos && size == right.size;
    }

    bool
    operator !=(const OpHolder& right) const
    {
      return !(*this == right);
    }
    
    Op op;
    unsigned long pos;
    unsigned long size;
  };
  
  typedef std::list<OpHolder> OpHolderList;

  TestBlock(SizeType sz): sz_(sz)
  {}
  
  void
  read(SizeType pos, void* /*buffer*/, SizeType sz)
  {
    ops_.push_back(OpHolder(OP_READ, pos, sz));
  }

  bool
  check_ops(const OpHolderList& ops) const
  {
    if (ops_.size() != ops.size())
    {
      return false;
    }
    
    OpHolderList::const_iterator right_it = ops.begin();
    
    for (OpHolderList::const_iterator it = ops_.begin();
      it != ops_.end(); ++it, ++right_it)
    {
      if (*it != *right_it)
      {
        return false;
      }
    }

    return true;
  }

  std::ostream&
  print_ops(std::ostream& ostr, const char* start = "") const
  {
    int i = 0;
    for (OpHolderList::const_iterator it = ops_.begin();
      it != ops_.end(); ++it, ++i)
    {
      ostr << start << "op #" << i << ": "
        << (it->op == OP_READ ? "read" : "write")
        << ": pos = " << it->pos << ", size = " << it->size
        << "." << std::endl;
    }
    return ostr;
  }
  
  bool
  operator ==(const TestBlock& right) const
  {
    return check_ops(right.ops_);
  }
  
  SizeType
  size() const
  {
    return sz_;
  }

  SizeType sz_;
  OpHolderList ops_;
};

struct EraseTestBlock
{
  EraseTestBlock(int id, SizeType sz, SizeType avail_size)
      : id_(id), sz_(sz), avail_size_(avail_size)
  {
  }
    
  void
  read(SizeType pos, void* /*buffer*/, SizeType sz)
  {
    std::cout << "read from #" << id_ << ": pos=" << pos
              << " sz=" << sz << std::endl;
  }

  void
  write(SizeType pos, void* /*buffer*/, SizeType sz)
  {
    std::cout << "write into #" << id_ << ": pos=" << pos
              << " sz=" << sz << std::endl;
  }
  
  SizeType
  size() const
  {
    return sz_;
  }

  void
  deallocate()
  {
    std::cout << "deallocate #" << id_ << std::endl;
  }

  SizeType
  available_size() const
  {
    return avail_size_;
  }

  int id_;
  SizeType sz_;
  SizeType avail_size_;
};

template <typename _T>
struct TestPosedBlock
{
  typedef _T BlockType;

  TestPosedBlock(SizeType pos_val, SizeType do_val, _T* blk)
    : pos(pos_val), data_offset(do_val), block(blk)
  {
  }
    
  SizeType pos;
  SizeType data_offset;
  _T* block;
};

template <typename _T>
struct Less
{
  bool
  operator ()(const _T& left, const _T& right) const
  {
    return left.pos < right.pos;
  }

  bool
  operator ()(const _T& left, SizeType right) const
  {
    return left.pos < right;
  }

  bool
  operator ()(SizeType left, const _T& right) const
  {
    return left < right.pos;
  }
};

  
int
main()
{
  int result = 0;
  
  typedef std::vector<TestPosedBlock<TestBlock> > BList1;

  {
    BList1 blist;
    std::unique_ptr<TestBlock> b1(new TestBlock(10));
    std::unique_ptr<TestBlock> b2(new TestBlock(10));
    std::unique_ptr<TestBlock> b3(new TestBlock(10));
    blist.push_back(TestPosedBlock<TestBlock>(0, 4, b1.get())); // 6
    blist.push_back(TestPosedBlock<TestBlock>(6, 2, b2.get())); // 8
    blist.push_back(TestPosedBlock<TestBlock>(14, 2, b3.get())); // 8
    PlainStorage::read_block_seq_part<BList1,
      Less<TestPosedBlock<TestBlock> > >(blist, 3, 0, 15);

    TestBlock::OpHolderList ops1;
    ops1.push_back(TestBlock::OpHolder(TestBlock::OP_READ, 7, 3));
    bool res = b1->check_ops(ops1);
    
    TestBlock::OpHolderList ops2;
    ops2.push_back(TestBlock::OpHolder(TestBlock::OP_READ, 2, 8));
    res &= b2->check_ops(ops2);

    TestBlock::OpHolderList ops3;
    ops3.push_back(TestBlock::OpHolder(TestBlock::OP_READ, 2, 4));
    res &= b3->check_ops(ops3);

    /*
    b1->print_ops(std::cout, "1> "); // 7 (3): 4 + 3, 10 - 4 - 3
    b2->print_ops(std::cout, "2> "); // 2 (8)
    b3->print_ops(std::cout, "3> "); // 2 (4)
    */

    if (!res)
    {
      std::cerr << "read test: incorrect result operation sequences."
        << std::endl;
    }

    result += (res ? 0 : 1);
  }
  
/*  
  {
    std::cout << "==== erase_excess_blocks ====" << std::endl;
    EraseTestBlock* rest = 0;
    
    BList2 blist;
    blist.push_back(TestPosedBlock<EraseTestBlock>(0, 4,
      new EraseTestBlock(0, 4, 4)));
    blist.push_back(TestPosedBlock<EraseTestBlock>(0, 2,
      new EraseTestBlock(1, 100, 100)));
    blist.push_back(TestPosedBlock<EraseTestBlock>(98, 2,
      new EraseTestBlock(2, 100, 100)));
    blist.push_back(TestPosedBlock<EraseTestBlock>(98*2, 2,
      new EraseTestBlock(3, 30, 40)));

    const int OLD_SIZE = 98*2 + 28;
    const int NEW_SIZE = 98*2 + 100;
    
    std::cout << "erase_excess_blocks: old-size=" << OLD_SIZE
      << ", new-size=" << NEW_SIZE << std::endl;
    
    std::cout
      << "result: "
      << PlainStorage::erase_excess_blocks<BList2, EraseTestBlock*>(
        blist, 100, 98*2 + 28, 98*2 + 100, rest)
      << std::endl;

    std::cout << "rest is " << (rest ? "non empty" : "empty") << std::endl;
  }
*/
  return result;
}

