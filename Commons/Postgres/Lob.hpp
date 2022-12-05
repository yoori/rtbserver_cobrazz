#ifndef COMMONS_POSTGRESS_LOB_HPP
#define COMMONS_POSTGRESS_LOB_HPP

#include<Commons/Postgres/Connection.hpp>

namespace AdServer
{
namespace Commons
{
namespace Postgres
{
  /*
   * Lob
   * Implement interface to work with PG large objects
   * */

  class Lob
  {
  public:

    explicit
    Lob(Connection* conn = 0, Oid oid = InvalidOid)
      noexcept;

    ~Lob() noexcept;

    /* 
     * Create a new large object
     * @param conn if set than assign for use this connection 
     * else try to use connection set before
     * */
    void create(Connection* conn = 0) /*throw(Exception)*/;

    /*
     * close large object
     * */
    void close() noexcept;

    /*
     * import file to large object
     * designed oid may be associated with object before call
     * @param file name 
     * */
    void import_file(const char* file_name) /*throw(Exception)*/;

    /*
     * export large object to file
     * oid should be associated with object before call
     * @param file name 
     * */
    void export_file(const char* file_name) /*throw(Exception)*/;

    /*
     * write buffer to large object
     * oid should be associated with object before call
     * @param buf buffer
     * @param len length of buffer
     * */
    void write(char* buf, size_t len) /*throw(Exception)*/;

    /*
     * read data to buffer from large object
     * oid should be associated with object before call
     * @param buf buffer
     * @param len length of buffer
     * */
    size_t read(char* buf, size_t len) /*throw(Exception)*/;

    /*
     * truncate size of large object
     * oid should be associated with object before call
     * @param len a new length of large object
     * */
    void truncate(size_t len) /*throw(Exception)*/;

    /*
     * tell position in the large object
     * oid should be associated with object before call
     * @returns position in lob object
     * */
    pg_int64 tell() /*throw(Exception)*/;

    /*
     * seek position in large object
     * oid should be associated with object before call
     * @param offset of new position
     * @param whence start point to calculate offset
     * SEEK_SET (seek from object start),
     * SEEK_CUR (seek from current position),
     * SEEK_END (seek from object end)
     * @returns position in lob object
     * */
    pg_int64 seek(pg_int64 offset, int whence) /*throw(Exception)*/;

    /*
     * erase large object
     * oid should be associated with object before call
     * */
    void unlink() /*throw(Exception)*/;

    /*
     * get length of large object
     * @returns length of large object
     * */
    pg_int64 length() /*throw(Exception)*/;

    Oid oid() const noexcept;

  private:
    void open_() /*throw(Exception)*/;

    void raise_exception_(const char* fn) /*throw(Exception)*/;

    void begin_() /*throw(Exception)*/;

    void end_() /*throw(Exception)*/;

  private:
    Connection_var conn_;
    Oid oid_;
    int fd_;
  };
}
}
}

namespace AdServer
{
namespace Commons
{
namespace Postgres
{
  inline
  Oid Lob::oid() const noexcept
  {
    return oid_;
  }
}
}
}
#endif //COMMONS_POSTGRESS_LOB_HPP
