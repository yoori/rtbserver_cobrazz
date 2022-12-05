namespace AutoTest
{
  namespace ORM
  {
    template<typename T>
    inline
    void
    Unused(
      const T& x)
    {
      (void)x;
    }
    
    template<typename T>
    void
    setin_ (
      DB::Query& query,
      T& var)
    {
      if(var.changed())
      {
        query << var;
      }
    }

    template<typename T>
    void
    insertin_(
      DB::Query& query,
      T& var)
    {
      if(var.changed() || !var.is_null())
      {
        query << var;
      }
    }
  }
}


