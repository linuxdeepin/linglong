# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

include(GNUInstallDirs)

function(configure_files)
  foreach(input ${ARGV})
    configure_file(${input} ${input} @ONLY)
  endforeach()
endfunction()

configure_files(
  # find -regex '\.\/\(share\|lib\|libexec\)\/.*' -type f | sort
  ./libexec/linglong/create-linglong-dirs
  ./libexec/linglong/fetch-git-repo
  ./libexec/linglong/upgrade-all
  ./lib/linglong/builder/template.yaml
  ./lib/linglong/config.yaml
  ./lib/linglong/container/config.d/10-basics.json
  ./lib/linglong/container/config.d/20-devices
  ./lib/linglong/container/config.d/25-host-rootfs.json
  ./lib/linglong/container/config.d/25-host-statics.json
  ./lib/linglong/container/config.d/30-host-ipc
  ./lib/linglong/container/config.d/40-xdg-runtime-dir
  ./lib/linglong/container/config.json
  ./lib/linglong/container/README.md
  ./lib/systemd/system-environment-generators/61-linglong
  ./lib/systemd/system/org.deepin.linglong.PackageManager.service
  ./lib/systemd/system/org.deepin.linglong.SystemHelper.service
  ./lib/systemd/user-environment-generators/61-linglong
  ./lib/systemd/user/linglong-upgrade.service
  ./lib/systemd/user/linglong-upgrade.timer
  ./lib/sysusers.d/linglong.conf
  ./share/bash-completion/completions/ll-builder
  ./share/bash-completion/completions/ll-cli
  ./share/dbus-1/system.d/org.deepin.linglong.PackageManager.conf
  ./share/dbus-1/system.d/org.deepin.linglong.SystemHelper.conf
  ./share/dbus-1/system-services/org.deepin.linglong.PackageManager.service
  ./share/dbus-1/system-services/org.deepin.linglong.SystemHelper.service
  ./share/mime/packages/deepin-linglong.xml)

# bash-completion
pkg_get_variable(BASH_COMPLETIONS_DIR bash-completion completionsdir)
if(NOT BASH_COMPLETIONS_DIR)
  set(BASH_COMPLETIONS_DIR
      ${CMAKE_INSTALL_DATADIR}/./bash-completion/completions)
endif()

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/./share/bash-completion/completions/
  DESTINATION ${BASH_COMPLETIONS_DIR})

# dbus
pkg_get_variable(DBUS_SYSTEM_BUS_SERVICES_DIR dbus-1 system_bus_services_dir)
if(NOT DBUS_SYSTEM_BUS_SERVICES_DIR)
  set(DBUS_SYSTEM_BUS_SERVICES_DIR
      ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services)
endif()
set(DBUS_SYSTEM_BUS_CONF_DIR ${DBUS_SYSTEM_BUS_SERVICES_DIR}/../system.d)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/./share/dbus-1/system.d/
        DESTINATION ${DBUS_SYSTEM_BUS_CONF_DIR})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/./share/dbus-1/system-services/
        DESTINATION ${DBUS_SYSTEM_BUS_SERVICES_DIR})

# linglong

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/./lib/linglong/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/linglong/)

# libexec

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/./libexec/linglong/
        DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/linglong/)

# systemd

pkg_get_variable(SYSTEMD_SYSTEM_UNIT_PATH systemd systemdsystemunitpath)
if(NOT SYSTEMD_SYSTEM_UNIT_PATH)
  set(SYSTEMD_SYSTEM_UNIT_PATH ${CMAKE_INSTALL_PREFIX}/lib/systemd/system)
endif()
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/./lib/systemd/system/
        DESTINATION ${SYSTEMD_SYSTEM_UNIT_PATH})

pkg_get_variable(SYSTEMD_USER_UNIT_PATH systemd systemduserunitpath)
if(NOT SYSTEMD_USER_UNIT_PATH)
  set(SYSTEMD_USER_UNIT_PATH ${CMAKE_INSTALL_PREFIX}/lib/systemd/user)
endif()
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/./lib/systemd/user/
        DESTINATION ${SYSTEMD_SYSTEM_UNIT_PATH})

pkg_get_variable(SYSTEMD_USER_GENERATOR_PATH systemd systemdusergeneratorpath)
if(NOT SYSTEMD_USER_GENERATOR_PATH)
  set(SYSTEMD_USER_GENERATOR_PATH
      ${CMAKE_INSTALL_PREFIX}/lib/systemd/user-environment-generators)
endif()
install(
  DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/./lib/systemd/user-environment-generators/
  DESTINATION ${SYSTEMD_USER_GENERATOR_PATH})

pkg_get_variable(SYSTEMD_SYSTEM_GENERATOR_PATH systemd systemdusergeneratorpath)
if(NOT SYSTEMD_SYSTEM_GENERATOR_PATH)
  set(SYSTEMD_SYSTEM_GENERATOR_PATH
      ${CMAKE_INSTALL_PREFIX}/lib/systemd/system-environment-generators)
endif()
install(
  DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/./lib/systemd/system-environment-generators/
  DESTINATION ${SYSTEMD_SYSTEM_GENERATOR_PATH})

# sysuser

pkg_get_variable(SYSTEMD_SYSUSERSDIR systemd sysusersdir)
if(NOT SYSTEMD_SYSUSERSDIR)
  set(SYSTEMD_SYSUSERSDIR ${CMAKE_INSTALL_PREFIX}/lib/sysusers.d)
endif()
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib/sysusers.d/
        DESTINATION ${SYSTEMD_SYSUSERSDIR})

# xdg
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/xdg/mime/packages/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
