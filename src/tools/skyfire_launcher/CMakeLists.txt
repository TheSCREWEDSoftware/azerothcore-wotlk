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

# Windows only: starts the 5.4.8 client and points it at this server (see Main.cpp)
add_executable(skyfire_launcher Main.cpp)

target_link_libraries(skyfire_launcher
  PRIVATE
    acore-core-interface)

set_target_properties(skyfire_launcher PROPERTIES FOLDER "tools")

install(TARGETS skyfire_launcher DESTINATION "${CMAKE_INSTALL_PREFIX}")
