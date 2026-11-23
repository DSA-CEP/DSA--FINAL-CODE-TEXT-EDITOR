#pragma once
#include "common.cpp"
#include "Stack.cpp"
#include "Action.cpp"
#include "Document.cpp"

// Manages Undo and Redo.
class UndoRedo
{
private:
    Stack<Action> undoStack;
    Stack<Action> redoStack;

public:
    void record(const Action &action)
    {
        redoStack.clear(); // a new edit makes the old redo history invalid

        // >>> CHANGE 4 (undo typing): the merge block below is new
        Action *last = undoStack.top();

        // A typed letter right after the previous typed letters (same word)
        // is added to that action, so Ctrl+Z removes the whole word at once.
        if (last != nullptr && action.type == INSERT_CHAR &&
            (last->type == INSERT_CHAR || last->type == TYPED_TEXT) &&
            last->afterLine == action.line && last->afterColumn == action.column &&
            action.text.charAt(0) != ' ' &&
            last->text.charAt(last->text.getLength() - 1) != ' ')
        {
            last->text.append(action.text);
            last->type = TYPED_TEXT;
            last->afterColumn = action.afterColumn;
            return;
        }

        undoStack.push(action);
    }

    void clearHistory()
    {
        undoStack.clear();
        redoStack.clear();
    }

    void undo(Document &document)
    {
        Action action;

        if (!undoStack.pop(action))
        {
            return;
        }

        switch (action.type)
        {
        case INSERT_CHAR:
            document.deleteCharacterAt(
                action.line, action.column);
            break;

        case DELETE_CHAR:
            document.insertCharacterAt(
                action.line, action.column, action.text.charAt(0));
            break;

        case SPLIT_LINE:
            document.joinLineAt(
                action.line, action.column);
            break;

        case JOIN_LINES:
            document.splitLineAt(
                action.line, action.column);
            break;

        case REPLACE_TEXT:
            // Undo the replacements in the opposite order they were made.
            for (int i = action.changes.size() - 1; i >= 0; i--)
            {
                LineChange &c = action.changes.at(i);

                document.removeText(c.line, c.column, c.after);
                document.insertText(c.line, c.column, c.before);
            }
            break;

        case TYPED_TEXT: // >>> CHANGE 4 (new line)
        case MERGE_TEXT:
            // Remove the whole merged block in one step.
            document.removeText(action.line, action.column, action.text);
            break;

        case DELETE_RANGE:
            // Put the deleted text back exactly where it was.
            document.insertText(action.line, action.column, action.text);
            break;
        }

        document.setCursor(
            action.beforeLine, action.beforeColumn);

        redoStack.push(action);
    }

    void redo(Document &document)
    {
        Action action;

        if (!redoStack.pop(action))
        {
            return;
        }

        switch (action.type)
        {
        case INSERT_CHAR:
            document.insertCharacterAt(
                action.line, action.column, action.text.charAt(0));
            break;

        case DELETE_CHAR:
            document.deleteCharacterAt(
                action.line, action.column);
            break;

        case SPLIT_LINE:
            document.splitLineAt(
                action.line, action.column);
            break;

        case JOIN_LINES:
            document.joinLineAt(
                action.line, action.column);
            break;

        case REPLACE_TEXT:
            // Make the same replacements again, in the same order.
            for (int i = 0; i < action.changes.size(); i++)
            {
                LineChange &c = action.changes.at(i);

                document.removeText(c.line, c.column, c.before);
                document.insertText(c.line, c.column, c.after);
            }
            break;

        case TYPED_TEXT: // >>> CHANGE 4 (new line)
        case MERGE_TEXT:
            // Insert the whole merged block again in one step.
            document.insertText(action.line, action.column, action.text);
            break;

        case DELETE_RANGE:
            // Delete the same range again in one step.
            document.removeText(action.line, action.column, action.text);
            break;
        }

        document.setCursor(
            action.afterLine, action.afterColumn);

        undoStack.push(action);
    } //END
};
