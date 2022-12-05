#ifndef __AUTOTESTS_COMMONS_ORM_HPP
#define __AUTOTESTS_COMMONS_ORM_HPP

#include <sstream>
#include <string>

#include <Sync/SyncPolicy.hpp>
#include <tests/AutoTests/Commons/Traits.hpp>
#include <tests/AutoTests/Commons/Connection.hpp>
#include <tests/AutoTests/Commons/Request/BaseRequest.hpp>

namespace AutoTest
{
  namespace ORM
  {
    namespace DB = ::AutoTest::DBC;

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    /**
     * @class IORMValue
     * @brief Base class to store DB fields
     */
    class IORMValue
    {
    public:

      /**
       * @brief Destructor.
       */      
      virtual ~IORMValue();

      /**
       * @brief Get query result.
       *
       * @param result
       * @return result
       */      
      virtual DB::Result&
      get(
        DB::Result& result) = 0;

      /**
       * @brief Set query.
       *
       * @param query
       * @return query
       */      
      virtual DB::Query&  set (DB::Query& query)   = 0;

      /**
       * @brief Is value changed.
       *
       * @return true if changed
       */
      bool
      changed () const;

      /**
       * @brief Enable change flag.
       *
       * @return value
       */
      IORMValue& change ();

      /**
       * @brief Mark value as null.
       *
       * @return value
       */
      IORMValue& null ();

      /**
       * @brief Mark value to not null.
       *
       * @return value
       */
      IORMValue& unnull ();


      /**
       * @brief Check value is null.
       *
       * @return true if null
       */
      bool is_null () const;
      
    protected:
      bool        _changed;
      bool        _null;
    };

    DB::Query& 
    operator<<(
      DB::Query& left,
      IORMValue& value);

    DB::Result&
    operator>>(
      DB::Result& left,
      IORMValue& value);

    /**
     * @class ORMValue
     * @brief Base class to typed DB fields
     */
    template<typename T>
    class ORMValue:
      public IORMValue
    {
    public:

      typedef T value_type;

      /**
       * @brief Default constructor.
       */   
      ORMValue();

      /**
       * @brief Constructor.
       *
       * @param value
       */   
      explicit ORMValue(const T& value);

      /**
       * @brief Copy constructor.
       *
       * @param other ORM value
       */         
      ORMValue(const ORMValue& from);

      /**
       * @brief Destructor.
       */   
      ~ORMValue();

      /**
       * @brief Set value (only post initialization).
       *
       * @param value
       * @return ORM value
       */  
      ORMValue&
      set(
        const T& value);

      /**
       * @brief Initialize value and get result.
       *
       * @param result
       * @return result
       */  
      DB::Result&
      get(
        DB::Result& result);


      /**
       * @brief Set query parameter and get query
       *        (only if changed).
       *
       * @param query
       * @return query
       */  
      DB::Query&
      set(
        DB::Query& query);


      /**
       * @brief Set query stream parameter and get query stream
       *        (only if changed).
       *
       * @param query stream
       * @return query stream
       */  
      template<typename Q>
      DB::QueryStream<Q>&
      set(
        DB::QueryStream<Q>& query);

      /**
       * @brief Assignment from value.
       *
       * @param value
       * @return ORM value
       */  
      ORMValue&
      operator=(
        const T& value);
      
      /**
       * @brief Assignment from ORMValue.
       *
       * @param value
       * @return ORM value
       */  
      ORMValue&
      operator=(
        const ORMValue& v);


      /**
       * @brief Get original value.
       *
       * @return value
       */        
      const T& value () const;

      /**
       * @brief Access to original value.
       *
       * @return value
       */        
      const T& operator* () const;
      
    protected:
      T           _value;
    };

    template<typename T>
    std::ostream& operator<<(
      std::ostream& out,
      const ORMValue<T>& val);

    template<typename T>
    DB::Query& 
    operator<<(
      DB::Query& left,
      ORMValue<T>& value);

    template<typename Q, typename T>
    Q& 
    operator<<(
      Q& query,
      ORMValue<T>& value);

    template<typename T>
    DB::Result&
    operator>>(
      DB::Result& left,
      ORMValue<T>& value);

    template<typename T>
    std::string
    strof(
      const ORMValue<T>& value);

    template<typename T1,typename T2>
    bool
    operator==(
      const ORMValue<T1>& lhs,
      const ORMValue<T2>& rhs);

    // Simple types
    typedef ORMValue<int>          ORMInt;
    typedef ORMValue<unsigned int> ORMUInt;
    typedef ORMValue<float>        ORMFloat;
    typedef ORMValue<double>       ORMDouble;
    typedef ORMValue<bool>         ORMBool;


    /**
     * @class ORMString
     * @brief Implement DB string fields
     */
    class ORMString : public ORMValue<std::string>
    {
      typedef ORMValue<std::string> Base;
    public:
      /**
       * @brief Default constructor.
       */   
      ORMString();

      /**
       * @brief Constructor.
       *
       * @param value
       */ 
      explicit ORMString(
        const std::string& value);

      /**
       * @brief Copy constructor.
       *
       * @param other ORM value
       */         
      ORMString(
        const ORMString& from);

      /**
       * @brief Assignment from const char*.
       *
       * @param value
       * @return ORM string
       */  
      ORMString&
      operator=(
        const char* value);
      
      /**
       * @brief Assignment from const string.
       *
       * @param value
       * @return ORM string
       */
      ORMString& operator=(
        const std::string& value);
    };

    const std::string&
    strof();

    DB::Query& 
    operator<<(
      DB::Query& left,
      ORMString& value);

    inline
    DB::Result&
    operator>>(
      DB::Result& left,
      ORMString& value);

    /**
     * @class ORMText
     * @brief Implement DB long (text) fields (such as LOB oracle)
     */
    class ORMText : public ORMValue<DB::TextField>
    {
      typedef ORMValue<DB::TextField> Base;
      
    public:
      
      /**
       * @brief Default constructor.
       */   
      ORMText();

      /**
       * @brief Constructor.
       *
       * @param string value for text field
       */ 
      explicit ORMText(
        const std::string& value);

      /**
       * @brief Make text field empty.
       */ 
      void
      set_empty ();

      /**
       * @brief Assignment from string.
       *
       * @param value
       * @return ORM text
       */  
      ORMText&
      operator=(
        const std::string& value);

      /**
       * @brief Assignment operator.
       */  
      ORMText& operator=(
        const ORMText& v);

    };

    std::ostream&
    operator<<(
      std::ostream& out,
      const ORMText& val);
    
    std::string
    strof(
      const ORMText&);

    
    /**
     * @class ORMDate
     * @brief Implement DB date fields
     */
    class ORMDate : public ORMValue<Generics::ExtendedTime>
    {
      typedef ORMValue<Generics::ExtendedTime> Base;

    public:
      /**
       * @brief Default constructor.
       */   
      ORMDate();

      /**
       * @brief Constructor.
       *
       * @param date value
       */ 
      explicit ORMDate(
        const value_type& value);
      
      /**
       * @brief Assignment from test time.
       *
       * @param value
       * @return ORM date
       */  
      ORMDate&
      operator=(
        const ::AutoTest::Time& value);
      
      /**
       * @brief Assignment from ExtendedTime.
       *
       * @param value
       * @return ORM date
       */  
      ORMDate&
      operator=(
        const value_type& value);

      /**
       * @brief Truncate date to day precision.
       *
       * @return ORM date
       */ 
      ORMDate&
      trunc();

      /**
       * @brief Set date to null (_ must _ be called ).
       *
       * @return ORM date
       */ 
      void
      set_null();

      /**
       * @brief Set date to now.
       *
       * @return ORM date
       */ 
       ORMDate&
       set_now ();
    };

    inline
    std::ostream&
    operator<< (
      std::ostream& out,
      const ORMDate& date);

    inline
    std::string
    strof(
      ORMDate& date);

    /**
     * @class ORMTimestamp
     * @brief Implement DB timestamp fields
     */
    class ORMTimestamp : public ORMValue <Generics::Time>
    {
      typedef ORMValue <Generics::Time> Base;

    public:
      
      /**
       * @brief Default constructor.
       */  
      ORMTimestamp();

      /**
       * @brief Constructor.
       *
       * @param timestamp value
       */ 
      explicit ORMTimestamp(
        const value_type& value);


      /**
       * @brief Assignment from test time.
       *
       * @param value
       * @return ORM date
       */  
      ORMTimestamp&
      operator=(
        const ::AutoTest::Time& value);

      /**
       * @brief Assignment from generics time.
       *
       * @param value
       * @return ORM date
       */    
      ORMTimestamp&
      operator=(
        const value_type& value);

      /**
       * @brief Truncate date to day precision.
       *
       * @return ORM date
       */ 
      ORMTimestamp&
        trunc();

      /**
       * @brief Set date to null (_ must _ be called ).
       *
       * @return ORM date
       */ 
      void
      set_null();

      /**
       * @brief Set date to now.
       *
       * @return ORM date
       */  
      ORMTimestamp&
      set_now();
    };

    inline
    std::ostream&
    operator<<(
      std::ostream& out,
      const ORMTimestamp& date);

    inline
    std::string
    strof(
      ORMTimestamp& date);

    struct postgres_connection {};

    
    /**
     * @class ORMObject
     * @brief Implement base class for DB table row
     */

    template <typename ConnectionType>
    class ORMObject
    {
    protected:
      DB::IConn& conn;
    public:

      typedef ConnectionType Connection;
      
      /**
       * @brief Constructor.
       *
       * @param connection
       */  
      explicit ORMObject(
        DB::IConn& connection);

      /**
       * @brief Copy constructor.
       */  
      ORMObject(
        const ORMObject& obj);

      /**
       * @brief Destructor.
       */  
      virtual ~ORMObject();

      /**
       * @brief Touch row (set VERSION field to now).
       *
       * @return true if success
       */  
      virtual
      bool
      touch();

      /**
       * @brief Select row (set VERSION field to now).
       *
       * @return true if success
       */  
      virtual
      bool
      select();

      /**
       * @brief Insert row (set VERSION field to now).
       *
       * @return true if success
       */        
      virtual
      bool
      insert(
        bool set_defaults = true);

      /**
       * @brief Update row (set VERSION field to now).
       *
       * @return true if success
       */              
      virtual
      bool
      update(
        bool set_defaults = true);

      /**
       * @brief Delete row (deactivate or real delete).
       *
       * @return true if success
       */              
      virtual
      bool
      del();

      /**
       * @brief Log row.
       *
       * @param logger
       * @param severity
       */    
      virtual
      void log_in(
        Logger&, 
        unsigned long = Logging::Logger::INFO);

      /**
       * @brief Log row to curent logger.
       *
       * @param severity
       */    
      void
      log(
        unsigned long severity = Logging::Logger::INFO);
    };

    template<typename T>
    inline T* null ();

    /**
     * @class ORMObjectMember
     * @brief Use to access ORMValue by name from ORMObject.
     */    
    struct ORMObjectMember
    { 
      const char* name;
      IORMValue* member;
      const char* default_value; 

      /**
       * @brief Get member value.
       */ 
      IORMValue&
      value(
        void* object) const;
    };

    /**
     * @class Restorer
     * @brief Restore changed entity.
     *
     * It is usefull when need restore initial entity state,
     * for example on test tear down.
     */
    class Restorer :
      public virtual ReferenceCounting::Interface,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:

      enum EntityType
      {
        EE_NOT_EXISTS = 0,
        EE_EXISTS
      };

      /**
       * @brief Constructor.
       *
       * @param entity type.
       */
      Restorer(
        EntityType entity_type);

      /**
       * @brief Restore entity to initial state.
       */      
      virtual void restore();

    protected:
      EntityType entity_type_;

      /**
       * @brief Destructor.
       */      
      virtual ~Restorer() noexcept;
    };

    /**
     * @class ORMRestorer
     * @brief Restore changed ORM entity.
     */
    template <typename Entity>
    class ORMRestorer :
      public Restorer,
      public Entity
    {
    public:

      /**
       * @brief Constructor.
       *
       * @param connection.
       * @param entity id.
       */
      ORMRestorer(
        DB::IConn& conn,
        unsigned long id);

      /**
       * @brief Constructor.
       *
       * @param entity.
       */
      ORMRestorer(
        const Entity& entity);

      /**
       * @brief Constructor.
       *
       * @param connection.
       */
      ORMRestorer(
        DB::IConn& conn);

    protected:
      /**
       * @brief Destructor.
       */      
      ~ORMRestorer() noexcept;

      /**
       * @brief Restore ORM entity to initial state.
       */  
      void restore();
    
    protected:
      Entity stored_;
    };

    typedef ReferenceCounting::SmartPtr<Restorer> Restorer_var;
    typedef std::list<Restorer_var> Restorers;


    /**
     * @class SerializeQueryManager_
     * @brief Serialize DB queries.
     *
     * It is usefull when need serialize sequence of queries,
     * for example call thread unsafed DB procedure.
     * Do not create this class directly, use Singleton logic:
     *   SerializeQueryManager::instance().execute(conn, query)
     */
    class SerializeQueryManager_ :
          public virtual ReferenceCounting::Interface,
          public virtual ReferenceCounting::DefaultImpl<>
    {

      typedef Sync::Policy::PosixThread SyncPolicy;
      typedef SyncPolicy::WriteGuard QueryGuard;
      SyncPolicy::Mutex lock_;
      
    public:
      SerializeQueryManager_();
      
      /**
       * @brief Execute query.
       *
       * @param connection
       * @param query
       * @return execute status
       */
      bool
      execute(
        AutoTest::DBC::IConn& connection,
        AutoTest::DBC::IQuery& query);

      /**
       * @brief Execute query (delete after execution).
       *
       * @param connection
       * @param query
       * @return execute status
       */
      bool
      execute(
        AutoTest::DBC::IConn& connection,
        AutoTest::DBC::IQuery* query);

      /**
       * @brief Get manager lock.
       *
       * @return lock
       */
      SyncPolicy::Mutex&
      lock();
    };
    

    typedef ReferenceCounting::SmartPtr<SerializeQueryManager_> SerializeQueryManager_var;
    typedef Generics::Singleton<SerializeQueryManager_, SerializeQueryManager_var> SerializeQueryManager;


    enum DisplayStatus
    {
      DS_INACTIVE = 0,
      DS_LIVE,
      DS_NOT_LIVE,
      DS_DELETED,
      DS_NOT_LIVE_DECLINED = 8,
      DS_NOT_LIVE_BY_CHANNEL_TARGET = 17,
      DS_NOT_LIVE_BY_OIX = 20
    };
    
    DECLARE_EXCEPTION(NoStatusException, eh::DescriptiveException);

    /**
     * @brief Get display status for object.
     *
     * @param connection
     * @param object name
     * @return status
     */
    int
    get_display_status_id(
      DB::IConn& conn,
      const char* object,
      DisplayStatus status);

  }//namespace ORM
}//namespace AutoTest

#include "ORM.ipp"

#endif //__AUTOTESTS_COMMONS_ORM_HPP
