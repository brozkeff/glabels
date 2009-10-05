/*
 *  ui-property-bar.c
 *  Copyright (C) 2003-2009  Jim Evins <evins@snaught.com>.
 *
 *  This file is part of gLabels.
 *
 *  gLabels is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  gLabels is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gLabels.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include "ui-property-bar.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>

#include "builder-util.h"
#include "font-combo.h"
#include "color-combo-button.h"
#include "stock-pixmaps/stockpixbufs.h"
#include "prefs.h"
#include "color.h"

#include "debug.h"


/*===========================================================================*/
/* Private macros and constants.                                             */
/*===========================================================================*/


/*===========================================================================*/
/* Private data types                                                        */
/*===========================================================================*/

struct _glUIPropertyBarPrivate {

	glView     *view;

	GtkBuilder *builder;

	GtkWidget  *tool_bar;

	/* Font selection */
	GtkWidget  *font_family_eventbox;
	GtkWidget  *font_family_combo;
	GtkWidget  *font_size_spin;
	GtkWidget  *font_bold_toggle;
	GtkWidget  *font_italic_toggle;

	/* Text alignemnt radios */
	GtkWidget  *text_align_left_radio;
	GtkWidget  *text_align_center_radio;
	GtkWidget  *text_align_right_radio;

	/* Color combos */
        GtkWidget  *text_color_eventbox;
	GtkWidget  *text_color_button;
        GtkWidget  *fill_color_eventbox;
	GtkWidget  *fill_color_button;
        GtkWidget  *line_color_eventbox;
	GtkWidget  *line_color_button;

	/* Line width */
	GtkWidget  *line_width_spin;

        /* Prevent recursion */
	gboolean    stop_signals;
};


/*===========================================================================*/
/* Private globals                                                           */
/*===========================================================================*/


/*===========================================================================*/
/* Local function prototypes                                                 */
/*===========================================================================*/

static void     gl_ui_property_bar_finalize      (GObject              *object);

static void     gl_ui_property_bar_construct     (glUIPropertyBar      *this);

static void     selection_changed_cb             (glUIPropertyBar      *this);

static void     font_family_changed_cb           (GtkComboBox          *combo,
						  glUIPropertyBar      *this);

static void     font_size_changed_cb             (GtkSpinButton        *spin,
						  glUIPropertyBar      *this);

static void     text_color_changed_cb            (glColorComboButton   *cc,
                                                  guint                 color,
						  gboolean              is_default,
						  glUIPropertyBar      *this);

static void     fill_color_changed_cb            (glColorComboButton   *cc,
                                                  guint                 color,
						  gboolean              is_default,
						  glUIPropertyBar      *this);

static void     line_color_changed_cb            (glColorComboButton   *cc,
                                                  guint                 color,
						  gboolean              is_default,
						  glUIPropertyBar      *this);

static void     line_width_changed_cb            (GtkSpinButton        *spin,
						  glUIPropertyBar      *this);

static void     font_bold_toggled_cb             (GtkToggleToolButton  *toggle,
						  glUIPropertyBar      *this);
						  
static void     font_italic_toggled_cb           (GtkToggleToolButton  *toggle,
						  glUIPropertyBar      *this);
						  
static void     text_align_toggled_cb            (GtkToggleToolButton  *toggle,
						  glUIPropertyBar      *this);
						  
static void     set_doc_items_sensitive          (glUIPropertyBar      *this,
						  gboolean              state);

static void     set_text_items_sensitive         (glUIPropertyBar      *this,
						  gboolean              state);

static void     set_fill_items_sensitive         (glUIPropertyBar      *this,
						  gboolean              state);

static void     set_line_color_items_sensitive   (glUIPropertyBar      *this,
						  gboolean              state);

static void     set_line_width_items_sensitive   (glUIPropertyBar      *this,
						  gboolean              state);


/****************************************************************************/
/* Boilerplate Object stuff.                                                */
/****************************************************************************/
G_DEFINE_TYPE (glUIPropertyBar, gl_ui_property_bar, GTK_TYPE_HBOX);


static void
gl_ui_property_bar_class_init (glUIPropertyBarClass *class)
{
	GObjectClass   *object_class     = G_OBJECT_CLASS (class);

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	gl_ui_property_bar_parent_class = g_type_class_peek_parent (class);

	object_class->finalize = gl_ui_property_bar_finalize;

	gl_debug (DEBUG_PROPERTY_BAR, "END");
}


static void
gl_ui_property_bar_init (glUIPropertyBar *this)
{
	gl_debug (DEBUG_PROPERTY_BAR, "START");

	this->priv = g_new0 (glUIPropertyBarPrivate, 1);

	gl_debug (DEBUG_PROPERTY_BAR, "END");
}


static void
gl_ui_property_bar_finalize (GObject *object)
{
	glUIPropertyBar *this = GL_UI_PROPERTY_BAR (object);

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	g_return_if_fail (object != NULL);
	g_return_if_fail (GL_IS_UI_PROPERTY_BAR (object));

	if (this->priv->view)
        {
		g_object_unref (G_OBJECT(this->priv->view));
	}
        if (this->priv->builder)
        {
                g_object_unref (G_OBJECT(this->priv->builder));
        }
	g_free (this->priv);

	G_OBJECT_CLASS (gl_ui_property_bar_parent_class)->finalize (object);

	gl_debug (DEBUG_PROPERTY_BAR, "END");
}


/****************************************************************************/
/* Create a NEW property_bar.                                               */
/****************************************************************************/
GtkWidget *
gl_ui_property_bar_new (void)
{
	glUIPropertyBar *this;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	this = g_object_new (GL_TYPE_UI_PROPERTY_BAR, NULL);

	gl_ui_property_bar_construct (this);

	gl_debug (DEBUG_PROPERTY_BAR, "END");

	return GTK_WIDGET (this);
}


/******************************************************************************/
/* Initialize property toolbar.                                               */
/******************************************************************************/
static void
gl_ui_property_bar_construct (glUIPropertyBar   *this)
{
	GtkBuilder    *builder;
        static gchar  *object_ids[] = { "property_toolbar",
                                        "adjustment1", "adjustment2",
                                        NULL };
        GError        *error = NULL;
	GdkPixbuf     *pixbuf = NULL;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	this->priv->stop_signals = TRUE;

        builder = gtk_builder_new ();
        gtk_builder_add_objects_from_file (builder,
                                           GLABELS_BUILDER_DIR "property-bar.builder",
                                           object_ids,
                                           &error);
	if (error) {
		g_critical ("%s\n\ngLabels may not be installed correctly!", error->message);
                g_error_free (error);
		return;
	}

        gl_builder_util_get_widgets (builder,
                                     "property_toolbar",        &this->priv->tool_bar,
                                     "font_family_eventbox",    &this->priv->font_family_eventbox,
                                     "font_size_spin",          &this->priv->font_size_spin,
                                     "font_bold_toggle",        &this->priv->font_bold_toggle,
                                     "font_italic_toggle",      &this->priv->font_italic_toggle,
                                     "text_align_left_radio",   &this->priv->text_align_left_radio,
                                     "text_align_center_radio", &this->priv->text_align_center_radio,
                                     "text_align_right_radio",  &this->priv->text_align_right_radio,
                                     "text_color_eventbox",     &this->priv->text_color_eventbox,
                                     "fill_color_eventbox",     &this->priv->fill_color_eventbox,
                                     "line_color_eventbox",     &this->priv->line_color_eventbox,
                                     "line_width_spin",         &this->priv->line_width_spin,
                                     NULL);

	gtk_container_add (GTK_CONTAINER (this), this->priv->tool_bar);

        this->priv->font_family_combo = gl_font_combo_new (gl_prefs->default_font_family);
        gtk_container_add (GTK_CONTAINER (this->priv->font_family_eventbox),
                           this->priv->font_family_combo);

        pixbuf = gdk_pixbuf_new_from_inline (-1, stock_text_24, FALSE, NULL);
        this->priv->text_color_button =
                gl_color_combo_button_new (pixbuf,
                                           _("Default"),
                                           GL_COLOR_TEXT_DEFAULT,
                                           gl_prefs->default_text_color);
        gl_color_combo_button_set_relief (GL_COLOR_COMBO_BUTTON(this->priv->text_color_button),
                                          GTK_RELIEF_NONE);
	g_object_unref (G_OBJECT (pixbuf));
        gtk_container_add (GTK_CONTAINER (this->priv->text_color_eventbox),
                           this->priv->text_color_button);

        pixbuf = gdk_pixbuf_new_from_inline (-1, stock_bucket_fill_24, FALSE, NULL);
        this->priv->fill_color_button =
                gl_color_combo_button_new (pixbuf,
                                           _("No Fill"),
                                           GL_COLOR_NO_FILL,
                                           gl_prefs->default_fill_color);
        gl_color_combo_button_set_relief (GL_COLOR_COMBO_BUTTON(this->priv->fill_color_button),
                                          GTK_RELIEF_NONE);
	g_object_unref (G_OBJECT (pixbuf));
        gtk_container_add (GTK_CONTAINER (this->priv->fill_color_eventbox),
                           this->priv->fill_color_button);

        pixbuf = gdk_pixbuf_new_from_inline (-1, stock_pencil_24, FALSE, NULL);
        this->priv->line_color_button =
                gl_color_combo_button_new (pixbuf,
                                           _("No Line"),
                                           GL_COLOR_NO_LINE,
                                           gl_prefs->default_line_color);
	gl_color_combo_button_set_relief (GL_COLOR_COMBO_BUTTON(this->priv->line_color_button),
                                          GTK_RELIEF_NONE);
	g_object_unref (G_OBJECT (pixbuf));
        gtk_container_add (GTK_CONTAINER (this->priv->line_color_eventbox),
                           this->priv->line_color_button);

        /* Save reference to gui tree so we don't lose tooltips */
        this->priv->builder = builder;

	set_doc_items_sensitive (this, FALSE);

	/* Font family entry widget */
	g_signal_connect (G_OBJECT (this->priv->font_family_combo),
			  "changed", G_CALLBACK (font_family_changed_cb), this);

	/* Font size entry widget */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON(this->priv->font_size_spin),
				   gl_prefs->default_font_size);

	g_signal_connect (G_OBJECT (this->priv->font_size_spin),
			  "changed", G_CALLBACK (font_size_changed_cb), this);


	/* Bold and Italic toggles */
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->font_bold_toggle),
					   (gl_prefs->default_font_weight == PANGO_WEIGHT_BOLD));
	g_signal_connect (G_OBJECT (this->priv->font_bold_toggle),
			  "toggled", G_CALLBACK (font_bold_toggled_cb), this);
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->font_italic_toggle),
					   gl_prefs->default_font_italic_flag);
	g_signal_connect (G_OBJECT (this->priv->font_italic_toggle),
			  "toggled", G_CALLBACK (font_italic_toggled_cb), this);


	/* Text alignment radio group */
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_left_radio),
					   (gl_prefs->default_text_alignment == PANGO_ALIGN_LEFT));
	g_signal_connect (G_OBJECT (this->priv->text_align_left_radio),
			  "toggled", G_CALLBACK (text_align_toggled_cb), this);
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_center_radio),
					   (gl_prefs->default_text_alignment == PANGO_ALIGN_CENTER));
	g_signal_connect (G_OBJECT (this->priv->text_align_center_radio),
			  "toggled", G_CALLBACK (text_align_toggled_cb), this);
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_right_radio),
					   (gl_prefs->default_text_alignment == PANGO_ALIGN_RIGHT));
	g_signal_connect (G_OBJECT (this->priv->text_align_right_radio),
			  "toggled", G_CALLBACK (text_align_toggled_cb), this);

	/* Text color widget */
	gl_color_combo_button_set_color (GL_COLOR_COMBO_BUTTON (this->priv->text_color_button),
                                         gl_prefs->default_text_color);
	g_signal_connect (G_OBJECT (this->priv->text_color_button),
			  "color_changed",
			  G_CALLBACK (text_color_changed_cb), this);

	/* Fill color widget */
	gl_color_combo_button_set_color (GL_COLOR_COMBO_BUTTON (this->priv->fill_color_button),
                                         gl_prefs->default_fill_color);
	g_signal_connect (G_OBJECT (this->priv->fill_color_button),
			  "color_changed",
			  G_CALLBACK (fill_color_changed_cb), this);

	/* Line color widget */
	gl_color_combo_button_set_color (GL_COLOR_COMBO_BUTTON (this->priv->line_color_button),
                                         gl_prefs->default_line_color);
	g_signal_connect (G_OBJECT (this->priv->line_color_button),
			  "color_changed",
			  G_CALLBACK (line_color_changed_cb), this);

	/* Line width entry widget */
	g_signal_connect (G_OBJECT (this->priv->line_width_spin),
			  "changed",
			  G_CALLBACK (line_width_changed_cb), this);

	this->priv->stop_signals = FALSE;

	gl_debug (DEBUG_PROPERTY_BAR, "END");
}


/****************************************************************************/
/* Fill widgets with default values.                                        */
/****************************************************************************/
static void
reset_to_default_properties (glView          *view,
			     glUIPropertyBar *this)
{

	gl_font_combo_set_family (GL_FONT_COMBO (this->priv->font_family_combo),
                                  view->default_font_family);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON(this->priv->font_size_spin),
				   view->default_font_size);

	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->font_bold_toggle),
					   (view->default_font_weight == PANGO_WEIGHT_BOLD));
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->font_italic_toggle),
					   view->default_font_italic_flag);

	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_left_radio),
					   (view->default_text_alignment == PANGO_ALIGN_LEFT));
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_center_radio),
					   (view->default_text_alignment == PANGO_ALIGN_CENTER));
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_right_radio),
					   (view->default_text_alignment == PANGO_ALIGN_RIGHT));

	gl_color_combo_button_set_color (GL_COLOR_COMBO_BUTTON(this->priv->text_color_button),
                                         view->default_text_color);

	gl_color_combo_button_set_color (GL_COLOR_COMBO_BUTTON(this->priv->fill_color_button),
                                         view->default_fill_color);

	gl_color_combo_button_set_color (GL_COLOR_COMBO_BUTTON(this->priv->line_color_button),
                                         view->default_line_color);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON(this->priv->line_width_spin),
				   view->default_line_width);
}


/****************************************************************************/
/* Set view associated with property_bar.                                   */
/****************************************************************************/
void
gl_ui_property_bar_set_view (glUIPropertyBar *this,
			     glView          *view)
{
	glLabel   *label;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	g_return_if_fail (view && GL_IS_VIEW (view));
	label = view->label;
	g_return_if_fail (label && GL_IS_LABEL (label));

	set_doc_items_sensitive (this, TRUE);

	this->priv->view = GL_VIEW (g_object_ref (G_OBJECT (view)));

	reset_to_default_properties (view, this);

	g_signal_connect_swapped (G_OBJECT(view), "selection_changed",
				  G_CALLBACK(selection_changed_cb), this);

	g_signal_connect_swapped (G_OBJECT(view->label), "changed",
				  G_CALLBACK(selection_changed_cb), this);

	gl_debug (DEBUG_PROPERTY_BAR, "END");
}


/*---------------------------------------------------------------------------*/
/* PRIVATE.  View "selection state changed" callback.                        */
/*---------------------------------------------------------------------------*/
static void
update_text_properties (glView *view,
			glUIPropertyBar *this)
{
	gboolean        can_text, is_first_object;
	gboolean        is_same_font_family, is_same_font_size;
	gboolean        is_same_text_color, is_same_is_italic;
	gboolean        is_same_is_bold, is_same_align;
	GList          *p;
	glLabelObject  *object;
	gchar          *selection_font_family, *font_family;
	gdouble         selection_font_size, font_size;
	guint           selection_text_color, text_color;
	glColorNode    *text_color_node;
	gboolean        selection_is_italic, is_italic;
	gboolean        selection_is_bold, is_bold;
	PangoAlignment  selection_align, align;

	can_text = gl_view_can_selection_text (view);
	set_text_items_sensitive (this, can_text);

	if (!can_text) 
		return;

	is_same_is_italic =
	is_same_is_bold =
	is_same_align =
	is_same_text_color =
	is_same_font_size =
	is_same_font_family = TRUE;
	selection_font_family = NULL;
	selection_font_size = -1;
        selection_align = PANGO_ALIGN_LEFT;
        selection_is_italic = TRUE;
        selection_is_bold = TRUE;
        selection_text_color = 0;
        
	is_first_object = TRUE;
	
	for (p = view->selected_object_list; p != NULL; p = p->next)
        {

		object = gl_view_object_get_object(GL_VIEW_OBJECT (p->data));
		if (!gl_label_object_can_text (object)) 
			continue;

		font_family = gl_label_object_get_font_family (object);
		if (font_family != NULL)
                {
			if (selection_font_family == NULL)
                        {
				selection_font_family = g_strdup (font_family);
                        }
			else 
                        {
				if (strcmp (font_family, selection_font_family) != 0)
                                {
					is_same_font_family = FALSE;
                                }
                        }
			g_free (font_family);
		}	

		font_size = gl_label_object_get_font_size (object);
		
		text_color_node = gl_label_object_get_text_color (object);
		if (text_color_node->field_flag)
                {
			/* If a merge field is set we use the default color for merged color*/
			text_color = GL_COLOR_MERGE_DEFAULT;
			
		}
                else
                {
			text_color = text_color_node->color;
		}
		gl_color_node_free (&text_color_node);
		
		is_italic = gl_label_object_get_font_italic_flag (object);
		is_bold = gl_label_object_get_font_weight (object) == PANGO_WEIGHT_BOLD;
		align = gl_label_object_get_text_alignment (object);

		if (is_first_object)
                {
			selection_font_size = font_size;
			selection_text_color = text_color;
			selection_is_italic = is_italic;
			selection_is_bold = is_bold;
			selection_align = align;
		}
                else
                {
			if (font_size != selection_font_size) 
				is_same_font_size = FALSE;
			if (text_color != selection_text_color)
				is_same_text_color = FALSE;
			if (is_italic != selection_is_italic)
				is_same_is_italic = FALSE;
			if (is_bold != selection_is_bold)
				is_same_is_bold = FALSE;
			if (align != selection_align)
				is_same_align = FALSE;
		}
		is_first_object = FALSE;
	}

	if (is_same_font_family && (selection_font_family != NULL)) 
		gl_debug (DEBUG_PROPERTY_BAR, "same font family = %s", 
			  selection_font_family);
	gl_font_combo_set_family (GL_FONT_COMBO (this->priv->font_family_combo),
                                  is_same_font_family?selection_font_family:"");
	g_free (selection_font_family);

	if (is_same_font_size)
        {
		gl_debug (DEBUG_PROPERTY_BAR, "same font size = %g", 
			  selection_font_size);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (this->priv->font_size_spin),
					   selection_font_size);
	}
        else
        {
		gtk_entry_set_text (GTK_ENTRY (this->priv->font_size_spin), "");
	}

	if (is_same_text_color)
        {
		gl_debug (DEBUG_PROPERTY_BAR, "same text color = %08x", selection_text_color);
		gl_color_combo_button_set_color (GL_COLOR_COMBO_BUTTON (this->priv->text_color_button),
                                                 selection_text_color);
	}

	if (is_same_is_italic)
        {
		gl_debug (DEBUG_PROPERTY_BAR, "same italic flag = %d", 
			  selection_is_italic);
        }
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->font_italic_toggle),
					   selection_is_italic && is_same_is_italic);

	if (is_same_is_bold)
        {
		gl_debug (DEBUG_PROPERTY_BAR, "same bold flag = %d",
			  selection_is_bold);
        }
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->font_bold_toggle),
					   selection_is_bold && is_same_is_bold);

	if (is_same_align) 
		gl_debug (DEBUG_PROPERTY_BAR, "same align");
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_left_radio),
					   (selection_align == PANGO_ALIGN_LEFT) &&
					   is_same_align);
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_center_radio),
					   (selection_align == PANGO_ALIGN_CENTER) &&
					   is_same_align);
	gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_right_radio),
					   (selection_align == PANGO_ALIGN_RIGHT) &&
					   is_same_align);
}


static void
update_fill_color (glView *view,
		   glUIPropertyBar *this)
{
	gboolean can, is_first_object;
	gboolean is_same_fill_color;
	GList *p;
	glLabelObject *object;
	guint selection_fill_color, fill_color;
	glColorNode *fill_color_node;

	can = gl_view_can_selection_fill (view);
	set_fill_items_sensitive (this, can);

	if (!can) 
		return;

	is_same_fill_color = TRUE;
	is_first_object = TRUE;
        selection_fill_color = 0;
	
	for (p = view->selected_object_list; p != NULL; p = p->next)
        {

		object = gl_view_object_get_object(GL_VIEW_OBJECT (p->data));
		if (!gl_label_object_can_fill (object)) 
			continue;

		fill_color_node = gl_label_object_get_fill_color (object);
		if (fill_color_node->field_flag)
                {
			/* If a merge field is set we use the default color for merged color*/
			fill_color = GL_COLOR_FILL_MERGE_DEFAULT;
			
		}
                else
                {
			fill_color = fill_color_node->color;
		}
		gl_color_node_free (&fill_color_node);

		if (is_first_object)
                {
			selection_fill_color = fill_color;
		}
                else
                {
			if (fill_color != selection_fill_color)
                        {
				is_same_fill_color = FALSE;
                        }
		}
		is_first_object = FALSE;
	}

	if (is_same_fill_color)
        {
		gl_debug (DEBUG_PROPERTY_BAR, "same fill color = %08x", selection_fill_color);
		gl_color_combo_button_set_color (GL_COLOR_COMBO_BUTTON (this->priv->fill_color_button),
                                                 selection_fill_color);
	}
}


static void
update_line_color (glView *view,
		   glUIPropertyBar *this)
{
	gboolean can, is_first_object;
	gboolean is_same_line_color;
	GList *p;
	glLabelObject *object;
	guint selection_line_color, line_color;
	glColorNode *line_color_node;

	can = gl_view_can_selection_line_color (view);
	set_line_color_items_sensitive (this, can);

	if (!can) 
		return;

	is_same_line_color = TRUE;
	is_first_object = TRUE;
        selection_line_color = 0;
	
	for (p = view->selected_object_list; p != NULL; p = p->next)
        {

		object = gl_view_object_get_object(GL_VIEW_OBJECT (p->data));
		if (!gl_label_object_can_line_color (object)) 
			continue;

		line_color_node = gl_label_object_get_line_color (object);
		if (line_color_node->field_flag)
                {
			/* If a merge field is set we use the default color for merged color*/
			line_color = GL_COLOR_MERGE_DEFAULT;
			
		}
                else
                {
			line_color = line_color_node->color;
		}
		gl_color_node_free (&line_color_node);

		if (is_first_object)
                {
			selection_line_color = line_color;
		}
                else
                {
			if (line_color != selection_line_color)
                        {
				is_same_line_color = FALSE;
                        }
		}
		is_first_object = FALSE;
	}

	if (is_same_line_color)
        {
		gl_debug (DEBUG_PROPERTY_BAR, "same line color = %08x", selection_line_color);
		gl_color_combo_button_set_color (GL_COLOR_COMBO_BUTTON (this->priv->line_color_button),
                                                 selection_line_color);
	}
}


static void
update_line_width (glView *view,
		   glUIPropertyBar *this)
{
	gboolean can, is_first_object;
	gboolean is_same_line_width;
	GList *p;
	glLabelObject *object;
	gdouble selection_line_width, line_width;

	can = gl_view_can_selection_line_width (view);
	set_line_width_items_sensitive (this, can);

	if (!can) 
		return;

	is_same_line_width = TRUE;
	is_first_object = TRUE;
        selection_line_width = 0;
	
	for (p = view->selected_object_list; p != NULL; p = p->next)
        {

		object = gl_view_object_get_object(GL_VIEW_OBJECT (p->data));
		if (!gl_label_object_can_line_width (object)) 
			continue;

		line_width = gl_label_object_get_line_width (object);

		if (is_first_object)
                {
			selection_line_width = line_width;
		}
                else
                {
			if (line_width != selection_line_width)
                        {
				is_same_line_width = FALSE;
                        }
		}
		is_first_object = FALSE;
	}

	if (is_same_line_width)
        {
		gl_debug (DEBUG_PROPERTY_BAR, "same line width = %g", selection_line_width);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (this->priv->line_width_spin),
					   selection_line_width);
	}
        else
        {
		gtk_entry_set_text (GTK_ENTRY (this->priv->line_width_spin), "");
	}
}


static void 
selection_changed_cb (glUIPropertyBar *this)
{
	glView *view = this->priv->view;
	
	g_return_if_fail (view && GL_IS_VIEW (view));
	g_return_if_fail (this && GL_IS_UI_PROPERTY_BAR (this));

	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	if (gl_view_is_selection_empty (view))
        {
		/* No selection: make all controls active. */
		reset_to_default_properties (view, this);
		set_doc_items_sensitive (this, TRUE);
	}
        else
        {
		update_text_properties (view, this);
		update_fill_color (view, this);
		update_line_color (view, this);
		update_line_width (view, this);
	}

	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}


/*--------------------------------------------------------------------------*/
/* PRIVATE.  Font family entry changed.                                     */
/*--------------------------------------------------------------------------*/
static void
font_family_changed_cb (GtkComboBox     *combo,
			glUIPropertyBar *this)
{
	gchar *font_family;

	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	font_family = gl_font_combo_get_family (GL_FONT_COMBO (combo));
	if ( strlen(font_family) )
        {
		gl_view_set_selection_font_family (this->priv->view,
						   font_family);
		gl_view_set_default_font_family   (this->priv->view,
						   font_family);
	}
	g_free (font_family);

	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}


/*--------------------------------------------------------------------------*/
/* PRIVATE.  Font size spin button changed.                                 */
/*--------------------------------------------------------------------------*/
static void
font_size_changed_cb (GtkSpinButton        *spin,
		      glUIPropertyBar      *this)
{
	gdouble font_size;

	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	font_size = gtk_spin_button_get_value (spin);

	gl_view_set_selection_font_size (this->priv->view,
					 font_size);
	gl_view_set_default_font_size   (this->priv->view,
					 font_size);

	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}


/*--------------------------------------------------------------------------*/
/* PRIVATE.  Text color combo changed.                                      */
/*--------------------------------------------------------------------------*/
static void
text_color_changed_cb (glColorComboButton   *cc,
                       guint                 color,
		       gboolean              is_default,
		       glUIPropertyBar      *this)
{
	glColorNode *text_color_node;

	g_return_if_fail (this && GL_IS_UI_PROPERTY_BAR (this));

	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	text_color_node = gl_color_node_new_default ();
	text_color_node->color = color;
	
	gl_debug (DEBUG_PROPERTY_BAR, "Color=%08x, Is_default=%d",
		  color, is_default);

	if (is_default)
        {
		text_color_node->color = gl_prefs->default_text_color;
		gl_view_set_selection_text_color (this->priv->view,
						  text_color_node);
		gl_view_set_default_text_color   (this->priv->view,
						  gl_prefs->default_text_color);
	}
        else
        {
		gl_view_set_selection_text_color (this->priv->view,
						  text_color_node);
		gl_view_set_default_text_color   (this->priv->view,
						  text_color_node->color);
	}

	gl_color_node_free (&text_color_node);
	
	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}


/*--------------------------------------------------------------------------*/
/* PRIVATE.  Fill color combo changed.                                      */
/*--------------------------------------------------------------------------*/
static void
fill_color_changed_cb (glColorComboButton   *cc,
                       guint                 color,
		       gboolean              is_default,
		       glUIPropertyBar      *this)
{
	glColorNode *fill_color_node;

	g_return_if_fail (this && GL_IS_UI_PROPERTY_BAR (this));

	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	fill_color_node = gl_color_node_new_default ();

	fill_color_node->color = color;

	gl_debug (DEBUG_PROPERTY_BAR, "Color=%08x, Is_default=%d",
		  color, is_default);

	if (is_default)
        {

		fill_color_node->color = GL_COLOR_NONE;
		gl_view_set_selection_fill_color (this->priv->view,
						  fill_color_node);
		gl_view_set_default_fill_color   (this->priv->view,
						  fill_color_node->color);
	}
        else
        {
		gl_view_set_selection_fill_color (this->priv->view,
						  fill_color_node);
		gl_view_set_default_fill_color   (this->priv->view,
						  fill_color_node->color);
	}
	gl_color_node_free (&fill_color_node);
	
	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}


/*--------------------------------------------------------------------------*/
/* PRIVATE.  Line color combo changed.                                      */
/*--------------------------------------------------------------------------*/
static void
line_color_changed_cb (glColorComboButton   *cc,
                       guint                 color,
		       gboolean              is_default,
		       glUIPropertyBar      *this)
{
	glColorNode *line_color_node;

	g_return_if_fail (this && GL_IS_UI_PROPERTY_BAR (this));

	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	line_color_node = gl_color_node_new_default ();
	line_color_node->color = color;

	gl_debug (DEBUG_PROPERTY_BAR, "Color=%08x, Is_default=%d",
		  color, is_default);

	if (is_default)
        {
		line_color_node->color = GL_COLOR_NONE;
		gl_view_set_selection_line_color (this->priv->view,
						  line_color_node);
		gl_view_set_default_line_color   (this->priv->view,
						  line_color_node->color);
	}
        else
        {
		gl_view_set_selection_line_color (this->priv->view,
						  line_color_node);
		gl_view_set_default_line_color   (this->priv->view,
						  line_color_node->color);
	}
	gl_color_node_free (&line_color_node);

	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}


/*--------------------------------------------------------------------------*/
/* PRIVATE.  Line width spin button changed.                                */
/*--------------------------------------------------------------------------*/
static void
line_width_changed_cb (GtkSpinButton        *spin,
		       glUIPropertyBar      *this)
{
	gdouble line_width;

	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	if (this->priv->view)
        {
		line_width = gtk_spin_button_get_value (spin);

		gl_view_set_selection_line_width (this->priv->view,
						  line_width);
		gl_view_set_default_line_width   (this->priv->view,
						  line_width);
	}

	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}


/*---------------------------------------------------------------------------*/
/* PRIVATE.  Font bold toggled callback.                                     */
/*---------------------------------------------------------------------------*/
static void
font_bold_toggled_cb (GtkToggleToolButton  *toggle,
		      glUIPropertyBar      *this)
{
	gboolean        state;
	PangoWeight     weight;


	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	state = gtk_toggle_tool_button_get_active (toggle);

	weight = state ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;

	gl_view_set_selection_font_weight (this->priv->view, weight);
	gl_view_set_default_font_weight   (this->priv->view, weight);

	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}
						  

/*---------------------------------------------------------------------------*/
/* PRIVATE.  Font italic toggled callback.                                   */
/*---------------------------------------------------------------------------*/
static void
font_italic_toggled_cb (GtkToggleToolButton  *toggle,
			glUIPropertyBar      *this)
{
	gboolean state;

	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	state = gtk_toggle_tool_button_get_active (toggle);

	gl_view_set_selection_font_italic_flag (this->priv->view, state);
	gl_view_set_default_font_italic_flag   (this->priv->view, state);

	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}
						  

/*---------------------------------------------------------------------------*/
/* PRIVATE.  Text align toggled callback.                                    */
/*---------------------------------------------------------------------------*/
static void
text_align_toggled_cb (GtkToggleToolButton  *toggle,
		       glUIPropertyBar      *this)
{
	if (this->priv->stop_signals) return;
	this->priv->stop_signals = TRUE;

	gl_debug (DEBUG_PROPERTY_BAR, "START");

	if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_left_radio)))
	{		
		gl_view_set_selection_text_alignment (this->priv->view,
						      PANGO_ALIGN_LEFT);
		gl_view_set_default_text_alignment   (this->priv->view,
						      PANGO_ALIGN_LEFT);
	}

	if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_center_radio)))
	{		
		gl_view_set_selection_text_alignment (this->priv->view,
						      PANGO_ALIGN_CENTER);
		gl_view_set_default_text_alignment   (this->priv->view,
						      PANGO_ALIGN_CENTER);
	}

	if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (this->priv->text_align_right_radio)))
	{
		gl_view_set_selection_text_alignment (this->priv->view,
						      PANGO_ALIGN_RIGHT);
		gl_view_set_default_text_alignment   (this->priv->view,
						      PANGO_ALIGN_RIGHT);
	}

	gl_debug (DEBUG_PROPERTY_BAR, "END");

	this->priv->stop_signals = FALSE;
}


/*---------------------------------------------------------------------------*/
/* PRIVATE.  Set sensitivity of doc controls.                                */
/*---------------------------------------------------------------------------*/
static void
set_doc_items_sensitive (glUIPropertyBar      *this,
			 gboolean              state)
{
	gtk_widget_set_sensitive (this->priv->font_family_combo,       state);
	gtk_widget_set_sensitive (this->priv->font_size_spin,          state);
	gtk_widget_set_sensitive (this->priv->font_bold_toggle,        state);
	gtk_widget_set_sensitive (this->priv->font_italic_toggle,      state);
	gtk_widget_set_sensitive (this->priv->text_align_left_radio,   state);
	gtk_widget_set_sensitive (this->priv->text_align_center_radio, state);
	gtk_widget_set_sensitive (this->priv->text_align_right_radio,  state);
	gtk_widget_set_sensitive (this->priv->text_color_button,       state);
	gtk_widget_set_sensitive (this->priv->fill_color_button,       state);
	gtk_widget_set_sensitive (this->priv->line_color_button,       state);
	gtk_widget_set_sensitive (this->priv->line_width_spin,         state);
}


/*---------------------------------------------------------------------------*/
/* PRIVATE.  Set sensitivity of text related controls.                       */
/*---------------------------------------------------------------------------*/
static void
set_text_items_sensitive (glUIPropertyBar      *this,
			  gboolean              state)
{
	gtk_widget_set_sensitive (this->priv->font_family_combo,       state);
	gtk_widget_set_sensitive (this->priv->font_size_spin,          state);
	gtk_widget_set_sensitive (this->priv->font_bold_toggle,        state);
	gtk_widget_set_sensitive (this->priv->font_italic_toggle,      state);
	gtk_widget_set_sensitive (this->priv->text_align_left_radio,   state);
	gtk_widget_set_sensitive (this->priv->text_align_center_radio, state);
	gtk_widget_set_sensitive (this->priv->text_align_right_radio,  state);
	gtk_widget_set_sensitive (this->priv->text_color_button,       state);
}


/*---------------------------------------------------------------------------*/
/* PRIVATE.  Set sensitivity of fill related controls.                       */
/*---------------------------------------------------------------------------*/
static void
set_fill_items_sensitive (glUIPropertyBar      *this,
			  gboolean              state)
{
	gtk_widget_set_sensitive (this->priv->fill_color_button,       state);
}


/*---------------------------------------------------------------------------*/
/* PRIVATE.  Set sensitivity of line color related controls.                 */
/*---------------------------------------------------------------------------*/
static void
set_line_color_items_sensitive (glUIPropertyBar      *this,
				gboolean              state)
{
	gtk_widget_set_sensitive (this->priv->line_color_button,       state);
}


/*---------------------------------------------------------------------------*/
/* PRIVATE.  Set sensitivity of line width related controls.                 */
/*---------------------------------------------------------------------------*/
static void
set_line_width_items_sensitive (glUIPropertyBar      *this,
				gboolean              state)
{
	gtk_widget_set_sensitive (this->priv->line_width_spin,         state);
}



/*
 * Local Variables:       -- emacs
 * mode: C                -- emacs
 * c-basic-offset: 8      -- emacs
 * tab-width: 8           -- emacs
 * indent-tabs-mode: nil  -- emacs
 * End:                   -- emacs
 */