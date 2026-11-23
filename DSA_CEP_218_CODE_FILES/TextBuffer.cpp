#pragma once
#include "common.cpp"

// ================= TEXT BUFFER (our own string) =================

// Stores text in a character array that grows when it is full.
//
//   data      -> the characters, always followed by a '\0' (when data != nullptr)
//   length    -> how many characters are used
//   capacity  -> how many characters fit before we must grow
//
// Example: "Hi" with capacity 16 looks like  [H][i][\0][ ][ ]...(16 slots + 1)
//
// Ownership: a TextBuffer owns its array. The destructor frees it.
// Copying makes a NEW array (deep copy), so two buffers never share memory.
class TextBuffer
{
private:
    char *data;
    int length;
    int capacity;

    // Makes sure there is room for 'totalWanted' characters.
    // If the array is too small, a bigger array is allocated (at least double),
    // the old characters are copied over and the old array is freed.
    // Returns false if memory could not be allocated (text stays unchanged).
    bool makeRoom(int totalWanted)
    {
        if (totalWanted <= capacity)
        {
            return true;
        }

        int newCapacity = capacity * 2; // doubling keeps appending fast
        if (newCapacity < totalWanted)
            newCapacity = totalWanted;
        if (newCapacity < 16)
            newCapacity = 16;

        // +1 slot for the final '\0'.
        char *newData = new (nothrow) char[newCapacity + 1];

        if (newData == nullptr)
        {
            return false; // old array is still valid and untouched
        }

        for (int i = 0; i < length; i++)
        {
            newData[i] = data[i];
        }

        newData[length] = '\0';

        delete[] data; // free the OLD array (deleting nullptr is allowed)
        data = newData;
        capacity = newCapacity;
        return true;
    }

    // Changes the length and keeps the '\0' at the end.
    // data is only nullptr while the buffer has never held any text (length 0).
    void setLength(int newLength)
    {
        length = newLength;

        if (data != nullptr)
        {
            data[length] = '\0';
        }
    }

public:
    TextBuffer()
    {
        data = nullptr; // no memory is allocated for empty text
        length = 0;
        capacity = 0;
    }

    // Lets us write TextBuffer name = "hello";
    TextBuffer(const char *text)
    {
        data = nullptr;
        length = 0;
        capacity = 0;
        append(text);
    }

    // 'count' copies of one character, e.g. TextBuffer(5, '=') is "=====".
    TextBuffer(int count, char ch)
    {
        data = nullptr;
        length = 0;
        capacity = 0;
        appendRepeated(ch, count);
    }

    // Copy constructor: makes a NEW array with the same characters.
    TextBuffer(const TextBuffer &other)
    {
        data = nullptr;
        length = 0;
        capacity = 0;
        *this = other;
    }

    // Assignment: copy the characters of 'other' into our own array.
    TextBuffer &operator=(const TextBuffer &other)
    {
        if (this == &other)
        {
            return *this; // a = a does nothing
        }

        if (makeRoom(other.length))
        {
            for (int i = 0; i < other.length; i++)
            {
                data[i] = other.data[i];
            }

            setLength(other.length);
        }

        return *this;
    }

    ~TextBuffer()
    {
        delete[] data; // give the array back
    }

    int getLength() const
    {
        return length;
    }

    bool isEmpty() const
    {
        return length == 0;
    }

    // Returns the character at 'index', or '\0' if the index is invalid.
    char charAt(int index) const
    {
        if (index < 0 || index >= length)
        {
            return '\0';
        }

        return data[index];
    }

    // Changes one character. Returns false if the index is invalid.
    bool setChar(int index, char ch)
    {
        if (index < 0 || index >= length)
        {
            return false;
        }

        data[index] = ch;
        return true;
    }

    // Text as a normal C string (for cout and Windows functions).
    // Do not keep this pointer after the buffer changes.
    const char *c_str() const
    {
        if (data == nullptr)
        {
            return "";
        }

        return data;
    }

    void clear()
    {
        setLength(0); // memory is kept, so typing again is fast
    }

    // Cuts the text so only the first 'newLength' characters remain.

    void truncate(int newLength)
    {
        if (newLength >= 0 && newLength < length)
        {
            setLength(newLength);
        }
    }

    // Adds one character at the end.
    bool append(char ch)
    {
        if (!makeRoom(length + 1))
        {
            return false;
        }

        data[length] = ch;
        setLength(length + 1);
        return true;
    }

    // Adds a C string at the end (do not pass this buffer's own c_str()).
    bool append(const char *text)
    {
        if (text == nullptr)
        {
            return true;
        }

        int count = 0;
        while (text[count] != '\0')
        {
            count++;
        }

        if (!makeRoom(length + count))
        {
            return false;
        }

        for (int i = 0; i < count; i++)
        {
            data[length + i] = text[i];
        }

        setLength(length + count);
        return true;
    }

    // Adds another TextBuffer at the end.
    bool append(const TextBuffer &other)
    {
        if (&other == this)
        {
            // Appending to itself:copy first, because growing moves our array.
            TextBuffer copy(other);
            return append(copy);
        }

        if (!makeRoom(length + other.length))
        {
            return false;
        }

        for (int i = 0; i < other.length; i++)
        {
            data[length + i] = other.data[i];
        }

        setLength(length + other.length);
        return true;
    }

    // Adds the same character 'count' times.
    bool appendRepeated(char ch, int count)
    {
        if (count <= 0)
        {
            return true;
        }

        if (!makeRoom(length + count))
        {
            return false;
        }

        for (int i = 0; i < count; i++)
        {
            data[length + i] = ch;
        }

        setLength(length + count);
        return true;
    }

    // Adds a whole number as digits, e.g. 205 adds "205".
    bool appendNumber(int number)
    {
        long long value = number; // long long so that the biggest negative int works

        if (value < 0)
        {
            if (!append('-'))
                return false;
            value = -value;
        }

        char digits[24];
        int count = 0;

        if (value == 0)
        {
            digits[count++] = '0';
        }

        while (value > 0)
        {
            digits[count++] = (char)('0' + (value % 10)); // last digit first
            value = value / 10;
        }

        while (count > 0) // so write them backwards
        {
            if (!append(digits[--count]))
                return false;
        }

        return true;
    }

    // Inserts one character so that it ends up at index 'position'.
    // All characters from 'position' on move one step to the right.
    // Valid positions: 0 (start) up to length (end).
    bool insertChar(int position, char ch)
    {
        if (position < 0 || position > length)
        {
            return false;
        }

        if (!makeRoom(length + 1))
        {
            return false;
        }

        // Move from the back so nothing is overwritten.
        for (int i = length; i > position; i--)
        {
            data[i] = data[i - 1];
        }

        data[position] = ch;
        setLength(length + 1);
        return true;
    }

    // Removes 'count' characters starting at 'position'.
    // The characters after them move to the left.
    void eraseRange(int position, int count)
    {
        if (position < 0 || position >= length || count <= 0)
        {
            return;
        }

        if (count > length - position)
        {
            count = length - position; // never go past the end
        }

        for (int i = position; i + count < length; i++)
        {
            data[i] = data[i + count];
        }

        setLength(length - count);
    }

    // Returns a NEW buffer with 'count' characters starting at 'from'.
    // Out ofrange values are cut to the valid range, so it never fails.
    TextBuffer substr(int from, int count) const
    {
        TextBuffer result;

        if (from < 0)
            from = 0;
        if (from > length)
            from = length;
        if (count > length - from)
            count = length - from;

        if (count > 0 && result.makeRoom(count))
        {
            for (int i = 0; i < count; i++)
            {
                result.data[i] = data[from + i];
            }

            result.setLength(count);
        }

        return result;
    }

    // Same characters in the same order?
    bool equals(const TextBuffer &other) const
    {
        if (length != other.length)
        {
            return false;
        }

        for (int i = 0; i < length; i++)
        {
            if (data[i] != other.data[i])
            {
                return false;
            }
        }

        return true;
    }

    // True if the text contains at least one character from 'characters'.
    bool containsAnyOf(const char *characters) const
    {
        for (int i = 0; i < length; i++)
        {
            for (int j = 0; characters[j] != '\0'; j++)
            {
                if (data[i] == characters[j])
                {
                    return true;
                }
            }
        }

        return false;
    }
};
