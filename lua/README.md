This folder contains the Lua source files for the 2 functions that are hardcoded as bytecode in [main.cpp](../src/main.cpp), in order for the Teensy to evaluate payloads over serial in the Playdate's lua runtime (see [here](https://github.com/jaames/playdate-reverse-engineering/blob/main/usb/usb.md#eval) for details).

To generate tye bytecode:

  1) write lua function calls in a separate lua file (see [lua](/lua) folder). Add padding to make room for the data you'll need to transmit.
  2) compile the files to `.pdz` using Playdate SDK's `pdc`
  3) extract `.luac` with [this Python script](https://raw.githubusercontent.com/jaames/playdate-reverse-engineering/main/tools/pdz.py)

     e.g  `python pdz.py main.pdx\processMessage.pdz .`

  4) convert `.luac` to C byte array with [this tool](https://notisrac.github.io/FileToCArray/)
  5) paste into C source code and replace the padding with custom data (see  [main.cpp](../src/main.cpp))
