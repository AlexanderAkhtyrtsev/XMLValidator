#pragma once
#include "xmlelement.h"


class XMLDocument
{
    std::ifstream fileStream;
public:
    XMLDocument(std::string file);
    ~XMLDocument();
    void validate();
};

