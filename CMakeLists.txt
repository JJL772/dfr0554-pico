cmake_minimum_required(VERSION 3.13)

if (NOT DEFINED $ENV{PICO_SDK_PATH})
	set(PICO_SDK_FETCH_FROM_GIT on)
endif()

include(cmake/pico_sdk_import.cmake)
include(cmake/install_uf2.cmake)

project(pico-w-dfr0554)

pico_sdk_init()

add_executable(
	dfr0554_test

	test/dfr0554_test.c
	dfr0554/dfr0554.c
)

target_link_libraries(dfr0554_test pico_stdlib hardware_i2c)

pico_add_extra_outputs(dfr0554_test)

install_uf2(dfr0554_test)
