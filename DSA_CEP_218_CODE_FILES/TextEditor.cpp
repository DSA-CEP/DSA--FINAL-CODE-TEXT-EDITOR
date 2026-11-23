#pragma once
#include "common.cpp"
#include "TextBuffer.cpp"
#include "DynamicArray.cpp"
#include "TextHelpers.cpp"
#include "Document.cpp"
#include "UndoRedo.cpp"
#include "FileManager.cpp"
#include "Selection.cpp"
#include "Search.cpp"
#include "EditorDisplay.cpp"

// ================= TEXT EDITOR =================

// Coordinates everything: it reads the keyboard, asks the user for input,
// moves the cursor and edits the document, and uses the other classes:
//   Document    - the text and the cursor
//   UndoRedo    - the edit history
//   Search      - the search matches (it never moves the cursor itself)
//   Selection   - the selected range
//   EditorDisplay - drawing on the console
//   FileManager - saving and opening files
class TextEditor
{
private:
    Document document;
    UndoRedo history;
    Search search;
    Selection selection;
    EditorDisplay display;

    TextBuffer filename; // "" means a new document that was never saved
    TextBuffer status;   // message shown in the last row

    // ---------- Asking for file names and opening files ----------

    // Shows an error under the filename prompt and waits for a key.
    void showFilenameError(const char *message)
    {
        cout << message << "\n";
        cout << "Press any key to try again...";
        _getch();
        cout << "\n";
    }

    // Gets a filename and validates that the user explicitly enters .txt.
    // requireUnique is used for new documents.
    // Returns the full path, or "" when the user enters nothing.
    TextBuffer getFilename(bool requireUnique = false)
    {
        TextBuffer name;

        while (true)
        {
            // Use a clean prompt screen so the filename input never overlaps
            // the editor's footer or the current editing cursor.
            system("cls");
            cout << "Enter filename (example: notes.txt): ";
            readLine(name);

            if (name.isEmpty())
                return TextBuffer("");

            if (!FileManager::hasTxtExtension(name))
            {
                showFilenameError("ERROR: INVALID FILE EXTENSION. ONLY .txt FILES ARE ALLOWED.");
                continue;
            }

            if (FileManager::hasInvalidCharacters(name))
            {
                showFilenameError("ERROR: INVALID FILENAME. ENTER A NAME, NOT A PATH.");
                continue;
            }

            TextBuffer fullPath = FileManager::buildPath(name);

            if (requireUnique && FileManager::exists(fullPath))
            {
                showFilenameError("ERROR: FILE NAME ALREADY USED. PLEASE ENTER A UNIQUE NAME.");
                continue;
            }

            return fullPath;
        }
    }

    // Opens a file into 'target' and shows the result message to the user.
    bool loadFile(Document &target, const TextBuffer &path)
    {
        TextBuffer error;

        if (!FileManager::openFile(target, path, error))
        {
            display.showMessage(error);
            return false;
        }

        display.showMessage("DOCUMENT OPENED SUCCESSFULLY!");
        return true;
    }

    // ---------- Search (Ctrl+F2, F3, F4) ----------

    // Text for the status line, for example "MATCH 2 OF 5: hello (CTRL+N/F3 NEXT, F4 LIST)".
    TextBuffer matchInfoText()
    {
        TextBuffer info("MATCH ");
        info.appendNumber(search.getCurrentMatchIndex() + 1);
        info.append(" OF ");
        info.appendNumber(search.getMatches().size());
        info.append(": ");
        info.append(displayText(search.getSearchTerm()));
        info.append(" (CTRL+N/F3 NEXT, F4 LIST)");
        return info;
    }

    // Shared by Ctrl+F2 and F4: moves the cursor to the current match, shows the
    // list of all matches and then sets the status line.
    void showFoundMatches()
    {
        SearchMatch match;

        if (search.getCurrentMatch(match))
        {
            document.setCursor(match.line, match.column);
        }

        display.showSearchResults(search);

        status = "FOUND ";
        status.appendNumber(search.getMatches().size());
        status.append(" MATCHES - ");
        status.append(matchInfoText());
    }

    // Returns the selected text (lines separated by '\n'), or "" if nothing is selected.
    TextBuffer getSelectedText()
    {
        int startLine, startColumn, endLine, endColumn;

        if (!selection.getRange(document, startLine, startColumn, endLine, endColumn))
        {
            return TextBuffer("");
        }

        return selection.getText(document, startLine, startColumn, endLine, endColumn);
    }

    // Asks for a word or phrase and finds all matches (Ctrl+Space, F2 and Ctrl+F2).
    // Line breaks are found automatically. If text was selected, pressing Enter
    // alone searches for the selected text (it may cover several lines).
    void searchDocument(const TextBuffer &selectedText)
    {
        system("cls");
        cout << "========== SEARCH DOCUMENT ==========" << endl;

        if (!selectedText.isEmpty())
        {
            cout << "Selected text: " << displayText(selectedText).c_str() << endl;
            cout << "Press Enter to search for it, or type another word or phrase: ";
        }
        else
        {
            cout << "Enter a word or phrase: ";
        }

        TextBuffer searchTerm;
        readLine(searchTerm);

        if (searchTerm.isEmpty())
        {
            searchTerm = selectedText;
        }
        else
        {
            searchTerm = unescapeLineBreaks(searchTerm); // typed \n still works
        }

        if (searchTerm.isEmpty())
        {
            status = "SEARCH CANCELLED - EMPTY SEARCH TERM";
            return;
        }

        // find() selects the first match when there is one.
        if (!search.find(document, searchTerm))
        {
            status = "NO MATCHES FOUND: ";
            status.append(displayText(searchTerm));
            return;
        }

        showFoundMatches();
    }

    // F4: shows the match list of the last search again. The matches are found
    // again first, so edits made since the search are included.
    void listSearchMatches()
    {
        if (search.getSearchTerm().isEmpty())
        {
            status = "SEARCH FIRST USING CTRL+F2";
            return;
        }

        if (!search.refresh(document))
        {
            status = "NO MATCHES FOUND: ";
            status.append(displayText(search.getSearchTerm()));
            return;
        }

        showFoundMatches();
    }

    // Ctrl+N and F3: moves to the next match (after the last one it wraps to the first).
    void goToNextSearchMatch()
    {
        // >>> CHANGE 3 (next match): was search.nextMatch(), now binary search from the cursor
        if (!search.nextMatchAfter(document.getCursorLineIndex(), document.getCursorColumn()))
        {
            status = "SEARCH FIRST USING CTRL+F2";
            return;
        }

        SearchMatch match;
        search.getCurrentMatch(match);
        document.setCursor(match.line, match.column);

        status = matchInfoText();
    }

    // ---------- Replace One (F8) and Replace All (F7) ----------

    // Asks for the search term and the replacement term.
    // Returns false if the search term is empty (operation cancelled).
    // An empty replacement is allowed and simply deletes the matched text.
    bool getReplaceTerms(const char *title, TextBuffer &searchTerm, TextBuffer &replacement)
    {
        system("cls");
        cout << "========== " << title << " ==========" << endl;
        cout << "Enter a word or phrase to find (line breaks are matched automatically): ";
        readLine(searchTerm);
        searchTerm = unescapeLineBreaks(searchTerm);

        if (searchTerm.isEmpty())
        {
            return false;
        }

        cout << "Enter the replacement (leave empty to delete): ";
        readLine(replacement);
        replacement = unescapeLineBreaks(replacement);

        return true;
    }

    // Replace One (F8): the user picks one occurrence with Up/Down,
    // Enter replaces it, Esc cancels without changing anything.
    void replaceOneOccurrence()
    {
        TextBuffer searchTerm;
        TextBuffer replacement;

        if (!getReplaceTerms("REPLACE ONE OCCURRENCE", searchTerm, replacement))
        {
            status = "REPLACE CANCELLED - EMPTY SEARCH TERM";
            return;
        }

        DynamicArray<SearchMatch> matches;
        document.findAllMatches(searchTerm, matches);

        if (matches.isEmpty())
        {
            status = "NO MATCHES FOUND: ";
            status.append(displayText(searchTerm));
            return;
        }

        // Remember the cursor so Esc can put it back.
        int originalLine = document.getCursorLineIndex();
        int originalColumn = document.getCursorColumn();

        int total = matches.size();
        int selected = 0;

        // Only used to highlight the selected match (even over several lines).
        Search preview;
        preview.find(document, searchTerm);

        // Let the user choose a match. The editor is redrawn each time so the
        // cursor and highlight jump to the selected match.
        while (true)
        {
            document.setCursor(matches.at(selected).line, matches.at(selected).column);
            preview.setCurrentMatch(selected);

            TextBuffer pickStatus("FOUND ");
            pickStatus.appendNumber(total);
            pickStatus.append(" | MATCH ");
            pickStatus.appendNumber(selected + 1);
            pickStatus.append(" OF ");
            pickStatus.appendNumber(total);
            pickStatus.append(" | UP/DOWN: MOVE | ENTER: REPLACE | ESC: CANCEL");

            display.render(document, selection, preview, filename, pickStatus);

            int key = _getch();

            // ESC cancels without touching the document.
            if (key == 27)
            {
                document.setCursor(originalLine, originalColumn);
                status = "REPLACE CANCELLED - DOCUMENT NOT CHANGED";
                return;
            }

            // Enter confirms the selected match.
            if (key == 13)
            {
                break;
            }

            // Arrow keys come as two codes: 0 or 224, then the real code.
            if (key == 0 || key == 224)
            {
                key = _getch();

                if (key == 72) // Up (wraps from the first match to the last)
                {
                    selected--;
                    if (selected < 0)
                        selected = total - 1;
                }
                else if (key == 80) // Down (wraps from the last match to the first)
                {
                    selected++;
                    if (selected >= total)
                        selected = 0;
                }
            }
        }

        SearchMatch match = matches.at(selected);

        // The document replaces the text and remembers the line before/after,
        // so undo/redo are exact.
        LineChange change;
        bool changed = document.replaceAt(match.line, match.column,
                                          searchTerm, replacement, change);

        // The new text may have several lines: the cursor goes to its end.
        DynamicArray<TextBuffer> newLines;
        splitLines(replacement, newLines);
        int afterLine = match.line + newLines.size() - 1;
        int afterColumn = newLines.at(newLines.size() - 1).getLength();

        if (newLines.size() == 1)
        {
            afterColumn += match.column;
        }

        // Undo puts the cursor back on the match, redo puts it after the new text.
        Action action(
            REPLACE_TEXT, match.line, match.column, replacement,
            match.line, match.column, afterLine, afterColumn);

        action.changes.add(change);

        // Nothing to record if the replacement is the same as the old text.
        if (changed)
        {
            history.record(action);
        }

        document.setCursor(afterLine, afterColumn);

        search.clearResults();

        status = "REPLACED MATCH ";
        status.appendNumber(selected + 1);
        status.append(" OF ");
        status.appendNumber(total);
        status.append(" WITH: ");
        status.append(displayText(replacement));
    }

    // Replace All (F7): replaces every occurrence in one
    // undoable action.
    void replaceAllOccurrences()
    {
        TextBuffer searchTerm;
        TextBuffer replacement;

        if (!getReplaceTerms("REPLACE ALL OCCURRENCES", searchTerm, replacement))
        {
            status = "REPLACE ALL CANCELLED - EMPTY SEARCH TERM";
            return;
        }

        int originalLine = document.getCursorLineIndex();
        int originalColumn = document.getCursorColumn();

        // ONE action holds the changes of all lines, so a single Ctrl+Z undoes
        // everything and a single Ctrl+Y redoes everything.
        Action action(
            REPLACE_TEXT, originalLine, originalColumn, replacement,
            originalLine, originalColumn, originalLine, originalColumn);

        int replacementCount = document.replaceAll(searchTerm, replacement,
                                                   action.changes);

        if (replacementCount == 0)
        {
            status = "NO MATCHES FOUND: ";
            status.append(displayText(searchTerm));
            return;
        }

        if (!action.changes.isEmpty())
        {
            history.record(action);
        }

        // Line lengths may have changed, so setCursor keeps the column valid.
        document.setCursor(originalLine, originalColumn);

        search.clearResults();

        status = "REPLACED ";
        status.appendNumber(replacementCount);
        status.append(" OCCURRENCE(S) OF: ");
        status.append(displayText(searchTerm));
        status.append(" (CTRL+Z TO UNDO)");
    }

    // ---------- Merge another file (Ctrl+O) ----------

    // Merge Document (Ctrl+O): inserts another .txt file at the cursor.
    // The source file is only read, never changed, and the current document
    // keeps its filename. The merge is stored as ONE undo action.
    void mergeDocument()
    {
        TextBuffer sourceName = getFilename();

        if (sourceName.isEmpty())
        {
            status = "MERGE CANCELLED";
            return;
        }

        // Read the source into a temporary document so the current one is kept.
        Document source;

        if (!loadFile(source, sourceName))
        {
            status = "MERGE FAILED - NOTHING WAS CHANGED";
            return;
        }

        // Take the lines of the source document.
        DynamicArray<TextBuffer> lines;
        source.getLines(lines);

        // An empty file is loaded as one empty line, so there is nothing to add.
        if (lines.size() == 1 && lines.at(0).isEmpty())
        {
            status = "NOTHING TO MERGE - SOURCE FILE IS EMPTY";
            return;
        }

        int count = lines.size();
        TextBuffer text = linesToText(lines);

        int line = document.getCursorLineIndex();
        int column = document.getCursorColumn();

        // The cursor ends right after the last inserted character.
        int afterLine = line + count - 1;
        int afterColumn = lines.at(count - 1).getLength();

        if (count == 1)
        {
            afterColumn = column + lines.at(0).getLength();
        }

        Action action(
            MERGE_TEXT, line, column, text,
            line, column, afterLine, afterColumn);

        document.insertText(line, column, text);
        history.record(action);

        document.setCursor(afterLine, afterColumn);

        search.clearResults();

        status = "MERGED ";
        status.appendNumber(count);
        status.append(" LINE(S) FROM ");
        status.append(sourceName);
        status.append(" (CTRL+Z TO UNDO)");
    }

    // ---------- Delete the selected text ----------

    // Deletes the selected text (one word, a range, or several lines) as ONE
    // undoable action. Returns false if nothing is selected, so the caller
    // can fall back to the normal single-character delete.
    bool deleteSelection()
    {
        int startLine, startColumn, endLine, endColumn;

        if (!selection.getRange(document, startLine, startColumn, endLine, endColumn))
        {
            return false;
        }

        TextBuffer text = selection.getText(document, startLine, startColumn,
                                            endLine, endColumn);

        // Undo and redo both put the cursor at the start of the deleted range.
        Action action(
            DELETE_RANGE, startLine, startColumn, text,
            startLine, startColumn, startLine, startColumn);

        document.removeText(startLine, startColumn, text);
        history.record(action);

        document.setCursor(startLine, startColumn);
        selection.clear();

        search.clearResults();

        status = "SELECTION DELETED (CTRL+Z TO UNDO)";
        return true;
    }

    // Saving (Ctrl+S, Ctrl+A)

    // Ctrl + S: saves to the current file. A new document asks for a name first.
    void saveDocument()
    {
        if (filename.isEmpty())
        {
            TextBuffer newFilename = getFilename(true);

            if (!newFilename.isEmpty())
                filename = newFilename;
        }

        if (filename.isEmpty())
            return;

        if (FileManager::saveFile(document, filename))
            status = "DOCUMENT SAVED SUCCESSFULLY";
        else
            status = "ERROR: DOCUMENT COULD NOT BE SAVED";
    }

    // Ctrl + A: Rename. The current file gets the new name and the old
    // file name is removed from the folder (it is not copied).
    // A document that was never saved has no old file, so it is simply
    // saved under the chosen name.
    void saveDocumentAs()
    {
        TextBuffer newFilename = getFilename(false);

        if (newFilename.isEmpty())
        {
            status = "RENAME CANCELLED";
            return;
        }

        // Same name as now: nothing to rename, just save the changes.
        if (!filename.isEmpty() && newFilename.equals(filename))
        {
            if (FileManager::saveFile(document, filename))
                status = "NAME UNCHANGED - DOCUMENT SAVED";
            else
                status = "ERROR: DOCUMENT COULD NOT BE SAVED";
            return;
        }

        // A different existing file would be replaced, so ask first.
        if (FileManager::exists(newFilename))
        {
            system("cls");
            cout << "The file already exists:\n"
                 << newFilename.c_str() << "\n\n";
            cout << "Replace this file? (Y/N): ";
            int answer = _getch();
            cout << (char)answer << "\n";

            if (answer != 'y' && answer != 'Y')
            {
                status = "RENAME CANCELLED - ORIGINAL FILE KEPT";
                return;
            }
        }

        // Rename the file on disk first, so the old name disappears.
        // If the old file is missing (never saved / deleted outside the
        // program) there is nothing to rename and we just save below
        if (!filename.isEmpty() && FileManager::exists(filename))
        {
            if (!FileManager::renameFile(filename, newFilename))
            {
                status = "ERROR: FILE COULD NOT BE RENAMED";
                return;
            }
        }

        // Write the current text (including unsaved changes) to the new name.
        if (FileManager::saveFile(document, newFilename))
        {
            filename = newFilename; // editor now uses the new path and name
            status = "DOCUMENT RENAMED TO ";
            status.append(newFilename);
        }
        else
        {
            // The rename already happened, so keep pointing at the file
            // that now exists on disk.
            if (FileManager::exists(newFilename))
                filename = newFilename;

            status = "ERROR: DOCUMENT COULD NOT BE SAVED";
        }
    }

    // ---------- Moving the cursor and deleting characters ----------

    // Moves the cursor for the Up, Down, Left and Right keys.
    // 'line' and 'column' are the cursor position before the move.
    void moveCursor(int key, int line, int column)
    {
        Node *current = document.getLineAt(line);

        if (current == nullptr)
        {
            return;
        }

        switch (key)
        {
        case 72: // Up
            if (line > 0)
            {
                document.setCursor(line - 1, column);
            }
            break;

        case 80: // Down
            if (current->next != nullptr)
            {
                document.setCursor(line + 1, column);
            }
            break;

        case 75: // Left
            if (column > 0)
            {
                document.setCursor(line, column - 1);
            }
            else if (line > 0)
            {
                // At the start of a line: go to the end of the line above.
                Node *previous = document.getLineAt(line - 1);

                document.setCursor(line - 1, previous->data.getLength());
            }
            break;

        case 77: // Right
            if (column < current->data.getLength())
            {
                document.setCursor(line, column + 1);
            }
            else if (current->next != nullptr)
            {
                // At the end of a line: go to the start of the next line.
                document.setCursor(line + 1, 0);
            }
            break;
        }
    }

    // Delete key: removes the character under the cursor, or joins the next line
    // to this one when the cursor is at the end of the line.
    void deleteKeyPressed(int line, int column)
    {
        Node *current = document.getLineAt(line);

        if (current == nullptr)
        {
            return;
        }

        if (column < current->data.getLength())
        {
            TextBuffer removed(1, current->data.charAt(column));

            Action action(
                DELETE_CHAR, line, column, removed,
                line, column, line, column);

            history.record(action);

            document.deleteCharacterAt(line, column);
        }
        else if (current->next != nullptr)
        {
            Action action(
                JOIN_LINES, line, column, "",
                line, column, line, column);

            // Record only if the join really happened.
            if (document.joinLineAt(line, column))
            {
                history.record(action);
            }
        }
    }

    // Backspace key: removes the character before the cursor, or joins this
    // line to the end of the line above when the cursor is at the line start.
    void backspacePressed(int line, int column)
    {
        if (column > 0)
        {
            Node *current = document.getLineAt(line);

            if (current == nullptr)
            {
                return;
            }

            TextBuffer removed(1, current->data.charAt(column - 1));

            Action action(
                DELETE_CHAR, line, column - 1, removed,
                line, column, line, column - 1);

            history.record(action);

            document.deleteCharacterAt(line, column - 1);
            document.setCursor(line, column - 1);
        }
        else if (line > 0)
        {
            Node *previous = document.getLineAt(line - 1);

            int previousLength = previous->data.getLength();

            Action action(
                JOIN_LINES, line - 1, previousLength, "",
                line, column, line - 1, previousLength);

            if (document.joinLineAt(line - 1, previousLength))
            {
                history.record(action);
                document.setCursor(line - 1, previousLength);
            }
        }
    }

    // Reading keys (called by run)

    // Arrow keys, Delete and F-keys send two codes: 0 or 224 first, then the
    // real key code. 'key' is already the second code.
    void handleExtendedKey(int key, int line, int column)
    {
        // The selection is cleared below, so keep its text for the search prompt.
        TextBuffer selectedText;

        if (key == 32 || key == 60 || key == 0x5F)
        {
            selectedText = getSelectedText();
        }

        // Shift + a movement key starts or extends the selection.
        // _getch() cannot report Shift, so Windows is asked for its state.
        bool isMoveKey = (key == 72 || key == 80 || key == 75 ||
                          key == 77);
        bool shiftHeld = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        if (isMoveKey && shiftHeld)
        {
            // The selection starts where the cursor was before the move.
            if (!selection.isActive())
            {
                selection.start(line, column);
            }
        }
        else if (key == 83 && deleteSelection())
        {
            return; // Delete removed the selected text.
        }
        else
        {
            selection.clear(); // Any other key cancels the selection.
        }

        // Search: Ctrl+Space (can arrive as NUL followed by ASCII space,
        // code 32), F2 (code 60) and Ctrl+F2 (code 0x5F on MSVC).
        if (key == 32 || key == 60 || key == 0x5F)
        {
            searchDocument(selectedText);
            return;
        }

        // F3 (scan code 61) moves to the next match.
        if (key == 61)
        {
            goToNextSearchMatch();
            return;
        }

        // F4 (scan code 62) lists every match of the last search.
        if (key == 62)
        {
            listSearchMatches();
            return;
        }

        // F7 (scan code 65) replaces every occurrence.
        if (key == 65)
        {
            replaceAllOccurrences();
            return;
        }

        // F8 (scan code 66) replaces one selected occurrence.
        if (key == 66)
        {
            replaceOneOccurrence();
            return;
        }

        if (key == 83) // Delete
        {
            deleteKeyPressed(line, column);
            search.clearResults();
        }
        else
        {
            moveCursor(key, line, column);
        }
    }

    // Normal keys: Ctrl shortcuts, Enter, Backspace and printable characters.
    void handleNormalKey(int key, int line, int column)
    {
        // Backspace deletes the selected text; any other key cancels it.
        if (key == 8 && deleteSelection())
        {
            return;
        }

        selection.clear();

        switch (key)
        {
        case 1: // Ctrl + A - Rename
            saveDocumentAs();
            break;

        case 14: // Ctrl + N - Next search match (reliable fallback for F3)
            goToNextSearchMatch();
            break;

        case 19: // Ctrl + S - Save
            saveDocument();
            break;

        case 15: // Ctrl + O - Merge another document at the cursor
            mergeDocument();
            break;

        case 26: // Ctrl + Z
            history.undo(document);
            search.clearResults();
            break;

        case 25: // Ctrl + Y
            history.redo(document);
            search.clearResults();
            break;

        case 13: // Enter - split the line at the cursor
        {
            Action action(
                SPLIT_LINE, line, column, "",
                line, column, line + 1, 0);

            // Record only if the line was really split.
            if (document.splitLineAt(line, column))
            {
                history.record(action);
                document.setCursor(line + 1, 0);
                search.clearResults();
            }

            break;
        }

        case 8: // Backspace
            backspacePressed(line, column);
            search.clearResults();
            break;

        default:
            // Normal printable characters.
            if (key >= 32 && key <= 126)
            {
                Action action(
                    INSERT_CHAR, line, column,
                    TextBuffer(1, (char)key),
                    line, column, line, column + 1);

                // Record only if the character was really inserted.
                if (document.insertCharacterAt(line, column, (char)key))
                {
                    history.record(action);
                    document.setCursor(line, column + 1);
                    search.clearResults();
                }
            }

            break;
        }
    }

public:
    TextEditor()
    {
        filename = "";
        status = "";
    }

    // The editor owns a document, so it must not be copied.
    TextEditor(const TextEditor &) = delete;
    TextEditor &operator=(const TextEditor &) = delete;

    // Menu option 2: asks for a file name and opens that file.
    // Returns true when the document was opened and the editor can start.
    bool openExistingDocument()
    {
        system("cls");

        TextBuffer path = getFilename();

        if (path.isEmpty())
        {
            return false;
        }

        if (!loadFile(document, path))
        {
            return false;
        }

        filename = path;
        history.clearHistory();
        search.clear();
        selection.clear();
        return true;
    }

    // Main loop: draw, read one key, handle it. Esc returns to the main menu.
    void run()
    {
        bool running = true;
        status = "";

        selection.clear();

        while (running)
        {
            // While text is selected, the status row shows the selection shortcuts.
            int selStartLine, selStartColumn, selEndLine, selEndColumn;

            if (selection.getRange(document, selStartLine, selStartColumn,
                                   selEndLine, selEndColumn))
            {
                display.render(document, selection, search, filename,
                               "SELECTED | DEL/BACKSPACE: DELETE | SHIFT+ARROWS: CHANGE | OTHER KEY: CANCEL");
            }
            else
            {
                display.render(document, selection, search, filename, status);
            }

            int key = _getch();

            // ESC returns to the main menu.
            if (key == 27)
            {
                running = false;
                break;
            }

            int line = document.getCursorLineIndex();
            int column = document.getCursorColumn();

            if (key == 0 || key == 224)
            {
                handleExtendedKey(_getch(), line, column);
            }
            else
            {
                handleNormalKey(key, line, column);
            }
        }
    }
};
