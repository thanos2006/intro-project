cmake_minimum_required(VERSION 3.16)
project(wav_processor)


set(CMAKE_CXX_STANDARD 17)

include_directories(.)


add_executable(wav_processor wav_processor.cpp kiss_fft.c)

target_sources(wav_processor PRIVATE AudioFile.h json.hpp)

install(TARGETS wav_processor DESTINATION bin)
