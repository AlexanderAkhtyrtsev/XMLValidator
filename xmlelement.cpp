#include "xmlelement.h"
#include <stack>
#include <iostream>
#include <map>

BasicXMLElement::BasicXMLElement() {
    m_tagName = "";
    m_lastError = "";
    m_isClosed = false;
}

BasicXMLElement::~BasicXMLElement() {}


string BasicXMLElement::tagName() const {
    return m_tagName;
}


bool BasicXMLElement::isClosed() const {
    return m_isClosed;
}

string BasicXMLElement::getLastError() const {
    return m_lastError;
}

BasicXMLElement::ReturnValue BasicXMLElement::readAttribute(ifstream &in,
                                                            std::pair<string, string> *p_attribute) {

    string buf = "", key = "", value = "";
    bool part = false; // key=attribute
    char ch, quot; // in.get();

    if (ignore_spaces(in)) {
        this->m_lastError = "unexpected end of file.\n";
        return BasicXMLElement::Error;
    }

    bool quot_opened = false;
    while (!in.eof()) {
        ch = in.get();

        if (!quot_opened && ch == '>') {
            return BasicXMLElement::TagEnd;
        }

        if (!part && ch == '=') {
            part = true;
            quot = in.get(); // quot is ' or "
            key = buf;
            buf = "";
            quot_opened = true;
        } else if (part){
            if ( ch == quot ) {
                value = buf.substr(1, buf.size());

                if (p_attribute != nullptr) {
                    (*p_attribute) = std::make_pair(key, value);
                }

                break;
            }
        }
        buf += ch;
    }

    // CHECKING FOR ERRORS

    /* According to technical specification:
        Symbols [<, >, &, ', "] if are used as part of element's/attribute's
        value must be encoded: [&lt;, &gt;, &amp;, &apos;, &quot;]

        Despite the fact that symbol ">" is allowed in XML attributes
        but its better to replace by "&gt;",
        error near unencoded symbol ">" will be generated.
    */
    // (this is right regex)   [\\w\\W]*[\\\"<'][\\w\\W]*
    if ( pattern_match(value, "[\\w\\W]*[\\\"<>'][\\w\\W]*") ||
         pattern_match(value, "[\\w\\W]*&(?!(gt|lt|apos|quot|amp);)[\\w\\W]*") )
    {
        this->m_lastError = "unencoded symbol: key: '" + key + "' in value: '" + value + "'";
        return BasicXMLElement::Error;
    }

    // Warnigng about ">"
    /* According to XML rules
    else if (pattern_match(value, "[\\w\\W]*>[\\w\\W]*"))
        std::cerr << "Warning: unencoded symbol '>' found in attribute '"
                  << key << "' tag: '" << tagName() << "'\n";
    */

    // Attribute name validation
    // according to tech specification:
    // 9. Element's/attribute's (as well as namespace's) name must folow
    // the rules of C++ identifier naming.
    // (despite the XML rules)

    if (!pattern_match(key, "[a-zA-Z][0-9a-zA-Z_]*(\\:?[a-zA-Z_]+[0-9a-zA-Z_]*)?|[a-zA-Z]")) {
        this->m_lastError = "invalid key: '" + key +
                "'\n\tName must folow the rules of C++ identifier naming";
        return BasicXMLElement::Error;
    }

    return BasicXMLElement::OK;
}


// XML Prolog class

XMLProlog::XMLProlog() {}

XMLProlog::~XMLProlog() {}

XMLProlog::ReturnValue XMLProlog::read(ifstream &in) {

    in >> this->m_tagName;

    // tag name validation
    if ( !pattern_match(this->m_tagName, "<\\?xml") ||
         pattern_match(this->m_tagName, "<\\?xml\\?>"))
    {
        this->m_lastError = "invalid XML prolog: '" + this->m_tagName + "'";
        return XMLProlog::Error;
    }

    XMLProlog::ReturnValue res;
    std::pair<string, string> attribute;
    std::map<string, string> attributes;

    while ( !in.eof() ) {
        if ((res = this->readAttribute(in, &attribute)) == XMLProlog::TagEnd) {
            in.seekg(-2, std::ios::cur);

            if ( ((char) in.get()) == '?' ) {
                this->m_isClosed = true;
            }

            in.seekg(1, std::ios::cur);
            break;

        } else if (res == XMLProlog::Error) {
            return res;
        }

        // check for dublicates
        if ( attributes.find( attribute.first ) !=  attributes.end() ) {
            this->m_lastError = "found dublicate-key in prolog: '"  + attribute.first + "';\n";
            return Error;
        }

        // Checking XML-prolog attributes according to tech. specification
        if (attribute.first == "version") {
            if (attribute.second != "1.0") {
                this->m_lastError = "unknown XML version: '"  + attribute.first + "';\n";
                return Error;
            }
        } else if (attribute.first == "encoding") {
            if (attribute.second != "utf-8") {
                this->m_lastError = "unknown XML encoding: '" + attribute.first + "';\n";
                return Error;
            }
        } else {
            this->m_lastError = "unknown XML Prolog attribute: '" + attribute.first + "';\n";
            return Error;
        }

        attributes.insert(attribute);
    }

    return XMLProlog::OK;
}

// XML Element class
XMLElement::XMLElement() {}

XMLElement::ReturnValue XMLElement::read(ifstream &in) {
    char ch;
    string tag = "", xmlns = "";
    bool readNamespace = false;

    if (ignore_spaces(in)) {
        this->m_lastError = "unexpected end of file.";
        return XMLElement::Error;
    }
    while ( !in.eof() ) {
        // read while not '>' or space
        if ( pattern_match(string ("") + (ch = in.get()), "[^>\\s]" ) ) {
            if (ch == ':') {
                if (readNamespace) {
                    this->m_lastError = "invalid namespace name near: '" + tag+xmlns + "'\n";
                    return XMLElement::Error;
                } else {
                    readNamespace = true;
                    continue;
                }
            }
            if (readNamespace) {
                xmlns += ch;
            } else {
                tag += ch;
            }
        } else {
            in.seekg(-1, std::ios::cur);
            break;
        }
    }

    this->m_tagName = tag+(xmlns == "" ? "" : ":" + xmlns);

    // tag name validation
    // according to tech specification:
    // 9. Element's/attribute's (as well as namespace's) name must folow
    // the rules of C++ identifier naming.
    // (despite the XML rules)

    if (!pattern_match(tag, "<[a-zA-Z_][0-9A-Za-z_]+|<[a-zA-Z_]")) {
        this->m_lastError = "invalid tag name: '" + this->m_tagName + "'\n";
        return XMLElement::Error;
    }

    if (xmlns != "" && !pattern_match(xmlns, "[a-zA-Z_][a-zA-Z0-9_]*")) {
        this->m_lastError = "invalid namespace: '" + xmlns +
                "'\nname must folow the rules of C++ identifier naming.\n";
        return XMLElement::Error;
    }

    ReturnValue res;
    std::pair<string, string> attribute;
    std::map<string, string> attributes;

    while (!in.eof()) {
        if ((res = this->readAttribute(in, &attribute)) == XMLElement::TagEnd) {

            in.seekg(-2, std::ios::cur);

            if (((char)in.get()) == '/') { // if self-close tag
                this->m_isClosed = true; // tag closed
            }
            in.get();
            break;
        }
        else if (res == XMLElement::Error){
            return res;
        }

        // check for key dublicates
        if ( attributes.find( attribute.first ) !=  attributes.end() )  {
            this->m_lastError = "found dublicate-key in tag: '" + this->m_tagName + "'; key: '" +
                    attribute.first + "' already exists;\n";
            return Error;
        }

        attributes.insert(attribute);
    }
    return XMLElement::OK;
}



XMLComment::XMLComment() {}

XMLComment::ReturnValue XMLComment::read(std::ifstream &in)
{
    if (ignore_spaces(in)) {
        this->m_lastError = "unexpected end of file.";
        return XMLComment::Error;
    }

    string buf = "";
    for(unsigned i=0; i<4; i++) {
        if (in.eof()) {
            this->m_lastError = "unexpected end of file.";
            return XMLComment::Error;
        }
        buf += (char) in.get();
    }

    if (!pattern_match(buf, "<!\\-\\-")) {
        this->m_lastError = "Error in XML-comment: '" + buf + "'\n";
        return XMLComment::Error;
    }

    buf = "";
    while (!in.eof()) {
        buf += (char) in.get();
        if (pattern_match(buf, "[\\w\\W]*\\-\\->")) {
            this->m_isClosed = true;
            break;
        }
    }

    if (!this->isClosed()) {
        this->m_lastError = "Error in XML-comment: comment not completed.\n";
        return XMLComment::Error;
    }

    if (pattern_match(buf, "[\\w\\W]*\\-\\-[^>][\\w\\W]*")) {
        this->m_lastError = "Error in XML-comment: double dash:\n" + buf + "\n";
        return XMLComment::Error;
    }

    return XMLComment::OK;
}



// functions-helpers

bool ignore_spaces(ifstream &in) {
    char ch;
    unsigned i = 0;
    while (!in.eof()){
        i++;
        if(!std::isspace(ch = in.get()))
            break;
    }

    if (in.eof())
        return true;

    else if (i > 0)
        in.seekg(-1, std::ios::cur);
    return false;
}
