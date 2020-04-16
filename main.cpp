#include "xmldocument.h"


int main(int argc, char *argv[])
{
    if (argc > 1) {
        bool err = false;
        for(int i=1; i<argc; i++) {
            cout << "File: " << argv[i] << "\n";
            XMLDocument xmlDocument(argv[i]);

            if (xmlDocument.validate()) {
                err = true;
            } else {
                cout << "OK.\n";
            }

            cout << "--------\n";
        }

        // Even if one xml-file is invalid returned 1 exit code.
        if (err) {
            return 1;
        }
    } else {
        cerr << "No files to open.\n";
    }

    return 0;
}
