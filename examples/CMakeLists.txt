add_executable(tq_basic basic.cpp)
target_link_libraries(tq_basic PRIVATE teaspeak_query)

add_executable(tq_test test_basic.cpp)
target_link_libraries(tq_test PRIVATE teaspeak_query)