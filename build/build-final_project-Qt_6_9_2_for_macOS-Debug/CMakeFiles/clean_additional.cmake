# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/StaticGLEW_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/StaticGLEW_autogen.dir/ParseCache.txt"
  "CMakeFiles/final_project_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/final_project_autogen.dir/ParseCache.txt"
  "StaticGLEW_autogen"
  "final_project_autogen"
  )
endif()
