/*
Copyright 2010-2011 Ed Bow <edxbow@gmail.com>

This file is part of Quake Live - Demo Tools (QLDT).

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef DTHUFFMAN_H
#define DTHUFFMAN_H

#define NYT HMAX					/* NYT = Not Yet Transmitted */
#define INTERNAL_NODE (HMAX+1)

struct node_t {
    node_t* left;
    node_t* right;
    node_t* parent;   /* tree structure */
    node_t* next;
    node_t* prev;     /* doubly-linked list */
    node_t** head;    /* highest ranked node in block */
    int		weight;
    int		symbol;
};

#define HMAX 256 /* Maximum symbol */

struct huff_t {
    int			blocNode;
    int			blocPtrs;

    node_t*		tree;
    node_t*		lhead;
    node_t*		ltail;
    node_t*		loc[ HMAX + 1 ];
    node_t**	freelist;

    node_t		nodeList[ 514 ];
    node_t*		nodePtrs[ 514 ];
};

typedef unsigned char byte;

class DtHuffman {
public:
    DtHuffman();

    void offsetReceive( node_t* node, int* ch, byte* fin, int* offset );
    void offsetTransmit( huff_t* huff, int ch, byte* fout, int* offset );
    void putBit( int bit, byte* fout, int* offset );
    int getBit( byte* fout, int* offset );

private:
    int	bloc;

    void add_bit( char bit, byte* fout );
    int get_bit( byte* fin );
    void send( node_t* node, node_t* child, byte* fout );
};

#endif // DTHUFFMAN_H
