set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(
    ${CMAKE_SOURCE_DIR}/sdk/tests
)

macro_add_unittest_definitions()

ecm_add_tests(
    KisScreentoneGeneratorTest.cpp

    NAME_PREFIX "plugins-generators-"
    LINK_LIBRARIES kritaimage Qt5::Test
)
