// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "version.h"

#include <string>

// Name of client reported in the 'version' message. Report the same name
// for both bitcoind and bitcoin-qt, to make it harder for attackers to
// target servers or GUI users specifically.
const std::string CLIENT_NAME("Guðbrandur Þorláksson");

//Removed the logic of the version naming to substitute with the following
// CLIENT_VERSION_SUFFIX will be used to define test builds or similar.  Otherwise, the build is named
// with BUILD_DESC_FROM_RELEASE, which will append "-release" to the version.
// Instead of setting specific date, which was previously something in 2014, the date & time will be
// determined at the time of the build.

#define CLIENT_VERSION_SUFFIX   ""
#define BUILD_DATE __DATE__ ", " __TIME__

// First, include build.h if requested
#ifdef HAVE_BUILD_INFO
#    include "build.h"
#endif

#define BUILD_DESC_WITH_SUFFIX(maj,min,rev,build,suffix) \
    "v" DO_STRINGIZE(maj) "." DO_STRINGIZE(min) "." DO_STRINGIZE(rev) "." DO_STRINGIZE(build) "-" DO_STRINGIZE(suffix)

#define BUILD_DESC_FROM_RELEASE(maj,min,rev,build) \
    "v" DO_STRINGIZE(maj) "." DO_STRINGIZE(min) "." DO_STRINGIZE(rev) "." DO_STRINGIZE(build) "-release"

#ifndef BUILD_DESC
#    ifdef BUILD_SUFFIX
#        define BUILD_DESC BUILD_DESC_WITH_SUFFIX(CLIENT_VERSION_MAJOR, CLIENT_VERSION_MINOR, CLIENT_VERSION_REVISION, CLIENT_VERSION_BUILD, BUILD_SUFFIX)
#    else
#        define BUILD_DESC BUILD_DESC_FROM_RELEASE(CLIENT_VERSION_MAJOR, CLIENT_VERSION_MINOR, CLIENT_VERSION_REVISION, CLIENT_VERSION_BUILD)
#    endif
#endif

const std::string CLIENT_BUILD(BUILD_DESC CLIENT_VERSION_SUFFIX);
const std::string CLIENT_DATE(BUILD_DATE);
