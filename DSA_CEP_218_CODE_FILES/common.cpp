#pragma once
#include <iostream>
#include <fstream>
#include <new> // new (nothrow): allocation returns nullptr instead of throwing
#include <conio.h>
#include <windows.h>  //winAPI
using namespace std;

/*
 ===================== HOW THIS PROGRAM IS BUILT =====================

 CUSTOM ADTs:

   TextBuffer          a growing character array (our own "string")
   DynamicArray<T>     a growing array (our own "vector")
   Stack<T>            a stack built on DynamicArray (used for undo / redo)
   Node + Document     doubly linked list, one node per line of text

 PROGRAM CLASSES (same design as before):

   Document       all lines (doubly linked list) + the cursor + editing
   Action         one edit that can be undone (type, position, text)
   UndoRedo       two stacks of Actions; undo() and redo() use them
   FileManager    saves and opens files, checks file names (no screen output)
   Selection      where the selection starts and what it covers
   Search         the matches of the last search and the current match
   EditorDisplay  draws everything on the console (changes nothing)
   TextEditor     the boss: reads keys and uses all the classes above

 What happens when you press a key (example: typing the letter 'a'):

   1. TextEditor::run()               draws the screen, then waits for a key
   2. TextEditor::handleNormalKey()   sees 'a' is a normal character
   3. Document::insertCharacterAt()   puts 'a' into the line (TextBuffer::insertChar)
   4. UndoRedo::record()              stores an Action so Ctrl+Z can undo it
   5. Document::setCursor()           moves the cursor one step right
   6. run() loops and EditorDisplay::render() draws the new text

 MEMORY RULES (the whole project follows these 4 rules):
   1. Every class that calls new[] / new owns that memory.
   2. The destructor frees it (delete[] for arrays, delete for nodes).
   3. Classes that own memory have a COPY CONSTRUCTOR and operator= that make
      a full copy (TextBuffer, DynamicArray), or copying is forbidden
      (Document, TextEditor).
   4. new (nothrow) is used, so if memory runs out we get nullptr and the
      edit is skipped instead of the program crashing.

 Reading order: TextBuffer -> DynamicArray -> Stack -> helpers -> Node ->
 Document -> UndoRedo -> FileManager -> Selection -> Search ->
 EditorDisplay (console details) -> TextEditor -> main.

*/

// Action types
const int INSERT_CHAR = 1;
const int DELETE_CHAR = 2;
const int SPLIT_LINE = 3;
const int JOIN_LINES = 4;
const int REPLACE_TEXT = 5;
const int MERGE_TEXT = 6;
const int DELETE_RANGE = 7;
const int TYPED_TEXT = 8; // >>> CHANGE 4 (undo typing): several typed letters merged into one action
