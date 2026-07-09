#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

enum Mode
{
  MODE_DISABLED,
  MODE_ROUND_ROBIN
};

struct Config
{
  enum Mode mode;
  int cpus[CPU_SETSIZE];
  unsigned int cpu_count;
  int auto_cpus;
  int verbose;
};

struct StartContext
{
  void* (*start_routine)(void*);
  void* arg;
};

typedef int (*PthreadCreate)(
  pthread_t*,
  const pthread_attr_t*,
  void* (*)(void*),
  void*);

static pthread_once_t init_once = PTHREAD_ONCE_INIT;
static struct Config config;
static uint64_t next_cpu_index = 0;
static PthreadCreate real_pthread_create = NULL;

static int
is_enabled_value(const char* value)
{
  return value &&
    (strcmp(value, "1") == 0 ||
      strcmp(value, "yes") == 0 ||
      strcmp(value, "true") == 0 ||
      strcmp(value, "on") == 0 ||
      strcmp(value, "round_robin") == 0);
}

static void
write_literal(const char* text)
{
  if(text)
  {
    const size_t size = strlen(text);
    while(write(STDERR_FILENO, text, size) == -1 && errno == EINTR)
    {}
  }
}

static void
write_uint(uint64_t value)
{
  char buf[32];
  char* pos = buf + sizeof(buf);
  *--pos = '\0';
  do
  {
    *--pos = (char)('0' + value % 10);
    value /= 10;
  }
  while(value != 0);
  write_literal(pos);
}

static long
current_tid()
{
  return syscall(SYS_gettid);
}

static uint64_t
mix_uint64(uint64_t value)
{
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}

static void
shuffle_auto_cpus()
{
  if(!config.auto_cpus || config.cpu_count < 2)
  {
    return;
  }

  uint64_t state =
    ((uint64_t)getpid() << 32) ^
    (uint64_t)current_tid() ^
    (uintptr_t)&config;

  for(unsigned int i = config.cpu_count - 1; i > 0; --i)
  {
    state = mix_uint64(state);
    const unsigned int j = (unsigned int)(state % (i + 1));
    const int cpu = config.cpus[i];
    config.cpus[i] = config.cpus[j];
    config.cpus[j] = cpu;
  }
}

static const char*
skip_spaces(const char* pos)
{
  while(pos && isspace((unsigned char)*pos))
  {
    ++pos;
  }
  return pos;
}

static int
parse_uint_value(const char** pos_ptr, int* value)
{
  const char* pos = skip_spaces(*pos_ptr);
  if(!pos || !isdigit((unsigned char)*pos))
  {
    return 0;
  }

  unsigned int result = 0;
  while(isdigit((unsigned char)*pos))
  {
    result = result * 10 + (unsigned int)(*pos - '0');
    if(result >= CPU_SETSIZE)
    {
      return 0;
    }
    ++pos;
  }

  *value = (int)result;
  *pos_ptr = pos;
  return 1;
}

static void
add_cpu(int cpu)
{
  if(cpu < 0 || cpu >= CPU_SETSIZE)
  {
    return;
  }

  for(unsigned int i = 0; i < config.cpu_count; ++i)
  {
    if(config.cpus[i] == cpu)
    {
      return;
    }
  }

  if(config.cpu_count < CPU_SETSIZE)
  {
    config.cpus[config.cpu_count++] = cpu;
  }
}

static void
init_auto_cpus()
{
  cpu_set_t cpu_set;
  if(sched_getaffinity(0, sizeof(cpu_set), &cpu_set) == 0)
  {
    for(int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
    {
      if(CPU_ISSET(cpu, &cpu_set))
      {
        add_cpu(cpu);
      }
    }

    if(config.cpu_count != 0)
    {
      return;
    }
  }

  long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  if(cpu_count <= 0 || cpu_count > CPU_SETSIZE)
  {
    cpu_count = CPU_SETSIZE;
  }

  for(int cpu = 0; cpu < cpu_count; ++cpu)
  {
    add_cpu(cpu);
  }
}

static void
parse_cpu_list(const char* value)
{
  if(!value || *skip_spaces(value) == '\0' ||
    strcmp(skip_spaces(value), "auto") == 0)
  {
    config.auto_cpus = 1;
    init_auto_cpus();
    return;
  }

  const char* pos = value;
  while(1)
  {
    int begin = 0;
    if(!parse_uint_value(&pos, &begin))
    {
      config.cpu_count = 0;
      init_auto_cpus();
      return;
    }

    int end = begin;
    pos = skip_spaces(pos);
    if(*pos == '-')
    {
      ++pos;
      if(!parse_uint_value(&pos, &end) || end < begin)
      {
        config.cpu_count = 0;
        init_auto_cpus();
        return;
      }
    }

    for(int cpu = begin; cpu <= end; ++cpu)
    {
      add_cpu(cpu);
    }

    pos = skip_spaces(pos);
    if(*pos == '\0')
    {
      break;
    }
    if(*pos != ',')
    {
      config.cpu_count = 0;
      init_auto_cpus();
      return;
    }
    ++pos;
  }

  if(config.cpu_count == 0)
  {
    init_auto_cpus();
  }
}

static void
init_config()
{
  const char* mode = getenv("ADS_THREAD_AFFINITY");
  if(!is_enabled_value(mode))
  {
    config.mode = MODE_DISABLED;
    return;
  }

  config.mode = MODE_ROUND_ROBIN;
  config.verbose = is_enabled_value(getenv("ADS_THREAD_AFFINITY_VERBOSE"));
  parse_cpu_list(getenv("ADS_THREAD_AFFINITY_CPUS"));
  shuffle_auto_cpus();

  real_pthread_create = (PthreadCreate)dlsym(RTLD_NEXT, "pthread_create");

  if(config.verbose)
  {
    write_literal("thread-affinity-preload: mode=round_robin cpus=");
    for(unsigned int i = 0; i < config.cpu_count; ++i)
    {
      if(i != 0)
      {
        write_literal(",");
      }
      write_uint((unsigned int)config.cpus[i]);
    }
    write_literal("\n");
  }
}

static void
pin_current_thread()
{
  pthread_once(&init_once, init_config);

  if(config.mode != MODE_ROUND_ROBIN || config.cpu_count == 0)
  {
    return;
  }

  const uint64_t index = __atomic_fetch_add(
    &next_cpu_index,
    1,
    __ATOMIC_RELAXED);
  const int cpu = config.cpus[index % config.cpu_count];

  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpu, &cpu_set);

  const int result = pthread_setaffinity_np(
    pthread_self(),
    sizeof(cpu_set),
    &cpu_set);

  if(config.verbose)
  {
    write_literal("thread-affinity-preload: tid=");
    write_uint((uint64_t)current_tid());
    write_literal(" cpu=");
    write_uint((uint64_t)cpu);
    if(result != 0)
    {
      write_literal(" error=");
      write_uint((uint64_t)result);
    }
    write_literal("\n");
  }
}

static void*
thread_start_wrapper(void* arg)
{
  struct StartContext* context = (struct StartContext*)arg;
  void* (*start_routine)(void*) = context->start_routine;
  void* start_arg = context->arg;
  free(context);

  pin_current_thread();
  return start_routine(start_arg);
}

int
pthread_create(
  pthread_t* thread,
  const pthread_attr_t* attr,
  void* (*start_routine)(void*),
  void* arg)
{
  pthread_once(&init_once, init_config);

  if(!real_pthread_create)
  {
    real_pthread_create = (PthreadCreate)dlsym(RTLD_NEXT, "pthread_create");
    if(!real_pthread_create)
    {
      return EAGAIN;
    }
  }

  if(config.mode != MODE_ROUND_ROBIN)
  {
    return real_pthread_create(thread, attr, start_routine, arg);
  }

  struct StartContext* context = (struct StartContext*)malloc(
    sizeof(struct StartContext));
  if(!context)
  {
    return real_pthread_create(thread, attr, start_routine, arg);
  }

  context->start_routine = start_routine;
  context->arg = arg;

  const int result = real_pthread_create(
    thread,
    attr,
    thread_start_wrapper,
    context);
  if(result != 0)
  {
    free(context);
  }

  return result;
}

__attribute__((constructor)) static void
thread_affinity_preload_init()
{
  pin_current_thread();
}
