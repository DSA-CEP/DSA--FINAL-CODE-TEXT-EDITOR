#pragma once
#include "common.cpp"
#include "TextBuffer.cpp"
#include "DynamicArray.cpp"

// are the exact text at line/column before and after the replacement.
// Used by REPLACE_TEXT so one Action can undo/redo many replacements.
struct LineChange
{
    int line;
    int column;
    TextBuffer before;
    TextBuffer after;

    LineChange()
    {
        line = 0;
        column = 0;
    }
};

// Action class stores information about an editing operation.
// Copying an Action is safe: TextBuffer and DynamicArray copy themselves fully.
class Action
{
public:
    int type;
    int line;
    int column;
    TextBuffer text;

    int beforeLine;
    int beforeColumn;

    int afterLine;
    int afterColumn;

    // Only used by REPLACE_TEXT (one entry per replacement, last-in-document first).
    DynamicArray<LineChange> changes;

    Action()
    {
        type = 0;
        line = 0;
        column = 0;
        beforeLine = 0;
        beforeColumn = 0;
        afterLine = 0;
        afterColumn = 0;
    }

    Action(int t, int l, int c, const TextBuffer &txt,
           int bl, int bc, int al, int ac)
    {
        type = t;
        line = l;
        column = c;
        text = txt;
        beforeLine = bl;
        beforeColumn = bc;
        afterLine = al;
        afterColumn = ac;
    }
};
