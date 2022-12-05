#ifndef BARRIER_HPP
#define BARRIER_HPP

#include <Generics/Uncopyable.hpp>
#include <pthread.h>

class Barrier: private Generics::Uncopyable
{
public:
  Barrier(unsigned int count) noexcept;

  ~Barrier() noexcept;

  bool wait() noexcept;

private:
  pthread_barrier_t barrier_;
};

inline
Barrier::Barrier(unsigned int count) noexcept
{
  pthread_barrier_init(&barrier_, NULL, count);
}

inline
Barrier::~Barrier() noexcept
{
  pthread_barrier_destroy(&barrier_);
}

inline
bool
Barrier::wait() noexcept
{
  int res = pthread_barrier_wait(&barrier_);
  return (res == PTHREAD_BARRIER_SERIAL_THREAD);
}

#endif  /*BARRIER_HPP*/
