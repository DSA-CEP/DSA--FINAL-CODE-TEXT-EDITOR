#pragma once
#include "common.cpp"
#include "DynamicArray.cpp"
#include "Document.cpp"

// ================= SEARCH =================

// Keeps the state of the current search: the search term, every match and
// which match is the current one. It does NOT move the cursor, read keys or
// print anything. The editor does that using the data stored here.
class Search
{
private:
    TextBuffer searchTerm;
    DynamicArray<SearchMatch> matches;
    int currentMatchIndex; // -1 means "no current match"

public:
    Search()
    {
        currentMatchIndex = -1;
    }

    // Stores the term, asks the document for all matches and selects the
    // first one. The term is stored even when nothing matches (F4 uses it).
    // An empty term is ignored: the old search stays as it was.
    bool find(Document &document, const TextBuffer &term)
    {
        if (term.isEmpty())
        {
            return false;
        }

        searchTerm = term;
        document.findAllMatches(term, matches);
        currentMatchIndex = -1;

        if (matches.isEmpty())
        {
            return false;
        }

        currentMatchIndex = 0;
        return true;
    }

    // Finds the matches of the stored term again (used by F4, so edits made
    // since the search are included). Keeps the current match number if it
    // still exists, otherwise goes back to the first match.
    bool refresh(Document &document)
    {
        if (searchTerm.isEmpty())
        {
            return false;
        }

        document.findAllMatches(searchTerm, matches);

        if (matches.isEmpty())
        {
            currentMatchIndex = -1;
            return false;
        }

        if (currentMatchIndex < 0 || currentMatchIndex >= matches.size())
        {
            currentMatchIndex = 0;
        }

        return true;
    }

    // Moves to the next match. After the last one it wraps to the first.
    // Returns false when there are no matches.
    bool nextMatch()
    {
        if (matches.isEmpty() || currentMatchIndex < 0)
        {
            return false;
        }

        currentMatchIndex++;

        if (currentMatchIndex >= matches.size())
        {
            currentMatchIndex = 0;
        }

        return true;
    }


    // Moves to the first match AFTER the cursor (line, column), using binary
    // search on the sorted match list: O(log k). Wraps to the first match.
    bool nextMatchAfter(int line, int column)
    {
        if (matches.isEmpty())
        {
            return false;
        }

        int low = 0;
        int high = matches.size(); // the answer is somewhere in low..high

        while (low < high)
        {
            int mid = (low + high) / 2;
            const SearchMatch &m = matches.at(mid);

            if (m.line < line || (m.line == line && m.column <= column))
            {
                low = mid + 1; // this match is not after the cursor: go right
            }
            else
            {
                high = mid; // this match is after the cursor: go left
            }
        }

        currentMatchIndex = (low == matches.size()) ? 0 : low; // wrap around
        return true;
    }

    // Moves to the previous match. Before the first one it wraps to the last.
    // (No keyboard shortcut uses this yet.)
    bool previousMatch()
    {
        if (matches.isEmpty() || currentMatchIndex < 0)
        {
            return false;
        }

        currentMatchIndex--;

        if (currentMatchIndex < 0)
        {
            currentMatchIndex = matches.size() - 1;
        }

        return true;
    }

    // Selects one match by index. An invalid index changes nothing.
    bool setCurrentMatch(int index)
    {
        if (index < 0 || index >= matches.size())
        {
            return false;
        }

        currentMatchIndex = index;
        return true;
    }

    // Forgets everything: term and matches.
    void clear()
    {
        searchTerm.clear();
        matches.clear();
        currentMatchIndex = -1;
    }

    // Forgets only the matches and keeps the term. Called after the text
    // changes, because the old line/column numbers may no longer be correct.
    void clearResults()
    {
        matches.clear();
        currentMatchIndex = -1;
    }

    bool hasMatches() const
    {
        return !matches.isEmpty();
    }

    const DynamicArray<SearchMatch> &getMatches() const
    {
        return matches;
    }

    // Gives the line and column of the current match.
    // Returns false when there is no current match.
    bool getCurrentMatch(SearchMatch &result) const
    {
        if (currentMatchIndex < 0 || currentMatchIndex >= matches.size())
        {
            return false;
        }

        result = matches.at(currentMatchIndex);
        return true;
    }

    const TextBuffer &getSearchTerm() const
    {
        return searchTerm;
    }

    int getCurrentMatchIndex() const
    {
        return currentMatchIndex;
    }
};
