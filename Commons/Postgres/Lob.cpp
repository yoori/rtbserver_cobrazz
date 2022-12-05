#include<iostream>
#include<libpq/libpq-fs.h>
#include"Lob.hpp"
#include<Commons/Postgres/ResultSet.hpp>


namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
#define WRITE_CHUNK_SIZE 1024 * 1024 * 4
#define READ_CHUNK_SIZE 1024 * 1024 * 4

    Lob::Lob(Connection* conn, Oid oid) noexcept
      : conn_(ReferenceCounting::add_ref(conn)),
        oid_(oid),
        fd_(-1)
    {
    }

    Lob::~Lob() noexcept
    {
      close();
    }

    void Lob::create(Connection* conn) /*throw(Exception)*/
    {
      close();

      if (conn)
      {
        conn_ = ReferenceCounting::add_ref(conn);
      }
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      begin_();
      oid_ = lo_create(conn_->connection_(), InvalidOid);
      end_();
      if (oid_ == InvalidOid)
      {
        raise_exception_(__func__);
      }
    }

    void Lob::close() noexcept
    {
      if(fd_ != -1)
      {
        int res = lo_close(conn_->connection_(), fd_);
        if (res)
        {
          std::cerr << "res = " << res  <<
            " lo_close() failed:" << PQerrorMessage(conn_->connection_())
            << " fd = " << fd_ << " oid = " << oid_;
        }
        try
        {
          end_();
        }
        catch (const Postgres::Exception& ex)
        {
          std::cerr << "end_() failed:" << ex.what()
            << " fd = " << fd_ << " oid = " << oid_;
        }
        fd_ = -1;
      }
    }

    void Lob::import_file(const char* file_name) /*throw(Exception)*/
    {
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      begin_();
      oid_ = lo_import_with_oid(conn_->connection_(), file_name, oid_);
      if (oid_ == InvalidOid)
      {
        raise_exception_(__func__);
      }
      end_();
    }

    void Lob::export_file(const char* file_name) /*throw(Exception)*/
    {
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      begin_();
      if (lo_export(conn_->connection_(), oid_, file_name) != 1)
      {
        raise_exception_(__func__);
      }
      end_();
    }

    void Lob::raise_exception_(const char* fn) /*throw(Exception)*/
    {
      Stream::Error err;
      err << fn << ": ";
      err  << PQerrorMessage(conn_->connection_());
      throw Exception(err);
    }

    void Lob::open_() /*throw(Exception)*/
    {
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      begin_();
      fd_ = lo_open(conn_->connection_(), oid_, INV_READ | INV_WRITE);
      if (fd_ == -1)
      {
        raise_exception_(__func__);
      }
    }

    void Lob::write(char* buf, size_t length) /*throw(Exception)*/
    {
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      if(fd_ == -1)
      {
        open_();
      }
      size_t w_size = 0;
      while(w_size < length)
      {
        size_t len = length - w_size;
        if (len > WRITE_CHUNK_SIZE)
        {
          len = WRITE_CHUNK_SIZE;
        }
        int w_trans = lo_write(
          conn_->connection_(), fd_, buf + w_size, len);
        if (w_trans == -1)
        {
          raise_exception_(__func__);
        }
        else
        {
          w_size += w_trans;
        }
      }
    }

    size_t Lob::read(char* buf, size_t length) /*throw(Exception)*/
    {
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      if(fd_ == -1)
      {
        open_();
      }
      size_t r_size = 0;
      while(r_size < length)
      {
        size_t len = length - r_size;
        if (len > READ_CHUNK_SIZE)
        {
          len = READ_CHUNK_SIZE;
        }
        int r_trans = lo_read(
          conn_->connection_(), fd_, buf + r_size, len);
        if (r_trans == -1)
        {
          raise_exception_(__func__);
        }
        else
        {
          r_size += r_trans;
        }
        if(static_cast<size_t>(r_trans) < len)
        {//end of lob
          break;
        }
      }
      return r_size;
    }

    void Lob::truncate(size_t len) /*throw(Exception)*/
    {
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      if (fd_ == -1)
      {
        open_();
      }
      int res = lo_truncate64(conn_->connection_(), fd_, len);
      if (res == -1)
      {
        raise_exception_(__func__);
      }
    }

    pg_int64 Lob::tell() /*throw(Exception)*/
    {
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      if (fd_ == -1)
      {
        open_();
      }
      pg_int64 res = lo_tell64(conn_->connection_(), fd_);
      if (res == -1)
      {
        raise_exception_(__func__);
      }
      return res;
    }

    pg_int64 Lob::seek(pg_int64 offset, int whence) /*throw(Exception)*/
    {
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      if (fd_ == -1)
      {
        open_();
      }
      pg_int64 res = lo_lseek64(conn_->connection_(), fd_, offset, whence);
      if (res == -1)
      {
        raise_exception_(__func__);
      }
      return res;
    }

    void Lob::unlink() /*throw(Exception)*/
    {
      if (!conn_)
      {
        throw Exception("Not set connection");
      }
      begin_();
      int res = lo_unlink(conn_->connection_(), oid_);
      end_();
      if (res == -1)
      {
        raise_exception_(__func__);
      }
    }

    void Lob::begin_() /*throw(Exception)*/
    {
      close();
      ResultSet_var res = conn_->execute_query("begin");
    }

    void Lob::end_() /*throw(Exception)*/
    {
      ResultSet_var res = conn_->execute_query("end");
    }

    pg_int64 Lob::length() /*throw(Exception)*/
    {
      pg_int64 pos = tell();
      pg_int64 res = seek(0, SEEK_END);
      seek(pos, SEEK_SET);
      return res;
    }

  }
}
}

