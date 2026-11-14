#
# This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# Battle.net protocol buffers (generated with protoc 2.6.1), from TrinityCore 6.x src/server/proto.

CollectSourceFiles(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE_SOURCES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

add_library(proto
  ${PRIVATE_SOURCES})

CollectIncludeDirectories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC_INCLUDES
  # Exclude
  ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders)

target_include_directories(proto
  PUBLIC
    ${PUBLIC_INCLUDES}
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR})

target_compile_definitions(proto
  PRIVATE
    -DACORE_API_EXPORT_PROTO)

# generated code: no warnings of our own to fix
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(proto PRIVATE /W0)
else()
  target_compile_options(proto PRIVATE -w)
endif()

target_link_libraries(proto
  PRIVATE
    acore-dependency-interface
  PUBLIC
    common
    protobuf)

set_target_properties(proto
  PROPERTIES
    FOLDER
      "server")

if (BUILD_SHARED_LIBS)
  if (UNIX)
    install(TARGETS proto
      LIBRARY
        DESTINATION lib)
  elseif (WIN32)
    install(TARGETS proto
      RUNTIME
        DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()
endif()
