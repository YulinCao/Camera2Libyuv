# Android application build config for libusb
# Copyright © 2012-2013 RealVNC Ltd. <toby.gray@realvnc.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#
APP_PLATFORM := android-17
APP_ABI := all

APP_CFLAGS := \
  -std=gnu11 \
  -Wall \
  -Wextra \
  -Wshadow \
  -Wunused \
  -Wwrite-strings \
  -Werror=format-security \
  -Werror=implicit-function-declaration \
  -Werror=implicit-int \
  -Werror=init-self \
  -Werror=missing-prototypes \
  -Werror=strict-prototypes \
  -Werror=undef \
  -Werror=uninitialized

# Workaround for MIPS toolchain linker being unable to find liblog dependency
# of shared object in NDK versions at least up to r9.
#
APP_LDFLAGS := -llog
