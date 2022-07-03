setup:

don't use PowerShell.exe, use cmd.exe

D:\Github\emsdk\emsdk_env.bat

make

use FileZilla to upload

copy to /kosmokleaner/temp/stacker...
  index.html
  index.js
  index.wasm

// see https://emscripten.org/docs/porting/Debugging.html for
// same size .wasm but more log during compile
  set EMCC_DEBUG=1
// much bigger .wasm
  set EMCC_AUTODEBUG=1

on "process_begin: CreateProcess(NULL, uname -s, ...) failed." error:
  https://github.com/ThomasMertes/seed7/blob/master/src/read_me.txt


emrun --no_browser web/index.html

e.g. http://localhost:6931/web