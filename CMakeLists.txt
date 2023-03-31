cmake_minimum_required(VERSION 3.5)

project(SupperCOM VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Compiler is ${CMAKE_CXX_COMPILER_ID}")
if(CMAKE_HOST_WIN32)
	if(MSVC)
		set(CMAKE_PREFIX_PATH "D:/Qt/Qt6/msvc")
	else()
		set(CMAKE_PREFIX_PATH "D:/Qt/Qt6/gcc")
	endif()
endif()
if(CMAKE_HOST_UNIX)
	set(CMAKE_PREFIX_PATH "/opt/qt-gcc")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
	find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort Core5Compat LinguistTools)
else()
	find_package(Qt5 REQUIRED COMPONENTS Widgets SerialPort LinguistTools)
endif()

file(GLOB_RECURSE RC_FILES src/resources/logo.rc)

set(PROJECT_SOURCES
		src/main.cpp
		src/mainwindow.cpp
		src/recvthread.cpp
		src/sendthread.cpp
		src/serial_port.cpp
		src/mainwindow.h
		src/mainwindow.ui)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
	qt_add_executable(SupperCOM MANUAL_FINALIZATION ${PROJECT_SOURCES} ${RC_FILES})
	# Define target properties for Android with Qt 6 as:
	#    set_property(TARGET SupperCOM APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
	#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
	# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
	qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
	target_link_libraries(SupperCOM PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::SerialPort Qt${QT_VERSION_MAJOR}::Core5Compat)
else()
	if(ANDROID)
		add_library(SupperCOM SHARED ${PROJECT_SOURCES})
		# Define properties for Android with Qt 5 after find_package() calls as:
		#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
	else()
		add_executable(SupperCOM ${PROJECT_SOURCES} ${RC_FILES})
	endif()
	qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
	target_link_libraries(SupperCOM PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::SerialPort)
endif()

set_target_properties(SupperCOM PROPERTIES
		MACOSX_BUNDLE_GUI_IDENTIFIER lwhttpd.org
		MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
		MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
		MACOSX_BUNDLE TRUE
		WIN32_EXECUTABLE TRUE)

if(QT_VERSION_MAJOR EQUAL 6)
	qt_finalize_executable(SupperCOM)
endif()

if(WIN32)
#	if(MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
#		set(DEBUG_SUFFIX "d")
#	endif()
	set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
	if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
		set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
		if(NOT EXISTS "${QT_INSTALL_PATH}/bin")
			set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
		endif()
	endif()
	if(EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
	endif()
	foreach (QT_LIB Core Gui Widgets SerialPort)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${QT_INSTALL_PATH}/bin/Qt${QT_VERSION_MAJOR}${QT_LIB}${DEBUG_SUFFIX}.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
	endforeach (QT_LIB)
	if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${QT_INSTALL_PATH}/bin/Qt${QT_VERSION_MAJOR}Core5Compat${DEBUG_SUFFIX}.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
	endif()
endif()
