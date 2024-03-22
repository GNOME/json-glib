#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2021 Emmanuele Bassi
# SPDX-License-Identifier: LGPL-2.1-or-later

import os
import shutil


references = [
    'doc/json-glib-1.0',
]

sourceroot = os.environ.get('MESON_SOURCE_ROOT')
buildroot = os.environ.get('MESON_BUILD_ROOT')
distroot = os.environ.get('MESON_DIST_ROOT')

for reference in references:
    src_path = os.path.join(buildroot, reference)
    if os.path.isdir(src_path):
        dst_path = os.path.join(distroot, reference)
        shutil.copytree(src_path, dst_path)
