add_subdirectory(Grpc)
add_subdirectory(PmrTests)

add_executable(GrpcAlgsTest GrpcAlgsTest.cpp)
target_link_libraries(GrpcAlgsTest PRIVATE Generics)

add_executable(AsyncMutexTest
  AsyncMutexTest.cpp
)

# Compile the implementation here so the linker can intercept its publication unlock.
add_executable(AsyncMutexPublicationTest
  AsyncMutexPublicationTest.cpp
  ../../Commons/AsyncMutex.cpp
  ../../Commons/Coro/Utils.cpp
)

target_link_libraries(AsyncMutexPublicationTest PRIVATE Boost::headers Threads::Threads)
target_link_options(AsyncMutexPublicationTest PRIVATE "-Wl,--wrap=pthread_mutex_unlock")

add_executable(OrderedAsyncTaskWindowTest
  OrderedAsyncTaskWindowTest.cpp
)

add_executable(ExecutorPoolTest
  ExecutorPoolTest.cpp
)

add_executable(StartableAwaitableTest
  StartableAwaitableTest.cpp
)

add_executable(MonoStartableAwaitableTest MonoStartableAwaitableTest.cpp)
target_link_libraries(MonoStartableAwaitableTest PRIVATE Commons Generics)

add_executable(CooperativeGroupTest CooperativeGroupTest.cpp)
target_link_libraries(CooperativeGroupTest PRIVATE Commons)
target_link_options(CooperativeGroupTest PRIVATE "-Wl,--wrap=pthread_mutex_lock")

add_executable(FastSchedulerPerfTest
  FastSchedulerPerfTest.cpp
)

add_executable(HttpServerAsyncTest
  HttpServerAsyncTest.cpp
)

add_executable(UserInfoManipTest
  UserInfoManipTest.cpp
)

target_link_libraries(AsyncMutexTest
  PRIVATE
    Commons
)

target_link_libraries(OrderedAsyncTaskWindowTest
  PRIVATE
    Commons
)

target_link_libraries(ExecutorPoolTest
  PRIVATE
    Commons
)

target_link_libraries(StartableAwaitableTest
  PRIVATE
    Commons
)

target_link_libraries(FastSchedulerPerfTest
  PRIVATE
    Commons
    Generics
)

target_link_libraries(HttpServerAsyncTest
  PRIVATE
    Commons
    HttpServer
)

target_link_libraries(UserInfoManipTest
  PRIVATE
    Commons
)
