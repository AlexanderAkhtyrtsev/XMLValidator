#pragma once

#include "xmlelement.h"


class XMLDocument
{
private:
    ifstream fileStream;
public:
    XMLDocument(string file);
    ~XMLDocument();
    int validate();
};

