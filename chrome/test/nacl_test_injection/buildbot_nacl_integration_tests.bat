rem Copyright (c) 2011 The Chromium Authors. All rights reserved.
rem Use of this source code is governed by a BSD-style license that can be
rem found in the LICENSE file.

rem Shim script to allow chrome tree to control how nacl's tests get run.
rem NOTE: if you change this file, also change the corresponding .sh file.

rem TODO(bradnelson): Make this a nop until we can rev nacl safely.
exit /b 0

cd %~dp0..\..\..
python_slave src\native_client\build\buildbot_chrome_nacl_stage.py ^
 --mode Release
