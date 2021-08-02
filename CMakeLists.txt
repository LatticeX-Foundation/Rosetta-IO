cmake_minimum_required(VERSION 2.8)
SET(LIBNAME io)
project(${LIBNAME})

# setup directory where we should look for cmake modules files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#3rd
#include_directories(${CMAKE_SOURCE_DIR}/third_party/rapidjson/include)
#include_directories(${CMAKE_SOURCE_DIR}/third_party/spdlog-1.6.1/include)
if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    set(SPDLOG_MASTER_PROJECT ON)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/rapidjson/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include)
    message(STATUS "======== IO CURRENT ========")
else()
    include_directories(${CMAKE_SOURCE_DIR}/third_party/rapidjson/include)
    include_directories(${CMAKE_SOURCE_DIR}/third_party/spdlog/include)
    message(STATUS "======== IO SUBDIRECTORY ========")
endif()

# threads
find_package(Threads REQUIRED)

SET(ADD_LINK_LIB_FLAGS "-Wl,--rpath=$ORIGIN../:$ORIGIN")
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${ADD_LINK_LIB_FLAGS}")

IF(USE_GMTASSL)
    # GMSSL
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd/gmssl/include)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd/gmssl/lib)
    link_libraries(tacrypto tassl)
ELSE()
    # OpenSSL
    find_package(OpenSSL REQUIRED)
    include_directories(${OPENSSL_INCLUDE_DIR})
    link_libraries(${OPENSSL_LIBRARIES})
ENDIF()


############################### Begin
# libraries
file(GLOB_RECURSE srcs src/*.cpp)
add_library(${LIBNAME} SHARED ${srcs})
target_include_directories(${LIBNAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
#target_link_libraries(${LIBNAME} PUBLIC pthread)
add_dependencies(${LIBNAME} spdlog)
target_link_libraries(${LIBNAME} PUBLIC spdlog pthread)
#if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
#    target_link_libraries(${LIBNAME} PUBLIC pthread)
#else()
#    add_dependencies(${LIBNAME} spdlog)
#    target_link_libraries(${LIBNAME} PUBLIC spdlog pthread)
#endif()

set_target_properties(${LIBNAME} PROPERTIES FOLDER "io"
    APPEND_STRING PROPERTY LINK_FLAGS " ${ADD_LINK_LIB_FLAGS}")

install(TARGETS ${LIBNAME} DESTINATION lib)
install(DIRECTORY include/io DESTINATION include)
if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    add_subdirectory(third_party/spdlog)
endif()
#IF(ROSETTA_COMPILE_TESTS)
## examples
#function(compile_examples projname)
#    set(proj ${LIBNAME}-examples-${projname})
#    add_executable(${proj} ./examples/${projname}.cpp) 
#    target_link_libraries(${proj} ${LIBNAME})
#endfunction()
#compile_examples(net_broadcast)
#compile_examples(net_interactive)
#compile_examples(net_multiclients)
#compile_examples(net_sync)
#compile_examples(net_msgid)
##
#compile_examples(netio_ex)
#
## tests
#function(compile_tests projname)
#    set(proj ${LIBNAME}-tests-${projname})
#    add_executable(${proj} ./tests/${projname}.cpp ./tests/test.cpp) 
#    target_link_libraries(${proj} ${LIBNAME})
#endfunction()
#compile_tests(test_net_io)
#compile_tests(test_parallel_net_io)
################################ End
#ENDIF()
