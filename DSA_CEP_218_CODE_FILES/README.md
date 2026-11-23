# Text Editor in C++ (CS-218 CEP)

A console-based text editor written in pure C++ (C++11). The program is split
into one .cpp file per module. All custom data structures are implemented from
scratch — no std::string, std::vector, std::list, or std::stack is used.

## Requirements
- Windows (the program uses conio.h and windows.h for console handling)
- MinGW-w64 g++ compiler

## How to Compile

Open a command prompt in this folder and run:

    g++ -std=c++11 -Wall main.cpp -o TextEditor.exe

Or in one line:

    g++ -std=c++11 main.cpp -o TextEditor.exe

This produces TextEditor.exe. Only main.cpp needs to be compiled — it
includes every other .cpp file in the correct dependency order, and each
file has a #pragma once guard, so the whole project builds as one unit.

## How to Run

    TextEditor.exe

## Program Workflow

1. **Main menu** — choose:
   - `1` Create a new document
   - `2` Open an existing document
   - `3` Exit
2. **Editing screen** — type text, move the cursor with arrow keys, and use
   keyboard shortcuts (undo/redo, search, save, etc.).
3. **Save** — files are written to a `TextEditorFiles` folder created next to
   the .exe when the program starts.
4. **Rename (Ctrl+A)** — the current file is renamed on disk: the new name
   replaces the old one and the old file name is removed from the folder.
   Unsaved changes are written to the renamed file, and the editor then
   uses the new name and path. If the new name already belongs to another
   file, the program asks before replacing it.

## File Map (each file = one module)

| File | Module |
|---|---|
| main.cpp | Main menu (create / open / exit), program entry point |
| common.cpp | Standard includes, action-type constants, overview comment |
| TextBuffer.cpp | Own string class (growing character array) |
| DynamicArray.cpp | Own vector class |
| Stack.cpp | Stack built on DynamicArray (used for undo/redo) |
| TextHelpers.cpp | Helpers: SearchMatch, lowerChar, splitLines, readLine, ... |
| Action.cpp | LineChange + Action (one undoable edit) |
| Node.cpp | One line of the linked list |
| Document.cpp | Doubly linked list of lines, cursor, editing operations, KMP search |
| UndoRedo.cpp | Undo/redo stacks + typing merge |
| FileManager.cpp | Open/save documents, file-name validation |
| Selection.cpp | Selection range |
| Search.cpp | Search matches + binary search for the next match |
| EditorDisplay.cpp | Console drawing (status bar, screen refresh) |
| TextEditor.cpp | Main editor loop and key handling |

## Build Order (handled automatically by the includes)

common -> TextBuffer / DynamicArray / Node -> TextHelpers / Stack /
Action -> Document -> UndoRedo / FileManager / Selection / Search ->
EditorDisplay -> TextEditor -> main
