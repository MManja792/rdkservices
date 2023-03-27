set(PLUGIN_NAME DeviceProvisioning)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

add_definitions (-DUSE_THUNDER_COMMUNICATION)

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(IARMBus)
find_package(TR181)

add_library(${MODULE_NAME} SHARED
        DeviceProvisioning.cpp
        Module.cpp
        ../helpers/tptimer.cpp
        ../helpers/utils.cpp
        )

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})

target_include_directories(${MODULE_NAME}
        PRIVATE
        ${IARMBUS_INCLUDE_DIRS}
        ${TR181_INCLUDE_DIRS}
        ../helpers
        )

target_link_libraries(${MODULE_NAME}
        PRIVATE
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${IARMBUS_LIBRARIES}
        ${SECAPI_LIB}
        ${TR181_LIBRARIES}
        ${NAMESPACE}SecurityUtil
        )

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
