
#ifndef _AUTOTESTS_COMMONS_STATS_DIFFSTATS_HPP
#define _AUTOTESTS_COMMONS_STATS_DIFFSTATS_HPP

#include <tests/AutoTests/Commons/Stats/ORMStats.hpp>

namespace AutoTest
{
  namespace ORM
  {
    /**
     * @class DiffStats
     * @brief Base class for statistic with diffs
     */
    template<typename T, int Count>
    class DiffStats: public BasicStats<T, Count>
    {

      typedef T Self;
      typedef BasicStats<T, Count> Base;
      
    public:

      class Diffs;
      /**
       * @brief Constructor.
       *
       * @param Statistic description.
       */
      DiffStats(
        const char* description);
            
      /**
       * @brief Print diff.
       * @param output stream.
       * @param diff.
       */

      static void print_diff(
        std::ostream& out, 
        const stats_diff_type& diff);

      static void print_diff(
        std::ostream& out,
        const Diffs& values);

      /**
       * @brief Print diff with statistic values.
       * @param output stream.
       * @param diff.
       * @param real (current) statistic
       */
      void print_diff(
        std::ostream& out, 
        const stats_diff_type& diff,
        const DiffStats<T, Count>& real) const;
      
      void print_diff(
        std::ostream& out, 
        const Diffs& diff,
        const DiffStats<T, Count>& real) const;
    };
  }
}

#include "DiffStats.tpp"

#endif  //_AUTOTESTS_COMMONS_STATS_DIFFSTATS_HPP
