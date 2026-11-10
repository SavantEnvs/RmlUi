# Builds the fuzz harnesses against a sanitized static RmlUi. Configured by mayhem/build.sh, which passes
# RMLUI_SOURCE_DIR and (through the environment) the compiler flags and STANDALONE_FUZZ_MAIN.
cmake_minimum_required(VERSION 3.16)
project(rmlui_fuzz C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("${RMLUI_SOURCE_DIR}" rmlui EXCLUDE_FROM_ALL)

separate_arguments(FUZZ_ENGINE_FLAGS UNIX_COMMAND "$ENV{LIB_FUZZING_ENGINE}")

add_library(standalone_main OBJECT "$ENV{STANDALONE_FUZZ_MAIN}")
add_library(harness_support OBJECT "${LSAN_OFF_SOURCE}" "${CMAKE_CURRENT_LIST_DIR}/../embedded_fonts.cc")
target_compile_definitions(harness_support PRIVATE
	"RML_FONT_REGULAR=\"${RMLUI_SOURCE_DIR}/Samples/assets/LatoLatin-Regular.ttf\""
	"RML_FONT_BOLD=\"${RMLUI_SOURCE_DIR}/Samples/assets/LatoLatin-Bold.ttf\"")

foreach(target IN ITEMS fuzz_rml_document fuzz_style_sheet fuzz_data_expression)
	add_executable(${target} "${CMAKE_CURRENT_LIST_DIR}/../${target}.cc")
	target_link_libraries(${target} PRIVATE rmlui_core ${FUZZ_ENGINE_FLAGS})
	target_sources(${target} PRIVATE $<TARGET_OBJECTS:harness_support>)

	add_executable(${target}-standalone "${CMAKE_CURRENT_LIST_DIR}/../${target}.cc")
	target_link_libraries(${target}-standalone PRIVATE rmlui_core)
	target_sources(${target}-standalone PRIVATE $<TARGET_OBJECTS:harness_support> $<TARGET_OBJECTS:standalone_main>)
endforeach()
