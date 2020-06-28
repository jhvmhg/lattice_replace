cmake_minimum_required(VERSION 3.5)
project(lattice_replace)

set(CMAKE_CXX_STANDARD 14)
set(KALDI_PATH $ENV{KALDI_PATH})

include_directories(
        ${KALDI_PATH}/src
        ${KALDI_PATH}/tools/openfst/include
        )


add_executable(lattice_replace lattice-map.cpp)

target_link_libraries(lattice_replace
        ${KALDI_PATH}/src/lib/libkaldi-online2${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-nnet3${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-chain${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-nnet2${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-cudamatrix${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-decoder${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-lat${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-lm${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-fstext${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-hmm${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-feat${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-transform${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-gmm${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-tree${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-util${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-matrix${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/src/lib/libkaldi-base${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${KALDI_PATH}/tools/openfst/lib/libfst${CMAKE_SHARED_LIBRARY_SUFFIX}
        ${EXT_LIB}
        /usr/lib/x86_64-linux-gnu/libpthread${CMAKE_SHARED_LIBRARY_SUFFIX}
        )