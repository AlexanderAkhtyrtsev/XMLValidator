#include "xmlelement.h"
#include <stack>
#include <iostream>
#include <map>

BasicXMLElement::BasicXMLElement()
{
    m_tagName = "";
    lastError = "";
    closed = false;
}

BasicXMLElement::~BasicXMLElement()
{}


string BasicXMLElement::tagName() const
{
    return m_tagName;
}


bool BasicXMLElement::isClosed() const
{
    return closed;
}

string BasicXMLElement::getLastError() const
{
    return lastError;
}

BasicXMLElement::ReturnValue BasicXMLElement::readAttribute(std::ifstream &in, std::pair<string, string> *p_attribute)
{

    string buf = "", key = "", value = "";
    bool part = false;
    char ch, quot; // in.get();

    ignore_spaces(in);
    bool quot_opened = false;
    while (!in.eof())
    {
        ch = in.get();

        if (!quot_opened && ch == '>')
        {
            return BasicXMLElement::TagEnd;
        }

        if (!part && ch == '=')
        {
            part = true;
            quot = in.get();
            key = buf;
            buf = "";
            quot_opened = true;
        }
        else if (part){
            if ( ch == quot ){
                value = buf.substr(1, buf.size());

                if (p_attribute != nullptr)
                    (*p_attribute) = std::make_pair(key, value);

                break;
            }
        }
        buf += ch;
    }

    // CHECKING FOR ERRORS


    /* According to technical specification:
        Symbols [<, >, &, ', "] if are used as part of element's/attribute's
        value must be encoded: [&lt;, &gt;, &amp;, &apos;, &quot;]

        Despite the fact that symbol ">" is allowed in XML attributes but its better to replace by "&gt;",
        error near unencoded symbol ">" will be generated.
    */
    //                         [\\w\\W]*[\\\"<'][\\w\\W]*
    if ( pattern_match(value, "[\\w\\W]*[\\\"<>'][\\w\\W]*") ||
         pattern_match(value, "[\\w\\W]*&(?!(gt|lt|apos|quot|amp);)[\\w\\W]*") )
    {
        this->lastError = "unencoded symbol: key: '" + key + "' in value: '" + value + "'";
        return BasicXMLElement::Error;
    }

    /* According to XML rules
    else if (pattern_match(value, "[\\w\\W]*>[\\w\\W]*"))
        std::cerr << "Warning: unencoded symbol '>' found in attribute '" << key << "' tag: '" << tagName() << "'\n";

    */

    // attribute name validation
    // according to tech specification:
    // 9. Element's/attribute's (as well as namespace's) name must folow the rules of C++ identifier naming.
    // (despite the XML rules)

    if ( !pattern_match(key, "[a-zA-Z][0-9a-zA-Z_]*(\\:?[a-zA-Z_]+[0-9a-zA-Z_]*)?|[a-zA-Z]")  )
    {
        this->lastError = "invalid key: '" + key + "'";
        return BasicXMLElement::Error;
    }

    return BasicXMLElement::OK;
}


// XML Prolog class

XMLProlog::XMLProlog()
{}

XMLProlog::~XMLProlog()
{}

XMLProlog::ReturnValue XMLProlog::read(std::ifstream &in)
{
    in >> this->m_tagName;

    // tag name validation
    if ( !pattern_match(this->m_tagName, "<\\?xml") || pattern_match(this->m_tagName, "<\\?xml\\?>")) {
        this->lastError = "E: invalid XML prolog: '" + this->m_tagName + "'";
        return XMLProlog::Error;
    }
    XMLProlog::ReturnValue res;
    std::pair<string, string> attribute;
    std::map<string, string> attributes;
    while ( !in.eof() ) {
        if ( (res = this->readAttribute(in, &attribute)) == XMLProlog::TagEnd )
        {
            in.seekg(-2, std::ios::cur);

            if (((char)in.get()) == '?')
                this->closed = true;

            in.seekg(1, std::ios::cur);
            break;
        }
        else if (res == XMLProlog::Error) return res;

        if ( attributes.find( attribute.first ) !=  attributes.end() )
        {
            this->lastError = "found dublicate-key in prolog: '"  + attribute.first + "';\n";
            return Error;
        }

        if (attribute.first == "version")
        {
            if (attribute.second != "1.0") {
                this->lastError = "unknown XML version: '"  + attribute.first + "';\n";
                return Error;
            }
        } else if (attribute.first == "encoding")
        {
            if (attribute.second != "utf-8") {
                this->lastError = "unknown XML encoding: '"  + attribute.first + "';\n";
                return Error;
            }
        } else {
            this->lastError = "unknown XML Prolog attribute: '"  + attribute.first + "';\n";
            return Error;
        }

        attributes.insert(attribute);
    }

    return XMLProlog::OK;
}

// XML Element class
XMLElement::XMLElement()
{}

XMLElement::ReturnValue XMLElement::read(std::ifstream &in)
{
    char ch;
    string tag = "", xmlns = "";
    bool readNamespace = false;

    ignore_spaces(in);
    while ( !in.eof() )
    {
        // read while not '>' or space
        if ( pattern_match(string ("") + (ch = in.get()), "[^>\\s]" ) )
        {
            if (ch == ':')
            {
                if (readNamespace)
                {
                    this->lastError = "invalid namespace name near: '" + tag+xmlns + "'\n";
                    return XMLElement::Error;
                    break;
                }
                else {
                    readNamespace = true;
                    continue;
                }
            }
            if (readNamespace) xmlns += ch;
            else tag += ch;
        }
        else {
            in.seekg(-1, std::ios::cur);
            break;
        }
    }

    this->m_tagName = tag+(xmlns == "" ? "" : ":"+xmlns);

    // tag name validation
    // according to tech specification:
    // 9. Element's/attribute's (as well as namespace's) name must folow the rules of C++ identifier naming.
    // (despite the XML rules)

    if ( !pattern_match(tag, "<[a-zA-Z_][0-9A-Za-z_]+|<[a-zA-Z_]") ) {
       this->lastError = "invalid tag name: '" + this->m_tagName + "'\n";
        return XMLElement::Error;
    }

    if (xmlns != "" && !pattern_match(xmlns, "[a-zA-Z_][a-zA-Z0-9_]*"))
    {
        this->lastError = "invalid namespace: '" + xmlns + "'\nname must folow the rules of C++ identifier naming.\n";
        return XMLElement::Error;
    }

    ReturnValue res;
    std::pair<string, string> attribute;
    std::map<string, string> attributes;

    while ( !in.eof() ) {
        if ((res = (*this).readAttribute(in, &attribute)) == XMLElement::TagEnd)
        {
            in.seekg(-2, std::ios::cur);
            if (((char)in.get()) == '/') // if self-close tag
            {
                this->closed = true; // tag closed
            }
            in.get(); // read byte
            break;
        }
        else if (res == XMLElement::Error){
            return res;
        }

        // check for key dublicates
        if ( attributes.find( attribute.first ) !=  attributes.end() )
        {
            this->lastError = "found dublicate-key in tag: '" + this->m_tagName + "'; key: '" + attribute.first + "' already exists;\n";
            return Error;
        }

        attributes.insert(attribute);
    }
    return XMLElement::OK;
}



XMLComment::XMLComment()
{}

XMLComment::ReturnValue XMLComment::read(std::ifstream &in)
{
    ignore_spaces(in);
    string buf = "";
    for(unsigned i=0; i<4; i++)
    {
        buf += (char) in.get();
    }

    if ( !pattern_match(buf, "<!\\-\\-") )
    {
        this->lastError = "Error in XML-comment: '" + buf + "'\n";
        return XMLComment::Error;
    }

    buf = "";
    while (!in.eof())
    {
        buf += (char) in.get();
        if ( pattern_match(buf, "[\\w\\W]*\\-\\->") ) {
            closed = true;
            break;
        }
    }

    if (!this->isClosed())
    {
        this->lastError = "Error in XML-comment: comment not completed.\n";
        return XMLComment::Error;
    }

    if (pattern_match(buf, "[\\w\\W]*\\-\\-[^>][\\w\\W]*"))
    {
        this->lastError = "Error in XML-comment: double dash:\n" + buf + "\n";
        return XMLComment::Error;
    }

    return XMLComment::OK;
}



// functions-helpers

bool ignore_spaces(std::ifstream &in)
{
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
