cmake_minimum_required(VERSION 3.25) # сучасні можливості CUDA і C++17
project(transcripter C CXX)
project(transcripter VERSION 1.8.2)

# Встановлюємо стандарт C++
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Увімкнути GPU бекенд
option(GGML_CUDA "Enable CUDA backend" ON)
option(GGML_CUBLAS "Enable cuBLAS for CUDA" ON)

if(GGML_CUDA)
    enable_language(CUDA)
    message(STATUS "Building with CUDA support for GPU")
endif()

add_subdirectory(dependencies/whisper)

add_executable(transcripter "src/main.cpp")

# link whisper
target_link_libraries(transcripter PRIVATE whisper)

# Common compiling flags
add_library(COMMON_FLAGS INTERFACE)
# Sanitizer for tests
set(ENABLE_SANITIZERS OFF)
if (ENABLE_SANITIZERS AND MSVC)
    string(REPLACE "/RTC1" "" _cxx_dbg "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "${_cxx_dbg}" CACHE STRING "" FORCE)

    string(REPLACE "/RTC1" "" _c_dbg "${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_C_FLAGS_DEBUG "${_c_dbg}" CACHE STRING "" FORCE)

    message(STATUS "Sanitizers enabled: /RTC1 removed from Debug flags")
    # --- Create SANITIZERS interface only when requested
    add_library(SANITIZERS INTERFACE)
    if(MSVC)
      target_compile_options(SANITIZERS INTERFACE
        $<$<CONFIG:Debug>:/fsanitize=address>
      )
      target_link_options(SANITIZERS INTERFACE
        $<$<CONFIG:Debug>:/fsanitize=address>
      )
    else()
      target_compile_options(SANITIZERS INTERFACE
        $<$<CONFIG:Debug>:-fsanitize=address>
      )
      target_link_options(SANITIZERS INTERFACE
        $<$<CONFIG:Debug>:-fsanitize=address>
      )
    endif()
endif()

if(MSVC)
    # MSVC
    target_compile_options(COMMON_FLAGS INTERFACE
        # Debug
        $<$<CONFIG:Debug>:
            /Zi                 # Debug symbols
            /Od                 # Disable optimizations
            /W4                 # High warnings
            /EHsc               # Exception model
            /Oy-                # Keep frame pointers for backtrace
        >
        # Release
        $<$<CONFIG:Release>:
            /O2                 # Optimize speed
            /Ot                 # Favor fast code
            /Oi                 # Enable intrinsics
            /GL                 # Whole program optimization
            /DNDEBUG            # Disable asserts
            /W3                 # Moderate warning level
            /fp:fast            # Fast floating point
        >
    )
    if(NOT ENABLE_SANITIZERS)
      target_compile_options(COMMON_FLAGS INTERFACE 
      $<$<CONFIG:Debug>:
            /RTC1               # Runtime checks
        >
      )
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # GCC / Clang
    target_compile_options(COMMON_FLAGS INTERFACE
        # Debug
        $<$<CONFIG:Debug>:
            -g                  # Debug symbols
            -O0                 # No optimization
            -fno-omit-frame-pointer  # Keep frame pointers
            -Wall -Wextra -Wpedantic # Warnings
            -fno-inline         # Better stack traces
            -fexceptions        # Enable exceptions
        >
        # Release
        $<$<CONFIG:Release>:
            -O3                 # Max speed optimization
            -march=native       # Optimize for current CPU
            -mtune=native
            -DNDEBUG            # Disable asserts
            -ffast-math         # Fast floating point (test carefully)
            -funroll-loops      # Optional: unroll loops
            -fstrict-aliasing   # Strict aliasing
            -flto               # Link Time Optimization
        >
    )
else()
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()
# C++ standard
target_compile_features(COMMON_FLAGS INTERFACE cxx_std_23)

add_subdirectory(src)
target_link_libraries(transcripter PRIVATE ${DynamicLibraries})
target_include_directories(transcripter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_definitions(transcripter PRIVATE GGML_CUDA GGML_CUBLAS)

set_target_properties(transcripter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

add_subdirectory(tests)