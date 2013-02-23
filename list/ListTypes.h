/*****************************************************************************

  ListTypes.h

  Important data types used throughout ListXP

*****************************************************************************/

#ifndef _LISTTYPES_H
#define _LISTTYPES_H


#include <algorithm>
#include <deque>
#include <string>
#include <strstream>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <iterator>
#include <vector>
#include <functional>
#include <algorithm>
#include <deque>
#include <list>


using namespace std;
#include "guard.h"


typedef vector<string> stringvec;


typedef unsigned __int64 uint64;
typedef signed __int64 sint64;
typedef unsigned __int32 uint32;
typedef signed __int32 sint32;
typedef unsigned __int16 uint16;
typedef signed __int16 sint16;
typedef unsigned __int8 uint8;
typedef signed __int8 sint8;


typedef uint16 width_t;


#define LODWORD(ui64) ((uint32)((ui64) & 0xffffffff))
#define HIDWORD(ui64) ((uint32)((ui64) >> 32))



#endif // _LISTTYPES_H