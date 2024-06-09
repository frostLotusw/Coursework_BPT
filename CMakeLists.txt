cmake_minimum_required(VERSION 3.22.1)
project(code)

set(CMAKE_CXX_STANDARD 17)

add_executable(code b_plus_tree.h
        page_manager.h
        utils/qsort.h
        utils/vector.h
        utils/heap.h
        utils/pair.h
        utils/hash.h
        utils/binary_search.h
        utils/fast_read.h
        main.cpp)
