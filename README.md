# XMLValidator
Test task for an interview.

**Requirements for the task:**

A program should be able to validate any XML file structure according to the following rules:

- Open tag is a construct that begins with < and ends with >: <tag>
- Close tag is a construct that begins with </ and ends with >: </tag>
- Any open tag must be closed. A pair of open and close tags is called element: <element></element>
- As a special case, there is a self-close tag, which forms an element itself: <element />
- Element can contain child elements: <element01><element02 /><element03 /></element01>
- Element can contain a value: <element>Some value</element>
- Open tag can contain an attributes (a key/value pairs). Its name must be unique inside an element: <element attribute01="some value" attribute02="123" />
- Element's/attribute's name can include namespace: <nse:element nsa:attribute="" />
- Element's/attribute's (as well as namespace's) name must folow the rules of C++ identifier naming.
- XML document must contain exactly one root element that is the parent of all other elements
- XML document can contain XML prolog (it is optional). If it exists, it must come first in the document. It starts with <? and ands with ?> and can contain attributes: <?xml version="1.0" encoding="UTF-8" ?>
- Symbols [<, >, &, ', "] if are used as part of element's/attribute's value must be encoded: [&lt;, &gt;, &amp;, &apos;, &quot;]
- XML document can contain comments. A comment starts with <!-- and ends with -->: <!-- Some comment -->

Restriction: Please use only C++ Standard Library. Do not use any external library or framework.
