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

#ifndef DTNPFUNCTIONS_H
#define DTNPFUNCTIONS_H

typedef void (*PluginThreadCallback)(void *);

extern "C" {

void pcLog( const char* str ) {
    printf( str );
}

NPObject* _getwindowobject( NPP ) {
    return NULL;
}

NPObject* _getpluginelement(NPP) {
    return NULL;
}

void _getstringidentifiers( const NPUTF8**, int32_t, NPIdentifier* ) {

}

bool _identifierisstring( NPIdentifier ) {
    return false;
}

NPIdentifier _getintidentifier( int32_t ) {
    return 0;
}

int32_t _intfromidentifier( NPIdentifier ) {
    return 0;
}

NPObject* _createobject( NPP, NPClass* ) {
    return NULL;
}

NPObject* _retainobject( NPObject* ) {
    return NULL;
}

bool _invokeDefault( NPP, NPObject*, const NPVariant*, uint32_t, NPVariant* ) {
    return false;
}

bool _evaluate( NPP, NPObject*, NPString*, NPVariant* ) {
    return false;
}

bool _setproperty( NPP, NPObject*, NPIdentifier, const NPVariant* ) {
    return false;
}

bool _removeproperty( NPP, NPObject*, NPIdentifier ) {
    return false;
}

bool _hasproperty( NPP, NPObject*, NPIdentifier ) {
    return false;
}

bool _hasmethod( NPP, NPObject*, NPIdentifier ) {
    return false;
}

bool _enumerate( NPP, NPObject*, NPIdentifier**, uint32_t* ) {
    return false;
}

bool _construct( NPP, NPObject*, const NPVariant*, uint32_t, NPVariant* ) {
    return false;
}

void _releasevariantvalue( NPVariant* ) {

}

void _setexception( NPObject*, const NPUTF8* ) {

}

  ////////////////////////////////////////////////////////////////////////
  // Static stub functions that are exported to the 4.x plugin as entry
  // points via the CALLBACKS variable.
  //

static NPError _requestread( NPStream*, NPByteRange* ) {
    return 1;
}

static NPError _geturlnotify( NPP, const char*, const char*, void* ) {
    return 1;
}

NPObject* NPAllocate( NPP, NPClass* ) {
    return NULL;
}

void NPDeallocate( NPObject* ) {

}

void NPInvalidate( NPObject* ) {

}

bool NPHasMethod( NPObject*, NPIdentifier ) {
    return false;
}

bool NPInvoke(	NPObject*, NPIdentifier, const NPVariant*, uint32_t, NPVariant* ) {
    return false;
}

bool NPInvokeDefault( NPObject*, const NPVariant*, uint32_t, NPVariant* ) {
    return false;
}

bool NPHasProperty( NPObject*, NPIdentifier ) {
    return false;
}

bool NPGetProperty( NPObject*, NPIdentifier, NPVariant* ) {
    return false;
}

bool NPSetProperty( NPObject*, NPIdentifier, const NPVariant* ) {
    return false;
}

bool NPRemoveProperty( NPObject*, NPIdentifier ) {
    return false;
}

bool NPEnumeration( NPObject*, NPIdentifier**, uint32_t* ) {
    return false;
}

bool NPConstruct( NPObject*, const NPVariant*, uint32_t, NPVariant* ) {
    return false;
}

static NPError _setvalue( NPP, NPPVariable, void* ) {
    return 1;
}

static NPError _geturl( NPP, const char*, const char* ) {
    return 1;
}

static NPError _posturlnotify(	NPP, const char*, const char*, uint32, const char*, NPBool, void* ) {
    return 1;
}

static NPError _posturl(	NPP, const char*, const char*, uint32, const char*, NPBool ) {
    return 1;
}

static NPError _newstream( NPP, NPMIMEType, const char*, NPStream** ) {
    return 1;
}

static int32 _write( NPP, NPStream*, int32, void* ) {
    return 0;
}

static NPError _destroystream( NPP, NPStream*, NPError ) {
    return 1;
}

static void _status( NPP, const char* ) {

}

static void _memfree( void* ) {

}

static uint32 _memflush( uint32 ) {
    return 0;
}

static void _reloadplugins( NPBool ) {

}

static void _invalidaterect( NPP, NPRect* ) {

}

static void _invalidateregion( NPP, NPRegion ) {

}

static void _forceredraw( NPP ) {

}

static void _pushpopupsenabledstate( NPP, NPBool ) {

}

static void _poppopupsenabledstate( NPP ) {

}

static void _pluginthreadasynccall( NPP, PluginThreadCallback, void* ) {

}

static const char* _useragent( NPP ) {
    return NULL;
}

static void* _memalloc( uint32 ) {
    return NULL;
}

}

#endif // DTNPFUNCTIONS_H
