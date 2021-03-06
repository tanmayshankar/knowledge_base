cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)

# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
#set(ROS_BUILD_TYPE RelWithDebInfo)

rosbuild_init()

#set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#uncomment if you have defined messages
#rosbuild_genmsg()
#uncomment if you have defined services
#rosbuild_gensrv()


find_package(PCL 1.7 REQUIRED)

include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

# rosbuild_add_executable (spatial_rel_calc src/spatial_rel_calc.cpp)
# target_link_libraries (spatial_rel_calc ${PCL_LIBRARIES})

rosbuild_add_executable (spatial_rel src/spatial_relationships.cpp)
target_link_libraries (spatial_rel ${PCL_LIBRARIES})

rosbuild_add_executable (spatial_file src/spatial_file.cpp)
target_link_libraries (spatial_file ${PCL_LIBRARIES})

rosbuild_add_executable (spatial_mean_var src/spatial_mean_var.cpp)
target_link_libraries (spatial_mean_var ${PCL_LIBRARIES})

# rosbuild_add_executable(relative_coord src/relative_coord.cpp)
#target_link_libraries(relative_coord ${PCL_LIBRARIES})

rosbuild_add_executable(knowledge_base_sample src/knowledge_base_sample.cpp)

#rosbuild_add_executable (spatial_mean_var_noread src/spatial_mean_var_noread.cpp)
#target_link_libraries (spatial_mean_var_noread ${PCL_LIBRARIES})
#common commands for building c++ executables and libraries
#rosbuild_add_library(${PROJECT_NAME} src/example.cpp)
#target_link_libraries(${PROJECT_NAME} another_library)
#rosbuild_add_boost_directories()
#rosbuild_link_boost(${PROJECT_NAME} thread)
#rosbuild_add_executable(example examples/example.cpp)
#target_link_libraries(example ${PROJECT_NAME})
