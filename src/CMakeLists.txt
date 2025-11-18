# CMakeLists.txt for the Core module
set(CORE_INCLUDES "${CMAKE_SOURCE_DIR}/include/")
file(GLOB_RECURSE ALL_CPP CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

set(CORE_SRC "")
foreach(f ${ALL_CPP})
    file(RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}" "${f}")
    string(FIND "${rel}" "/" pos)
    if(NOT pos EQUAL -1)
        list(APPEND CORE_SRC "${f}")
    endif()
endforeach()

message(STATUS "Found cpp in subdirs: ${CORE_SRC}")

add_library(CoreModule STATIC ${CORE_SRC})

target_include_directories(CoreModule PUBLIC
    ${CORE_INCLUDES}
)
target_compile_features(CoreModule PUBLIC cxx_std_17)

set_target_properties(CoreModule PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Adding CoreModule to DynamicModules
list(APPEND DynamicLibraries CoreModule)

#          DynamicModules                       
set(DynamicLibraries ${DynamicLibraries} PARENT_SCOPE)