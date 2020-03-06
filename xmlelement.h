#pragma once
#include <fstream>
#include <string>
#include <fstream>
#include <iostream>
#include <regex>
//#include <pair>

using std::string;

bool ignore_spaces(std::ifstream &in);

inline bool pattern_match(string str, string pattern)
{
    return std::regex_match(str, std::regex(pattern, std::regex::ECMAScript));
}

class BasicXMLElement
{
public:
    enum ReturnValue {OK, Error, TagEnd};
    BasicXMLElement();
    ~BasicXMLElement();
    virtual ReturnValue read(std::ifstream&) = 0;
    ReturnValue readAttribute(std::ifstream&, std::pair<string, string> * = nullptr);
    string tagName() const;
    bool isClosed() const;
    string getLastError() const;
protected:
    string m_tagName, lastError;
    bool closed;
};


class XMLProlog : public BasicXMLElement
{
public:
    XMLProlog();
    ~XMLProlog();
    ReturnValue read(std::ifstream &in);
};


class XMLElement : public BasicXMLElement
{
public:
    XMLElement();
    ReturnValue read(std::ifstream&);
};



class XMLComment : public BasicXMLElement
{
public:
    XMLComment();
    ReturnValue read(std::ifstream&);
};

