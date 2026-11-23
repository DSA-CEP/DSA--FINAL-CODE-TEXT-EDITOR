#pragma once
#include "common.cpp"
#include "TextBuffer.cpp"
#include "DynamicArray.cpp"

//SMALL TEXT HELPERS

// General-purpose text helpers: they belong to no single class.

// Lowercase version of one character, used for case-insensitive comparison.
inline char lowerChar(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return (char)(ch - 'A' + 'a'); // 'A'..'Z' are in a row, so add the distance
    }

    return ch;
}

// One search result, stored as zero-based line and column.
struct SearchMatch
{
    int line;
    int column;

    SearchMatch()
    {
        line = 0;
        column = 0;
    }
};

//multi-line text: lines are stored as ONE text separated by '\n'.
// Cuts the text into lines at every '\n' and puts them into 'lines'.
// Always gives at least one line.
inline void splitLines(const TextBuffer &text, DynamicArray<TextBuffer> &lines)
{
    lines.clear();
    TextBuffer currentLine;

    for (int i = 0; i < text.getLength(); i++)
    {
        char ch = text.charAt(i);

        if (ch == '\n')
        {
            lines.add(currentLine);
            currentLine.clear();
        }
        else
        {
            currentLine.append(ch);
        }
    }

    lines.add(currentLine);
}

// Opposite of splitLines: joins the lines with '\n' between them.
inline TextBuffer linesToText(const DynamicArray<TextBuffer> &lines)
{
    TextBuffer text;

    for (int i = 0; i < lines.size(); i++)
    {
        if (i > 0)
        {
            text.append('\n');
        }

        text.append(lines.at(i));
    }

    return text;
}

// Turns a typed "\n"  into a real line break, so the user can
// type multi-line search and replacement text on one console line.
inline TextBuffer unescapeLineBreaks(const TextBuffer &text)
{
    TextBuffer result;

    for (int i = 0; i < text.getLength(); i++)
    {
        if (text.charAt(i) == '\\' && i + 1 < text.getLength() && text.charAt(i + 1) == 'n')
        {
            result.append('\n');
            i++;
        }
        else
        {
            result.append(text.charAt(i));
        }
    }

    return result;
}

// Opposite of unescapeLineBreaks: shows a line break as "\n" so one-line
// messages (status line, search list) stay on one line.
inline TextBuffer displayText(const TextBuffer &text)
{
    TextBuffer result;

    for (int i = 0; i < text.getLength(); i++)
    {
        if (text.charAt(i) == '\n')
        {
            result.append("\\n");
        }
        else
        {
            result.append(text.charAt(i));
        }
    }

    return result;
}

// Reads one line typed by the user (without the '\n') into 'line'.
// Replaces getline(cin, string). Any line length is allowed.
// Returns false when the input has ended (nothing could be read).
inline bool readLine(TextBuffer &line)
{
    line.clear();
    char ch;
    bool readSomething = false;

    while (cin.get(ch))
    {
        readSomething = true;

        if (ch == '\n')
        {
            return true;
        }

        line.append(ch);
    }

    return readSomething;
}

// One replaced piece of text (may span several lines): 'before' and 'after'
