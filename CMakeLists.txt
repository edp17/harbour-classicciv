cmake_minimum_required(VERSION 3.10)
project(harbour-classicciv LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick)

include(FindPkgConfig)
pkg_check_modules(SAILFISHAPP REQUIRED sailfishapp)

add_executable(harbour-classicciv
    src/main.cpp
    src/DosboxRunner.h
    src/DosboxRunner.cpp
)

target_include_directories(harbour-classicciv PRIVATE
    ${SAILFISHAPP_INCLUDE_DIRS}
)

target_compile_options(harbour-classicciv PRIVATE
    ${SAILFISHAPP_CFLAGS_OTHER}
)

target_link_libraries(harbour-classicciv
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    ${SAILFISHAPP_LIBRARIES}
)

install(TARGETS harbour-classicciv
    RUNTIME DESTINATION bin
)

# Main QML (keep same approach as your sherlock)
install(DIRECTORY qml/
        DESTINATION share/${PROJECT_NAME}/qml
        FILES_MATCHING
        PATTERN "*.qml"
        PATTERN "cover" EXCLUDE
)

install(DIRECTORY qml/cover/
        DESTINATION share/${PROJECT_NAME}/qml/cover
        FILES_MATCHING
        PATTERN "*.qml"
        PATTERN "*.png"
)

install(FILES
 ${CMAKE_SOURCE_DIR}/3rdparty/dosbox-staging/build/subprojects/iir1-1.9.3/iir/libiir.so.1.9.3
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/harbour-classicciv
)

# also add symlinks so the loader can resolve SONAMEs if needed
install(CODE "
  execute_process(COMMAND ln -sf libiir.so.1.9.3 \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/harbour-classicciv/libiir.so.1)
  execute_process(COMMAND ln -sf libiir.so.1 \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/harbour-classicciv/libiir.so)
")

install(FILES harbour-classicciv.desktop
    DESTINATION share/applications
)

install(FILES icons/harbour-classicciv.png
    DESTINATION share/icons/hicolor/256x256/apps
)


# create SONAME-like symlinks inside that same directory
install(CODE "
  file(MAKE_DIRECTORY \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${HCC_LIBDIR}\")
  execute_process(COMMAND ln -sf libiir.so.1.9.3 \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${HCC_LIBDIR}/libiir.so.1\")
  execute_process(COMMAND ln -sf libiir.so.1 \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${HCC_LIBDIR}/libiir.so\")
")

# Cover icon used by CoverPage.qml as "icon.png"
install(FILES icons/harbour-classicciv.png
    DESTINATION share/${PROJECT_NAME}/qml/cover
    RENAME icon.png
)

# ---------------------------------------------------------------------------
# Bundle dosbox-staging subproject shared libraries into a private runtime dir
# ---------------------------------------------------------------------------

include(GNUInstallDirs)

set(HCC_PRIV_LIBDIR "${CMAKE_INSTALL_LIBDIR}/harbour-classicciv")
set(HCC_DOSBOX_SUBP "${CMAKE_SOURCE_DIR}/3rdparty/dosbox-staging/build/subprojects")

install(FILES
  ${HCC_DOSBOX_SUBP}/iir1-1.9.3/iir/libiir.so.1.9.3
  ${HCC_DOSBOX_SUBP}/libslirp-v4.7.0/libslirp.so.0.4.0
  ${HCC_DOSBOX_SUBP}/fluidsynth-2.3.3/libfluidsynth.so
  ${HCC_DOSBOX_SUBP}/munt-libmt32emu_2_7_1/libmt32emu.so
  ${HCC_DOSBOX_SUBP}/SDL2_net-2.2.0/libsdl2_net.so
  DESTINATION ${HCC_PRIV_LIBDIR}
)

# Bundle private runtime libs (built during %build into _deps/usr)
set(PRIVLIB_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}")

install(FILES
    ${CMAKE_SOURCE_DIR}/_deps/usr/lib/libopusfile.so.0
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
    OPTIONAL
)