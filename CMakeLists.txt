# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.23)

project(
  linglong
  VERSION 1.4.0
  DESCRIPTION "a container based package manager for deepin"
  HOMEPAGE_URL "https://github.com/linuxdeepin/linglong"
  LANGUAGES CXX)

set(LINGLONG_USERNAME
    "deepin-linglong"
    CACHE STRING "The username for linglong package manager")
set(LINGLONG_ROOT
    "/var/lib/linglong"
    CACHE STRING "The location where linglong related program puts their data")
set(LINGLONG_ENABLE_TESTING
    True
    CACHE BOOL "Enable test of linglong or not")
set(LINGLONG_HOST_PATH
    "/run/host"
    CACHE STRING "The location where linglong host path mapping")

# Find dependencies

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# NOTE(black_desk): Qt keywords conflict with glib.
add_definitions("-DQT_NO_KEYWORDS")

# NOTE(black_desk): Enable Qt logging with context.
add_definitions("-DQT_MESSAGELOGCONTEXT")

find_package(QT NAMES Qt5)

find_package(
  Qt5
  COMPONENTS Core
             Concurrent
             DBus
             Network
             WebSockets
             Sql
             Gui
  REQUIRED)

include(./cmake/CPM.cmake)

# All dependencies can be added with CPM should be added with CPM.

CPMFindPackage(
  NAME docopt
  VERSION 0.6.1
  GITHUB_REPOSITORY docopt/docopt.cpp
  GIT_TAG v0.6.3
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL ON)

CPMFindPackage(
  NAME expected
  VERSION 1.0.0
  GITHUB_REPOSITORY TartanLlama/expected
  GIT_TAG v1.1.0
  GIT_SHALLOW ON
  OPTIONS "EXPECTED_BUILD_TESTS OFF"
  FIND_PACKAGE_ARGUMENTS "NAMES tl-expected"
  EXCLUDE_FROM_ALL ON)

if(NOT TARGET tl::expected)
  # NOTE(black_desk): v1.0.0 version of tl::expected doesn't have a namespaced
  # alias so we have to add it here.
  add_library(tl::expected ALIAS expected)
endif()

CPMFindPackage(
  NAME yaml-cpp
  VERSION 0.7.0
  GITHUB_REPOSITORY jbeder/yaml-cpp
  GIT_TAG yaml-cpp-0.7.0
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL ON)

if(NOT TARGET yaml-cpp::yaml-cpp)
  add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()

CPMFindPackage(
  NAME ocppi
  VERSION 0.2.2
  GITHUB_REPOSITORY black-desk/ocppi
  GIT_TAG v0.2.2
  GIT_SHALLOW ON
  OPTIONS "OCPPI_ENABLE_TESTING OFF" "OCPPI_BUILD_EXAMPLES OFF"
  FIND_PACKAGE_ARGUMENTS "NAMES ocppi COMPONENTS common runtime"
  EXCLUDE_FROM_ALL ON)

CPMFindPackage(
  NAME spdlog
  VERSION 1.10.0
  GITHUB_REPOSITORY "gabime/spdlog"
  GIT_TAG "v1.13.0"
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL ON
  OPTIONS
    "CMAKE_POSITION_INDEPENDENT_CODE ON" # NOTE:
                                         # https://github.com/gabime/spdlog/issues/1190
    "SPDLOG_FMT_EXTERNAL ON")

find_package(PkgConfig REQUIRED)

pkg_search_module(glib2 REQUIRED IMPORTED_TARGET glib-2.0)
pkg_search_module(ostree1 REQUIRED IMPORTED_TARGET ostree-1)
pkg_search_module(systemd REQUIRED IMPORTED_TARGET libsystemd)

function(get_real_target_name output target)
  get_target_property("${output}" "${target}" ALIASED_TARGET)
  if("${output}" STREQUAL "")
    set("${output}" "${target}")
  endif()
  set("${output}"
      ${${output}}
      PARENT_SCOPE)
endfunction()

include(./cmake/PFL.cmake)

pfl_init(
  ENABLE_TESTING
  ${LINGLONG_ENABLE_TESTING}
  EXTERNALS
  ytj
  http/client
  INSTALL
  ON)

if(LINGLONG_ENABLE_TESTING)
  include(CTest)
endif()

include(GNUInstallDirs)

add_subdirectory(tools/qdbusxml2cpp)

pfl_add_library(
  INTERNAL
  VERSION
  ${PROJECT_VERSION}
  INS
  src/linglong/utils/configure.h.in
  SOURCES
  # find -regex '\./src/.+\.[ch]\(pp\)?' -type f -printf '%P\n'| sort
  src/linglong/adaptors/package_manager/package_manager1.cpp
  src/linglong/adaptors/package_manager/package_manager1.h
  src/linglong/api/dbus/v1/package_manager.cpp
  src/linglong/api/dbus/v1/package_manager.h
  src/linglong/api/types/helper.cpp
  src/linglong/api/types/helper.h
  src/linglong/api/types/v1/ApplicationConfiguration.hpp
  src/linglong/api/types/v1/ApplicationConfigurationPermissionsBind.hpp
  src/linglong/api/types/v1/ApplicationConfigurationPermissions.hpp
  src/linglong/api/types/v1/BuilderConfig.hpp
  src/linglong/api/types/v1/BuilderProject.hpp
  src/linglong/api/types/v1/BuilderProjectPackage.hpp
  src/linglong/api/types/v1/BuilderProjectSource.hpp
  src/linglong/api/types/v1/CliContainer.hpp
  src/linglong/api/types/v1/CommonResult.hpp
  src/linglong/api/types/v1/Generators.hpp
  src/linglong/api/types/v1/helper.hpp
  src/linglong/api/types/v1/LayerInfo.hpp
  src/linglong/api/types/v1/LinglongAPIV1.hpp
  src/linglong/api/types/v1/OciConfigurationPatch.hpp
  src/linglong/api/types/v1/PackageInfo.hpp
  src/linglong/api/types/v1/PackageManager1GetRepoInfoResult.hpp
  src/linglong/api/types/v1/PackageManager1GetRepoInfoResultRepoInfo.hpp
  src/linglong/api/types/v1/PackageManager1InstallParameters.hpp
  src/linglong/api/types/v1/PackageManager1ModifyRepoParameters.hpp
  src/linglong/api/types/v1/PackageManager1Package.hpp
  src/linglong/api/types/v1/PackageManager1ResultWithTaskID.hpp
  src/linglong/api/types/v1/PackageManager1SearchParameters.hpp
  src/linglong/api/types/v1/PackageManager1SearchResult.hpp
  src/linglong/api/types/v1/PackageManager1UninstallParameters.hpp
  src/linglong/api/types/v1/PackageManager1UpdateParameters.hpp
  src/linglong/api/types/v1/RepoConfig.hpp
  src/linglong/builder/config.cpp
  src/linglong/builder/config.h
  src/linglong/builder/file.cpp
  src/linglong/builder/file.h
  src/linglong/builder/linglong_builder.cpp
  src/linglong/builder/linglong_builder.h
  src/linglong/builder/source_fetcher.cpp
  src/linglong/builder/source_fetcher.h
  src/linglong/cli/cli.cpp
  src/linglong/cli/cli.h
  src/linglong/cli/json_printer.cpp
  src/linglong/cli/json_printer.h
  src/linglong/cli/printer.cpp
  src/linglong/cli/printer.h
  src/linglong/package/architecture.cpp
  src/linglong/package/architecture.h
  src/linglong/package/fuzzy_reference.cpp
  src/linglong/package/fuzzy_reference.h
  src/linglong/package/layer_dir.cpp
  src/linglong/package/layer_dir.h
  src/linglong/package/layer_file.cpp
  src/linglong/package/layer_file.h
  src/linglong/package/layer_packager.cpp
  src/linglong/package/layer_packager.h
  src/linglong/package_manager/package_manager.cpp
  src/linglong/package_manager/package_manager.h
  src/linglong/package_manager/task.cpp
  src/linglong/package_manager/task.h
  src/linglong/package/reference.cpp
  src/linglong/package/reference.h
  src/linglong/package/version.cpp
  src/linglong/package/version.h
  src/linglong/package/version_range.cpp
  src/linglong/package/version_range.h
  src/linglong/repo/config.cpp
  src/linglong/repo/config.h
  src/linglong/repo/ostree_repo.cpp
  src/linglong/repo/ostree_repo.h
  src/linglong/runtime/container_builder.cpp
  src/linglong/runtime/container_builder.h
  src/linglong/runtime/container.cpp
  src/linglong/runtime/container.h
  src/linglong/utils/command/env.cpp
  src/linglong/utils/command/env.h
  src/linglong/utils/command/ocppi-helper.cpp
  src/linglong/utils/command/ocppi-helper.h
  src/linglong/utils/configure.cpp
  src/linglong/utils/dbus/log.cpp
  src/linglong/utils/dbus/log.h
  src/linglong/utils/dbus/register.cpp
  src/linglong/utils/dbus/register.h
  src/linglong/utils/error/details/error_impl.cpp
  src/linglong/utils/error/details/error_impl.h
  src/linglong/utils/error/error.cpp
  src/linglong/utils/error/error.h
  src/linglong/utils/finally/finally.cpp
  src/linglong/utils/finally/finally.h
  src/linglong/utils/global/initialize.cpp
  src/linglong/utils/global/initialize.h
  src/linglong/utils/serialize/json.cpp
  src/linglong/utils/serialize/json.h
  src/linglong/utils/serialize/yaml.cpp
  src/linglong/utils/serialize/yaml.h
  src/linglong/utils/std_helper/qdebug_helper.h
  src/linglong/utils/transaction.cpp
  src/linglong/utils/transaction.h
  src/linglong/utils/xdg/desktop_entry.cpp
  src/linglong/utils/xdg/desktop_entry.h
  APPS
  ll-builder
  ll-cli
  ll-package-manager
  llpkg
  COMPILE_FEATURES
  PUBLIC
  cxx_std_17
  LINK_LIBRARIES
  PUBLIC
  PkgConfig::glib2
  PkgConfig::ostree1
  PkgConfig::systemd
  Qt::Concurrent
  Qt::Core
  Qt::DBus
  Qt::Network
  Qt::Sql
  Qt::WebSockets
  QtLinglongRepoClientAPI
  docopt
  ocppi::ocppi
  spdlog::spdlog
  tl::expected
  yaml-cpp::yaml-cpp
  ytj::ytj)

function(
  linglong_add_dbus_adaptor
  target
  xml
  includes
  parent_class
  basename
  classname)
  get_real_target_name(target ${target})

  set(DBUS_ADAPTOR_SOURCE)

  set(Qt5DBus_QDBUSXML2CPP_EXECUTABLE qdbusxml2cpp)
  qt5_add_dbus_adaptor(DBUS_ADAPTOR_SOURCE ${xml} ${includes} ${parent_class}
                       ${basename} ${classname})
  # FIXME(black_desk): `qt5_add_dbus_adaptor` will use basename as include and
  # this cannot be configured at all. This is just a workaround.
  target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  target_sources(${target} PRIVATE ${DBUS_ADAPTOR_SOURCE})
endfunction()

linglong_add_dbus_adaptor(
  linglong::linglong
  ${PROJECT_SOURCE_DIR}/api/dbus/org.deepin.linglong.PackageManager1.xml
  linglong/package_manager/package_manager.h
  linglong::service::PackageManager
  src/linglong/adaptors/package_manager/gen_org_deepin_linglong_packagemanager1
  OrgDeepinLinglongPackagemanager1Adaptor)

function(linglong_add_dbus_interface target xml basename) # include
  get_real_target_name(target "${target}")

  set_source_files_properties(${xml} NO_NAMESPACE ON)

  set(include "${ARGV3}")
  if(include)
    set_source_files_properties(${xml} PROPERTIES INCLUDE ${include})
  endif()

  set(INTERFACE_SOURCES)
  set(Qt5DBus_QDBUSXML2CPP_EXECUTABLE qdbusxml2cpp)
  qt5_add_dbus_interface(INTERFACE_SOURCES ${xml} ${basename})
  target_sources(${target} PRIVATE ${INTERFACE_SOURCES})
endfunction()

linglong_add_dbus_interface(
  linglong::linglong
  ${PROJECT_SOURCE_DIR}/api/dbus/org.deepin.linglong.PackageManager1.xml
  src/linglong/api/dbus/v1/gen_org_deepin_linglong_packagemanager1)

add_subdirectory(misc)
