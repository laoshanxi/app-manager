##########################################################################
# Unit Test
##########################################################################
project(test_utility)

add_executable(${PROJECT_NAME} main.cpp)

add_catch_test(${PROJECT_NAME})

##########################################################################
# Link
##########################################################################
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Threads::Threads
    cpprest
    rest
    ${OPENSSL_LIBRARIES}
    security
    application
    process
    prometheus
    common
)
