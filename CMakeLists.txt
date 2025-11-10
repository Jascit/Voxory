cmake_minimum_required(VERSION 3.5) # for add_link_options and implicit target directories.
project("transcriter" C CXX)
project("transcriter" VERSION 1.8.2)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/whisper)
