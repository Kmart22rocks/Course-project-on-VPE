cmake_minimum_required(VERSION 3.16)
project(HotelManagementSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network Test)

# Основное приложение с графическим интерфейсом
add_executable(HotelManagementSystem
        main.cpp
        models.h
        bookingcalculator.h
        bookingcalculator.cpp
        currencyapiclient.h
        currencyapiclient.cpp
        networkservice.h
        networkservice.cpp
        analyticswidget.h
        analyticswidget.cpp
        dbmanager.h
        dbmanager.cpp
        bookingdialog.h
        bookingdialog.cpp
        guesthistorydialog.h
        guesthistorydialog.cpp
        mainwindow.h
        mainwindow.cpp
)

target_link_libraries(HotelManagementSystem PRIVATE
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
)

# Подсистема модульного тестирования (Unit-тесты без GUI)
enable_testing()

add_executable(hotel_tests
        tst_hotel.cpp
        models.h
        bookingcalculator.h
        bookingcalculator.cpp
        currencyapiclient.h
        currencyapiclient.cpp
        networkservice.h
        networkservice.cpp
        dbmanager.h
        dbmanager.cpp
)

target_link_libraries(hotel_tests PRIVATE
        Qt6::Sql
        Qt6::Network
        Qt6::Test
)

add_test(NAME HotelLogicTests COMMAND hotel_tests)