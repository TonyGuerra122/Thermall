cmake_minimum_required(VERSION 3.15)

# Runtime C estático (/MT): o .exe não depende do VC++ Redistributable.
# Precisa bater com o PDCurses, que o Makefile.vc compila com /MT em qualquer configuração.
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

# Nome do projeto
project(Thermall C)

# Define o padrão C
set(CMAKE_C_STANDARD 99)

# Inclui o módulo ExternalProject
include(ExternalProject)

# Define o diretório onde o PDCurses será baixado e compilado
set(PDCURSES_INSTALL_DIR ${CMAKE_BINARY_DIR}/pdcurses)
set(PDCURSES_SOURCE_DIR ${PDCURSES_INSTALL_DIR}/src/PDCurses)

# Adiciona o projeto externo PDCurses
ExternalProject_Add(
    PDCurses
    GIT_REPOSITORY https://github.com/wmcbrine/PDCurses.git
    GIT_TAG 3.9
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_COMMAND} -E copy_directory ${PDCURSES_SOURCE_DIR}/common ${PDCURSES_SOURCE_DIR}/wincon/common
                  && cd ${PDCURSES_SOURCE_DIR}/wincon && nmake -f Makefile.vc WIDE=Y UTF8=Y
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1
    PREFIX ${PDCURSES_INSTALL_DIR}
)

# Adiciona o diretório de inclusão
include_directories(${PDCURSES_SOURCE_DIR})

# Adiciona o diretório da biblioteca
link_directories(${PDCURSES_SOURCE_DIR}/wincon)

# Adiciona o executável
add_executable(Thermall src/main.c "include/line_text.h" "src/text_line.c" "include/terminal.h" "src/terminal.c")

target_include_directories(Thermall PRIVATE include)

# PDCurses com suporte a Unicode (precisa bater com WIDE=Y UTF8=Y acima)
target_compile_definitions(Thermall PRIVATE PDC_WIDE PDC_FORCE_UTF8)

# Fontes em UTF-8
if(MSVC)
    target_compile_options(Thermall PRIVATE /utf-8)
endif()

# Assegura que a biblioteca PDCurses é construída antes do projeto principal
add_dependencies(Thermall PDCurses)

# Linka a biblioteca PDCurses
target_link_libraries(Thermall pdcurses)