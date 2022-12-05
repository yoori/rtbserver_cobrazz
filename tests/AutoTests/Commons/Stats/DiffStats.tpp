
namespace AutoTest
{
  namespace ORM
  {
    template<typename T, int Count>
    DiffStats<T, Count>::DiffStats (
      const char* description) :
      Base(description)
    {}

    template<typename T, int Count>
    void  DiffStats<T, Count>::print_diff(
      std::ostream& out, 
      const stats_diff_type& diff)
    {
      Base::print_diff(out, diff);
    }


    template<typename T, int Count>
    void
    DiffStats<T, Count>::print_diff(
      std::ostream& out,
      const Diffs& values)
    {
      for(int i = 0; i < Count-1; ++i)
      {
        Base::print_diff(out, values[i]);
        out << ", ";
      }
      Base::print_diff(out, values[Count-1]);
    }

    template<typename T, int Count>
    void DiffStats<T, Count>::print_diff(
      std::ostream& out, 
      const stats_diff_type& diff,
      const DiffStats<T, Count>& real) const
    {
      Base::print_diff(out, diff, real);
    }

    template<typename T, int Count>
    void
    DiffStats<T, Count>::print_diff(
      std::ostream& out, 
      const Diffs& diff,
      const DiffStats<T, Count>& real) const
    {
      std::ostringstream diff_out;
      bool not_expected = false;
      for(int i = 0;  i < Count; ++i)
      {
        diff_out << "  " <<
          Base::field_names[i] <<
          " = " << Base::values[i] <<
          " + ";
        Base::print_diff(diff_out, diff[i]);
        diff_out << " -> " << real[i] << ';';
        if(diff[i] != any_stats_diff &&
          !equal(diff[i], real[i],
            Base::values[i]))
        {
          not_expected = true;
          diff_out << " *** ";
        }
        diff_out << std::endl;
      }
      if (not_expected) out << "!FAILED! ";
      static_cast<const T*>(this)->print_idname (out);
      out << diff_out.str();
    }
  }
}
