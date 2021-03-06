cmake_minimum_required(VERSION 3.1)
project (MAXENT)

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
find_package(ALPSCore 2.0 REQUIRED)

list(APPEND LINK_ALL ${ALPSCore_LIBRARIES})
#note: LAPACK also finds BLAS
find_package(LAPACK)
if(LAPACK_FOUND AND USE_LAPACK)
    set(HAVE_BLAS 1)
    set(HAVE_LAPACK 1)
    message(STATUS "Using LAPACK and BLAS routines for SVD")
    list(APPEND LINK_ALL ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES})
else()
    set(HAVE_BLAS 0)
    set(HAVE_LAPACK 0)
endif()
configure_file(
	"${PROJECT_SOURCE_DIR}/cmake/maxent_config.hpp.in"
	"${PROJECT_BINARY_DIR}/config/maxent_config.hpp"
    )

include_directories("${PROJECT_BINARY_DIR}/config")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
message(STATUS "Finding packages, please stand by...")
find_package(GSL REQUIRED)
list(APPEND LINK_ALL ${GSL_LIBRARIES})

# new ALPSCore provides Eigen
if (NOT ALPSCore_HAS_EIGEN_VERSION)
  find_package (Eigen3 3.1 REQUIRED)
endif()

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-Wno-return-type-c-linkage SUPPORTS_FLAG)
if(SUPPORTS_FLAG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-return-type-c-linkage")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")

#let gcc take advantage of Eigen3 vectorization
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
endif()

include_directories(${GSL_INCLUDE_DIR})
include_directories(${EIGEN3_INCLUDE_DIR})

set(LIB_FILES 
  ./src/maxent_helper.cpp
  ./src/maxent_params.cpp
  ./src/maxent_grid.cpp
  ./src/maxent_kernel.cpp
  ./src/maxent_simulation.cpp
  ./src/maxent_backcont.cpp
  ./src/default_model.cpp
  )

ADD_LIBRARY(libmaxent ${LIB_FILES})
target_link_libraries(libmaxent ${LINK_ALL})
#remove default "lib" prefix
set_target_properties(libmaxent PROPERTIES PREFIX "")
#executable
add_executable(maxent 
  ./src/maxent.cpp
  )
target_link_libraries(maxent libmaxent ${LINK_ALL})

#testing setup
option(Testing "Enable testing" ON)

if (Testing)
  add_library(gtest ./test/gtest-all.cc ./test/gtest_main.cc)
  list(APPEND LINK_ALL libmaxent)
  enable_testing(test)
  add_subdirectory(test)
endif (Testing)


#add companion utilities
add_subdirectory(legendre_convert)
add_subdirectory(kk) 
if(PADE)
  add_subdirectory(pade/pade_arbitrary_degree)
endif(PADE)

#install
install(TARGETS maxent DESTINATION bin)
