/*	Filename: carbon_cocoa.h
	Wrappers for calling Cocoa from Carbon (because Qt is Carbonized)
	Specialized for handling Sparkle interface

	Apple's "Carbon-Cocoa Integration Guide" explains the basics:
	http://developer.apple.com/documentation/Cocoa/Conceptual/CarbonCocoaDoc/index.html#//apple_ref/doc/uid/TP30000893

	Created June 30, 2007	Brandon Fosdick <bfoz@bfoz.net>
*/

#ifndef CARBON_COCOAH
#define	CARBON_COCOAH

#include <Carbon/Carbon.h>

#include <QString>

namespace Cocoa
{
	void initialize();
	void checkForUpdates();
	QString FSRefToPath(FSRef);
};

#endif	// CARBON_COCOAH