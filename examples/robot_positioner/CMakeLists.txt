cmake_minimum_required(VERSION 3.1)
include(../../cmake/bob_robotics.cmake)
BoB_project(SOURCES robot_positioner.cc
            BOB_MODULES common hid robots robots/control vicon)
