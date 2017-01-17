add_library(oblipc SHARED
  client.c
  message.c
  mountpoint.c
  protocol.c
  remote.c
  server.c
)

target_link_libraries(oblipc PUBLIC oblcore oblnet)

install(TARGETS oblipc
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

# add_subdirectory(test)
