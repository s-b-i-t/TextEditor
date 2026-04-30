# TextEditor

`TextEditor` is a terminal-based editor for Linux and WSL built entirely with C++.  

Furthermore the codebase is designed to be easy to read and modular (see architecture below).

Feel free to modify for your own purposes and add more features (:

## Features

- Command mode and insert mode
- Arrow keys and `h/j/k/l` navigation
- Save with `Ctrl-S`
- Undo with `Ctrl-Z`
- Redo with `Ctrl-Y`
- Line numbers
- Syntax highlighting for C, C++, and Python

## Architecture
The project is centered around two design patterns: Model-View-Controller (MVC) & Command

- `Model.*` holds document data and cursor state.
- `Controller.*` handles input, editor behavior, status updates, and syntax highlighting.
- `TextView.*` owns terminal interaction and screen rendering.
- `Command.*` Abstraction of user commands so that they can be executed and unexecuted.
- `main.cpp` is the entry point and event loop.

## Build

Requirements:

- Linux or WSL
- `g++` with C++17 support
- `make`
- A terminal to run binary

To build the binary:
1. Run 'make' command from repo root.

2. This creates your binary at 'bin/text-editor' which you can then execute it using './text-editor file-name'.

3. Optional: Add binary to system path to access the editor from any directory.
- To do this first look for environment variables in windows search.
- Click environment variables then new environment variable and add the path of your binary.

## Controls

- Start in command mode
- `i` enters insert mode
- `Esc` or `Ctrl-A` returns to command mode
- Arrow keys or `h` `j` `k` `l` move the cursor
- `Enter` inserts a new line
- `x` or `Delete` deletes the character under the cursor
- `Backspace` removes text in insert mode
- `Ctrl-S` saves
- `Ctrl-Z` undoes
- `Ctrl-Y` redoes
- `Ctrl-Q` quits

## Syntax Highlighting
Supported languages:

- C
- C++
- Python
Keyword files:
- [`dependencies/ckeywords.txt`](dependencies/ckeywords.txt)
- [`dependencies/cppkeywords.txt`](dependencies/cppkeywords.txt)
- [`dependencies/pykeywords.txt`](dependencies/pykeywords.txt)
