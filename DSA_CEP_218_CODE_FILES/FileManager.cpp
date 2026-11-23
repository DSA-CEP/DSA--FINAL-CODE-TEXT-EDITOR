#pragma once
#include "common.cpp"
#include "TextBuffer.cpp"
#include "DynamicArray.cpp"
#include "TextHelpers.cpp"

// ================= FILE MANAGER =================

// Only works with files and file names. It never prints, asks or waits for
// a key. Every result is returned to the caller, which decides what to show.
// Files are read and written one character at a time with ifstream/ofstream
// (no string class is used).
class FileManager
{
private:
    static TextBuffer folderName()
    {
        return TextBuffer("TextEditorFiles\\");
    }

public:
    // Creates the document folder if it does not already exist.
    static void createDocumentFolder()
    {
        CreateDirectoryA("TextEditorFiles", NULL);
    }

    // Returns true if a file or folder with this path already exists.
    static bool exists(const TextBuffer &path)
    {
        return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
    }

    // Only .txt is supported. Do not silently change another extension.
    static bool hasTxtExtension(const TextBuffer &filename)
    {
        int length = filename.getLength();

        return length > 4 &&
               filename.substr(length - 4, 4).equals(TextBuffer(".txt"));
    }

    // Rejects paths and Windows-invalid filename characters.
    static bool hasInvalidCharacters(const TextBuffer &filename)
    {
        return filename.containsAnyOf("\\/:*?\"<>|");
    }

    // "notes.txt" becomes "TextEditorFiles\\notes.txt".
    static TextBuffer buildPath(const TextBuffer &filename)
    {
        TextBuffer path = folderName();
        path.append(filename);
        return path;
    }

    // Renames a file on disk. After success the old path no longer exists.
    // An existing file at newPath is replaced. Returns false on failure,
    // in which case the old file is left untouched.
    static bool renameFile(const TextBuffer &oldPath, const TextBuffer &newPath)
    {
        return MoveFileExA(oldPath.c_str(), newPath.c_str(),
                           MOVEFILE_REPLACE_EXISTING) != 0;
    }

    // Save document as a normal readable text file.
    static bool saveFile(Document &document, const TextBuffer &filename)
    {
        ofstream file(filename.c_str());

        if (!file)
        {
            return false;
        }

        Node *current = document.getLineAt(0);

        while (current != nullptr)
        {
            file.write(current->data.c_str(), current->data.getLength());
            file.put('\n');
            current = current->next;
        }

        file.close();

        if (file.fail())
        {
            return false;
        }

        return true;
    }

    // Open a normal readable text file into the document.
    // On failure it returns false and puts the reason in 'error'.
    static bool openFile(Document &document, const TextBuffer &filename, TextBuffer &error)
    {
        ifstream file(filename.c_str());

        if (!file)
        {
            error = "ERROR: FILE NOT FOUND!";
            return false;
        }

        DynamicArray<TextBuffer> lines;
        TextBuffer line;
        bool lineStarted = false; // true when characters were read after the last '\n'
        char ch;

        while (file.get(ch))
        {
            if (ch == '\n')
            {
                if (!lines.add(line))
                {
                    error = "ERROR: OUT OF MEMORY!";
                    return false;
                }

                line.clear();
                lineStarted = false;
            }
            else
            {
                if (!line.append(ch))
                {
                    error = "ERROR: OUT OF MEMORY!";
                    return false;
                }

                lineStarted = true;
            }
        }

        if (file.bad())
        {
            error = "ERROR: COULD NOT READ FILE!";
            return false;
        }

        file.close();

        // The last line may have no '\n' after it.
        if (lineStarted)
        {
            lines.add(line);
        }

        if (!document.replaceLines(lines))
        {
            error = "ERROR: OUT OF MEMORY!";
            return false;
        }

        return true;
    }
};
