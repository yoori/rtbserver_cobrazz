package Common::ChunkDescription;

use Class::Struct
  ChunkDescription => [
    index => '$',
    total_chunks => '$',
    host => '$',
    hosts => '$',
    path => '$',
    version => '$',
    divided => '$',
  ];

1;