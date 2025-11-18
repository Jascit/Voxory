cmake_minimum_required(VERSION 3.25)

project(NoyxTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

set(TEST_DETAILS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test_framework/include")
set(TESTS_ALL_LIBRARIES_DYNAMIC "")
set(TEST_SUBDIRECTORIES "")

set(CORE_TEST_FLAG ON)

foreach(CORE)
    if(${MOD}_TEST_FLAG)
        list(APPEND TEST_SUBDIRECTORIES ${MOD})
    endif()
endforeach()

if(CORE_TEST_FLAG)
  list(APPEND TEST_SUBDIRECTORIES CORE)
endif()

message(STATUS "------TOP_TEST CMAKE------")
foreach(SUBDIR ${TEST_SUBDIRECTORIES})
    set(SUBDIR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}")
    if(EXISTS "${SUBDIR_PATH}" AND IS_DIRECTORY "${SUBDIR_PATH}")
        message(STATUS "TOP_TEST: Adding directory to test: ${SUBDIR}")
        add_subdirectory("${SUBDIR_PATH}")
    else()
        message(WARNING "Directory ${SUBDIR_PATH} does not exist.")
    endif()
endforeach()


add_executable (tests 
  "${CMAKE_CURRENT_SOURCE_DIR}/test_framework/test_main.cpp"
) 

target_link_libraries(tests PRIVATE
    ${TESTS_ALL_LIBRARIES_DYNAMIC}
)

target_include_directories(tests PUBLIC
    ${TEST_DETAILS_PATH}
)

add_test(NAME tests COMMAND tests)