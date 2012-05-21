/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This Original Code has been modified by IBM Corporation.
 * Modifications made by IBM described herein are
 * Copyright (c) International Business Machines
 * Corporation, 1999
 *
 * Modifications to Mozilla code or documentation
 * identified per MPL Section 3.3
 *
 * Date         Modified by     Description of modification
 * 12/09/1999   IBM Corp.       Support for IBM codepages - 850,852,55,857,862,864
 * 06/24/2003   IBM Corp.       Support for IBM codepage 1046
 *
 */

#ifndef nsUCvIBMCID_h___
#define nsUCvIBMCID_h___

#include "nsISupports.h"


// Class ID for our CP850ToUnicode charset converter
// {2D524FD0-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP850TOUNICODE_CID \
  { 0x2d524fd0, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our CP852ToUnicode charset converter
// {2D524FD1-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP852TOUNICODE_CID \
  { 0x2d524fd1, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our CP855ToUnicode charset converter
// {2D524FD2-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP855TOUNICODE_CID \
  { 0x2d524fd2, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our CP857ToUnicode charset converter
// {2D524FD3-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP857TOUNICODE_CID \
  { 0x2d524fd3, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our CP862ToUnicode charset converter
// {2D524FD4-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP862TOUNICODE_CID \
  { 0x2d524fd4, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our CP864ToUnicode charset converter
// {2D524FD5-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP864TOUNICODE_CID \
  { 0x2d524fd5, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP850 charset converter
// {2D524FD6-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP850_CID \
  { 0x2d524fd6, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP852 charset converter
// {2D524FD7-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP852_CID \
  { 0x2d524fd7, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP855 charset converter
// {2D524FD8-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP855_CID \
  { 0x2d524fd8, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP857 charset converter
// {2D524FD9-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP857_CID \
  { 0x2d524fd9, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP862 charset converter
// {2D524FDA-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP862_CID \
  { 0x2d524fda, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP864 charset converter
// {2D524FDB-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP864_CID \
  { 0x2d524fdb, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}
  
// Class ID for our CP864iToUnicode charset converter
// {2D524FDC-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP864ITOUNICODE_CID \
  { 0x2d524fdc, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP864i charset converter
// {2D524FDD-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP864I_CID \
  { 0x2d524fdd, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our CP869ToUnicode charset converter
// {2D524FE0-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP869TOUNICODE_CID \
  { 0x2d524fe0, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP869 charset converter
// {2D524FE1-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP869_CID \
  { 0x2d524fe1, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

  // Class ID for our CP125ToUnicode charset converter
// {2D524FE2-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP1125TOUNICODE_CID \
  { 0x2d524fe2, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP1125 charset converter
// {2D524FE3-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP1125_CID \
  { 0x2d524fe3, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

  // Class ID for our CP11311ToUnicode charset converter
// {2D524FE4-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_CP1131TOUNICODE_CID \
  { 0x2d524fe4, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

// Class ID for our UnicodeToCP1131 charset converter
// {2D524FE5-AE74-11d3-ABF7-0004ACEEFA51}
#define NS_UNICODETOCP1131_CID \
  { 0x2d524fe5, 0xae74, 0x11d3, {0xab, 0xf7, 0x0, 0x4, 0xac, 0xee, 0xfa, 0x51}}

#endif /* nsUCvIBMCID_h___ */
