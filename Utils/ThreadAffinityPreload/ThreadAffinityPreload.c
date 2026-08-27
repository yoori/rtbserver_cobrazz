#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
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
  MODE_ROUND_ROBIN,
  MODE_ROUND_ROBIN_ALL,
  MODE_ROUND_ROBIN_BY_NAME
};

#define THREAD_TYPE_STATE_COUNT 128

struct Config
{
  enum Mode mode;
  int cpus[CPU_SETSIZE];
  unsigned int cpu_count;
  cpu_set_t original_cpu_set;
  int original_cpu_set_available;
  int auto_cpus;
  int restrict_to_original_cpu_set;
  int numa_node;
  int numa_node_set;
  int verbose;
};

struct ThreadTypeState
{
  int used;
  pid_t pid;
  int numa_node;
  char name[16];
  uint64_t base;
  uint64_t next;
};

struct StartContext
{
  void* (*start_routine)(void*);
  void* arg;
};

typedef int (*PthreadCreate)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);

typedef int (*PthreadSetname)(pthread_t, const char*);

static pthread_once_t init_once = PTHREAD_ONCE_INIT;
static struct Config config;
static uint64_t next_cpu_index = 0;
static pthread_mutex_t thread_type_states_lock = PTHREAD_MUTEX_INITIALIZER;
static struct ThreadTypeState thread_type_states[THREAD_TYPE_STATE_COUNT];
static __thread int current_thread_cpu = -1;
static PthreadCreate real_pthread_create = NULL;
static PthreadSetname real_pthread_setname = NULL;

static int
is_enabled_value(const char* value)
{
  return value &&
    (strcmp(value, "1") == 0 ||
      strcmp(value, "yes") == 0 ||
      strcmp(value, "true") == 0 ||
      strcmp(value, "on") == 0 ||
      strcmp(value, "round_robin") == 0 || strcmp(value, "round_robin_by_name") == 0);
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

static uint64_t
hash_name(const char* name)
{
  uint64_t result = 1469598103934665603ULL;
  while (name && *name)
  {
    result ^= (unsigned char)*name++;
    result *= 1099511628211ULL;
  }
  return result;
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

  if (config.restrict_to_original_cpu_set &&
    config.original_cpu_set_available &&
    !CPU_ISSET(cpu, &config.original_cpu_set))
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

static int
parse_cpu_list_into(const char* value, int* cpus, unsigned int* cpu_count)
{
  if (!value || *skip_spaces(value) == '\0' || strcmp(skip_spaces(value), "auto") == 0)
  {
    return 0;
  }

  const char* pos = value;
  while (1)
  {
    int begin = 0;
    if (!parse_uint_value(&pos, &begin))
    {
      return 0;
    }

    int end = begin;
    pos = skip_spaces(pos);
    if (*pos == '-')
    {
      ++pos;
      if (!parse_uint_value(&pos, &end) || end < begin)
      {
        return 0;
      }
    }

    for (int cpu = begin; cpu <= end; ++cpu)
    {
      int already_added = 0;
      for (unsigned int i = 0; i < *cpu_count; ++i)
      {
        if (cpus[i] == cpu)
        {
          already_added = 1;
          break;
        }
      }

      if (!already_added)
      {
        if (*cpu_count >= CPU_SETSIZE)
        {
          return 0;
        }
        cpus[(*cpu_count)++] = cpu;
      }
    }

    pos = skip_spaces(pos);
    if (*pos == '\0')
    {
      break;
    }

    if (*pos != ',')
    {
      return 0;
    }
    ++pos;
  }

  return *cpu_count != 0;
}

static int
parse_cpu_list_value(const char* value)
{
  int cpus[CPU_SETSIZE];
  unsigned int cpu_count = 0;
  if (!parse_cpu_list_into(value, cpus, &cpu_count))
  {
    return 0;
  }

  for (unsigned int i = 0; i < cpu_count; ++i)
  {
    add_cpu(cpus[i]);
  }

  return config.cpu_count != 0;
}

static int
load_numa_cpus(int node, const char* cpu_filter)
{
  char path[128];
  const int path_size = snprintf(
    path,
    sizeof(path),
    "/sys/devices/system/node/node%d/cpulist",
    node);
  if (path_size <= 0 || (size_t)path_size >= sizeof(path))
  {
    return 0;
  }

  const int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd == -1)
  {
    return 0;
  }

  char value[4096];
  const ssize_t size = read(fd, value, sizeof(value) - 1);
  close(fd);
  if (size <= 0)
  {
    return 0;
  }

  value[size] = '\0';
  int node_cpus[CPU_SETSIZE];
  unsigned int node_cpu_count = 0;
  if (!parse_cpu_list_into(value, node_cpus, &node_cpu_count))
  {
    return 0;
  }

  int selected_cpus[CPU_SETSIZE];
  unsigned int selected_cpu_count = 0;
  if (cpu_filter && *skip_spaces(cpu_filter) != '\0')
  {
    if (!parse_cpu_list_into(cpu_filter, selected_cpus, &selected_cpu_count))
    {
      return 0;
    }
  }

  for (unsigned int i = 0; i < node_cpu_count; ++i)
  {
    if (!cpu_filter || *skip_spaces(cpu_filter) == '\0')
    {
      add_cpu(node_cpus[i]);
      continue;
    }

    for (unsigned int j = 0; j < selected_cpu_count; ++j)
    {
      if (selected_cpus[j] == (int)i)
      {
        add_cpu(node_cpus[i]);
        break;
      }
    }
  }

  return config.cpu_count != 0;
}

static int
parse_affinity_cpus(const char* value)
{
  char expression[4096];
  const char* begin = skip_spaces(value);
  if (!begin || *begin == '\0' || strcmp(begin, "auto") == 0)
  {
    config.auto_cpus = 1;
    init_auto_cpus();
    return config.cpu_count != 0;
  }

  const size_t value_size = strlen(begin);
  if (value_size >= sizeof(expression))
  {
    return 0;
  }
  memcpy(expression, begin, value_size + 1);

  char* end = expression + value_size;
  while (end > expression && isspace((unsigned char)end[-1]))
  {
    --end;
  }
  *end = '\0';
  const size_t expression_size = strlen(expression);
  if (expression[0] == '(')
  {
    if (expression_size <= 1 || expression[expression_size - 1] != ')')
    {
      return 0;
    }
    expression[expression_size - 1] = '\0';
    memmove(expression, expression + 1, strlen(expression));
  }

  char* node_spec = NULL;
  char* cpu_spec = NULL;
  char* segment = expression;
  while (segment)
  {
    char* next = strchr(segment, ';');
    if (next)
    {
      *next = '\0';
    }

    segment = (char*)skip_spaces(segment);
    if (strncmp(segment, "n:", 2) == 0 && !node_spec)
    {
      node_spec = segment + 2;
    }
    else if (strncmp(segment, "c:", 2) == 0 && !cpu_spec)
    {
      cpu_spec = segment + 2;
    }
    else if (strchr(segment, ':') != NULL)
    {
      return 0;
    }
    else if (*segment != '\0')
    {
      cpu_spec = segment;
    }

    segment = next ? next + 1 : NULL;
  }

  config.cpu_count = 0;
  if (node_spec)
  {
    int nodes[CPU_SETSIZE];
    unsigned int node_count = 0;
    if (*skip_spaces(node_spec) != '\0' && !parse_cpu_list_into(node_spec, nodes, &node_count))
    {
      return 0;
    }

    config.restrict_to_original_cpu_set = 1;
    config.numa_node_set = 1;
    config.numa_node = node_count == 1 ? nodes[0] : -2;
    if (node_count == 0)
    {
      unsigned int loaded_nodes = 0;
      for (int node = 0; node < CPU_SETSIZE; ++node)
      {
        char path[128];
        const int path_size = snprintf(
          path,
          sizeof(path),
          "/sys/devices/system/node/node%d/cpulist",
          node);
        if (path_size > 0 && (size_t)path_size < sizeof(path) && access(path, R_OK) == 0)
        {
          if (!load_numa_cpus(node, cpu_spec))
          {
            return 0;
          }
          ++loaded_nodes;
        }
      }
      return loaded_nodes != 0 && config.cpu_count != 0;
    }

    for (unsigned int i = 0; i < node_count; ++i)
    {
      if (!load_numa_cpus(nodes[i], cpu_spec))
      {
        return 0;
      }
    }
    return config.cpu_count != 0;
  }

  return cpu_spec ? parse_cpu_list_value(cpu_spec) : 0;
}

static struct ThreadTypeState*
get_thread_type_state(const char* name, int numa_node)
{
  const pid_t pid = getpid();
  const size_t name_size = strnlen(name ? name : "", 15);
  const uint64_t name_hash = hash_name(name);

  pthread_mutex_lock(&thread_type_states_lock);

  struct ThreadTypeState* free_state = NULL;
  for (unsigned int i = 0; i < THREAD_TYPE_STATE_COUNT; ++i)
  {
    struct ThreadTypeState* state = &thread_type_states[i];
    if (!state->used)
    {
      if (!free_state)
      {
        free_state = state;
      }
      continue;
    }

    if (state->pid == pid &&
      state->numa_node == numa_node &&
      strncmp(state->name, name ? name : "", sizeof(state->name)) == 0)
    {
      pthread_mutex_unlock(&thread_type_states_lock);
      return state;
    }
  }

  if (!free_state)
  {
    pthread_mutex_unlock(&thread_type_states_lock);
    return NULL;
  }

  free_state->used = 1;
  free_state->pid = pid;
  free_state->numa_node = numa_node;
  memcpy(free_state->name, name ? name : "", name_size);
  free_state->name[name_size] = '\0';
  free_state->base = mix_uint64(
    ((uint64_t)(uint32_t)pid << 32) ^
    name_hash ^
    ((uint64_t)(uint32_t)(numa_node + 1) << 48) ^
    (uintptr_t)free_state);
  free_state->next = 0;

  pthread_mutex_unlock(&thread_type_states_lock);
  return free_state;
}

static uint64_t
next_cpu_index_for_name(const char* name)
{
  const int numa_node = config.numa_node_set ? config.numa_node : -1;
  struct ThreadTypeState* state = get_thread_type_state(name, numa_node);
  if (!state)
  {
    return __atomic_fetch_add(&next_cpu_index, 1, __ATOMIC_RELAXED);
  }

  return state->base +
    __atomic_fetch_add(&state->next, 1, __ATOMIC_RELAXED);
}

static void
init_config()
{
  if (sched_getaffinity(0, sizeof(config.original_cpu_set), &config.original_cpu_set) == 0)
  {
    config.original_cpu_set_available = 1;
  }

  const char* mode = getenv("ADS_THREAD_AFFINITY");
  if (mode && strcmp(mode, "round_robin_all") == 0)
  {
    config.mode = MODE_ROUND_ROBIN_ALL;
  }
  else if (mode && strcmp(mode, "round_robin_by_name") == 0)
  {
    config.mode = MODE_ROUND_ROBIN_BY_NAME;
  }
  else if (is_enabled_value(mode))
  {
    config.mode = MODE_ROUND_ROBIN;
  }
  else
  {
    config.mode = MODE_DISABLED;
    return;
  }

  config.verbose = is_enabled_value(getenv("ADS_THREAD_AFFINITY_VERBOSE"));

  if (!parse_affinity_cpus(getenv("ADS_THREAD_AFFINITY_CPUS")))
  {
    config.cpu_count = 0;
    config.auto_cpus = 1;
    init_auto_cpus();
  }
  shuffle_auto_cpus();

  real_pthread_create = (PthreadCreate)dlsym(RTLD_NEXT, "pthread_create");
  real_pthread_setname = (PthreadSetname)dlsym(RTLD_NEXT, "pthread_setname_np");

  if (config.verbose)
  {
    write_literal("thread-affinity-preload: mode=");
    if (config.mode == MODE_ROUND_ROBIN_ALL)
    {
      write_literal("round_robin_all");
    }
    else if (config.mode == MODE_ROUND_ROBIN_BY_NAME)
    {
      write_literal("round_robin_by_name");
    }
    else
    {
      write_literal("round_robin");
    }
    write_literal(" cpus=");
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
apply_current_thread_affinity(const char* thread_name)
{
  pthread_once(&init_once, init_config);

  if (config.mode == MODE_DISABLED || config.cpu_count == 0)
  {
    return 0;
  }

  if (current_thread_cpu >= 0)
  {
    return 1;
  }

  const uint64_t index = config.mode == MODE_ROUND_ROBIN_BY_NAME ?
    next_cpu_index_for_name(thread_name) :
    __atomic_fetch_add(&next_cpu_index, 1, __ATOMIC_RELAXED);
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

static void*
thread_start_wrapper(void* arg)
{
  struct StartContext* context = (struct StartContext*)arg;
  void* (*start_routine)(void*) = context->start_routine;
  void* start_arg = context->arg;
  free(context);

  if (config.mode != MODE_ROUND_ROBIN_BY_NAME)
  {
    apply_current_thread_affinity("");
  }
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

  if (!real_pthread_create)
  {
    real_pthread_create = (PthreadCreate)dlsym(RTLD_NEXT, "pthread_create");
    if (!real_pthread_create)
    {
      return EAGAIN;
    }
  }

  if (config.mode != MODE_ROUND_ROBIN_ALL)
  {
    return real_pthread_create(thread, attr, start_routine, arg);
  }

  struct StartContext* context = malloc(sizeof(struct StartContext));
  if (!context)
  {
    return real_pthread_create(thread, attr, start_routine, arg);
  }

  context->start_routine = start_routine;
  context->arg = arg;

  const int result = real_pthread_create(thread, attr, thread_start_wrapper, context);
  if (result != 0)
  {
    free(context);
  }

  return result;
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
  const int affinity_by_default =
    config.mode == MODE_ROUND_ROBIN_ALL || config.mode == MODE_ROUND_ROBIN_BY_NAME;
  const char* effective_name = no_affinity || apply_affinity ? name + 3 : name;

  if (no_affinity && config.mode != MODE_DISABLED && pthread_equal(thread, pthread_self()))
  {
    release_current_thread_affinity();
    return real_pthread_setname(thread, effective_name);
  }

  if ((apply_affinity || affinity_by_default) && pthread_equal(thread, pthread_self()))
  {
    apply_current_thread_affinity(effective_name);
  }

  if (config.mode == MODE_DISABLED ||
    current_thread_cpu < 0 ||
    (!apply_affinity && !affinity_by_default) ||
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

__attribute__((constructor)) static void
thread_affinity_preload_init()
{
  pthread_once(&init_once, init_config);
  if (config.mode == MODE_ROUND_ROBIN_ALL)
  {
    apply_current_thread_affinity("");
  }
}
