# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

return()

CPMFindPackage(
  NAME googletest
  GITHUB_REPOSITORY google/googletest
  GIT_TAG v1.14.0
  VERSION 1.12.1
  OPTIONS "INSTALL_GTEST OFF" "gtest_force_shared_crt"
  FIND_PACKAGE_ARGUMENTS "NAMES GTest"
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL ON)

pfl_add_executable(
  INTERNAL
  SOURCES
  ./src/main.cpp
  ./src/linglong/api/dbus/v1/mock_app_manager.h
  ./src/linglong/cli/cli_test.cpp
  ./src/linglong/package/version_test.cpp
  ./src/linglong/package/version_range_test.cpp
  ./src/linglong/package/reference_test.cpp
  ./src/linglong/cli/dbus_reply.h
  ./src/linglong/utils/xdg/desktop_entry_test.cpp
  ./src/linglong/utils/error/result_test.cpp
  ./src/linglong/utils/transaction_test.cpp
  ./src/linglong/repo/ostree_repo_test.cpp
  COMPILE_FEATURES
  PUBLIC
  cxx_std_17
  LINK_LIBRARIES
  PUBLIC
  GTest::gmock
  linglong::linglong
  Qt::DBusPrivate)

include(GoogleTest)
get_real_target_name(linglong_tests linglong::linglong::tests)
message("${linglong_tests}")
gtest_discover_tests(${linglong_tests}
                     WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
add_subdirectory(src/qt)
