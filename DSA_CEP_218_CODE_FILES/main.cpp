#include "TextEditor.cpp"

// ================= MAIN MENU =================

int main()
{
    // Create the document folder when the program starts.
    FileManager::createDocumentFolder();

    bool applicationRunning = true;

    while (applicationRunning)
    {
        system("cls");

        cout << "========== TEXT EDITOR ==========" << endl;
        cout << "1. Create New Document" << endl;
        cout << "2. Open Existing Document" << endl;
        cout << "3. Exit" << endl;
        cout << "=================================" << endl;
        cout << "Enter your choice: ";

        TextBuffer choice;

        // Input ended (for example Ctrl+Z on the menu): leave the program.
        if (!readLine(choice))
        {
            break;
        }

        if (choice.equals("1"))
        {
            TextEditor editor;

            editor.run();
        }
        else if (choice.equals("2"))
        {
            TextEditor editor;

            if (editor.openExistingDocument())
            {
                editor.run();
            }
        }
        else if (choice.equals("3"))
        {
            applicationRunning = false;
        }
        else
        {
            EditorDisplay::showMessage("INVALID MENU CHOICE!");
        }
    }

    system("cls");
    cout << "Thank you for using the Text Editor!" << endl;

    return 0;
}
