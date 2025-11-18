cmake_minimum_required(VERSION 3.25) # сучасні можливості CUDA і C++17
project(transcripter C CXX)
project(transcripter VERSION 1.8.2)

# Встановлюємо стандарт C++
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)

endif()
set(DynamicLibraries "")
# Увімкнути GPU бекенд
option(GGML_CUDA "Enable CUDA backend" ON)
option(GGML_CUBLAS "Enable cuBLAS for CUDA" ON)

if(GGML_CUDA)
    enable_language(CUDA)
    message(STATUS "Building with CUDA support for GPU")
endif()
# Tests
add_subdirectory(dependencies/whisper)
# Ваш exe
add_executable(transcripter "src/main.cpp")

# Лінкуємо whisper
target_link_libraries(transcripter PRIVATE whisper)
add_subdirectory(src)
target_link_libraries(transcripter PRIVATE DynamicLibraries)
target_include_directories(transcripter PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Встановлюємо defines для GPU
target_compile_definitions(transcripter PRIVATE GGML_CUDA GGML_CUBLAS)
# Output директорії
set_target_properties(transcripter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

# Оптимізації під компілятор
if(MSVC)
    target_compile_options(transcripter PRIVATE
        $<$<CONFIG:Debug>:/RTC1 /MTd>
        $<$<CONFIG:Release>:/O2 /MT>
    )
    target_link_libraries(transcripter PRIVATE
        ole32.lib
        avrt.lib
    )
else()
    message(FATAL_ERROR "Цей приклад лише для Windows/MSVC")
endif()

add_subdirectory(tests)