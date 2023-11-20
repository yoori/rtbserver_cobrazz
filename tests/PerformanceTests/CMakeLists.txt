include_directories (
  ${rocksdb_SOURCE_DIR}/include
)

foreach(Path
  ProfilingCommons/ProfileMapBenchmark.cpp)
    get_filename_component(file_name ${Path} NAME_WE)
    add_executable(${file_name} ${Path})
    target_link_libraries(${file_name}
      ProfileMap
      UServerUtils
    )
endforeach()