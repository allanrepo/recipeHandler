#if !defined (XMLINTERFACE_H)
#define XMLINTERFACE_H

// XML parser for Unison panel reading 126603

/*
Copyright (c) 2003 Daniel W. Howard

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the
Software without restriction, including without
limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial portions of
the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <string.h>

/* basicxmlnode: simple xml node memory representation */
struct basicxmlnode
{
  char *tag; /* always non-NULL */
  char *text; /* body + all whitespace, always non-NULL */
  char **attrs; /* array of strings, NULL marks end */
  char **values; /* array of strings, NULL marks end */
  struct basicxmlnode **children; /* similar */
  int *childreni; /* children positions in text */
};

void deletebasicxmlnode( struct basicxmlnode * node );
struct basicxmlnode * readbasicxmlnode( FILE * fpi );


/*
Copyright (c) 2009 Eddie Lee

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the
Software without restriction, including without
limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial portions of
the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL
DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
<myNode id="attribute">value</myNode>
tag          attr       vals
*/

#include <vector>
#include <fstream>

/* Basically a wrapper around basicxmlnode (bxmlnode.h) */
class XML_Node
{
 private:
    std::string tag;
    std::string text;
    // attributes
    std::vector<std::string> attr;
    std::vector<std::string> vals;
    // vector of children..
    std::vector<XML_Node*> children;
    
    // initialize node given a bmxlnode (will also init children..)
    void initNode(basicxmlnode *bxmlnode);
    // check to see if the text is valid (not just blanks spaces)
    bool isValidText( const std::string &str ) const;

 public:
    XML_Node(); // blank node
    XML_Node( const std::string & file_name ); // create node from filename (root node)..
    XML_Node(basicxmlnode *bxmlnode); // create node from basicxmlnode struct..
    ~XML_Node(); // delete node and children..
    
    // tag
    std::string fetchTag() const { return this->tag; }

    // text
    bool hasText() const { return this->text.compare("") != 0; }
    std::string fetchText() const { return this->text; }
    
    // children
    int numChildren() const { return this->children.size(); }
    bool hasChildren() const { return !this->children.empty(); }

    int numAttr() const { return this->attr.size(); }
    int numVals() const { return this->vals.size(); }
    std::string fetchAttr(int index) const { return this->attr[index]; }
    std::string fetchVal(int index) const { return this->vals[index]; }

    
    // fetch children
    bool hasChild(const std::string & tag) const; // has child with tag
    XML_Node * fetchChild( int index ); // fetch child by index (returns null if not found)
    XML_Node * fetchChild( const std::string & tag, int index = 0 );  // returns first instance.. (returns null if not found)
    std::vector<XML_Node*> fetchChildren( const std::string & tag ); // fetch children with tag name..

};

#endif



