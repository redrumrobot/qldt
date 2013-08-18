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

#include "Huffman.h"

DtHuffman::DtHuffman() {
    bloc = 0;
}

void DtHuffman::putBit( int bit, byte* fout, int* offset ) {
    bloc = *offset;
    if ( ( bloc & 7 ) == 0 ) {
        fout[ ( bloc >> 3 ) ] = 0;
    }
    fout[ ( bloc >> 3 ) ] |= bit << ( bloc & 7 );
    bloc++;
    *offset = bloc;
}

int DtHuffman::getBit( byte* fin, int* offset ) {
    int t;
    bloc = *offset;
    t = ( fin[ ( bloc >> 3 ) ] >> ( bloc & 7 ) ) & 0x1;
    bloc++;
    *offset = bloc;
    return t;
}

/* Add a bit to the output file (buffered) */
void DtHuffman::add_bit( char bit, byte* fout ) {
    if ( ( bloc & 7 ) == 0 ) {
        fout[ ( bloc >> 3 ) ] = 0;
    }
    fout[ ( bloc >> 3 ) ] |= bit << ( bloc & 7 );
    bloc++;
}

/* Receive one bit from the input file (buffered) */
int DtHuffman::get_bit( byte* fin ) {
    int t;
    t = ( fin[ ( bloc >> 3 ) ] >> ( bloc & 7 ) ) & 0x1;
    bloc++;
    return t;
}

/* Get a symbol */
void DtHuffman::offsetReceive( node_t* node, int* ch, byte* fin, int* offset ) {
    bloc = *offset;

    while ( node && node->symbol == INTERNAL_NODE ) {
        if ( get_bit( fin ) ) {
            node = node->right;
        } else {
            node = node->left;
        }
    }

    if ( !node ) {
        *ch = 0;
        return;
    }

    *ch = node->symbol;
    *offset = bloc;
}

/* Send the prefix code for this node */
void DtHuffman::send( node_t* node, node_t* child, byte* fout ) {
    if ( node->parent ) {
        send( node->parent, node, fout );
    }
    if ( child ) {
        if ( node->right == child ) {
            add_bit( 1, fout );
        } else {
            add_bit( 0, fout );
        }
    }
}

/* Send a symbol */
void DtHuffman::offsetTransmit( huff_t* huff, int ch, byte* fout, int* offset ) {
    bloc = *offset;
    send( huff->loc[ ch ], 0, fout );
    *offset = bloc;
}
