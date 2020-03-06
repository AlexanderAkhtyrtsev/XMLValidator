#include "xmldocument.h"


int main()
{

    XMLDocument xmlDocument("/home/seven/sample.xml");
    xmlDocument.validate();

    return 0;
}
