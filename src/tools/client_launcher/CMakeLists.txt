# This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

CollectSourceFiles(${CMAKE_CURRENT_SOURCE_DIR} PRIVATE_SOURCES)


GroupSources(${CMAKE_CURRENT_SOURCE_DIR})

if(PLATFORM EQUAL 64)
  set(target_name client_launcher_64)
else()
  set(target_name client_launcher_32)
endif()

add_executable(${target_name} ${PRIVATE_SOURCES})

target_link_libraries(${target_name}
  PRIVATE
    acore-core-interface
  PUBLIC
    common
)

if(UNITY_BUILDS)
  set_target_properties(${target_name}
      PROPERTIES UNITY_BUILD ON)
  set_target_properties(${target_name}
      PROPERTIES
        UNITY_BUILD_MODE BATCH
        UNITY_BUILD_BATCH_SIZE 8)
endif()

set_target_properties(${target_name} PROPERTIES FOLDER "tools")

install(TARGETS ${target_name} DESTINATION "${CMAKE_INSTALL_PREFIX}")
