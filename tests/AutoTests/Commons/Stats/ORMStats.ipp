template<typename T, size_t Count>
bool AutoTest::ORM::clear_stats(StatsDB::IConn& connection,
  const char* column_name,
  const T (&column_values)[Count])
{
  StatsDB::Query query(connection.get_query(
     "SELECT nspname, relname FROM pg_attribute "
     "JOIN pg_class ON pg_class.oid = attrelid "
     "JOIN pg_namespace ON pg_namespace.oid = relnamespace "
     "WHERE attname = :1 AND relkind = 'r' "
     "AND (nspname in ('stat', 'ctr') OR "
     "(nspname = 'adserver' AND relname like 'snapshot_%'))"));
  query.set(tolower(column_name));

  StatsDB::Result result(query.ask());
  bool ret = true;
  while (result.next())
  {
    std::string schema_name, table_name;
    result.get(schema_name);
    result.get(table_name);
    std::ostringstream query_str;
    query_str << "DELETE FROM ONLY " << schema_name << "." <<
      table_name << " WHERE " << column_name << " in (";
    for (size_t i = 0; i < Count; ++i)
    {
      query_str << ":i" << i <<
        (i == Count - 1? ")": ", ");
    }
    StatsDB::Query delete_query(connection.get_query(query_str.str()));
    for (size_t i = 0; i < Count; ++i)
    {
      delete_query.set(column_values[i]);
    }
    ret &= delete_query.update() > 0;
    connection.commit();
  }
  return ret;
}

template<typename T>
bool AutoTest::ORM::clear_stats(StatsDB::IConn& connection,
  const char* column_name,
  const T& column_value)
{
  const T array[] = { column_value };
  return clear_stats(connection, column_name, array);
}
