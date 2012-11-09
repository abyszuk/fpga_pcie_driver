/*******************************************************************
 * Change History:
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1  2006/10/13 17:18:32  marcus
 * Implemented and tested most of C++ interface.
 *
 *******************************************************************/

#include "Exception.h"

using namespace pciDriver;

const char* Exception::descriptions[] = {
	"Unknown Exception",
	"Device Not Found",
	"Invalid BAR",
	"Internal Error",
	"Open failed",
	"Not Open",
	"Mmap failed",
	"Alloc failed",
	"SGmap failed",
	"Interrupt failed"
};


