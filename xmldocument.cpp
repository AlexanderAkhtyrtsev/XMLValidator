#include "xmldocument.h"
#include <stack>

XMLDocument::XMLDocument(string file) {
    fileStream.open(file);
}

XMLDocument::~XMLDocument() {
    fileStream.close();
}

int XMLDocument::validate() {

    if (!fileStream.is_open()) {
        cerr << "Error: could not open file.\n";
        return 1;
    }

    // check if file empty
    if (ignore_spaces(fileStream)) { // == EOF
        cerr << "Error: file is empty.\n";
        return 1;
    }

    std::stack<string> elementsStack;

    bool prologFound        = false,
         rootElementClosed  = false,
         elementFound       = false; // if any element before prolog exists

    while (!fileStream.eof()) {
        // if EOF
        if (ignore_spaces(fileStream)) {
            break;
        }

        string buf = "";
        for(unsigned i=0; i<2; i++) {
            buf += (char) fileStream.get();
        }

        fileStream.seekg(-2, std::ios::cur);

        // if header like xml-comment
        if (buf == "<!") {
            XMLComment comment;
            if (comment.read(fileStream)) {
                cerr << "Error while reading XML-comment:\n"
                          << comment.getLastError() << "\n";
                return 1;
            }

            // according to specification: prolog must come first in document.
            if (!prologFound && !elementFound) {
                elementFound = true;
            }
        }

        // if header like xml-prolog
        else if (buf == "<?") {
            if (!prologFound) {
                if (elementFound) {
                    cerr << "Error: prolog must come first in the document.\n";
                    return 1;
                }
                XMLProlog prolog;
                if (prolog.read(fileStream)) {
                    cerr << "Error while reading XML-prolog:\n" <<
                                 prolog.getLastError() << "\n";
                    return 1;
                }
                prologFound = true;
            }
            else {
                cerr << "Error: only one prolog in document allowed.\n";
                return 1;
            }
        }

        // if header like xml-element
        else if (pattern_match(buf, "<[a-zA-Z_0-9]")) {
            XMLElement element;
            if (!element.read(fileStream)) { // if no errors
                if (!element.isClosed()) {
                    if (elementsStack.size() == 0 && rootElementClosed) {
                        cerr << "Error: only one Root element in document is allowed"
                             << "\nbut found: '" << element.tagName() << "'\n";
                        return 1;
                    }
                    elementsStack.push( element.tagName().substr(1, element.tagName().size()) );
                }
            } else {
                cerr << "Error while reading element '" << element.tagName() << "':\n"
                     << element.getLastError() << "\n";
                return 1;
            }
            // according to specification: prolog must come first in document.
            if (!prologFound && !elementFound) {
                elementFound = true;
            }
        }

        else if (buf == "</") {
            char ch;
            string tag = "";
            while ( (ch = fileStream.get()) != '>' ) {
                tag += ch;
            }
            if (tag == "</"+elementsStack.top()) {
                if (elementsStack.size() == 1 && !rootElementClosed) {
                    rootElementClosed = true;
                }
                elementsStack.pop();
            } else {
                cerr << "Error: invalid tag: '" << tag << "', expected '" << elementsStack.top() << "';\n";
                return 1;
            }
        }

        else {
            string text = "";
            if (elementsStack.size() == 0) {
                cerr << "Error: reading value without tag (in the "
                     << (rootElementClosed ? "end" : "begin") << " of the file).\n";
                return 1;
            }
            while ( !fileStream.eof() ) {
                text += (char) fileStream.get();

                if (pattern_match(text, "[\\w\\W]*<[a-zA-Z]")) {
                    cerr << "Error: tag '" << elementsStack.top() << "' is not closed.\n";
                    return 1;
                }

                if ( pattern_match(text, "[\\w\\W]*<\\/[a-zA-Z0-9_]") ) {
                    do {
                        fileStream.seekg(-1, std::ios::cur);
                    } while ( fileStream.peek() != '<' );

                    text = text.substr(0, text.size()-3);

                    if ( pattern_match(text, "[\\w\\W]*[\\\"<>'][\\w\\W]*") ||
                         pattern_match(text, "[\\w\\W]*&(?!(gt|lt|apos|quot|amp);)[\\w\\W]*") )
                    {
                        cerr << "Error: unencoded character in value: '" << text
                             << "'\n in element '" << elementsStack.top() << "'\n";
                    } /*else if (pattern_match(text, "[\\w\\W]*>[\\w\\W]*")) // warn if found '>' symbol in value
                    {
                        // Symbol ">" is allowed it elements' value but its better to replace by &gt;
                        cerr << "Warning: '>' symbol found in value '" << text << "' tag: '" << elementsStack.top() << "'\n";
                    }*/
                    break;

                }
            }
            if (fileStream.eof()){
                cerr << "Unexpected end of file." << "\n";
                return 1;
            }
        }
    }
    return 0;
}
