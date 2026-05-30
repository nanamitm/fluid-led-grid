cmake_minimum_required(VERSION 3.21)
project(FluidLedGrid VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Sensors)
qt_standard_project_setup()

qt_add_executable(FluidLedGrid
    src/main.cpp
    src/fluid_particles.cpp
    src/fluid_model.cpp
    src/gravity_source.cpp
    src/fluid_grid_item.cpp
    src/wake_lock.cpp
    $<$<PLATFORM_ID:Windows>:resources/app.rc>
)

qt_add_qml_module(FluidLedGrid
    URI FluidLedGrid
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/FluidGrid.qml
        qml/SettingSlider.qml
    SOURCES
        src/fluid_grid_item.h
        src/fluid_grid_item.cpp
        src/fluid_model.h
        src/fluid_model.cpp
)

target_include_directories(FluidLedGrid PRIVATE src)

target_link_libraries(FluidLedGrid PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Sensors
)

set_target_properties(FluidLedGrid PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME "fluid-led-grid"
)

if(ANDROID)
    # Android 15+ 対応: 16KB ページサイズ
    add_link_options(-Wl,-z,max-page-size=16384)

    set_target_properties(FluidLedGrid PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
        QT_ANDROID_TARGET_SDK_VERSION 35
        QT_ANDROID_MIN_SDK_VERSION    28
    )

    set_property(TARGET FluidLedGrid APPEND PROPERTY
        QT_ANDROID_PACKAGE_NAME "net.nanamitm.fluidledgrid"
    )

    # リリース署名 (環境変数 QT_ANDROID_KEYSTORE_PATH が設定されている場合)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND DEFINED ENV{QT_ANDROID_KEYSTORE_PATH})
        set_property(TARGET FluidLedGrid APPEND PROPERTY
            QT_ANDROID_SIGN_APK                  TRUE
            QT_ANDROID_KEYSTORE_PATH             "$ENV{QT_ANDROID_KEYSTORE_PATH}"
            QT_ANDROID_KEYSTORE_ALIAS            "$ENV{QT_ANDROID_KEYSTORE_ALIAS}"
            QT_ANDROID_KEYSTORE_STORE_PASS       "$ENV{QT_ANDROID_KEYSTORE_STORE_PASS}"
            QT_ANDROID_KEYSTORE_KEY_PASS         "$ENV{QT_ANDROID_KEYSTORE_KEY_PASS}"
        )
    endif()
endif()
