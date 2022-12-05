/* $Id$
* @file ProfilingServerStats.ipp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Profiling server statistics
*/

namespace AdServer
{
  namespace Profiling
  {
    template <typename Functor>
    void
    ProfilingServerStats::enumerate_all(Functor& functor) const
      /*throw(eh::Exception)*/
    {
      functor(6);
      functor(FILTERED_MESSAGES_COUNT_, static_cast<int>(filtered_messages_count_));
      functor(KAFKA_ERRORS_COUNT_, static_cast<int>(kafka_errors()));
      functor(SENT_MESSAGES_COUNT_, static_cast<int>(sent_messages()));
      functor(SENT_MESSAGES_SIZE_, static_cast<int>(sent_messages_size()));
      functor(RECEIVED_MESSAGES_COUNT_, static_cast<int>(received_messages_count_));
      functor(RECEIVED_MESSAGES_SIZE_, static_cast<int>(received_messages_size_));
    }
  }
}
