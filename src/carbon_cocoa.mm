/*	Filename: carbon_cocoa.h
	Wrappers for calling Cocoa from Carbon (because Qt is Carbonized)
	Specialized for handling Sparkle interface

	Apple's "Carbon-Cocoa Integration Guide" explains the basics:
	http://developer.apple.com/documentation/Cocoa/Conceptual/CarbonCocoaDoc/index.html#//apple_ref/doc/uid/TP30000893

	Created June 30, 2007	Brandon Fosdick <bfoz@bfoz.net>
*/

#include <AppKit/AppKit.h>
#include <Sparkle/SUUpdater.h>

#include "carbon_cocoa.h"

namespace Cocoa
{
	// Initialize Cocoa and Sparkle
	void initialize()
	{
		NSApplicationLoad();	//Start Coca interface (requires 10.2 or later)
		SUUpdater* updater = [SUUpdater alloc];
		[updater checkForUpdatesInBackground];
	}

	void checkForUpdates()
	{
		SUUpdater* updater = [SUUpdater alloc];
		[updater checkForUpdates:nil];
	}

	QString FSRefToPath(FSRef fsref)
	{
		CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &fsref);
		if( !url )
			return QString();
		NSString* path = (NSString*)CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		[path autorelease];
		CFRelease(url);
		return QString::fromUtf8([path UTF8String]);
	}
}