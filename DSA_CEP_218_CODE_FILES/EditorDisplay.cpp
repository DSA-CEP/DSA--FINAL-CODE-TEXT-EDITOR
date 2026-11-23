#pragma once
#include "common.cpp"
#include "TextBuffer.cpp"
#include "Document.cpp"
#include "Selection.cpp"
#include "Search.cpp"

// ================= EDITOR DISPLAY =================

// (CONSOLE INTERFACE) THIS IS ONLY FOR INTERACTIVE TERMINAL SO THAT THE CODE APPEARS  USER
//FRIENDLY

class EditorDisplay
{
private:
    // First document line shown in the window (vertical scrolling).
    int firstVisibleLine;

    // Writes one row of text to the console at position x, y.
    // The text is cut or padded with spaces to exactly 'width' characters,
    // so a line never wraps and old text on that row is always overwritten.
    void writeConsoleRow(HANDLE console, int x, int y, const TextBuffer &text, int width)
    {
        if (width <= 0)
            return;

        TextBuffer row = text;
        int length = row.getLength();

        if (length > width)
            row.truncate(width);
        else if (length < width)
            row.appendRepeated(' ', width - length);

        COORD position;
        position.X = (SHORT)x;
        position.Y = (SHORT)y;

        DWORD written = 0;
        WriteConsoleOutputCharacterA(console, row.c_str(), (DWORD)row.getLength(),
                                     position, &written);
    }

    // Sets the colors of one console row: normal everywhere, highlight colors
    // from startX to endX (relative to the row start, endX not included).
    void colorConsoleRow(HANDLE console, int x, int y, int width,
                         WORD normal, WORD highlight, int startX, int endX)
    {
        DWORD written = 0;

        COORD position;
        position.X = (SHORT)x;
        position.Y = (SHORT)y;
        FillConsoleOutputAttribute(console, normal, (DWORD)width, position, &written);

        if (startX < endX)
        {
            COORD selected;
            selected.X = (SHORT)(x + startX);
            selected.Y = (SHORT)y;
            FillConsoleOutputAttribute(console, highlight, (DWORD)(endX - startX),
                                       selected, &written);
        }
    }

    // Horizontal scrolling: how many characters of the cursor's line are hidden
    // on the left so that the cursor stays visible. 0 means no scrolling.
    int getHorizontalOffset(int cursorColumn, int textWidth, int lineLength)
    {
        int offset = 0;

        if (cursorColumn >= textWidth)
        {
            offset = cursorColumn - textWidth + 1;
        }

        if (offset > lineLength)
        {
            offset = lineLength;
        }

        return offset;
    }

    // Draws the 6 rows at the top: title, file name and cursor information.
    void drawHeader(HANDLE console, int left, int top, int width,
                    const TextBuffer &filename, Document &document, int cursorLine)
    {
        TextBuffer border(width, '=');
        TextBuffer heading("TEXT EDITOR");

        // Put the heading in the middle of the row.
        int headingStart = (width - heading.getLength()) / 2;
        if (headingStart < 0)
            headingStart = 0;

        TextBuffer headingRow(headingStart, ' ');
        headingRow.append(heading); // writeConsoleRow pads the rest with spaces

        TextBuffer shownName = filename;
        if (shownName.isEmpty())
            shownName = "Untitled";

        TextBuffer fileRow(" File: ");
        fileRow.append(shownName);

        TextBuffer infoRow(" Current Line: ");
        infoRow.appendNumber(cursorLine + 1);
        infoRow.append(" | Column: ");
        infoRow.appendNumber(document.getCursorColumn() + 1);
        infoRow.append(" | Total Lines: ");
        infoRow.appendNumber(document.getLineCount());
        infoRow.append(" | Characters: ");
        infoRow.appendNumber(document.getCharacterCount());

        writeConsoleRow(console, left, top, border, width);
        writeConsoleRow(console, left, top + 1, headingRow, width);
        writeConsoleRow(console, left, top + 2, border, width);
        writeConsoleRow(console, left, top + 3, fileRow, width);
        writeConsoleRow(console, left, top + 4, infoRow, width);
        writeConsoleRow(console, left, top + 5, TextBuffer(width, '-'), width);
    }

    // Draws the 5 rows at the bottom: shortcut help and the status message.
    void drawFooter(HANDLE console, int left, int footerTop, int width,
                    const TextBuffer &status)
    {
        TextBuffer lastRow(" Arrows Move | Enter New Line | Backspace/Delete Remove");

        if (!status.isEmpty())
        {
            lastRow = " ";
            lastRow.append(status);
        }

        writeConsoleRow(console, left, footerTop, TextBuffer(width, '-'), width);
        writeConsoleRow(console, left, footerTop + 1,
                        " Ctrl+S Save | Ctrl+A Rename | Ctrl+O Merge | Ctrl+F2 Search | F4 List Of Search Results", width);
        writeConsoleRow(console, left, footerTop + 2,
                        " Ctrl+Z Undo | Ctrl+Y Redo | F7 Replace All | F8 Replace One | Esc Menu", width);
        writeConsoleRow(console, left, footerTop + 3,
                        " Shift+Arrows Select | Del/Backspace Delete Selection", width);
        writeConsoleRow(console, left, footerTop + 4, lastRow, width);
    }

public:
    EditorDisplay()
    {
        firstVisibleLine = 0;
    }

    // Displays a message and waits for a key.
    static void showMessage(const TextBuffer &message)
    {
        system("cls");
        cout << message.c_str() << endl;
        cout << "\nPress any key to continue...";
        _getch();
    }

    // Lists every match of the current search with 1-based line and column
    // numbers (the editor header also counts lines and columns from 1).
    void showSearchResults(const Search &search)
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);

        // How many matches fit on one screen page.
        int height = info.srWindow.Bottom - info.srWindow.Top + 1;
        int pageSize = height - 8;
        if (pageSize < 1)
            pageSize = 1;

        const DynamicArray<SearchMatch> &matches = search.getMatches();
        int total = matches.size();

        system("cls");
        cout << "========== SEARCH RESULTS ==========" << endl;
        cout << "Search term: " << displayText(search.getSearchTerm()).c_str() << endl;
        cout << "Total matches: " << total << endl;
        cout << "------------------------------------" << endl;

        for (int i = 0; i < total; i++)
        {
            cout << "Match " << (i + 1) << ": Line " << (matches.at(i).line + 1)
                 << ", Column " << (matches.at(i).column + 1) << endl;

            // Pause when the screen is full so nothing scrolls out of view.
            if ((i + 1) % pageSize == 0 && i + 1 < total)
            {
                cout << "-- " << (i + 1) << " of " << total
                     << " shown. Press any key for more, Esc to stop --";

                int key = _getch();

                if (key == 0 || key == 224)
                    _getch(); // Skip the second code of an arrow/function key.

                cout << endl;

                if (key == 27)
                    break;
            }
        }

        cout << "\nPress any key to return to the editor...";

        int key = _getch();

        if (key == 0 || key == 224)
            _getch();
    }

    void render(Document &document, const Selection &selection, const Search &search,
                const TextBuffer &filename, const TextBuffer &status)
    {
        HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(console, &info);

        // Work inside the visible window, not the full scrollback buffer.
        int width = info.srWindow.Right - info.srWindow.Left + 1;
        int height = info.srWindow.Bottom - info.srWindow.Top + 1;
        int left = info.srWindow.Left;
        int top = info.srWindow.Top;

        if (width < 20 || height < 11)
            return;

        const int headerRows = 6;
        const int footerRows = 5;

        int totalLines = document.getLineCount();
        int cursorLine = document.getCursorLineIndex();

        // Every row looks like: " 12 | text". The number column is as wide as
        // the biggest line number, so the "|" signs line up.
        TextBuffer totalText;
        totalText.appendNumber(totalLines);
        int lineNumberWidth = totalText.getLength();
        int prefixWidth = 1 + lineNumberWidth + 3;

        // Number of screen columns left for the text itself.
        int textWidth = width - prefixWidth;
        if (textWidth < 1)
            textWidth = 1;

        // Vertical scrolling: firstVisibleLine is the document line shown in the
        // first content row. It only moves when the cursor leaves the window.
        int contentRows = height - headerRows - footerRows;
        if (contentRows < 1)
            contentRows = 1;

        if (cursorLine < firstVisibleLine)
            firstVisibleLine = cursorLine;
        if (cursorLine >= firstVisibleLine + contentRows)
            firstVisibleLine = cursorLine - contentRows + 1;
        if (firstVisibleLine > totalLines - contentRows)
        {
            firstVisibleLine = totalLines - contentRows;
            if (firstVisibleLine < 0)
                firstVisibleLine = 0;
        }

        // Only the cursor's line is scrolled horizontally (when it is very long).
        Node *cursorNode = document.getLineAt(cursorLine);

        if (cursorNode == nullptr)
            return;

        int cursorOffset = getHorizontalOffset(document.getCursorColumn(), textWidth,
                                               cursorNode->data.getLength());

        drawHeader(console, left, top, width, filename, document, cursorLine);

        // Selected text is shown with inverted colors.
        int selStartLine = 0, selStartColumn = 0, selEndLine = 0, selEndColumn = 0;
        bool hasSelection = selection.getRange(document, selStartLine, selStartColumn,
                                               selEndLine, selEndColumn);

        // The current search match is highlighted only while the cursor is still
        // on it, so the highlight disappears by itself once the cursor moves or
        // the text under it changes.
        // A multi-line match is highlighted on every line it covers.
        bool hasMatch = false;
        int matchEndLine = 0;
        int matchEndColumn = 0;
        const TextBuffer &searchTerm = search.getSearchTerm();
        SearchMatch match;

        if (!searchTerm.isEmpty() && search.getCurrentMatch(match))
        {
            if (match.line == cursorLine &&
                match.column == document.getCursorColumn() &&
                document.matchesAt(match.line, match.column, searchTerm,
                                   matchEndLine, matchEndColumn))
            {
                hasMatch = true;
            }
        }

        // Highlight colors = normal colors with foreground and background swapped.
        WORD normalAttribute = info.wAttributes;
        WORD highlightAttribute = ((normalAttribute & 0x0F) << 4) |
                                  ((normalAttribute & 0xF0) >> 4);

        // Draw the document rows.
        Node *current = document.getLineAt(firstVisibleLine);

        for (int row = 0; row < contentRows; row++)
        {
            int screenY = top + headerRows + row;
            int lineIndex = firstVisibleLine + row;

            // Past the last line: draw an empty row (this also erases old text).
            if (current == nullptr)
            {
                writeConsoleRow(console, left, screenY, TextBuffer(""), width);
                colorConsoleRow(console, left, screenY, width,
                                normalAttribute, highlightAttribute, 0, 0);
                continue;
            }

            TextBuffer number;
            number.appendNumber(lineIndex + 1);

            TextBuffer linePrefix(" ");
            linePrefix.appendRepeated(' ', lineNumberWidth - number.getLength());
            linePrefix.append(number);
            linePrefix.append(" | ");

            // lineOffset = number of hidden characters on the left of this row.
            TextBuffer visibleText = current->data;
            int lineOffset = 0;

            if (lineIndex == cursorLine)
            {
                lineOffset = cursorOffset;
                visibleText = visibleText.substr(lineOffset, visibleText.getLength());
            }

            TextBuffer fullRow = linePrefix;
            fullRow.append(visibleText);
            writeConsoleRow(console, left, screenY, fullRow, width);

            // Work out which screen columns of this row get the highlight color.
            // A selected line break is shown as one extra highlighted cell
            // after the text. highlightEnd is not included.
            int highlightStart = 0;
            int highlightEnd = 0;

            if (hasSelection && lineIndex >= selStartLine && lineIndex <= selEndLine)
            {
                int fromColumn = 0;
                if (lineIndex == selStartLine)
                    fromColumn = selStartColumn;

                int toColumn = current->data.getLength() + 1;
                if (lineIndex == selEndLine)
                    toColumn = selEndColumn;

                highlightStart = prefixWidth + fromColumn - lineOffset;
                if (highlightStart < prefixWidth)
                    highlightStart = prefixWidth;

                highlightEnd = prefixWidth + toColumn - lineOffset;
                if (highlightEnd > width)
                    highlightEnd = width;
            }

            // The search match is highlighted only when the selection did not
            // highlight anything on this row.
            if (hasMatch && highlightStart == highlightEnd &&
                lineIndex >= match.line && lineIndex <= matchEndLine)
            {
                int fromColumn = 0;
                if (lineIndex == match.line)
                    fromColumn = match.column;

                int toColumn = current->data.getLength() + 1; // line break cell
                if (lineIndex == matchEndLine)
                    toColumn = matchEndColumn;

                highlightStart = prefixWidth + fromColumn - lineOffset;
                if (highlightStart < prefixWidth)
                    highlightStart = prefixWidth;

                highlightEnd = prefixWidth + toColumn - lineOffset;
                if (highlightEnd > width)
                    highlightEnd = width;
                if (highlightEnd < highlightStart)
                    highlightEnd = highlightStart;
            }

            colorConsoleRow(console, left, screenY, width,
                            normalAttribute, highlightAttribute,
                            highlightStart, highlightEnd);

            current = current->next;
        }

        drawFooter(console, left, top + height - footerRows, width, status);

        // Place the blinking cursor and keep it inside the text area.
        int cursorX = left + prefixWidth + document.getCursorColumn() - cursorOffset;
        int cursorY = top + headerRows + (cursorLine - firstVisibleLine);
        int maxX = info.srWindow.Right;
        int maxY = top + headerRows + contentRows - 1;

        if (cursorX > maxX)
            cursorX = maxX;
        if (cursorX < left)
            cursorX = left;
        if (cursorY > maxY)
            cursorY = maxY;
        if (cursorY < top + headerRows)
            cursorY = top + headerRows;

        COORD cursorPosition;
        cursorPosition.X = (SHORT)cursorX;
        cursorPosition.Y = (SHORT)cursorY;
        SetConsoleCursorPosition(console, cursorPosition);
    }
};
