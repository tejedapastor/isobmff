project(bgw_base)

include_directories(src)
include_directories(../libisomediafile/src)
include_directories(
    # Windows
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_LIST_DIR}/../libisomediafile/w32>
)
set(COMMON_SOURCE src/bgw.c src/bgw_tools.c)

add_executable(bgw_muxer ${COMMON_SOURCE} src/bgw_mux_main.c)

target_link_libraries(
  bgw_muxer
  
  LINK_PUBLIC
  
  $<$<PLATFORM_ID:Linux>:m>
  libisomediafile
)

add_executable(bgw_demuxer ${COMMON_SOURCE} src/bgw_demux_main.c)

target_link_libraries(
  bgw_demuxer
  
  LINK_PUBLIC
  
  $<$<PLATFORM_ID:Linux>:m>
  libisomediafile
)
