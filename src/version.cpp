// Copyright (c) 2012 The Bitcoin developers
// Copyright (c) 2015-2017 The Auroracoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "version.h"

#include <string>

// Name of client reported in the 'version' message. Report the same name
// for both bitcoind and bitcoin-qt, to make it harder for attackers to
// target servers or GUI users specifically.
const std::string CLIENT_NAME("Satoshi");

#define BUILD_DATE __DATE__ ", " __TIME__

// First, include build.h if requested
#ifdef HAVE_BUILD_INFO
#    include "build.h"
#endif

#define BUILD_DESC_FROM_RELEASE(maj,min,rev,build) \
    "v" DO_STRINGIZE(maj) "." DO_STRINGIZE(min) "." DO_STRINGIZE(rev) "." DO_STRINGIZE(build)

#ifndef BUILD_DESC
#   define BUILD_DESC BUILD_DESC_FROM_RELEASE(CLIENT_VERSION_MAJOR, CLIENT_VERSION_MINOR, CLIENT_VERSION_REVISION, CLIENT_VERSION_BUILD)
#endif

const std::string CLIENT_BUILD(BUILD_DESC);
const std::string CLIENT_DATE(BUILD_DATE);
