/*!
 * \file file.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief This file contains the file operations methods for GenericFS.
 */

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
