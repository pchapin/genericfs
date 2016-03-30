/****************************************************************************
FILE   : file.c
SUBJECT: File operation methods for the generic file system.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin

This file contains the file operation methods for the generic file system.

Please send comments or bug reports to

     Peter C. Chapin
     Computer Information Systems
     Vermont Technical College
     Randolph Center, VT 05061
     PChapin@vtc.vsc.edu
****************************************************************************/

#include <generated/autoconf.h>

// Pretty much required for a kernel module.
#define __NO_VERSION__
#include <linux/kernel.h>
#include <linux/module.h>

// Need for general file system stuff.
#include <linux/fs.h>
#include <linux/slab.h>

// Project specific.
#include "global.h"
#include "file.h"

// +++++
// File methods
// +++++
