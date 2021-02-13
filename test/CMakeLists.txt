# build test executable
ADD_EXECUTABLE(test/test-int_ptr test-int_ptr.cpp)
TARGET_LINK_LIBRARIES(test/test-int_ptr PRIVATE intptr Threads::Threads)
ADD_TEST(NAME int_ptr-build
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test/test-int_ptr)

ADD_EXECUTABLE(test/test-atomic_int_ptr test-atomic_int_ptr.cpp)
TARGET_LINK_LIBRARIES(test/test-atomic_int_ptr PRIVATE intptr Threads::Threads)
ADD_TEST(NAME atomic_int_ptr-build
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test/test-atomic_int_ptr)

ADD_EXECUTABLE(test/test-atomic_hammer test-atomic_hammer.cpp)
TARGET_LINK_LIBRARIES(test/test-atomic_hammer PRIVATE intptr Threads::Threads)
ADD_TEST(NAME atomic_hammer-build
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target test/test-atomic_hammer)

# execute test executable
ADD_TEST(NAME int_ptr-run
  COMMAND ${CMAKE_COMMAND} -DCMD=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/test-int_ptr -DOUT=test-int_ptr.log -P ${CMAKE_CURRENT_SOURCE_DIR}/redirect.cmake)
SET_TESTS_PROPERTIES(int_ptr-run PROPERTIES DEPENDS int_ptr-build)

ADD_TEST(NAME atomic_int_ptr-run
  COMMAND ${CMAKE_COMMAND} -DCMD=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/test-atomic_int_ptr -DOUT=test-atomic_int_ptr.log -P ${CMAKE_CURRENT_SOURCE_DIR}/redirect.cmake)
SET_TESTS_PROPERTIES(atomic_int_ptr-run PROPERTIES DEPENDS atomic_int_ptr-build)

ADD_TEST(NAME atomic_hammer-run
  COMMAND ${CMAKE_COMMAND} -DCMD=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/test-atomic_hammer -DOUT=test-atomic_hammer.log -P ${CMAKE_CURRENT_SOURCE_DIR}/redirect.cmake)
SET_TESTS_PROPERTIES(atomic_hammer-run PROPERTIES DEPENDS atomic_hammer-build)

# validate test result
ADD_TEST(NAME int_ptr-validate
  COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/test-int_ptr.expected test-int_ptr.log)
SET_TESTS_PROPERTIES(int_ptr-validate PROPERTIES DEPENDS int_ptr-run)

ADD_TEST(NAME atomic_int_ptr-validate
  COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/test-atomic_int_ptr.expected test-atomic_int_ptr.log)
SET_TESTS_PROPERTIES(atomic_int_ptr-validate PROPERTIES DEPENDS atomic_int_ptr-run)

ADD_TEST(NAME atomic_hammer-sort
  COMMAND ${CMAKE_COMMAND} -DCMD=sort -DARG=test-atomic_hammer.log -DOUT=test-atomic_hammer.sort -P ${CMAKE_CURRENT_SOURCE_DIR}/redirect.cmake)
SET_TESTS_PROPERTIES(atomic_hammer-sort PROPERTIES DEPENDS atomic_hammer-run)
ADD_TEST(NAME atomic_hammer-validate
  COMMAND ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/test-atomic_hammer.expected test-atomic_hammer.sort)
SET_TESTS_PROPERTIES(atomic_hammer-validate PROPERTIES DEPENDS atomic_hammer-sort)
