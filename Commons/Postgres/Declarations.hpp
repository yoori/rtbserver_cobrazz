#ifndef POSTGRES_DECLARATION_HPP
#define POSTGRES_DECLARATION_HPP

#include <endian.h>

#define BINARY_RECIVING_DATA 1
 /*
  * * Interpretation of high bits.
  * */

#define NUMERIC_SIGN_MASK 0xC000
#define NUMERIC_POS 0x0000
#define NUMERIC_NEG 0x4000
#define NUMERIC_SHORT 0x8000
#define NUMERIC_NAN 0xC000
#define NBASE 10000
#define DEC_DIGITS 4 

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(ConnectionError, Exception);
    DECLARE_EXCEPTION(SqlException, Exception);
    DECLARE_EXCEPTION(NotSupported, Exception);
    DECLARE_EXCEPTION(NotActive, Exception);

    enum DATA_FORMATS
    {
      TEXT_FORMAT = 0,
      BINARY_FORMAT = 1,
      AUTO
    };

    /*OID from postrges headers */
    enum OIDS
    {
      CHAROID = 18,
      INT8OID = 20,
      INT2OID = 21,
      INT4OID = 23,
      TEXTOID = 25,
      OIDOID = 26,
      BPCHAROID = 1042,
      VARCHAROID = 1043,
      DATEOID = 1082,
      TIMEOID = 1083,
      TIMESTAMPOID = 1114,
      NUMERICOID = 1700
    };
  }
}
}

#endif //POSTGRES_DECLARATION_HPP

