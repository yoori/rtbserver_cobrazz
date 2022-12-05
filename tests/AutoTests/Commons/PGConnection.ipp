namespace AutoTest
{
  namespace PQ
  {
    inline
    void
    Conn::commit()
    { }

    inline
    unsigned int 
    BasicQueryStream::size() const
    {
      return oids_.size();
    }
  }
}
