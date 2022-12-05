
template <typename Entity>
AutoTest::ORM::ORMRestorer<Entity>*
BaseDBUnit::create(unsigned long id)
  /*throw(eh::Exception)*/
{
  typedef AutoTest::ORM::ORMRestorer<Entity> ORMEntity;
  typedef typename Entity::Connection ConnectionType;

  ORMEntity* entity =
    new ORMEntity(
      get_conn(ConnectionType()), id);
  
  restorers_.push_front(entity);

  return entity;
}

template <typename Entity>
AutoTest::ORM::ORMRestorer<Entity>*
BaseDBUnit::create(const Entity& e)
  /*throw(eh::Exception)*/
{
  typedef AutoTest::ORM::ORMRestorer<Entity> ORMEntity;

  ORMEntity* entity =
    new ORMEntity(e);

  restorers_.push_front(entity);

  return entity;
}

template <typename Entity>
AutoTest::ORM::ORMRestorer<Entity>*
BaseDBUnit::create()
  /*throw(eh::Exception)*/
{
  typedef AutoTest::ORM::ORMRestorer<Entity> ORMEntity;
  typedef typename Entity::Connection ConnectionType;

  ORMEntity* entity =
    new ORMEntity(
      get_conn(ConnectionType()));
  
  restorers_.push_front(entity);

  return entity;
}

inline
AutoTest::DBC::Conn&
BaseDBUnit::get_conn(
  AutoTest::ORM::postgres_connection)
{
  return pq_conn_;
}

