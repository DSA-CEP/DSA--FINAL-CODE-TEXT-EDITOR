#pragma once
#include "common.cpp"

// ================= DYNAMIC ARRAY (our own vector) =================

// A list of items stored in one array that doubles in size when it is full.
//
//   items     -> the array
//   count     -> how many items are used
//   capacity  -> how many items fit before growing
//
// T must have a default constructor and a working operator=.
// Ownership: the array belongs to this object. Copying makes a full copy.

template <class T>
class DynamicArray
{
private:
    T *items;
    int count;
    int capacity;
    T fallback; // returned by at() when the index is invalid (never crashes)

    // Doubles the capacity. Returns false if memory could not be allocated.
    bool grow()
    {
        int newCapacity = 8;

        if (capacity > 0)
        {
            newCapacity = capacity * 2;
        }

        T *newItems = new (nothrow) T[newCapacity];

        if (newItems == nullptr)
        {
            return false;
        }

        for (int i = 0; i < count; i++)
        {
            newItems[i] = items[i];
        }

        delete[] items;
        items = newItems;
        capacity = newCapacity;
        return true;
    }

public:
    DynamicArray()
    {
        items = nullptr;
        count = 0;
        capacity = 0;
    }

    // Copy constructor: new array, every item copied.
    DynamicArray(const DynamicArray &other)
    {
        items = nullptr;
        count = 0;
        capacity = 0;
        *this = other;
    }

    DynamicArray &operator=(const DynamicArray &other)
    {
        if (this == &other)
        {
            return *this;
        }

        // Allocate the new array first. If that fails, we keep our old data.
        int newCapacity = other.count;
        if (newCapacity < 1)
            newCapacity = 1;

        T *newItems = new (nothrow) T[newCapacity];

        if (newItems == nullptr)
        {
            return *this;
        }

        for (int i = 0; i < other.count; i++)
        {
            newItems[i] = other.items[i];
        }

        delete[] items;
        items = newItems;
        capacity = newCapacity;
        count = other.count;
        return *this;
    }

    ~DynamicArray()
    {
        delete[] items;
    }

    // Adds one item at the end. Returns false if memory ran out.
    bool add(const T &value)
    {
        if (count == capacity && !grow())
        {
            return false;
        }

        items[count] = value;
        count++;
        return true;
    }

    int size() const
    {
        return count;
    }

    bool isEmpty() const
    {
        return count == 0;
    }

    // Item at 'index'. For an invalid index a harmless dummy item is returned.
    // GITHUB DESCCRIPTION 350/350 CHAR
    T &at(int index)
    {
        if (index < 0 || index >= count)
        {
            return fallback;
        }

        return items[index];
    }

    const T &at(int index) const
    {
        if (index < 0 || index >= count)
        {
            return fallback;
        }

        return items[index];
    }

    // Forgets the last item (the slot is reused by the next add).
    void removeLast()
    {
        if (count > 0)
        {
            count--;
        }
    }

    // Forgets all items. The array memory is kept for reuse.
    void clear()
    {
        count = 0;
    }
};
