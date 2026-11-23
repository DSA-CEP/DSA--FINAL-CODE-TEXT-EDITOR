#pragma once
#include "common.cpp"
#include "TextBuffer.cpp"

// Each node stores one line.
class Node
{
public:
    TextBuffer data; // the characters of this line (no '\n' inside)
    Node *next;
    Node *prev;

    Node(const TextBuffer &text)
    {
        data = text; // copies the characters
        next = nullptr;
        prev = nullptr;
    }
};
