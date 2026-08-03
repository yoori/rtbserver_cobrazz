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
  cpu_set_t original_cpu_set;
  int original_cpu_set_available;
  int auto_cpus;
  int verbose;
};

typedef int (*PthreadSetname)(pthread_t, const char*);

static pthread_once_t init_once = PTHREAD_ONCE_INIT;
static struct Config config;
static uint64_t next_cpu_index = 0;
static __thread int current_thread_cpu = -1;
static PthreadSetname real_pthread_setname = NULL;

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
  if (text)
  {
    const size_t size = strlen(text);
    while (write(STDERR_FILENO, text, size) == -1 && errno == EINTR)
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
  while (value != 0);

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
  if (!config.auto_cpus || config.cpu_count < 2)
  {
    return;
  }

  uint64_t state = ((uint64_t)getpid() << 32) ^ (uint64_t)current_tid() ^ (uintptr_t)&config;

  for (unsigned int i = config.cpu_count - 1; i > 0; --i)
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
  while (pos && isspace((unsigned char)*pos))
  {
    ++pos;
  }
  return pos;
}

static int
parse_uint_value(const char** pos_ptr, int* value)
{
  const char* pos = skip_spaces(*pos_ptr);
  if (!pos || !isdigit((unsigned char)*pos))
  {
    return 0;
  }

  unsigned int result = 0;
  while (isdigit((unsigned char)*pos))
  {
    result = result * 10 + (unsigned int)(*pos - '0');
    if (result >= CPU_SETSIZE)
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
  if (cpu < 0 || cpu >= CPU_SETSIZE)
  {
    return;
  }

  for (unsigned int i = 0; i < config.cpu_count; ++i)
  {
    if (config.cpus[i] == cpu)
    {
      return;
    }
  }

  if (config.cpu_count < CPU_SETSIZE)
  {
    config.cpus[config.cpu_count++] = cpu;
  }
}

static void
init_auto_cpus()
{
  cpu_set_t cpu_set;
  if (sched_getaffinity(0, sizeof(cpu_set), &cpu_set) == 0)
  {
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
    {
      if (CPU_ISSET(cpu, &cpu_set))
      {
        add_cpu(cpu);
      }
    }

    if (config.cpu_count != 0)
    {
      return;
    }
  }

  long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  if (cpu_count <= 0 || cpu_count > CPU_SETSIZE)
  {
    cpu_count = CPU_SETSIZE;
  }

  for (int cpu = 0; cpu < cpu_count; ++cpu)
  {
    add_cpu(cpu);
  }
}

static void
parse_cpu_list(const char* value)
{
  if (!value || *skip_spaces(value) == '\0' || strcmp(skip_spaces(value), "auto") == 0)
  {
    config.auto_cpus = 1;
    init_auto_cpus();
    return;
  }

  const char* pos = value;
  while (1)
  {
    int begin = 0;
    if (!parse_uint_value(&pos, &begin))
    {
      config.cpu_count = 0;
      init_auto_cpus();
      return;
    }

    int end = begin;
    pos = skip_spaces(pos);
    if (*pos == '-')
    {
      ++pos;
      if (!parse_uint_value(&pos, &end) || end < begin)
      {
        config.cpu_count = 0;
        init_auto_cpus();
        return;
      }
    }

    for (int cpu = begin; cpu <= end; ++cpu)
    {
      add_cpu(cpu);
    }

    pos = skip_spaces(pos);
    if (*pos == '\0')
    {
      break;
    }

    if (*pos != ',')
    {
      config.cpu_count = 0;
      init_auto_cpus();
      return;
    }
    ++pos;
  }

  if (config.cpu_count == 0)
  {
    init_auto_cpus();
  }
}

static void
init_config()
{
  if (sched_getaffinity(0, sizeof(config.original_cpu_set), &config.original_cpu_set) == 0)
  {
    config.original_cpu_set_available = 1;
  }

  const char* mode = getenv("ADS_THREAD_AFFINITY");
  if (!is_enabled_value(mode))
  {
    config.mode = MODE_DISABLED;
    return;
  }

  config.mode = MODE_ROUND_ROBIN;
  config.verbose = is_enabled_value(getenv("ADS_THREAD_AFFINITY_VERBOSE"));
  parse_cpu_list(getenv("ADS_THREAD_AFFINITY_CPUS"));
  shuffle_auto_cpus();

  real_pthread_setname = (PthreadSetname)dlsym(RTLD_NEXT, "pthread_setname_np");

  if (config.verbose)
  {
    write_literal("thread-affinity-preload: mode=round_robin cpus=");
    for (unsigned int i = 0; i < config.cpu_count; ++i)
    {
      if (i != 0)
      {
        write_literal(",");
      }
      write_uint((unsigned int)config.cpus[i]);
    }
    write_literal("\n");
  }
}

static int
apply_current_thread_affinity()
{
  pthread_once(&init_once, init_config);

  if (config.mode != MODE_ROUND_ROBIN || config.cpu_count == 0)
  {
    return 0;
  }

  if (current_thread_cpu >= 0)
  {
    return 1;
  }

  const uint64_t index = __atomic_fetch_add(&next_cpu_index, 1, __ATOMIC_RELAXED);
  const int cpu = config.cpus[index % config.cpu_count];

  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpu, &cpu_set);

  const int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set);
  if (result == 0)
  {
    current_thread_cpu = cpu;
  }

  if (config.verbose)
  {
    write_literal("thread-affinity-preload: tid=");
    write_uint((uint64_t)current_tid());
    write_literal(" cpu=");
    write_uint((uint64_t)cpu);
    if (result != 0)
    {
      write_literal(" error=");
      write_uint((uint64_t)result);
    }
    write_literal("\n");
  }

  return result == 0;
}

static void
release_current_thread_affinity()
{
  pthread_once(&init_once, init_config);

  if (!config.original_cpu_set_available)
  {
    current_thread_cpu = -1;
    return;
  }

  const int result = pthread_setaffinity_np(
    pthread_self(),
    sizeof(config.original_cpu_set),
    &config.original_cpu_set);
  if (result == 0)
  {
    current_thread_cpu = -1;
  }

  if (config.verbose)
  {
    write_literal("thread-affinity-preload: tid=");
    write_uint((uint64_t)current_tid());
    write_literal(" no-affinity");
    if (result != 0)
    {
      write_literal(" error=");
      write_uint((uint64_t)result);
    }
    write_literal("\n");
  }
}

int
pthread_setname_np(pthread_t thread, const char* name)
{
  pthread_once(&init_once, init_config);

  if (!real_pthread_setname)
  {
    real_pthread_setname = (PthreadSetname)dlsym(RTLD_NEXT, "pthread_setname_np");

    if (!real_pthread_setname)
    {
      return ENOSYS;
    }
  }

  const int no_affinity = strncmp(name, "na:", 3) == 0;
  const int apply_affinity = strncmp(name, "ca:", 3) == 0;
  const char* effective_name = no_affinity || apply_affinity ? name + 3 : name;

  if (no_affinity &&
    config.mode == MODE_ROUND_ROBIN &&
    pthread_equal(thread, pthread_self()))
  {
    release_current_thread_affinity();
    return real_pthread_setname(thread, effective_name);
  }

  if (apply_affinity && pthread_equal(thread, pthread_self()))
  {
    apply_current_thread_affinity();
  }

  if (config.mode != MODE_ROUND_ROBIN ||
    current_thread_cpu < 0 ||
    !apply_affinity ||
    !pthread_equal(thread, pthread_self()))
  {
    return real_pthread_setname(thread, effective_name);
  }

  char suffix[16];
  const int suffix_size = snprintf(suffix, sizeof(suffix), ":c%d", current_thread_cpu);

  if (suffix_size <= 0 || suffix_size >= 15)
  {
    return real_pthread_setname(thread, effective_name);
  }

  char thread_name[16];
  const size_t max_prefix_size = 15 - (size_t)suffix_size;
  const size_t prefix_size = strnlen(effective_name, max_prefix_size);
  memcpy(thread_name, effective_name, prefix_size);
  memcpy(thread_name + prefix_size, suffix, (size_t)suffix_size);
  thread_name[prefix_size + (size_t)suffix_size] = '\0';

  return real_pthread_setname(thread, thread_name);
}
