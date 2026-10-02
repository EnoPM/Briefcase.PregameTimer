add_library(Briefcase.PregameTimer.UE4SS SHARED
  PregameTimerUe4ss.cpp)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" MOD_VERSION)
string(STRIP "${MOD_VERSION}" MOD_VERSION)
configure_file(ModVersion.hpp.in generated/ModVersion.hpp @ONLY)
target_include_directories(Briefcase.PregameTimer.UE4SS PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(Briefcase.PregameTimer.UE4SS PRIVATE
  Briefcase::DeceiveInc
  nlohmann_json::nlohmann_json)
set_target_properties(Briefcase.PregameTimer.UE4SS PROPERTIES OUTPUT_NAME main)
