// STD
#include <chrono>
#include <future>
#include <iostream>
#include <vector>
#include <thread>

std::uint64_t benchmark_task(
  const std::size_t iterations,
  const std::size_t block_size)
{
  const auto start = std::chrono::high_resolution_clock::now();
  for (std::size_t i = 0; i < iterations; ++i)
  {
    void* ptr = malloc(block_size);
    if (!ptr)
    {
      std::cerr << "Allocation failed!" << std::endl;
      exit(1);
    }
    free(ptr);
  }
  const auto end = std::chrono::high_resolution_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  return duration.count();
}

int main()
{
  const std::size_t num_threads = 128;
  const std::size_t block_size = 128;
  const std::size_t iterations = 10000000;

  std::vector<std::jthread> threads;
  threads.reserve(num_threads);
  std::vector<std::future<std::uint64_t>> futures;
  futures.reserve(num_threads);

  for (std::size_t i = 0; i < num_threads; i += 1)
  {
    std::packaged_task<std::uint64_t()> task(
      [iterations, block_size] () {
        return benchmark_task(iterations, block_size);
       });
    futures.emplace_back(task.get_future());
    threads.emplace_back(std::move(task));
  }

  std::uint64_t result = 0;
  for (auto& f : futures)
  {
    result += f.get();
  }
  result /= num_threads;

  std::cout << "Total time="
            << result
            << "[ms]"
            << std::endl;
}