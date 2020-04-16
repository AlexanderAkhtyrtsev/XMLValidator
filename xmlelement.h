#pragma once

#include <fstream>
#include <string>
#include <fstream>
#include <iostream>
#include <regex>

using std::string;
using std::endl;
using std::cerr;
using std::cout;
using std::ifstream;

bool ignore_spaces(ifstream &in);

inline bool pattern_match(string str, string pattern) {
    return std::regex_match(str, std::regex(pattern, std::regex::ECMAScript));
}

class BasicXMLElement
{
public:
    enum ReturnValue {OK, Error, TagEnd};
    BasicXMLElement();
    ~BasicXMLElement();
    virtual ReturnValue read(ifstream &) = 0;
    ReturnValue readAttribute(ifstream &, std::pair<string, string> * = nullptr);
    string tagName() const;
    bool isClosed() const;
    string getLastError() const;
protected:
    string m_tagName, m_lastError;
    bool m_isClosed;
};


class XMLProlog : public BasicXMLElement
{
public:
    XMLProlog();
    ReturnValue read(ifstream &in);
};


class XMLElement : public BasicXMLElement
{
public:
    XMLElement();
    ReturnValue read(ifstream &);
};



class XMLComment : public BasicXMLElement
{
public:
    XMLComment();
    ReturnValue read(ifstream &);
};

