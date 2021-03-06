/*
 * e-mail-formatter-headers.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n-lib.h>

#include <libedataserver/libedataserver.h>

#include <e-util/e-util.h>
#include <libemail-engine/libemail-engine.h>
#include <shell/e-shell.h>

#include "e-mail-formatter-extension.h"
#include "e-mail-formatter-utils.h"
#include "e-mail-inline-filter.h"
#include "e-mail-part-headers.h"

typedef EMailFormatterExtension EMailFormatterHeaders;
typedef EMailFormatterExtensionClass EMailFormatterHeadersClass;

GType e_mail_formatter_headers_get_type (void);

G_DEFINE_TYPE (
	EMailFormatterHeaders,
	e_mail_formatter_headers,
	E_TYPE_MAIL_FORMATTER_EXTENSION)

static const gchar *formatter_mime_types[] = {
	"application/vnd.evolution.headers",
	NULL
};

static void
format_short_headers (EMailFormatter *formatter,
                      GString *buffer,
                      EMailPart *part,
                      guint32 flags,
                      GCancellable *cancellable)
{
	CamelMimePart *mime_part;
	GtkTextDirection direction;
	gchar *hdr_charset;
	gchar *evolution_imagesdir;
	gchar *subject = NULL;
	struct _camel_header_address *addrs = NULL;
	struct _camel_header_raw *header;
	GString *from;

	if (g_cancellable_is_cancelled (cancellable))
		return;

	mime_part = e_mail_part_ref_mime_part (part);
	direction = gtk_widget_get_default_direction ();

	hdr_charset = e_mail_formatter_dup_charset (formatter);
	if (!hdr_charset)
		hdr_charset = e_mail_formatter_dup_default_charset (formatter);

	evolution_imagesdir = g_filename_to_uri (EVOLUTION_IMAGESDIR, NULL, NULL);
	from = g_string_new ("");

	g_string_append_printf (
		buffer,
		"<table class=\"header\" "
		"id=\"__evo-short-headers\" style=\"display: %s\">",
		flags & E_MAIL_FORMATTER_HEADER_FLAG_COLLAPSED ? "table" : "none");

	header = mime_part->headers;
	while (header) {
		if (!g_ascii_strcasecmp (header->name, "From")) {
			GString *tmp;
			if (!(addrs = camel_header_address_decode (header->value, hdr_charset))) {
				header = header->next;
				continue;
			}
			tmp = g_string_new ("");
			e_mail_formatter_format_address (
				formatter, tmp, addrs, header->name, FALSE,
				!(flags & E_MAIL_FORMATTER_HEADER_FLAG_NOELIPSIZE));

			if (tmp->len > 0)
				g_string_printf (
					from, "%s: %s",
					_("From"), tmp->str);
			g_string_free (tmp, TRUE);

		} else if (!g_ascii_strcasecmp (header->name, "Subject")) {
			gchar *buf = NULL;
			subject = camel_header_unfold (header->value);
			buf = camel_header_decode_string (subject, hdr_charset);
			g_free (subject);
			subject = camel_text_to_html (
				buf, CAMEL_MIME_FILTER_TOHTML_PRESERVE_8BIT, 0);
			g_free (buf);
		}
		header = header->next;
	}

	g_free (hdr_charset);

	g_string_append (buffer, "<tr class=\"header\">");
	if (direction == GTK_TEXT_DIR_RTL)
		g_string_append (buffer, "<td class=\"header rtl\">");
	else
		g_string_append (buffer, "<td class=\"header ltr\">");
	g_string_append (buffer, "<strong>");
	if (subject != NULL && *subject != '\0')
		g_string_append (buffer, subject);
	else
		g_string_append (buffer, _("(no subject)"));
	g_string_append (buffer, "</strong>");
	if (from->len > 0)
		g_string_append_printf (buffer, " (%s)", from->str);
	g_string_append (buffer, "</td></tr>");

	g_string_append (buffer, "</table>");

	g_free (subject);
	if (addrs)
		camel_header_address_list_clear (&addrs);

	g_string_free (from, TRUE);
	g_free (evolution_imagesdir);

	g_object_unref (mime_part);
}

static void
write_contact_picture (CamelMimePart *mime_part,
                       gint size,
                       GString *buffer)
{
	gchar *b64, *content_type;
	CamelDataWrapper *dw;
	CamelContentType *ct;
	GByteArray *ba = NULL;

	dw = camel_medium_get_content (CAMEL_MEDIUM (mime_part));
	if (dw != NULL)
		ba = camel_data_wrapper_get_byte_array (dw);

	if (ba == NULL || ba->len == 0) {
		const gchar *filename;

		filename = camel_mime_part_get_filename (mime_part);

		if (filename != NULL) {
			if (size >= 0) {
				g_string_append_printf (
					buffer,
					"<img width=\"%d\" src=\"evo-file://%s\" />",
					size, filename);
			} else {
				g_string_append_printf (
					buffer,
					"<img src=\"evo-file://%s\" />",
					filename);
			}
		}

		return;
	}

	b64 = g_base64_encode (ba->data, ba->len);
	ct = camel_mime_part_get_content_type (mime_part);
	content_type = camel_content_type_simple (ct);

	if (size >= 0) {
		g_string_append_printf (
			buffer,
			"<img width=\"%d\" src=\"data:%s;base64,%s\">",
			size, content_type, b64);
	} else {
		g_string_append_printf (
			buffer,
			"<img src=\"data:%s;base64,%s\">",
			content_type, b64);
	}

	g_free (b64);
	g_free (content_type);
}

static void
format_full_headers (EMailFormatter *formatter,
                     GString *buffer,
                     EMailPart *part,
                     guint32 mode,
                     guint32 flags,
                     GCancellable *cancellable)
{
	CamelMimePart *mime_part;
	const gchar *charset;
	CamelContentType *ct;
	struct _camel_header_raw *header;
	const gchar *photo_name = NULL;
	guchar *face_header_value = NULL;
	gsize face_header_len = 0;
	gchar *header_sender = NULL, *header_from = NULL, *name;
	gboolean mail_from_delegate = FALSE;
	gboolean show_sender_photo;
	gchar *hdr_charset;
	gchar *evolution_imagesdir;
	const gchar *direction;

	g_return_if_fail (E_IS_MAIL_PART_HEADERS (part));

	if (g_cancellable_is_cancelled (cancellable))
		return;

	mime_part = e_mail_part_ref_mime_part (part);

	switch (gtk_widget_get_default_direction ()) {
		case GTK_TEXT_DIR_RTL:
			direction = "rtl";
			break;
		case GTK_TEXT_DIR_LTR:
			direction = "ltr";
			break;
		default:
			direction = "inherit";
			break;
	}

	ct = camel_mime_part_get_content_type (mime_part);
	charset = camel_content_type_param (ct, "charset");
	charset = camel_iconv_charset_name (charset);
	hdr_charset = e_mail_formatter_dup_charset (formatter);
	if (!hdr_charset)
		hdr_charset = e_mail_formatter_dup_default_charset (formatter);

	evolution_imagesdir = g_filename_to_uri (EVOLUTION_IMAGESDIR, NULL, NULL);

	g_string_append_printf (
		buffer,
		"<table cellspacing=\"0\" cellpadding=\"0\" "
		"border=\"0\" width=\"100%%\" "
		"id=\"__evo-full-headers\" "
		"style=\"display: %s; direction: %s;\">",
		flags & E_MAIL_FORMATTER_HEADER_FLAG_COLLAPSED ? "none" : "table",
		direction);

	header = mime_part->headers;
	while (header != NULL) {
		if (!g_ascii_strcasecmp (header->name, "Sender")) {
			struct _camel_header_address *addrs;
			GString *html;

			if (!(addrs = camel_header_address_decode (header->value, hdr_charset)))
				break;

			html = g_string_new ("");
			name = e_mail_formatter_format_address (
				formatter, html, addrs, header->name, FALSE,
				~(flags & E_MAIL_FORMATTER_HEADER_FLAG_NOELIPSIZE));

			header_sender = html->str;
			camel_header_address_list_clear (&addrs);

			g_string_free (html, FALSE);
			g_free (name);

		} else if (!g_ascii_strcasecmp (header->name, "From")) {
			struct _camel_header_address *addrs;
			GString *html;

			if (!(addrs = camel_header_address_decode (header->value, hdr_charset)))
				break;

			html = g_string_new ("");
			name = e_mail_formatter_format_address (
				formatter, html, addrs, header->name, FALSE,
				!(flags & E_MAIL_FORMATTER_HEADER_FLAG_NOELIPSIZE));

			header_from = html->str;
			camel_header_address_list_clear (&addrs);

			g_string_free (html, FALSE);
			g_free (name);

		} else if (!g_ascii_strcasecmp (header->name, "X-Evolution-Mail-From-Delegate")) {
			mail_from_delegate = TRUE;
		}

		header = header->next;
	}

	g_free (hdr_charset);

	if (header_sender && header_from && mail_from_delegate) {
		gchar *bold_sender, *bold_from;

		g_string_append (
			buffer,
			"<tr valign=\"top\"><td><table border=1 width=\"100%\" "
			"cellspacing=2 cellpadding=2><tr>");
		if (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL)
			g_string_append (
				buffer, "<td align=\"right\" width=\"100%\">");
		else
			g_string_append (
				buffer, "<td align=\"left\" width=\"100%\">");
		bold_sender = g_strconcat ("<b>", header_sender, "</b>", NULL);
		bold_from = g_strconcat ("<b>", header_from, "</b>", NULL);
		/* Translators: This message suggests to the receipients
		 * that the sender of the mail is different from the one
		 * listed in From field. */
		g_string_append_printf (
			buffer,
			_("This message was sent by %s on behalf of %s"),
			bold_sender, bold_from);
		g_string_append (buffer, "</td></tr></table></td></tr>");
		g_free (bold_sender);
		g_free (bold_from);
	}

	g_free (header_sender);
	g_free (header_from);

	g_string_append (
		buffer,
		"<tr valign=\"top\"><td width=\"100%\">"
		"<table class=\"header\">\n");

	g_free (evolution_imagesdir);

	/* dump selected headers */
	if (mode & E_MAIL_FORMATTER_MODE_ALL_HEADERS) {
		header = mime_part->headers;
		while (header != NULL) {
			e_mail_formatter_format_header (
				formatter, buffer,
				header->name,
				header->value,
				E_MAIL_FORMATTER_HEADER_FLAG_NOCOLUMNS, charset);
			header = header->next;
		}
	} else {
		CamelMedium *medium;
		gchar **default_headers;
		guint ii, length = 0;

		medium = CAMEL_MEDIUM (mime_part);

		default_headers =
			e_mail_part_headers_dup_default_headers (
			E_MAIL_PART_HEADERS (part));
		if (default_headers != NULL)
			length = g_strv_length (default_headers);

		for (ii = 0; ii < length; ii++) {
			const gchar *header_name;
			const gchar *header_value = NULL;

			header_name = default_headers[ii];

			/* X-Evolution-Mailer is a pseudo-header and
			 * requires special treatment to extract the
			 * real header value. */
			if (g_ascii_strcasecmp (header_name, "X-Evolution-Mailer") == 0) {
				/* Check for "X-MimeOLE" last,
				 * as it's the least preferred. */
				if (header_value == NULL)
					header_value = camel_medium_get_header (
						medium, "X-Mailer");
				if (header_value == NULL)
					header_value = camel_medium_get_header (
						medium, "User-Agent");
				if (header_value == NULL)
					header_value = camel_medium_get_header (
						medium, "X-Newsreader");
				if (header_value == NULL)
					header_value = camel_medium_get_header (
						medium, "X-MimeOLE");
			} else {
				header_value = camel_medium_get_header (
					medium, header_name);
			}

			if (header_value == NULL)
				continue;

			if (g_ascii_strcasecmp (header_name, "From") == 0)
				photo_name = header_value;

			if (g_ascii_strcasecmp (header_name, "Face") == 0) {
				if (face_header_value == NULL) {
					const gchar *cp = header_value;

					/* Skip over spaces */
					while (*cp == ' ')
						cp++;

					face_header_value = g_base64_decode (
						cp, &face_header_len);
					face_header_value = g_realloc (
						face_header_value,
						face_header_len + 1);
					face_header_value[face_header_len] = 0;
				}
				continue;
			}

			e_mail_formatter_format_header (
				formatter, buffer,
				header_name,
				header_value,
				0, charset);
		}

		g_strfreev (default_headers);
	}

	g_string_append (buffer, "</table></td>");

	show_sender_photo =
		e_mail_formatter_get_show_sender_photo (formatter);

	/* Prefer contact photos over archaic "Face" headers. */
	if (show_sender_photo && photo_name != NULL) {
		gchar *name;

		name = g_uri_escape_string (photo_name, NULL, FALSE);
		g_string_append (
			buffer,
			"<td align=\"right\" valign=\"top\">");
		g_string_append_printf (
			buffer,
			"<img src=\"mail://contact-photo?mailaddr=\" "
			"data-mailaddr=\"%s\" id=\"__evo-contact-photo\"/>",
			name);
		g_string_append (buffer, "</td>");

		g_free (name);

	} else if (face_header_value != NULL) {
		CamelMimePart *image_part;

		image_part = camel_mime_part_new ();
		camel_mime_part_set_content (
			image_part,
			(const gchar *) face_header_value,
			face_header_len, "image/png");

		g_string_append (
			buffer,
			"<td align=\"right\" valign=\"top\">");
		write_contact_picture (image_part, 48, buffer);
		g_string_append (buffer, "</td>");

		g_object_unref (image_part);
	}

	g_string_append (buffer, "</tr></table>");

	g_free (face_header_value);
	g_object_unref (mime_part);
}

static gboolean
emfe_headers_format (EMailFormatterExtension *extension,
                     EMailFormatter *formatter,
                     EMailFormatterContext *context,
                     EMailPart *part,
                     GOutputStream *stream,
                     GCancellable *cancellable)
{
	CamelMimePart *mime_part;
	GString *buffer;
	const gchar *direction;
	gboolean is_collapsable;
	gboolean is_collapsed;

	if (g_cancellable_is_cancelled (cancellable))
		return FALSE;

	mime_part = e_mail_part_ref_mime_part (part);
	if (mime_part == NULL)
		return FALSE;

	switch (gtk_widget_get_default_direction ()) {
		case GTK_TEXT_DIR_RTL:
			direction = "rtl";
			break;
		case GTK_TEXT_DIR_LTR:
			direction = "ltr";
			break;
		default:
			direction = "inherit";
			break;
	}

	is_collapsable =
		(context->flags & E_MAIL_FORMATTER_HEADER_FLAG_COLLAPSABLE);

	is_collapsed =
		(context->flags & E_MAIL_FORMATTER_HEADER_FLAG_COLLAPSED);

	buffer = g_string_new ("");

	g_string_append_printf (
		buffer,
		"%s id=\"%s\"><table class=\"-e-mail-formatter-header-color\" border=\"0\" width=\"100%%\" "
		"style=\"direction: %s\">"
		"<tr>",
		(context->mode != E_MAIL_FORMATTER_MODE_PRINTING) ?
			"<div class=\"headers -e-mail-formatter-body-color\"" :
			"<div class=\"headers\" style=\"background-color: #ffffff;\"",
		e_mail_part_get_id (part),
		direction);

	if (is_collapsable)
		g_string_append_printf (
			buffer,
			"<td valign=\"top\" width=\"16\">"
			"<img src=\"evo-file://%s/%s\" class=\"navigable\" "
			"     id=\"__evo-collapse-headers-img\" />"
			"</td>",
			EVOLUTION_IMAGESDIR,
			is_collapsed ? "plus.png" : "minus.png");

	g_string_append (buffer, "<td>");

	if (is_collapsable)
		format_short_headers (
			formatter,
			buffer, part,
			context->flags,
			cancellable);

	format_full_headers (
		formatter,
		buffer, part,
		context->mode,
		context->flags,
		cancellable);

	g_string_append (buffer, "</td>");

	g_string_append (buffer, "</tr></table></div>");

	g_output_stream_write_all (
		stream, buffer->str, buffer->len, NULL, cancellable, NULL);

	g_string_free (buffer, TRUE);

	g_object_unref (mime_part);

	return TRUE;
}

static void
e_mail_formatter_headers_class_init (EMailFormatterExtensionClass *class)
{
	class->mime_types = formatter_mime_types;
	class->priority = G_PRIORITY_LOW;
	class->format = emfe_headers_format;
}

static void
e_mail_formatter_headers_init (EMailFormatterExtension *extension)
{
}
