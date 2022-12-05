
#ifndef _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_ADMIN_TABLE_HPP_
#define _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_ADMIN_TABLE_HPP_

#include <string>
#include <vector>
#include <list>
#include <map>

#include <Stream/MemoryStream.hpp>
#include <eh/Exception.hpp>

class Table
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(OutOfRange, Exception);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  Table(unsigned long columns) /*throw(Exception, eh::Exception)*/;

public:

  struct Row : std::vector<std::string>
  {
    Row(unsigned long columns = 0) /*throw(eh::Exception)*/;

    void
    add_field(const std::string& value)
      /*throw(OutOfRange, eh::Exception)*/;

    void
    add_field(const char* value)
      /*throw(OutOfRange, eh::Exception)*/;

    template<typename T>
    void add_field(const T& value) /*throw(OutOfRange, eh::Exception)*/;

  private:
    unsigned long columns_;
  };

  struct Column
  {
    enum Type
    {
      TEXT,   // string value
      NUMBER, // integer value
      REAL    // floating-point value
    };

    Column(const char* nm = 0, Type tp = Column::TEXT) /*throw(eh::Exception)*/;

    std::string name;
    Type type;
  };

  enum Relation
  {
    RL_EQ = 0,  //or operation for sequence
    RL_CN, 
    RL_NG,
    RL_NL, 
    
    RL_NE = 128,//and operation for sequence
    RL_NC, 
    RL_GT, 
    RL_LT,
  };

  class ColumnOperationHandler
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(UnsupportedOperation, Exception);
    DECLARE_EXCEPTION(InvalidValue, Exception);

    ColumnOperationHandler()
    {
      op_fns_[RL_EQ] = &ColumnOperationHandler::eq;
      op_fns_[RL_NE] = &ColumnOperationHandler::ne;
      op_fns_[RL_CN] = &ColumnOperationHandler::cn;
      op_fns_[RL_NC] = &ColumnOperationHandler::nc;
      op_fns_[RL_GT] = &ColumnOperationHandler::gt;
      op_fns_[RL_LT] = &ColumnOperationHandler::lt;
      op_fns_[RL_NG] = &ColumnOperationHandler::ng;
      op_fns_[RL_NL] = &ColumnOperationHandler::nl;
    }

    virtual void init(const String::SubString &value1,
      const String::SubString &value2)
      /*throw(Exception)*/ = 0;

    virtual bool operator()(Relation op) /*throw(Exception)*/
    {
      if (!op_fns_[op])
      {
        Stream::Error ostr;
        ostr << "ColumnOperationHandler::operator()(): "
          << "Unsupported operation requested!";
        throw UnsupportedOperation(ostr);
      }
      return (this->*op_fns_[op])();
    }

    virtual ~ColumnOperationHandler() {}

  protected:
    virtual bool eq() = 0;
    virtual bool ne() = 0;
    virtual bool cn() = 0;
    virtual bool nc() = 0;
    virtual bool gt() = 0;
    virtual bool lt() = 0;
    virtual bool ng() = 0;
    virtual bool nl() = 0;

    typedef bool (ColumnOperationHandler::*op_fn)();

    std::map<Relation, op_fn> op_fns_;
  };

  class TextOperationHandler: public ColumnOperationHandler
  {
  public:
    TextOperationHandler(): ColumnOperationHandler(), value1_(), value2_()
    {
    }

    virtual void init(const String::SubString &value1,
      const String::SubString &value2)
      /*throw(Exception)*/
    {
      value1.assign_to(value1_);
      value2.assign_to(value2_);
    }

    virtual bool operator()(Relation op) /*throw(Exception)*/
    {
      return ColumnOperationHandler::operator()(op);
    }

    virtual ~TextOperationHandler() {}

  private:
    virtual bool eq() { return value1_ == value2_; }

    virtual bool ne() { return value1_ != value2_; }

    virtual bool cn()
    {
      return value1_.find(value2_) != std::string::npos;
    }

    virtual bool nc()
    {
      return value1_.find(value2_) == std::string::npos;
    }

    virtual bool gt() { return value1_ > value2_; }

    virtual bool lt() { return value1_ < value2_; }

    virtual bool ng() { return value1_ <= value2_; }

    virtual bool nl() { return value1_ >= value2_; }

    std::string value1_;
    std::string value2_;
  };

  template <class _ArgType>
  class NumericTypeOperationHandler: public ColumnOperationHandler
  {
  public:
    NumericTypeOperationHandler():
      ColumnOperationHandler(), value1_(), value2_()
    {
      op_fns_[RL_CN] = 0; // Unsupported operation
      op_fns_[RL_NC] = 0; // Unsupported operation
    }

    virtual void init(const String::SubString &value1,
      const String::SubString &value2)
      /*throw(Exception)*/
    {
      Stream::Parser iss1(value1);
      iss1 >> value1_;
      if (!iss1.eof())
      {
        Stream::Error ostr;
        ostr << "NumericTypeOperationHandler<>::init(): "
          << "Invalid value(1): " << value1;
        throw InvalidValue(ostr);
      }
      Stream::Parser iss2(value2);
      iss2 >> value2_;
      if (!iss2.eof())
      {
        Stream::Error ostr;
        ostr << "NumericTypeOperationHandler<>::init(): "
          << "Invalid value(2): " << value2;
        throw InvalidValue(ostr);
      }
    }

    virtual bool operator()(Relation op) /*throw(Exception)*/
    {
      return ColumnOperationHandler::operator()(op);
    }

    virtual ~NumericTypeOperationHandler() {}

  private:
    virtual bool eq() { return value1_ == value2_; }

    virtual bool ne() { return value1_ != value2_; }

    virtual bool cn() { return false; } // Unsupported

    virtual bool nc() { return false; } // Unsupported

    virtual bool gt() { return value1_ > value2_; }

    virtual bool lt() { return value1_ < value2_; }

    virtual bool ng() { return value1_ <= value2_; }

    virtual bool nl() { return value1_ >= value2_; }

    _ArgType value1_;
    _ArgType value2_;
  };

  typedef NumericTypeOperationHandler<long long> NumberOperationHandler;
  typedef NumericTypeOperationHandler<double> RealOperationHandler;

  struct FilterPrivate
  {
    std::string value;
    Relation relation;

    FilterPrivate(const char* vl = 0, Relation rel = RL_EQ)
      /*throw(eh::Exception)*/
      :value(vl), relation(rel) {}

    bool fulfill(const String::SubString& field, const char* typenam, ColumnOperationHandler* handler) const
      /*throw(eh::Exception)*/;

  private:
  };

  struct Filter
  {
    std::string column_name;
    Relation relation;

    Filter(const char* nm = 0, const char* vl = 0, Relation rel = RL_EQ)
      /*throw(eh::Exception)*/;

    bool fulfill(const String::SubString& field, Column::Type type) const
      /*throw(eh::Exception)*/;

  private:

    typedef std::map<Column::Type, const char*> ColumnTypeNameMap;
    typedef std::map<Column::Type, ColumnOperationHandler*> ColumnTypeHandlerMap;

    ColumnTypeNameMap    column_type_names_;
    ColumnTypeHandlerMap column_type_handlers_;

    typedef std::list<FilterPrivate> Filters;
    Filters filters_;
  };

  typedef std::list<Filter> Filters;

  struct Sorter
  {
    std::string column_name;
    bool descending;

    Sorter(const char* nm = 0, bool desc = false)
      /*throw(eh::Exception)*/;

    bool insert_before(const String::SubString& new_field,
                       const String::SubString& existing_field,
                       Column::Type field_type) const
      /*throw(eh::Exception)*/;
  };

  void column(unsigned long i,
              const Column& column)
    /*throw(OutOfRange, Exception, eh::Exception)*/;

  unsigned long columns() const /*throw(eh::Exception)*/;

  void add_row(const Row& row,
               const Filters& filters = Filters(),
               const Sorter& sorter = Sorter())
    /*throw(InvalidArgument, eh::Exception)*/;

  bool empty() const noexcept;

  unsigned long value_align() const noexcept;
  
  void dump(std::ostream& ostr, const char* prefix = 0) const /*throw(Exception, eh::Exception)*/;

  void clear() noexcept;

  static bool parse_filter(
    const char* c_str,
    std::string& name,
    Table::Relation& relation,
    std::string& value) noexcept;

private:
  typedef std::vector<Column> Columns;
  typedef std::list<Row> Rows;

  Columns columns_;
  Rows    rows_;
};

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

//
// Table class
//

inline
Table::Table(unsigned long columns) /*throw(Exception, eh::Exception)*/
    : columns_(columns)
{
}

inline
unsigned long
Table::columns() const /*throw(eh::Exception)*/
{
  return columns_.size();
}

inline
bool Table::empty() const noexcept
{
  return rows_.begin() == rows_.end();
}

inline
void Table::clear() noexcept
{
  rows_.clear();
}

//
// Table::Row class
//

inline
Table::Row::Row(unsigned long columns) /*throw(eh::Exception)*/
    : columns_(columns)
{
}

inline
void
Table::Row::add_field(const std::string& value)
  /*throw(OutOfRange, eh::Exception)*/
{
  if (size() >= columns_)
  {
    Stream::Error ostr;
    ostr << "Table::Row::add_field: number of fields (" << columns_
         << ") can't be exceeded";

    throw OutOfRange(ostr);
  }

  push_back(value);
}

inline
void
Table::Row::add_field(const char* value)
  /*throw(OutOfRange, eh::Exception)*/
{
  add_field(std::string(value));
}

template<typename T>
void
Table::Row::add_field(const T& value) /*throw(OutOfRange, eh::Exception)*/
{
  Stream::Dynamic ostr;
  ostr << value;

  add_field(ostr.str().str());
}

//
// Table::Column class
//

inline
Table::Column::Column(const char* nm, Type tp) /*throw(eh::Exception)*/
    : name(nm ? nm : ""),
      type(tp)
{
}

//
// Table::Sorter class
//

inline
Table::Sorter::Sorter(const char* nm, bool desc) /*throw(eh::Exception)*/
    : column_name(nm ? nm : ""),
      descending(desc)
{
}

#endif // _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_ADMIN_TABLE_HPP_
