set(FREERTOS_HEAP_ALLOCATOR 4)

list(APPEND SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/src/croutine.c
  ${CMAKE_CURRENT_LIST_DIR}/src/event_groups.c
  ${CMAKE_CURRENT_LIST_DIR}/src/list.c
  ${CMAKE_CURRENT_LIST_DIR}/src/queue.c
  ${CMAKE_CURRENT_LIST_DIR}/src/stream_buffer.c
  ${CMAKE_CURRENT_LIST_DIR}/src/tasks.c
  ${CMAKE_CURRENT_LIST_DIR}/src/timers.c
  ${CMAKE_CURRENT_LIST_DIR}/src/portable/GCC/${PROCESSOR_FAMILY}/port.c
  # include the specified heap allocator
  ${CMAKE_CURRENT_LIST_DIR}/src/portable/MemMang/heap_${FREERTOS_HEAP_ALLOCATOR}.c
  )
