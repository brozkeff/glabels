/*
 *  (GLABELS) Label and Business Card Creation program for GNOME
 *
 *  bc-gnubarcode.c:  front-end to GNU-barcode-library module
 *
 *  Copyright (C) 2001-2002  Jim Evins <evins@snaught.com>.
 *
 *  Some of this code is borrowed from the postscript renderer (ps.c)
 *  from the GNU barcode library:
 *
 *     Copyright (C) 1999 Alessaandro Rubini (rubini@gnu.org)
 *     Copyright (C) 1999 Prosa Srl. (prosa@prosa.it)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <config.h>

#include <ctype.h>
#include <string.h>

#include "bc.h"
#include "bc_gnubarcode.h"

#include "barcode.h"

#include "debug.h"

#define SHRINK_AMOUNT 0.15	/* shrink bars to account for ink spreading */
#define FONT_SCALE    0.95	/* Shrink fonts just a hair */

static glBarcode *render_pass1 (struct Barcode_Item *bci,
				gboolean text_flag,
				gdouble scale);

/*****************************************************************************/
/* Generate intermediate representation of barcode.                          */
/*****************************************************************************/
glBarcode *
gl_barcode_gnubarcode (glBarcodeStyle style,
		       gboolean text_flag,
		       gdouble scale,
		       gchar * digits)
{
	glBarcode *gbc;
	struct Barcode_Item *bci;
	gint flags;

	bci = Barcode_Create (digits);

	/* First encode using GNU Barcode library */
	switch (style) {
	case GL_BARCODE_STYLE_EAN:
		flags = BARCODE_EAN;
		break;
	case GL_BARCODE_STYLE_UPC:
		flags = BARCODE_UPC;
		break;
	case GL_BARCODE_STYLE_ISBN:
		flags = BARCODE_ISBN;
		break;
	case GL_BARCODE_STYLE_39:
		flags = BARCODE_39;
		break;
	case GL_BARCODE_STYLE_128:
		flags = BARCODE_128;
		break;
	case GL_BARCODE_STYLE_128C:
		flags = BARCODE_128C;
		break;
	case GL_BARCODE_STYLE_128B:
		flags = BARCODE_128B;
		break;
	case GL_BARCODE_STYLE_I25:
		flags = BARCODE_I25;
		break;
	case GL_BARCODE_STYLE_CBR:
		flags = BARCODE_CBR;
		break;
	case GL_BARCODE_STYLE_MSI:
		flags = BARCODE_MSI;
		break;
	case GL_BARCODE_STYLE_PLS:
		flags = BARCODE_PLS;
		break;
	default:
		WARN( "Illegal barcode style %d", style );
		flags = BARCODE_ANY;
		break;
	}
	Barcode_Encode (bci, flags);
	if (!bci->partial || !bci->textinfo) {
		WARN ("Barcode Data Invalid");
		Barcode_Delete (bci);
		return NULL;
	}

	/* now render with our custom back-end,
	   to create appropriate intermdediate format */
	gbc = render_pass1 (bci, text_flag, scale);

	Barcode_Delete (bci);
	return gbc;
}

/*--------------------------------------------------------------------------
 * PRIVATE.  Render to glBarcode intermediate representation of barcode.
 *
 *  Some of this code is borrowed from the postscript renderer (ps.c)
 *  from the GNU barcode library:
 *
 *     Copyright (C) 1999 Alessaandro Rubini (rubini@gnu.org)
 *     Copyright (C) 1999 Prosa Srl. (prosa@prosa.it)
 *
 *--------------------------------------------------------------------------*/
static glBarcode *
render_pass1 (struct Barcode_Item *bci,
	      gboolean text_flag,
	      gdouble scale)
{
	glBarcode *gbc;
	glBarcodeLine *line;
	glBarcodeChar *bchar;
	gdouble x;
	gint i, j, barlen;
	gdouble f1, f2;
	gint mode = '-';	/* text below bars */
	gdouble x0, y0, yr;
	guchar *p, c;

	/* First calculate barlen */
	barlen = bci->partial[0] - '0';
	for (p = bci->partial + 1; *p != 0; p++) {
		if (isdigit (*p)) {
			barlen += *p - '0';
		} else {
			if ((*p != '+') && (*p != '-')) {
				barlen += *p - 'a' + 1;
			}
		}
	}

	/* The width defaults to "just enough" */
	bci->width = barlen * scale + 1;

	/* The height defaults to 80 points (rescaled) */
	if (!bci->height)
		bci->height = 80 * scale;

	gbc = g_new0 (glBarcode, 1);

	/* Now traverse the code string and create a list of lines */
	x = bci->margin + (bci->partial[0] - '0') * scale;
	for (p = bci->partial + 1, i = 1; *p != 0; p++, i++) {
		/* special cases: '+' and '-' */
		if (*p == '+' || *p == '-') {
			mode = *p;	/* don't count it */
			i++;
			continue;
		}
		/* j is the width of this bar/space */
		if (isdigit (*p))
			j = *p - '0';
		else
			j = *p - 'a' + 1;
		if (i % 2) {	/* bar */
			x0 = x + (j * scale) / 2;
			y0 = bci->margin;
			yr = bci->height;
			if (text_flag) {	/* leave space for text */
				if (mode == '-') {
					/* text below bars: 10 or 5 points */
					yr -= (isdigit (*p) ? 10 : 5) * scale;
				} else {	/* '+' */
					/* above bars: 10 or 0 from bottom,
					   and 10 from top */
					y0 += 10 * scale;
					yr -= (isdigit (*p) ? 20 : 10) * scale;
				}
			}
			line = g_new0 (glBarcodeLine, 1);
			line->x = x0;
			line->y = y0;
			line->length = yr;
			line->width = (j * scale) - SHRINK_AMOUNT;
			gbc->lines = g_list_append (gbc->lines, line);
		}
		x += j * scale;

	}

	/* Now the text */
	mode = '-';		/* reinstantiate default */
	if (text_flag) {
		for (p = bci->textinfo; p; p = strchr (p, ' ')) {
			while (*p == ' ')
				p++;
			if (!*p)
				break;
			if (*p == '+' || *p == '-') {
				mode = *p;
				continue;
			}
			if (sscanf (p, "%lf:%lf:%c", &f1, &f2, &c) != 3) {
				WARN ("impossible data: %s", p);
				continue;
			}
			bchar = g_new0 (glBarcodeChar, 1);
			bchar->x = f1 * scale + bci->margin;
			if (mode == '-') {
				bchar->y =
				    bci->margin + bci->height - 8 * scale;
			} else {
				bchar->y = bci->margin;
			}
			bchar->fsize = f2 * FONT_SCALE * scale;
			bchar->c = c;
			gbc->chars = g_list_append (gbc->chars, bchar);
		}
	}

	/* Fill in other info */
	gbc->height = bci->height + 2.0 * bci->margin;
	gbc->width = bci->width + 2.0 * bci->margin;

	return gbc;
}