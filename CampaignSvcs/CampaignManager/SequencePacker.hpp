#ifndef _SEQUENCE_PACKER_HPP_
#define _SEQUENCE_PACKER_HPP_

#include <cassert>
#include <set>
#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Commons/Algs.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    /**
     * SequencePacker
     * Requirements:
     *  ElementType and ElementSeqType must be reference countable
     *    as result all methods work with ElementType and ElementSeqType pointers
     *  ElementType must provide operator==, and copy c-tor
     *  ElementSeqType must provide insert(ElementType*) method, and copy c-tor
     *  ElementSeqHashType must provide transitive hash
     *    for ElementSeqType and ElementType, and transitive hash_union.
     * Notes:
     *  transitive: A set + B set = C set => hash_union(hash(A), hash(B)) = hash(C)
     *  for sample: if A contains (a0, .., aN) element:
     *    hash(A) = a0 ^ .. ^ aN is transitive
     */
    template<typename ElementType, typename ElementSeqType>
    struct DefaultSequenceHash
    {
      unsigned long hash(const ElementType* el) const
      {
        return el->hash();
      }
      
      unsigned long hash_union(unsigned long left, unsigned long right) const
      {
        return left ^ right;
      }
      
      unsigned long hash(const ElementSeqType* el_seq) const
      {
        if(el_seq->empty())
        {
          return 0;
        }

        unsigned long res = hash(*(*el_seq->begin()));
        
        for(typename ElementSeqType::const_iterator el_it = ++el_seq->begin();
            el_it != el_seq->end(); ++el_it)
        {
          res = hash_union(res, hash(*el_it));
        }

        return res;
      }
    };

    struct ElementSeqBase;
    
    struct SequencePackerElementEraser:
      public ReferenceCounting::DefaultImpl<>
    {
      virtual void erase(const ElementSeqBase* el) = 0;
    };

    typedef
      ReferenceCounting::SmartPtr<SequencePackerElementEraser>
      SequencePackerElementEraser_var;
    
    class ElementSeqBase
    {
    public:
      void attach_eraser(SequencePackerElementEraser* eraser) const
      {
        eraser_ = ReferenceCounting::add_ref(eraser);
      }
      
      virtual ~ElementSeqBase() noexcept {}

      void unkeep_(ElementSeqBase* /*el*/)
      {
        if(eraser_.in())
        {
          eraser_->erase(this);
        }
      }
      
    protected:
      mutable SequencePackerElementEraser_var eraser_;
    };

    template<
      typename ElementType,
      typename ElementSeqType,
      typename ElementSeqHashType = DefaultSequenceHash<ElementType, ElementSeqType> >
    class SequencePacker: public ReferenceCounting::DefaultImpl<>
    {
    public:
      typedef ReferenceCounting::SmartPtr<const ElementType> Element_var;
      typedef ReferenceCounting::SmartPtr<const ElementSeqType> ConstElementSeq_var;
      
    protected:
      typedef ReferenceCounting::SmartPtr<ElementSeqType> ElementSeq_var;
      
      class ElementSeqHashAdapter
      {
      public:
        ElementSeqHashAdapter();
        
        /* adapters for search */
        ElementSeqHashAdapter(
          const ElementSeqType* el_seq,
          const ElementType* el);

        ElementSeqHashAdapter(
          const ElementSeqHashAdapter& el_seq_hash_adapter,
          const ElementType* el);

        /* adapter for insert */
        ElementSeqHashAdapter(
          const ElementSeqHashAdapter& el_seq_hash_adapter,
          const ElementSeqType* el_seq);

        ElementSeqHashAdapter& operator=(const ElementSeqHashAdapter& init);
        
        unsigned long hash() const;

        bool operator==(const ElementSeqHashAdapter& right) const;
        
        const ElementSeqType* list() const;

      private:
        static
        bool compare_lists_(
          const ElementSeqType* left,
          const ElementSeqType* right,
          const ElementType* left_cell = 0,
          const ElementType* right_cell = 0);        
        
      private:
        unsigned long hash_;
        unsigned long size_;
        const ElementSeqType* cell_list_;
        const ElementType* add_cell_;
      };

      virtual ~SequencePacker() noexcept;

    public:
      SequencePacker();

      ConstElementSeq_var get(const ElementSeqType* cmp_list, const ElementType* cell);

      void unkeep(const ElementSeqBase* el_seq);

      ConstElementSeq_var create_seq(
        const ElementSeqType* el_seq, const ElementType* el) noexcept;

      unsigned long ptrs_size_() const noexcept;
      
      unsigned long size_() const noexcept;

      unsigned long hashes_count_() const noexcept;

    protected:
      struct SequencePackerElementEraserImpl:
        public SequencePackerElementEraser
      {
        void set_packer(SequencePacker* seq_packer)
        {
          seq_packer_ = seq_packer;
        }

        virtual void erase(const ElementSeqBase* el)
        {
          if(seq_packer_)
          {
            seq_packer_->unkeep(el);
          }
        }

        SequencePacker* seq_packer_;
      };

      typedef ReferenceCounting::SmartPtr<SequencePackerElementEraserImpl>
        SequencePackerElementEraserImpl_var;

      typedef Generics::GnuHashTable<
        Algs::ConstPointerHashAdapter<ElementSeqBase>,
        ElementSeqHashAdapter>
        HashAdapterTable;

      typedef Generics::GnuHashSet<ElementSeqHashAdapter>
        HashAdapterSet;

      ElementSeq_var empty_list_;
      HashAdapterTable adapter_table_;
      HashAdapterSet adapters_;
      SequencePackerElementEraserImpl_var eraser_;
    }; /*class SequencePacker*/
  }
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter::ElementSeqHashAdapter()
      : hash_(0),
        size_(0),
        cell_list_(0),
        add_cell_(0)
    {}

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    typename SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter&
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter::operator=(
      const ElementSeqHashAdapter& init)
    {
      hash_ = init.hash_;
      size_ = init.size_;
      cell_list_ = init.cell_list_;
      add_cell_ = init.add_cell_;
      return *this;
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter::ElementSeqHashAdapter(
      const ElementSeqType* empty_seq,
      const ElementType* el)
      : size_(1),
        cell_list_(empty_seq),
        add_cell_(el)
    {
      ElementSeqHashType hasher;
      hash_ = hasher.hash(el);
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter::ElementSeqHashAdapter(
      const ElementSeqHashAdapter& hash_adapter,
      const ElementType* el)
      : size_(hash_adapter.size_ + 1),
        cell_list_(hash_adapter.cell_list_),
        add_cell_(el)
    {
      ElementSeqHashType hasher;
      hash_ = hasher.hash(el);
      hash_ = hasher.hash_union(hash_, hash_adapter.hash());
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter::ElementSeqHashAdapter(
      const ElementSeqHashAdapter& hash_adapter,
      const ElementSeqType* el_seq)
      : hash_(hash_adapter.hash_),
        size_(hash_adapter.size_),
        cell_list_(el_seq),
        add_cell_(0)
    {}

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    unsigned long
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter::hash() const
    {
      return hash_;
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    bool
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter::compare_lists_(
      const ElementSeqType* left,
      const ElementSeqType* right,
      const ElementType* left_cell,
      const ElementType* right_cell)
    {
      bool left_eq_right = (left_cell && right_cell && *right_cell == *left_cell);
      bool left_cell_found = (left_cell == 0 || left_eq_right);
      bool right_cell_found = (right_cell == 0 || left_eq_right);
      typename ElementSeqType::const_iterator left_it = left->begin();
      typename ElementSeqType::const_iterator right_it = right->begin();
      
      while(left_it != left->end() && right_it != right->end())
      {
        if(*(*left_it) == *(*right_it))
        {
          ++left_it;
          ++right_it;
        }
        else if(!left_cell_found && (*(*right_it) == *left_cell))
        {
          left_cell_found = true;
          ++right_it;
        }
        else if(!right_cell_found && (*right_cell == *(*left_it)))
        {
          right_cell_found = true;
          ++left_it;
        }
        else
        {
          return false;
        }
      }

      if(left_it != left->end())
      {
        return !right_cell_found && *(*left_it) == *right_cell &&
          ++left_it == left->end();
      }
      else if(right_it != right->end())
      {
        return !left_cell_found && *(*right_it) == *left_cell &&
          ++right_it == right->end();
      }

      return left_cell_found && right_cell_found;
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    bool
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter::operator==(
      const ElementSeqHashAdapter& right) const
    {
      if(cell_list_ == right.cell_list_)
      {
        return (add_cell_ == 0 && right.add_cell_ == 0) ||
          (add_cell_ && right.add_cell_ && *add_cell_ == *right.add_cell_);
      }

      return size_ == right.size_ &&
         compare_lists_(
           cell_list_, right.cell_list_,
           add_cell_, right.add_cell_);
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    typename SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
      ConstElementSeq_var
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    create_seq(
      const ElementSeqType* el_seq, const ElementType* el) noexcept
    {
      typename SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
        ElementSeq_var res(el_seq ? new ElementSeqType(*el_seq) : new ElementSeqType());
      res->insert(el);
      res->attach_eraser(eraser_);
      return res.retn();
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    const ElementSeqType*
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ElementSeqHashAdapter::list() const
    {
      return cell_list_;
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::SequencePacker()
    {
      empty_list_ = new ElementSeqType();
      eraser_ = new SequencePackerElementEraserImpl();
      eraser_->set_packer(this);
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ~SequencePacker() noexcept
    {
      eraser_->set_packer(0);
    }
      
    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    typename SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
      ConstElementSeq_var
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::get(
      const ElementSeqType* el_seq,
      const ElementType* el)
    {
      ElementSeqHashAdapter search_el_seq_adapter;

      if(el_seq)
      {
        typename HashAdapterTable::const_iterator el_seq_it =
          adapter_table_.find(el_seq);
        assert(el_seq_it != adapter_table_.end());
        search_el_seq_adapter = ElementSeqHashAdapter(el_seq_it->second, el);
      }
      else
      {
        search_el_seq_adapter = ElementSeqHashAdapter(empty_list_, el);
      }

      typename HashAdapterSet::const_iterator el_seq_adapter_it =
        adapters_.find(search_el_seq_adapter);

      if(el_seq_adapter_it == adapters_.end())
      {
        /* insert extended hash adapter (cell list + cell) */
        ConstElementSeq_var ret_list = create_seq(el_seq, el);
        ElementSeqHashAdapter new_el_seq_adapter(
          search_el_seq_adapter, ret_list.in());

        adapter_table_.insert(
          std::make_pair(ret_list.in(), new_el_seq_adapter));

        adapters_.insert(new_el_seq_adapter);

        return ret_list;
      }

      return ReferenceCounting::add_ref(el_seq_adapter_it->list());
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    void
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::unkeep(
      const ElementSeqBase* el_seq)
    {
      if(el_seq)
      {
        typename HashAdapterTable::iterator el_seq_it =
          adapter_table_.find(el_seq);

        assert(el_seq_it != adapter_table_.end());

        /*
        if(el_seq_it != adapter_table_.end())
        {
        */

        typename HashAdapterSet::iterator el_seq_adapter_it =
          adapters_.find(el_seq_it->second);
        assert(el_seq_adapter_it != adapters_.end());

        adapters_.erase(el_seq_adapter_it);
        adapter_table_.erase(el_seq_it);

        //}
      }
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    unsigned long
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    hashes_count_() const noexcept
    {
      std::set<unsigned long> hashes;
      for(typename HashAdapterSet::const_iterator it = adapters_.begin();
          it != adapters_.end(); ++it)
      {
        hashes.insert(it->hash());
      }
      
      return hashes.size();
    }

    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    unsigned long
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    ptrs_size_() const noexcept
    {
      return adapter_table_.size();
    }
      
    template<typename ElementType, typename ElementSeqType, typename ElementSeqHashType>
    inline
    unsigned long
    SequencePacker<ElementType, ElementSeqType, ElementSeqHashType>::
    size_() const noexcept
    {
      return adapters_.size();
    }
  }
}

#endif
