#pragma once
#include "common.cpp"
#include "TextBuffer.cpp"
#include "Document.cpp"

// ================= SELECTION =================

// The selection runs from the anchor (where Shift was first pressed)
// to the current cursor position.
class Selection
{
private:
    bool active;
    int anchorLine;
    int anchorColumn;

public:
    Selection()
    {
        active = false;
        anchorLine = 0;
        anchorColumn = 0;
    }

    bool isActive() const
    {
        return active;
    }

    // Starts a selection at this position (the cursor position before the move).
    void start(int line, int column)
    {
        active = true;
        anchorLine = line;
        anchorColumn = column;
    }

    void clear()
    {
        active = false;
    }

    // Gets the selection as start/end positions (start is always before end).
    // Returns false when nothing is selected or the selection is empty.
    bool getRange(Document &document, int &startLine, int &startColumn,
                  int &endLine, int &endColumn) const
    {
        if (!active)
        {
            return false;
        }

        int cursorLine = document.getCursorLineIndex();
        int cursorColumn = document.getCursorColumn();

        // Is the anchor before the cursor (selecting forward)?
        bool anchorFirst = false;

        if (anchorLine < cursorLine)
        {
            anchorFirst = true;
        }
        else if (anchorLine == cursorLine && anchorColumn <= cursorColumn)
        {
            anchorFirst = true;
        }

        if (anchorFirst)
        {
            startLine = anchorLine;
            startColumn = anchorColumn;
            endLine = cursorLine;
            endColumn = cursorColumn;
        }
        else
        {
            startLine = cursorLine;
            startColumn = cursorColumn;
            endLine = anchorLine;
            endColumn = anchorColumn;
        }

        // Anchor and cursor on the same spot means an empty selection.
        if (startLine == endLine && startColumn == endColumn)
        {
            return false;
        }

        return true;
    }

    // Returns the selected text, with lines joined by '\n' (same format that
    // insertText and removeText use).
    TextBuffer getText(Document &document, int startLine, int startColumn,
                       int endLine, int endColumn) const
    {
        TextBuffer text;

        for (int i = startLine; i <= endLine; i++)
        {
            Node *node = document.getLineAt(i);

            if (node == nullptr)
            {
                break; // invalid line: stop instead of crashing
            }

            // The first line starts at the selection start, the last line ends
            // at the selection end. Lines in between are taken completely.
            int from = 0;
            if (i == startLine)
                from = startColumn;

            int to = node->data.getLength();
            if (i == endLine)
                to = endColumn;

            text.append(node->data.substr(from, to - from));

            if (i < endLine)
            {
                text.append('\n');
            }
        }

        return text;
    }
};
