# CMakeLists.txt for the Core module
cmake_minimum_required(VERSION 3.14)
project(CoreModuleProject LANGUAGES CXX)

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

# C++ standard
target_compile_features(CoreModule PUBLIC cxx_std_23)

# Common compile definitions
target_compile_definitions(CoreModule PUBLIC
    $<$<CONFIG:Release>:NDEBUG>
)

# Platform/compiler-specific compile options
if(MSVC)
    # MSVC: Release flags (optimize for speed, enable whole program opt, fast math, use AVX2 if available)
    target_compile_options(CoreModule PRIVATE
        $<$<CONFIG:Release>:
            /O2          # optimize for speed
            /Ot          # favor code speed
            /Oi          # enable intrinsics
            /GL          # whole program optimization
            /fp:fast     # faster floating point
        >
    )

    # Linker options for MSVC in Release
    target_link_options(CoreModule PRIVATE
        $<$<CONFIG:Release>:
            /LTCG       # link-time code generation (use with /GL)
            /OPT:REF    # eliminate unreferenced functions/data
            /OPT:ICF    # fold identical COMDATs
        >
    )
else()
    # GCC/Clang flags for Release
    target_compile_options(CoreModule PRIVATE
        $<$<CONFIG:Release>:
            -O3
            -march=native
            -mtune=native
            -funroll-loops
            -fstrict-aliasing
            -ffast-math
            -fno-exceptions        # optional: remove if you rely on exceptions
            -fno-rtti             # optional: remove if you need RTTI
        >
    )

    # link-time optimization (LTO)
    target_link_options(CoreModule PRIVATE
        $<$<CONFIG:Release>:
            -flto
        >
    )
endif()

# Ensure PIC (useful for shared builds / linking into shared libs)
set_target_properties(CoreModule PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Adding CoreModule to DynamicModules
list(APPEND DynamicLibraries CoreModule)
set(DynamicLibraries ${DynamicLibraries} PARENT_SCOPE)
