#pragma once
#include "common.cpp"
#include "DynamicArray.cpp"

// ================= STACK (built on DynamicArray) =================

// Last in, first out. push adds on top, pop removes the top item.
// The top of the stack is the LAST item of the array.
template <class T>
class Stack
{
private:
    DynamicArray<T> items;

public:
    bool push(const T &value)
    {
        return items.add(value);
    }

    // Copies the top item into 'value' and removes it. False if empty.
    bool pop(T &value)
    {
        if (items.isEmpty())
        {
            return false;
        }

        value = items.at(items.size() - 1);
        items.removeLast();
        return true;
    }

    // >>> CHANGE 4 (undo typing): NEW method, lets record() look at the last action.
    // Pointer to the top item (nullptr if empty); nothing is removed.
    T *top()
    {
        if (items.isEmpty())
        {
            return nullptr;
        }

        return &items.at(items.size() - 1);
    }

    bool isEmpty() const
    {
        return items.isEmpty();
    }

    void clear()
    {
        items.clear();
    }
};
