# CMakeLists.txt
cmake_minimum_required(VERSION 3.24)
project(PicoVisualDesignerReconstructed VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network OpenGL OpenGLWidgets)

file(GLOB_RECURSE PVD_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE PVD_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")
qt_wrap_cpp(PVD_MOCS ${PVD_HEADERS})
add_executable(pico_visual_designer ${PVD_SOURCES} ${PVD_HEADERS} ${PVD_MOCS})
target_include_directories(pico_visual_designer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(pico_visual_designer PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network Qt6::OpenGL Qt6::OpenGLWidgets)

target_compile_definitions(pico_visual_designer PRIVATE
    PVD_RUNTIME_ROOT="${CMAKE_BINARY_DIR}/runtime")

file(GLOB_RECURSE PVD_DATABASES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/*.sqlite")
foreach(database_file IN LISTS PVD_DATABASES)
    file(RELATIVE_PATH database_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${database_file}")
    get_filename_component(database_dir "${database_rel}" DIRECTORY)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/runtime/${database_dir}")
    configure_file("${database_file}" "${CMAKE_BINARY_DIR}/runtime/${database_rel}" COPYONLY)
endforeach()

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}/runtime")

include(CTest)
if(WIN32)
    add_custom_target(format
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/format.ps1" -Group all
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    add_custom_target(format-check
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/format.ps1" -Group all -Check
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    add_custom_target(lint
        COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/lint.ps1"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
if(BUILD_TESTING)
    add_test(NAME database_integrity
        COMMAND ${CMAKE_COMMAND} -E env
            PVD_PROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/database_integrity.cmake)
    add_executable(multicore_generator_tests
        tests/multicore_generator_tests.cpp
        src/systems/generation/ProjectGenerator.cpp
        src/systems/generation/BoardStartupSanitation.cpp
        src/systems/generation/FunctionExecutionModel.cpp
        src/systems/components/PicoPinMap.cpp
        src/systems/project/ProjectStore.cpp)
    target_include_directories(multicore_generator_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(multicore_generator_tests PRIVATE
        PVD_RUNTIME_ROOT="${CMAKE_BINARY_DIR}/runtime")
    target_link_libraries(multicore_generator_tests PRIVATE Qt6::Core Qt6::Sql)
    add_test(NAME multicore_generator COMMAND multicore_generator_tests)
    add_executable(persistence_contract_tests
        tests/persistence_contract_tests.cpp
        src/systems/project/ProjectStore.cpp)
    target_include_directories(persistence_contract_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(persistence_contract_tests PRIVATE Qt6::Core Qt6::Sql)
    add_test(NAME persistence_contract COMMAND persistence_contract_tests)
    qt_wrap_cpp(SIO_MODEL_MOCS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/mainwindow/settings/settings_column3/SettingsColumn3.hpp")
    add_executable(sio_model_tests
        tests/sio_model_tests.cpp
        src/systems/generation/ProjectGenerator.cpp
        src/systems/generation/BoardStartupSanitation.cpp
        src/systems/generation/FunctionExecutionModel.cpp
        src/systems/components/PicoPinMap.cpp
        src/systems/components/FunctionCatalog.cpp
        src/systems/database/SqliteUtil.cpp
        src/systems/project/ProjectStore.cpp
        src/systems/mainwindow/settings/settings_column3/SettingsColumn3.cpp
        ${SIO_MODEL_MOCS})
    target_include_directories(sio_model_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(sio_model_tests PRIVATE
        PVD_RUNTIME_ROOT="${CMAKE_BINARY_DIR}/runtime")
    target_link_libraries(sio_model_tests PRIVATE Qt6::Widgets Qt6::Sql)
    add_test(NAME sio_model COMMAND sio_model_tests)
    set_tests_properties(sio_model PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_wrap_cpp(DIALOG_SERVICE_MOCS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/mainwindow/project/project_column3/ProjectColumn3.hpp")
    add_executable(dialog_service_tests
        tests/dialog_service_tests.cpp
        src/systems/mainwindow/project/project_column3/ProjectColumn3.cpp
        src/systems/mainwindow/dialog/AutomationFileDialogService.cpp
        src/systems/mainwindow/dialog/NativeFileDialogService.cpp
        src/systems/database/SqliteUtil.cpp
        ${DIALOG_SERVICE_MOCS})
    target_include_directories(dialog_service_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(dialog_service_tests PRIVATE Qt6::Widgets Qt6::Sql)
    add_test(NAME dialog_service COMMAND dialog_service_tests)
    set_tests_properties(dialog_service PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_wrap_cpp(BUILD_TRANSFER_CONTRACT_MOCS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/mainwindow/build/build_column3/BuildColumn3.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/mainwindow/transfer/transfer_column2/TransferColumn2.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/mainwindow/transfer/transfer_column3/TransferColumn3.hpp")
    add_executable(build_transfer_contract_tests
        tests/build_transfer_contract_tests.cpp
        src/systems/database/SqliteUtil.cpp
        src/systems/mainwindow/build/build_column3/BuildColumn3.cpp
        src/systems/mainwindow/transfer/transfer_column2/TransferColumn2.cpp
        src/systems/mainwindow/transfer/transfer_column3/TransferColumn3.cpp
        ${BUILD_TRANSFER_CONTRACT_MOCS})
    target_include_directories(build_transfer_contract_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(build_transfer_contract_tests PRIVATE
        PVD_RUNTIME_ROOT="${CMAKE_BINARY_DIR}/runtime"
        PVD_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(build_transfer_contract_tests PRIVATE Qt6::Widgets Qt6::Sql)
    add_test(NAME build_transfer_contract COMMAND build_transfer_contract_tests)
    set_tests_properties(build_transfer_contract PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_wrap_cpp(DEBUG_CONTRACT_MOCS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/mainwindow/debug/debug_column2/DebugColumn2.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/mainwindow/debug/debug_column3/DebugColumn3.hpp")
    add_executable(debug_contract_tests
        tests/debug_contract_tests.cpp
        src/systems/debug/MiProtocol.cpp
        src/systems/database/SqliteUtil.cpp
        src/systems/mainwindow/debug/debug_column2/DebugColumn2.cpp
        src/systems/mainwindow/debug/debug_column3/DebugColumn3.cpp
        ${DEBUG_CONTRACT_MOCS})
    target_include_directories(debug_contract_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(debug_contract_tests PRIVATE
        PVD_RUNTIME_ROOT="${CMAKE_BINARY_DIR}/runtime")
    target_link_libraries(debug_contract_tests PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)
    add_test(NAME debug_contract COMMAND debug_contract_tests)
    set_tests_properties(debug_contract PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    qt_wrap_cpp(POST_PROGRAM_DIAGNOSTIC_MOCS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/mainwindow/debug/debug_column3/DebugColumn3.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systems/mainwindow/transfer/transfer_column3/TransferColumn3.hpp")
    add_executable(post_program_hardware_diagnostic
        tests/post_program_hardware_diagnostic.cpp
        src/systems/debug/MiProtocol.cpp
        src/systems/database/SqliteUtil.cpp
        src/systems/mainwindow/debug/debug_column3/DebugColumn3.cpp
        src/systems/mainwindow/transfer/transfer_column3/TransferColumn3.cpp
        ${POST_PROGRAM_DIAGNOSTIC_MOCS})
    target_include_directories(post_program_hardware_diagnostic PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_definitions(post_program_hardware_diagnostic PRIVATE
        PVD_RUNTIME_ROOT="${CMAKE_BINARY_DIR}/runtime")
    target_link_libraries(post_program_hardware_diagnostic PRIVATE Qt6::Widgets Qt6::Network Qt6::Sql)
endif()
