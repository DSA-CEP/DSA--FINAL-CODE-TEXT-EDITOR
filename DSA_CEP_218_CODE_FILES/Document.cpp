#pragma once
#include "common.cpp"
#include "TextBuffer.cpp"
#include "DynamicArray.cpp"
#include "TextHelpers.cpp"
#include "Node.cpp"
#include "Action.cpp"

// Document management using a doubly linked list.
//
// Ownership: the Document owns EVERY node. Nodes are created with new
// (splitLineAt, replaceLines, constructor) and deleted with delete
// (joinLineAt, freeList). The destructor deletes all remaining nodes.
class Document
{
private:
    Node *head;
    Node *tail;
    int lineCount;
    Node *cursorLine;
    int cursorIndex; // >>> CHANGE 2 (line lookup): NEW, line number of cursorLine (so we never have to count it)
    int cursorColumn;

    // Deletes a whole chain of nodes, starting at 'first'.
    void freeList(Node *first)
    {
        Node *current = first;

        while (current != nullptr)
        {
            Node *temp = current;
            current = current->next; // save next BEFORE deleting this node
            delete temp;
        }
    }

    // Used for a space inside the term (not the first character): if the
    // spaces/tabs and line breaks from line/column on contain at least one
    // line break, all of them are skipped and true is returned.
    // Otherwise nothing changes and false is returned.
    bool skipLineBreakGap(Node *&current, int &line, int &column)
    {
        Node *probe = current;
        int probeLine = line;
        int probeColumn = column;
        bool crossed = false;

        while (true)
        {
            if (probeColumn < probe->data.getLength())
            {
                char ch = probe->data.charAt(probeColumn);

                if (ch != ' ' && ch != '\t')
                {
                    break;
                }

                probeColumn++;
            }
            else if (probe->next != nullptr)
            {
                probe = probe->next;
                probeLine++;
                probeColumn = 0;
                crossed = true;
            }
            else
            {
                break;
            }
        }

        if (!crossed)
        {
            return false;
        }

        current = probe;
        line = probeLine;
        column = probeColumn;
        return true;
    }

    // Checks (ignoring case)whether the term is in the document at line/column,
    // where current is the node of that line. The document is treated as ONE
    // continuous text: the end of every line (except the last) counts as a
    // single line-break character, so a match may run over several lines.
    //  a space in the term matches a space OR a line break
    //     ("hello world" is also found when "hello" ends one line and
    //      "world" starts the next one, even if the lines have spaces or
    //      tabs at the end / start next to the line break)
    // a line break in the term matches only a line break
    // Gives the line and column where the match ends.
    bool matchesFrom(Node *current, int line, int column, const TextBuffer &term,
                     int &endLine, int &endColumn)
    {
        if (current == nullptr || column < 0 || column > current->data.getLength())
        {
            return false;
        }

        for (int i = 0; i < term.getLength(); i++)
        {
            char wanted = lowerChar(term.charAt(i));

            // A space (not at the start of the term) may stand for a line break.
            if (wanted == ' ' && i > 0 && skipLineBreakGap(current, line, column))
            {
                while (i + 1 < term.getLength() && term.charAt(i + 1) == ' ')
                {
                    i++;
                }

                continue;
            }

            if (column == current->data.getLength())
            {
                // End of the line: this position is a line break.
                if (current->next == nullptr || (wanted != '\n' && wanted != ' '))
                {
                    return false;
                }

                current = current->next;
                line++;
                column = 0;
            }
            else
            {
                if (lowerChar(current->data.charAt(column)) != wanted)
                {
                    return false;
                }

                column++;
            }
        }

        endLine = line;
        endColumn = column;
        return true;
    }

public:
    Document()
    {
        head = new Node(TextBuffer(""));
        tail = head;
        cursorLine = head;
        cursorIndex = 0; // >>> CHANGE 2
        cursorColumn = 0;
        lineCount = 1;
    }

    // Copying is forbidden: the document owns its nodes (double delete).
    Document(const Document &) = delete;
    Document &operator=(const Document &) = delete;

    int getLineCount()
    {
        return lineCount;
    }

    // Counts all characters in the document, excluding newline characters.
    int getCharacterCount()
    {
        int total = 0;
        Node *current = head;

        while (current != nullptr)
        {
            total += current->data.getLength();
            current = current->next;
        }

        return total;
    }

    // Checks (ignoring case) whether the term is in the document at
    // line/column (see matchesFrom). Gives the line and column where the
    // match ends.
    bool matchesAt(int line, int column, const TextBuffer &term,
                   int &endLine, int &endColumn)
    {
        return matchesFrom(getLineAt(line), line, column, term, endLine, endColumn);
    }

    // KMP search (Knuth-Morris-Pratt)
    // A term with no space and no line break can never match across two
    // lines, so it can be searched line by line with KMP.
    static bool isSingleLineTerm(const TextBuffer &term)
    {
        for (int i = 0; i < term.getLength(); i++)
        {
            if (term.charAt(i) == ' ' || term.charAt(i) == '\n')
            {
                return false;
            }
        }

        return true;
    }

    // Builds the LPS table: lps[i] = length of the longest part at the start of
    // the term that is also a part ending at position i (case-insensitive).
    // Example "abab" -> 0 0 1 2. Returns false if memory ran out.
    static bool buildLps(const TextBuffer &term, DynamicArray<int> &lps)
    {
        lps.clear();
        lps.add(0);
        int length = 0; // how many letters are matched at the start of the term

        for (int i = 1; i < term.getLength(); i++)
        {
            while (length > 0 &&
                   lowerChar(term.charAt(i)) != lowerChar(term.charAt(length)))
            {
                length = lps.at(length - 1); // fall back, do not start again
            }

            if (lowerChar(term.charAt(i)) == lowerChar(term.charAt(length)))
            {
                length++;
            }

            lps.add(length);
        }

        return lps.size() == term.getLength();
    }

    // Scans ONE line from left to right (never going back) and adds every
    // match of the term to 'matches'. "Overlapping" matches are found too.
    static void kmpSearchLine(const TextBuffer &text, int lineNumber,
                              const TextBuffer &term, const DynamicArray<int> &lps,
                              DynamicArray<SearchMatch> &matches)
    {
        int termLength = term.getLength();
        int matched = 0; // letters of the term matched so far

        for (int i = 0; i < text.getLength(); i++)
        {
            char ch = lowerChar(text.charAt(i));

            while (matched > 0 && ch != lowerChar(term.charAt(matched)))
            {
                matched = lps.at(matched - 1); 
            }

            if (ch == lowerChar(term.charAt(matched)))
            {
                matched++;
            }

            if (matched == termLength)
            {
                SearchMatch match;
                match.line = lineNumber;
                match.column = i - termLength + 1;
                matches.add(match);

                matched = lps.at(matched - 1); // keep going: finds overlaps
            }
        }
    }

    // Finds every match (case-insensitive) of the term in the whole document,
    // line breaks included, and puts them into 'matches'. Every position is
    // tried, the position of a line break too, so overlapping matches are
    // found as well. 'matches' is empty if the term is empty or nothing matches.
    void findAllMatches(const TextBuffer &searchTerm, DynamicArray<SearchMatch> &matches)
    {
        matches.clear();

        if (searchTerm.isEmpty())
        {
            return;
        }

        // Fast way (KMP) for terms that cannot cross a line break.
        // Other terms (with a space) use the slower way below.
        DynamicArray<int> lps;
        bool useKmp = isSingleLineTerm(searchTerm) && buildLps(searchTerm, lps);

        int lineNumber = 0;
        Node *current = head;

        while (current != nullptr)
        {
            if (useKmp)
            {
                kmpSearchLine(current->data, lineNumber, searchTerm, lps, matches);
            }
            else
            {
                for (int column = 0; column <= current->data.getLength(); column++)
                {
                    int endLine, endColumn;

                    if (matchesFrom(current, lineNumber, column, searchTerm,
                                    endLine, endColumn))
                    {
                        SearchMatch match;
                        match.line = lineNumber;
                        match.column = column;
                        matches.add(match);
                    }
                }
            }

            current = current->next;
            lineNumber++;
        }
    }

    // Returns the node of line number 'index', or nullptr if it does not exist.
    Node *getLineAt(int index)
    {
        if (index < 0 || index >= lineCount)
        {
            return nullptr;
        }

        // Start from the closest of 3 points: head, tail or the cursor.
        Node *current = head;
        int pos = 0;

        if (lineCount - 1 - index < index) // tail is closer than head
        {
            current = tail;
            pos = lineCount - 1;
        }

        int toCursor = index - cursorIndex;
        int toStart = index - pos;

        if (toCursor < 0)
            toCursor = -toCursor;
        if (toStart < 0)
            toStart = -toStart;

        if (toCursor < toStart) // cursor is closer than head/tail
        {
            current = cursorLine;
            pos = cursorIndex;
        }

        // Walk from the start point to the wanted line.
        while (pos < index)
        {
            current = current->next;
            pos++;
        }
        while (pos > index)
        {
            current = current->prev;
            pos--;
        }

        return current;
    }

    int getCursorColumn()
    {
        return cursorColumn;
    }

    int getCursorLineIndex()
    {
        // >>> CHANGE 2 (line lookup): was a loop from head, now O(1)
        return cursorIndex;
    }

    // Moves the cursor. An invalid line is ignored; the column is cut to
    // 0 .. length of that line.
    void setCursor(int line, int column)
    {
        Node *target = getLineAt(line);

        if (target == nullptr)
        {
            return;
        }

        cursorLine = target;
        cursorIndex = line; // >>> CHANGE 2

        if (column > target->data.getLength())
        {
            column = target->data.getLength();
        }

        if (column < 0)
        {
            column = 0;
        }

        cursorColumn = column;
    }

    // Returns true if the character was really inserted.
    bool insertCharacterAt(int line, int column, char ch)
    {
        Node *target = getLineAt(line);

        if (target == nullptr)
        {
            return false;
        }

        return target->data.insertChar(column, ch);
    }

    void deleteCharacterAt(int line, int column)
    {
        Node *target = getLineAt(line);

        if (target != nullptr)
        {
            target->data.eraseRange(column, 1);
        }
    }

    // Breaks one line into two at 'column'. The text after the column
    // moves into a new node that is linked in after the current line.
    // Returns false if nothing was changed....
    bool splitLineAt(int line, int column)
    {
        Node *current = getLineAt(line);

        if (current == nullptr)
        {
            return false;
        }

        if (column < 0)
        {
            column = 0;
        }

        if (column > current->data.getLength())
        {
            column = current->data.getLength();
        }

        int expectedLength = current->data.getLength() - column;
        Node *newNode = new (nothrow) Node(current->data.substr(column, expectedLength));

        // If memory ran out, the node or its text is missing: change nothing.
        if (newNode == nullptr)
        {
            return false;
        }

        if (newNode->data.getLength() != expectedLength)
        {
            delete newNode;
            return false;
        }

        current->data.truncate(column); // the old line keeps only the left part

        newNode->next = current->next;
        newNode->prev = current;

        if (current->next != nullptr)
        {
            current->next->prev = newNode;
        }
        else
        {
            tail = newNode;
        }

        current->next = newNode;
        lineCount++;

        if (line < cursorIndex) // >>> CHANGE 2: a line was added above the cursor
        {
            cursorIndex++;
        }

        return true;
    }

    // Adds the next line to the end of this line and removes the next node.
    // The column parameter is not needed, it is kept so the calls stay the same.
    // Returns false if nothing was changed.
    bool joinLineAt(int line, int column)
    {
        (void)column; // not used, see comment above
        Node *current = getLineAt(line);

        if (current == nullptr || current->next == nullptr)
        {
            return false;
        }

        Node *nextNode = current->next;

        // Copy the text first. If this fails, the next node is NOT deleted.
        if (!current->data.append(nextNode->data))
        {
            return false;
        }

        // Unlink nextNode from the list.
        current->next = nextNode->next;

        if (nextNode->next != nullptr)
        {
            nextNode->next->prev = current;
        }
        else
        {
            tail = current;
        }

        // The cursor must never point to a deleted node.
        if (cursorLine == nextNode)
        {
            cursorLine = current;
            cursorIndex = line; // >>> CHANGE 2
            cursorColumn = current->data.getLength();
        }
        else if (line + 1 < cursorIndex) // >>> CHANGE 2: a line above the cursor was removed
        {
            cursorIndex--;
        }

        delete nextNode; // we own the node, so we free it
        lineCount--;
        return true;
    }

    // Inserts multi-line text at line/column.
    // Example: inserting "AB\nCD" into "12|34" (cursor at |) gives
    //     12AB
    //     CD34
    // The text before the cursor joins the first inserted line and the
    // text after the cursor joins the last inserted line.
    // Also used to undo a selection deletion.
    void insertText(int line, int column, const TextBuffer &text)
    {
        DynamicArray<TextBuffer> lines;
        splitLines(text, lines);
        int count = lines.size();
        Node *first = getLineAt(line);

        if (first == nullptr)
        {
            return;
        }

        // Keep the column inside the line so substr() can never fail.
        int length = first->data.getLength();

        if (column < 0)
        {
            column = 0;
        }

        if (column > length)
        {
            column = length;
        }

        TextBuffer before = first->data.substr(0, column);              // text left of the cursor
        TextBuffer after = first->data.substr(column, length - column); // text right of the cursor

        // One line only: just put the text in the middle.
        if (count == 1)
        {
            TextBuffer merged = before;
            bool ok = merged.append(lines.at(0)) && merged.append(after);

            if (ok)
            {
                first->data = merged;
            }

            return;
        }

        TextBuffer firstLine = before;

        if (!firstLine.append(lines.at(0)))
        {
            return;
        }

        first->data = firstLine;

        // Create one new line below the previous one for every extra inserted line.
        for (int i = 1; i < count; i++)
        {
            Node *previous = getLineAt(line + i - 1);

            if (!splitLineAt(line + i - 1, previous->data.getLength()))
            {
                return;
            }

            getLineAt(line + i)->data = lines.at(i);
        }

        // The old text that was after the cursor goes behind the last inserted line.
        getLineAt(line + count - 1)->data.append(after);
    }

    // Exact reverse of insertText. Also used to delete a selected range .
    // 'text' must be exactly the text that is in the document at line OR column.
    void removeText(int line, int column, const TextBuffer &text)
    {
        DynamicArray<TextBuffer> lines;
        splitLines(text, lines);
        int count = lines.size();
        Node *first = getLineAt(line);
        Node *last = getLineAt(line + count - 1);

        if (first == nullptr || last == nullptr)
        {
            return;
        }

        // Column where the removed text ends, inside the last affected line.
        int endColumn = lines.at(count - 1).getLength();

        if (count == 1)
        {
            endColumn = column + lines.at(0).getLength();
        }

        if (column < 0)
        {
            column = 0;
        }

        if (column > first->data.getLength())
        {
            column = first->data.getLength();
        }

        if (endColumn > last->data.getLength())
        {
            endColumn = last->data.getLength();
        }

        TextBuffer before = first->data.substr(0, column);                                   // kept text before the range
        TextBuffer after = last->data.substr(endColumn, last->data.getLength() - endColumn); // kept text after the range

        // Join all affected lines into the first line, then replace its text
        // with just the kept parts.
        for (int i = 1; i < count; i++)
        {
            joinLineAt(line, column);
        }

        TextBuffer kept = before;

        if (kept.append(after))
        {
            first->data = kept;
        }
    }

    // Replaces every occurrence of 'term' (one or more lines) in the whole
    // document. Matches are taken left to right, never overlap, and the
    // replaced text is never searched again. Replacements are made from the
    // end of the document to the start so earlier positions stay correct.
    // Every real change is added to 'changes' in that order so the caller can
    // undo it. Returns how many replacements were made.
    int replaceAll(const TextBuffer &term, const TextBuffer &replacement,
                   DynamicArray<LineChange> &changes)
    {
        DynamicArray<SearchMatch> matches;
        DynamicArray<SearchMatch> chosen;
        findAllMatches(term, matches);
        int lastEndLine = 0;
        int lastEndColumn = 0;

        for (int i = 0; i < matches.size(); i++)
        {
            int endLine, endColumn;
            const SearchMatch &m = matches.at(i);

            // Skip a match that starts inside the previous replaced match.
            if (!chosen.isEmpty() && (m.line < lastEndLine ||
                                      (m.line == lastEndLine && m.column < lastEndColumn)))
            {
                continue;
            }

            matchesAt(m.line, m.column, term, endLine, endColumn);
            lastEndLine = endLine;
            lastEndColumn = endColumn;
            chosen.add(m);
        }

        for (int i = chosen.size() - 1; i >= 0; i--)
        {
            LineChange change;

            if (replaceAt(chosen.at(i).line, chosen.at(i).column, term, replacement, change))
            {
                changes.add(change);
            }
        }

        return chosen.size();
    }

    // Puts all lines of the document into 'lines'.
    void getLines(DynamicArray<TextBuffer> &lines)
    {
        lines.clear();
        Node *current = head;

        while (current != nullptr)
        {
            lines.add(current->data);
            current = current->next;
        }
    }

    // Replaces the match of 'term' (one or more lines) at line/column with
    // 'replacement'. 'change' receives the exact old and new text, so the
    // caller can undo it. Returns true if the text really changed.
    bool replaceAt(int line, int column, const TextBuffer &term,
                   const TextBuffer &replacement, LineChange &change)
    {
        int endLine, endColumn;

        if (!matchesAt(line, column, term, endLine, endColumn))
        {
            return false;
        }

        change.line = line;
        change.column = column;
        change.before = getText(line, column, endLine, endColumn);
        change.after = replacement;

        if (change.before.equals(change.after))
        {
            return false;
        }

        removeText(line, column, change.before);
        insertText(line, column, replacement);
        return true;
    }

    // Returns the text from line/column up to endLine/endColumn
    // (lines separated by '\n'(ENTER KEY)).
    TextBuffer getText(int line, int column, int endLine, int endColumn)
    {
        TextBuffer text;
        Node *current = getLineAt(line);

        for (int i = line; i <= endLine && current != nullptr; i++)
        {
            int from = (i == line) ? column : 0;
            int to = (i == endLine) ? endColumn : current->data.getLength();

            text.append(current->data.substr(from, to - from));

            if (i < endLine)
            {
                text.append('\n');
            }

            current = current->next;
        }

        return text;
    }

    // Replace document contents with lines loaded from a file.
    // The new list is built FIRST. Only if that works, the old list is freed.
    // So if memory runs out, the old document stays unchanged.
    bool replaceLines(const DynamicArray<TextBuffer> &lines)
    {
        Node *newHead = nullptr;
        Node *newTail = nullptr;
        int newCount = 0;

        if (lines.isEmpty())
        {
            newHead = new (nothrow) Node(TextBuffer(""));

            if (newHead == nullptr)
            {
                return false;
            }

            newTail = newHead;
            newCount = 1;
        }
        else
        {
            for (int i = 0; i < lines.size(); i++)
            {
                Node *newNode = new (nothrow) Node(lines.at(i));

                if (newNode == nullptr || newNode->data.getLength() != lines.at(i).getLength())
                {
                    delete newNode;    // deleting nullptr is allowed
                    freeList(newHead); // throw away the half built list
                    return false;
                }

                if (newHead == nullptr)
                {
                    newHead = newNode;
                    newTail = newNode;
                }
                else
                {
                    newTail->next = newNode;
                    newNode->prev = newTail;
                    newTail = newNode;
                }

                newCount++;
            }
        }

        freeList(head); // free every old node
        head = newHead;
        tail = newTail;
        lineCount = newCount;
        cursorLine = head;
        cursorIndex = 0; // >>> CHANGE 2
        cursorColumn = 0;
        return true;
    }

    ~Document()
    {
        freeList(head);
    }
};
