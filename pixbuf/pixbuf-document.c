/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/*
 * Copyright (C) 2004, Anders Carlsson <andersca@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "pixbuf-document.h"
#include "ev-document-thumbnails.h"

struct _PixbufDocumentClass
{
	GObjectClass parent_class;
};

struct _PixbufDocument
{
	GObject parent_instance;

	GdkPixbuf *pixbuf;
	EvOrientation orientation;
};

typedef struct _PixbufDocumentClass PixbufDocumentClass;

static void pixbuf_document_document_iface_init (EvDocumentIface *iface);
static void pixbuf_document_document_thumbnails_iface_init (EvDocumentThumbnailsIface *iface);

G_DEFINE_TYPE_WITH_CODE (PixbufDocument, pixbuf_document, G_TYPE_OBJECT,
                         { G_IMPLEMENT_INTERFACE (EV_TYPE_DOCUMENT,
						  pixbuf_document_document_iface_init);
			 G_IMPLEMENT_INTERFACE (EV_TYPE_DOCUMENT_THUMBNAILS,
						pixbuf_document_document_thumbnails_iface_init)				   
				   });

static gboolean
pixbuf_document_load (EvDocument  *document,
		      const char  *uri,
		      GError     **error)
{
	PixbufDocument *pixbuf_document = PIXBUF_DOCUMENT (document);
	
	gchar *filename;
	GdkPixbuf *pixbuf;

	/* FIXME: We could actually load uris  */
	filename = g_filename_from_uri (uri, NULL, error);
	if (!filename)
		return FALSE;
	
	pixbuf = gdk_pixbuf_new_from_file (filename, error);

	if (!pixbuf)
		return FALSE;

	pixbuf_document->pixbuf = pixbuf;
	
	return TRUE;
}

static gboolean
pixbuf_document_save (EvDocument  *document,
		      const char  *uri,
		      GError     **error)
{
	g_warning ("pixbuf_document_save not implemented"); /* FIXME */
	return TRUE;
}

static int
pixbuf_document_get_n_pages (EvDocument  *document)
{
	return 1;
}

static EvOrientation
pixbuf_document_get_orientation (EvDocument *document)
{
	PixbufDocument *pixbuf_document = PIXBUF_DOCUMENT (document);

	return pixbuf_document->orientation;
}

static void
pixbuf_document_set_orientation (EvDocument *document,
			         EvOrientation   orientation)
{
	PixbufDocument *pixbuf_document = PIXBUF_DOCUMENT (document);

	pixbuf_document->orientation = orientation;
}

static GdkPixbuf *
rotate_pixbuf (EvDocument *document, GdkPixbuf *pixbuf)
{
	PixbufDocument *pixbuf_document = PIXBUF_DOCUMENT (document);

	switch (pixbuf_document->orientation)
	{
		case EV_ORIENTATION_LANDSCAPE:
			return gdk_pixbuf_rotate_simple (pixbuf, 90);
		case EV_ORIENTATION_UPSIDEDOWN:
			return gdk_pixbuf_rotate_simple (pixbuf, 180);
		case EV_ORIENTATION_SEASCAPE:
			return gdk_pixbuf_rotate_simple (pixbuf, 270);
		default:
			return g_object_ref (pixbuf);
	}
}

static void
pixbuf_document_get_page_size (EvDocument   *document,
			       int           page,
			       double       *width,
			       double       *height)
{
	PixbufDocument *pixbuf_document = PIXBUF_DOCUMENT (document);

	if (pixbuf_document->orientation == EV_ORIENTATION_PORTRAIT ||
	    pixbuf_document->orientation ==  EV_ORIENTATION_UPSIDEDOWN) {
		*width = gdk_pixbuf_get_width (pixbuf_document->pixbuf);
		*height = gdk_pixbuf_get_height (pixbuf_document->pixbuf);
	} else {
		*width = gdk_pixbuf_get_height (pixbuf_document->pixbuf);
		*height = gdk_pixbuf_get_width (pixbuf_document->pixbuf);
	}
}

static GdkPixbuf*
pixbuf_document_render_pixbuf (EvDocument      *document,
			       EvRenderContext *rc)
{
	PixbufDocument *pixbuf_document = PIXBUF_DOCUMENT (document);
	GdkPixbuf *scaled_pixbuf, *rotated_pixbuf;

	scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf_document->pixbuf,
					         gdk_pixbuf_get_width (pixbuf_document->pixbuf) * rc->scale,
					         gdk_pixbuf_get_height (pixbuf_document->pixbuf) * rc->scale,
					         GDK_INTERP_BILINEAR);

	rotated_pixbuf = rotate_pixbuf (document, scaled_pixbuf);
	g_object_unref (scaled_pixbuf);

	return rotated_pixbuf;
}

static void
pixbuf_document_finalize (GObject *object)
{
	PixbufDocument *pixbuf_document = PIXBUF_DOCUMENT (object);

	g_object_unref (pixbuf_document->pixbuf);
	
	G_OBJECT_CLASS (pixbuf_document_parent_class)->finalize (object);
}

static void
pixbuf_document_class_init (PixbufDocumentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = pixbuf_document_finalize;
}

static gboolean
pixbuf_document_can_get_text (EvDocument *document)
{
	return FALSE;
}

static EvDocumentInfo *
pixbuf_document_get_info (EvDocument *document)
{
	EvDocumentInfo *info;

	info = g_new0 (EvDocumentInfo, 1);
	info->fields_mask = 0;

	return info;
}

static void
pixbuf_document_document_iface_init (EvDocumentIface *iface)
{
	iface->load = pixbuf_document_load;
	iface->save = pixbuf_document_save;
	iface->can_get_text = pixbuf_document_can_get_text;
	iface->get_n_pages = pixbuf_document_get_n_pages;
	iface->get_page_size = pixbuf_document_get_page_size;
	iface->render_pixbuf = pixbuf_document_render_pixbuf;
	iface->get_info = pixbuf_document_get_info;
	iface->get_orientation = pixbuf_document_get_orientation;
	iface->set_orientation = pixbuf_document_set_orientation;
}

static GdkPixbuf *
pixbuf_document_thumbnails_get_thumbnail (EvDocumentThumbnails   *document,
					  gint 			  page,
					  gint			  size,
					  gboolean                border)
{
	PixbufDocument *pixbuf_document = PIXBUF_DOCUMENT (document);
	GdkPixbuf *pixbuf;
	gdouble scale_factor;
	gint height;
	
	scale_factor = (gdouble)size / gdk_pixbuf_get_width (pixbuf_document->pixbuf);

	height = gdk_pixbuf_get_height (pixbuf_document->pixbuf) * scale_factor;
	
	pixbuf = gdk_pixbuf_scale_simple (pixbuf_document->pixbuf, size, height,
					  GDK_INTERP_BILINEAR);
	
	return pixbuf;
}

static void
pixbuf_document_thumbnails_get_dimensions (EvDocumentThumbnails *document,
					   gint                  page,
					   gint                  suggested_width,
					   gint                  *width,
					   gint                  *height)
{
	PixbufDocument *pixbuf_document = PIXBUF_DOCUMENT (document);
	gdouble page_ratio;

	page_ratio = ((double)gdk_pixbuf_get_height (pixbuf_document->pixbuf)) /
		     gdk_pixbuf_get_width (pixbuf_document->pixbuf);
	*width = suggested_width;
	*height = (gint) (suggested_width * page_ratio);
}

static void
pixbuf_document_document_thumbnails_iface_init (EvDocumentThumbnailsIface *iface)
{
	iface->get_thumbnail = pixbuf_document_thumbnails_get_thumbnail;
	iface->get_dimensions = pixbuf_document_thumbnails_get_dimensions;
}


static void
pixbuf_document_init (PixbufDocument *pixbuf_document)
{
}
