cmake_minimum_required(VERSION 3.22)

project(01-hello-world CXX)

add_executable(hello_cout_dynamic cout_main.cxx)
target_compile_features(hello_cout_dynamic PRIVATE cxx_std_20)

message(VERBOSE "${CMAKE_SYSTEM_NAME}")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(VERBOSE "use gcc/linux specific code here")
    # install static version of standard libraries >sudo dnf install
    # libstdc++-static glibc-static
    add_executable(hello_cout_static cout_main.cxx)
    target_link_options(hello_cout_static PRIVATE -static)
    target_compile_features(hello_cout_static PRIVATE cxx_std_20)

    add_executable(hello_nostartfiles hack_main.cxx)
    target_link_options(hello_nostartfiles PRIVATE -nostartfiles)
    target_compile_features(hello_nostartfiles PRIVATE cxx_std_20)

    enable_language(ASM)
    add_executable(hello_asm)
    target_sources(hello_asm PRIVATE syscall.s hack_asm_main.cxx)
    target_link_options(
        hello_asm
        PRIVATE
        -nostartfiles
        -nostdlib)
endif()

if(UNIX OR MINGW) # BONUS for curious (works only in xterm, gnome-terminal or in
                  # Konsole
    add_executable(hello_cout_colored cout_main_colored.cxx)
    target_compile_features(hello_cout_colored PRIVATE cxx_std_20)
endif(UNIX OR MINGW)
