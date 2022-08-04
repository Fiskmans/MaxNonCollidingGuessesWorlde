project(max_non_colliding_guesses_wordle)

set(WORDLIST_PATH ${CMAKE_SOURCE_DIR}/english-words/words_alpha.txt)
configure_file("config.h.in" "config.h")

add_executable(executable "main.cpp")

target_include_directories(executable PUBLIC ${CMAKE_BINARY_DIR})