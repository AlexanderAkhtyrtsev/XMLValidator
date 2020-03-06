#include "xmldocument.h"
#include <stack>

XMLDocument::XMLDocument(string file)
{
    fileStream.open(file);
}

XMLDocument::~XMLDocument()
{
    fileStream.close();
}

void XMLDocument::validate()
{
    if (!fileStream.is_open())
    {
        std::cerr << "Error: could not open file.\n";
        return;
    }

    ignore_spaces(fileStream);

    std::stack<string> elementsStack;

    bool prologFound        = false,
         rootElementClosed  = false,
         elementFound       = false;

    while (!fileStream.eof())
    {
        // if EOF
        if ( ignore_spaces(fileStream) ) break;
        string buf = "";
        for(unsigned i=0; i<2; i++)
        {
            buf += (char) fileStream.get();
        }
        fileStream.seekg(-2, std::ios::cur);

        // if header like xml-comment
        if (buf == "<!")
        {
            XMLComment comment;
            if ( comment.read(fileStream) )
            {
                std::cerr << "Error while reading XML-comment:\n" << comment.getLastError() << "\n";
                break;
            }
            // according to specification: prolog must come first in document.
            if (!prologFound && !elementFound) elementFound = true;
        }
        // if header like xml-prolog
        else if (buf == "<?")
        {
            if (!prologFound){
                if (elementFound){
                    std::cerr << "Error: prolog must come first in the document.\n";
                    break;
                }
                XMLProlog prolog;
                if (prolog.read(fileStream))
                {
                    std::cerr << "Error while reading XML-prolog:\n" << prolog.getLastError() << "\n";
                    break;
                }
                prologFound = true;
            }
            else {
                std::cerr << "Error: only one prolog in document allowed.\n";
                break;
            }
        }

        // if header like xml-element
        else if ( pattern_match(buf, "<[a-zA-Z_0-9]") )
        {
            XMLElement element;
            if (!element.read(fileStream))  // if no errors
            {
                if (!element.isClosed()){
                    if (elementsStack.size() == 0 && rootElementClosed) {
                        std::cerr << "Error: only one Root element in document is allowed\n\tbut found: '"
                                  << element.tagName() << "'\n";
                        break;
                    }
                    elementsStack.push( element.tagName().substr(1, element.tagName().size()) );
                }
            } else {
                std::cerr << "Error while reading element '" << element.tagName() << "':\n"
                          << element.getLastError() << "\n";
                break;
            }
            // according to specification: prolog must come first in document.
            if (!prologFound && !elementFound) elementFound = true;
        }

        else if (buf == "</")
        {
            char ch;
            string tag = "";
            while ( (ch = fileStream.get()) != '>' )
            {
                tag += ch;
            }
            if (tag == "</"+elementsStack.top())
            {
                elementsStack.pop();
                if (!rootElementClosed) rootElementClosed = true;
            } else {
                std::cerr << "Error: invalid tag: '" << tag << "', expected '" << elementsStack.top() << "';\n";
                break;
            }
        }

        else {
            string text = "";
            while ( !fileStream.eof() )
            {
                text += (char) fileStream.get();
                if ( pattern_match(text, "[\\w\\W]*<\\/[a-zA-Z0-9_\\-\\:\\.]") )
                {
                    do {
                        fileStream.seekg(-1, std::ios::cur);
                    }
                    while ( fileStream.peek() != '<' );


                    text = text.substr(0, text.size()-3);
                    if ( pattern_match(text, "[\\w\\W]*[\\\"<>'][\\w\\W]*") ||
                         pattern_match(text, "[\\w\\W]*&(?!(gt|lt|apos|quot|amp);)[\\w\\W]*") )
                    {
                        std::cerr << "Error: unencoded character in value: '" << text <<
                                     "'\n in element '" << elementsStack.top() << "'" << std::endl;
                    } /*else if (pattern_match(text, "[\\w\\W]*>[\\w\\W]*")) // warn if found '>' symbol in value
                    {
                        // Symbol ">" is allowed it elements' value but its better to replace by &gt;
                        std::cerr << "Warning: '>' symbol found in value '" << text << "' tag: '" << elementsStack.top() << "'\n";
                    }*/
                    break;

                }
            }
            if (fileStream.eof()){
                std::cerr << "Unexpected end of file." << "\n";
                break;
            }
        }
    }
}
