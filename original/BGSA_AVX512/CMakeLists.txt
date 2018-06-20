cmake_minimum_required(VERSION 3.9)
project(BGSA_AVX512)

set(CMAKE_CXX_STANDARD 11)

add_executable(BGSA_AVX512
        align_core.c
        align_core.h
        cal.h
        cal_mic.c
        config.h
        convert.c
        file.c
        file.h
        global.c
        global.h
        main.c
        thread.c
        thread.h
        timer.h
        util.c
        util.h)
