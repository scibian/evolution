/*
 * Code for checking for duplicates when doing EContact work.
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
 *
 * Authors:
 *		Christopher James Lahey <clahey@ximian.com>
 *		Chris Toshok <toshok@ximian.com>
 *
 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eab-contact-merging.h"
#include "eab-contact-compare.h"
#include <gtk/gtk.h>
#include <string.h>
#include "addressbook/gui/widgets/eab-contact-display.h"
#include "e-util/e-util.h"
#include "e-util/e-util-private.h"
#include <glib/gi18n.h>

#include <camel/camel.h>

typedef struct dropdown_data dropdown_data;
typedef enum {
	E_CONTACT_MERGING_ADD,
	E_CONTACT_MERGING_COMMIT,
	E_CONTACT_MERGING_FIND
} EContactMergingOpType;

typedef struct {
	EContactMergingOpType op;
	ESourceRegistry *registry;
	EBookClient *book_client;
	/*contact is the new contact which the user has tried to add to the addressbook*/
	EContact *contact;
	/*match is the duplicate contact already existing in the addressbook*/
	EContact *match;
	GList *avoid;
	EABMergingAsyncCallback cb;
	EABMergingIdAsyncCallback id_cb;
	EABMergingContactAsyncCallback c_cb;
	gpointer closure;
} EContactMergingLookup;

struct dropdown_data {
	EContact *match;
	EContactField field;

	/* for E_CONTACT_EMAIL field */
	GList *email_attr_list_item;
	EVCardAttribute *email_attr;
};
static void match_query_callback (EContact *contact, EContact *match, EABContactMatchType type, gpointer closure);

#define SIMULTANEOUS_MERGING_REQUESTS 20

static GList *merging_queue = NULL;
static gint running_merge_requests = 0;

static void
add_lookup (EContactMergingLookup *lookup)
{
	if (running_merge_requests < SIMULTANEOUS_MERGING_REQUESTS) {
		running_merge_requests++;
		eab_contact_locate_match_full (
			lookup->registry, lookup->book_client,
			lookup->contact, lookup->avoid,
			match_query_callback, lookup);
	}
	else {
		merging_queue = g_list_append (merging_queue, lookup);
	}
}

static void
finished_lookup (void)
{
	running_merge_requests--;

	while (running_merge_requests < SIMULTANEOUS_MERGING_REQUESTS) {
		EContactMergingLookup *lookup;

		if (!merging_queue)
			break;

		lookup = merging_queue->data;

		merging_queue = g_list_remove_link (merging_queue, merging_queue);

		running_merge_requests++;
		eab_contact_locate_match_full (
			lookup->registry, lookup->book_client,
			lookup->contact, lookup->avoid,
			match_query_callback, lookup);
	}
}

static void
free_lookup (EContactMergingLookup *lookup)
{
	g_object_unref (lookup->registry);
	g_object_unref (lookup->book_client);
	g_object_unref (lookup->contact);
	g_list_free (lookup->avoid);
	if (lookup->match)
		g_object_unref (lookup->match);
	g_free (lookup);
}

static void
final_id_cb (EBookClient *book_client,
             const GError *error,
             const gchar *id,
             gpointer closure)
{
	EContactMergingLookup *lookup = closure;

	if (lookup->id_cb)
		lookup->id_cb (
			lookup->book_client,
			error, id, lookup->closure);

	free_lookup (lookup);

	finished_lookup ();
}

static void
final_cb_as_id (EBookClient *book_client,
                const GError *error,
                gpointer closure)
{
	EContactMergingLookup *lookup = closure;

	if (lookup->id_cb)
		lookup->id_cb (
			lookup->book_client,
			error, lookup->contact ?
				e_contact_get_const (
				lookup->contact, E_CONTACT_UID) : NULL,
			lookup->closure);

	free_lookup (lookup);

	finished_lookup ();
}

static void
final_cb (EBookClient *book_client,
          const GError *error,
          gpointer closure)
{
	EContactMergingLookup *lookup = closure;

	if (lookup->cb)
		lookup->cb (lookup->book_client, error, lookup->closure);

	free_lookup (lookup);

	finished_lookup ();
}

static void
modify_contact_ready_cb (GObject *source_object,
                         GAsyncResult *result,
                         gpointer user_data)
{
	EBookClient *book_client = E_BOOK_CLIENT (source_object);
	EContactMergingLookup *lookup = user_data;
	GError *error = NULL;

	g_return_if_fail (book_client != NULL);
	g_return_if_fail (lookup != NULL);

	e_book_client_modify_contact_finish (book_client, result, &error);

	if (lookup->op == E_CONTACT_MERGING_ADD)
		final_cb_as_id (book_client, error, lookup);
	else
		final_cb (book_client, error, lookup);

	if (error != NULL)
		g_error_free (error);
}

static void
add_contact_ready_cb (GObject *source_object,
                      GAsyncResult *result,
                      gpointer user_data)
{
	EBookClient *book_client = E_BOOK_CLIENT (source_object);
	EContactMergingLookup *lookup = user_data;
	gchar *uid = NULL;
	GError *error = NULL;

	g_return_if_fail (book_client != NULL);
	g_return_if_fail (lookup != NULL);

	e_book_client_add_contact_finish (book_client, result, &uid, &error);

	final_id_cb (book_client, error, uid, lookup);

	if (error != NULL)
		g_error_free (error);
}

static void
doit (EContactMergingLookup *lookup,
      gboolean force_modify)
{
	if (lookup->op == E_CONTACT_MERGING_ADD) {
		if (force_modify)
			e_book_client_modify_contact (lookup->book_client, lookup->contact, NULL, modify_contact_ready_cb, lookup);
		else
			e_book_client_add_contact (lookup->book_client, lookup->contact, NULL, add_contact_ready_cb, lookup);
	} else if (lookup->op == E_CONTACT_MERGING_COMMIT)
		e_book_client_modify_contact (lookup->book_client, lookup->contact, NULL, modify_contact_ready_cb, lookup);
}

static void
cancelit (EContactMergingLookup *lookup)
{
	GError *error;

	error = g_error_new_literal (
		G_IO_ERROR, G_IO_ERROR_CANCELLED, _("Cancelled"));

	if (lookup->op == E_CONTACT_MERGING_ADD) {
		final_id_cb (lookup->book_client, error, NULL, lookup);
	} else if (lookup->op == E_CONTACT_MERGING_COMMIT) {
		final_cb (lookup->book_client, error, lookup);
	}

	g_error_free (error);
}

static void
dialog_map (GtkWidget *window,
            GdkEvent *event,
            GtkWidget *table)
{
	GtkAllocation allocation;
	gint h, w;

	gtk_widget_get_allocation (table, &allocation);

	/* Spacing around the table */
	w = allocation.width + 30;
	/* buttons and outer spacing */
	h = allocation.height + 60;
	if (w > 400)
		w = 400;
	if (h > 450)
		h = 450;

	gtk_widget_set_size_request (window, w, h);
}

static void
dropdown_changed (GtkWidget *dropdown,
                  dropdown_data *data)
{
	gchar *str = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (dropdown));

	if (str && *str)
		e_contact_set (data->match, data->field, str);
	else
		e_contact_set (data->match, data->field, NULL);

	g_free (str);
}

static void
email_dropdown_changed (GtkWidget *dropdown,
                        dropdown_data *data)
{
	gchar *str = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (dropdown));

	if (str && *str)
		data->email_attr_list_item->data = data->email_attr;
	else
		data->email_attr_list_item->data = NULL;

	g_free (str);
}

static void
remove_contact_ready_cb (GObject *source_object,
                         GAsyncResult *result,
                         gpointer user_data)
{
	EBookClient *book_client = E_BOOK_CLIENT (source_object);
	EContactMergingLookup *lookup = user_data;
	GError *error = NULL;

	g_return_if_fail (book_client != NULL);
	g_return_if_fail (lookup != NULL);

	e_book_client_remove_contact_finish (book_client, result, &error);

	if (error != NULL) {
		g_warning (
			"%s: Failed to remove contact: %s",
			G_STRFUNC, error->message);
		g_error_free (error);
	}

	e_book_client_add_contact (
		book_client, lookup->contact, NULL,
		add_contact_ready_cb, lookup);
}

static gint
mergeit (EContactMergingLookup *lookup)
{
	GtkWidget *scrolled_window, *label, *hbox, *dropdown;
	GtkWidget *content_area;
	GtkWidget *dialog;
	GtkTable *table;
	EContactField field;
	gchar *string = NULL, *string1 = NULL;
	GList *match_email_attr_list, *contact_email_attr_list, *miter, *citer, *use_email_attr_list;
	GHashTable *match_emails; /* emails in the 'match' contact */
	gint row = -1;
	gint value = 0, result;

	dialog = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog), _("Merge Contact"));
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	table = (GtkTable *) gtk_table_new (20, 2, FALSE);
	gtk_container_set_border_width ((GtkContainer *) table, 12);
	gtk_table_set_row_spacings (table, 6);
	gtk_table_set_col_spacings (table, 2);

	gtk_dialog_add_buttons (
		GTK_DIALOG (dialog),
		_("_Cancel"), GTK_RESPONSE_CANCEL,
		_("_Merge"), GTK_RESPONSE_OK,
		NULL);

	match_emails = g_hash_table_new_full (camel_strcase_hash, camel_strcase_equal, g_free, NULL);
	match_email_attr_list = e_contact_get_attributes (lookup->match, E_CONTACT_EMAIL);
	contact_email_attr_list = e_contact_get_attributes (lookup->contact, E_CONTACT_EMAIL);
	use_email_attr_list = NULL;

	for (miter = match_email_attr_list; miter; miter = g_list_next (miter)) {
		EVCardAttribute *attr = miter->data;
		gchar *email;

		email = e_vcard_attribute_get_value (attr);
		if (email && *email) {
			g_hash_table_insert (match_emails, email, attr);
			use_email_attr_list = g_list_prepend (use_email_attr_list, attr);
		} else {
			g_free (email);
		}
	}

	use_email_attr_list = g_list_reverse (use_email_attr_list);

	/*we match all the string fields of the already existing contact and the new contact.*/
	for (field = E_CONTACT_FULL_NAME; field != (E_CONTACT_LAST_SIMPLE_STRING -1); field++) {
		dropdown_data *data = NULL;
		string = (gchar *) e_contact_get_const (lookup->contact, field);
		string1 = (gchar *) e_contact_get_const (lookup->match, field);

		/*the field must exist in the new as well as the duplicate contact*/
		if (string && *string) {
			if (field == E_CONTACT_EMAIL_1) {
				for (citer = contact_email_attr_list; citer; citer = g_list_next (citer)) {
					EVCardAttribute *attr = citer->data;
					gchar *email;

					email = e_vcard_attribute_get_value (attr);
					if (email && *email) {
						if (!g_hash_table_lookup (match_emails, email)) {
							dropdown_data *data;

							/* the email is not set in both contacts */
							use_email_attr_list = g_list_append (use_email_attr_list, attr);

							row++;
							label = gtk_label_new (_("Email"));
							hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
							gtk_box_pack_start (GTK_BOX (hbox), (GtkWidget *) label, FALSE, FALSE, 0);
							gtk_table_attach_defaults (table, (GtkWidget *) hbox, 0, 1, row, row + 1);

							dropdown = gtk_combo_box_text_new ();
							gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dropdown), email);

							data = g_new0 (dropdown_data, 1);

							gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dropdown), "");

							gtk_combo_box_set_active (GTK_COMBO_BOX (dropdown), 0);
							data->field = E_CONTACT_EMAIL;
							data->match = lookup->match;
							data->email_attr_list_item = g_list_last (use_email_attr_list);
							data->email_attr = attr;

							g_signal_connect (
								dropdown, "changed",
								G_CALLBACK (email_dropdown_changed), data);
							g_object_set_data_full (G_OBJECT (dropdown), "eab-contact-merging::dropdown-data", data, g_free);

							hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
							gtk_box_pack_start (GTK_BOX (hbox), (GtkWidget *) dropdown, FALSE, FALSE, 0);
							gtk_table_attach_defaults (table, (GtkWidget *) hbox, 1, 2, row, row + 1);
							gtk_widget_show ((GtkWidget *) dropdown);
						}
					}
					g_free (email);
				}
				continue;
			}

			if (field == E_CONTACT_EMAIL_2 || field == E_CONTACT_EMAIL_3 || field == E_CONTACT_EMAIL_4) {
				/* emails are compared above */
				continue;
			}

			if (((field == E_CONTACT_FULL_NAME) && (!g_ascii_strcasecmp (string, string1)))) {
				row++;
				label = gtk_label_new (e_contact_pretty_name (field));
				hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_box_pack_start (GTK_BOX (hbox), (GtkWidget *) label, FALSE, FALSE, 0);
				gtk_table_attach_defaults (table, (GtkWidget *) hbox, 0, 1, row, row + 1);

				label = gtk_label_new (string);
				hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_box_pack_start (GTK_BOX (hbox), (GtkWidget *) label, FALSE, FALSE, 0);
				gtk_table_attach_defaults (table, (GtkWidget *) hbox, 1, 2, row, row + 1);
				continue;
			}

			/*for all string fields except name and email*/
			if (!(string1 && *string1) || (g_ascii_strcasecmp (string, string1))) {
				row++;
				label = gtk_label_new (e_contact_pretty_name (field));
				hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_box_pack_start (GTK_BOX (hbox), (GtkWidget *) label, FALSE, FALSE, 0);
				gtk_table_attach_defaults (table, (GtkWidget *) hbox, 0, 1, row, row + 1);
				data = g_new0 (dropdown_data, 1);
				dropdown = gtk_combo_box_text_new ();
				gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dropdown), string);
				e_contact_set (lookup->match, field, string);

				if (string1 && *string1)
					gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dropdown), string1);
				else
					gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dropdown), "");

				gtk_combo_box_set_active (GTK_COMBO_BOX (dropdown), 0);
				data->field = field;
				data->match = lookup->match;

				if (field == E_CONTACT_NICKNAME || field == E_CONTACT_GIVEN_NAME)
					gtk_widget_set_sensitive ((GtkWidget *) dropdown, FALSE);

				g_signal_connect (
					dropdown, "changed",
					G_CALLBACK (dropdown_changed), data);
				g_object_set_data_full (G_OBJECT (dropdown), "eab-contact-merging::dropdown-data", data, g_free);

				hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
				gtk_box_pack_start (GTK_BOX (hbox), (GtkWidget *) dropdown, FALSE, FALSE, 0);
				gtk_table_attach_defaults (table, (GtkWidget *) hbox, 1, 2, row, row + 1);
				gtk_widget_show_all ((GtkWidget *) dropdown);
			}
		}
	}

	gtk_window_set_default_size (GTK_WINDOW (dialog), 420, 300);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (table));
	gtk_box_pack_start (GTK_BOX (content_area), GTK_WIDGET (scrolled_window), TRUE, TRUE, 0);
	gtk_widget_show (scrolled_window);
	g_signal_connect (
		dialog, "map-event",
		G_CALLBACK (dialog_map), table);
	gtk_widget_show_all ((GtkWidget *) table);
	result = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (result) {
	case GTK_RESPONSE_OK:
		citer = NULL;
		for (miter = use_email_attr_list; miter; miter = g_list_next (miter)) {
			if (miter->data)
				citer = g_list_prepend (citer, miter->data);
		}
		citer = g_list_reverse (citer);
		e_contact_set_attributes (lookup->match, E_CONTACT_EMAIL, citer);
		g_list_free (citer);

		g_object_unref (lookup->contact);
		lookup->contact = g_object_ref (lookup->match);
		e_book_client_remove_contact (
			lookup->book_client,
			lookup->match, NULL,
			remove_contact_ready_cb, lookup);
		value = 1;
		break;
	case GTK_RESPONSE_CANCEL:
	default:
		value = 0;
		break;
	}
	gtk_widget_destroy (dialog);
	g_list_free_full (match_email_attr_list, (GDestroyNotify) e_vcard_attribute_free);
	g_list_free_full (contact_email_attr_list, (GDestroyNotify) e_vcard_attribute_free);
	g_list_free (use_email_attr_list);
	g_hash_table_destroy (match_emails);

	return value;
}

static gboolean
check_if_same (EContact *contact,
               EContact *match)
{
	EContactField field;
	GList *email_attr_list;
	gint num_of_email;
	gchar *str = NULL, *string = NULL, *string1 = NULL;
	gboolean res = TRUE;

	email_attr_list = e_contact_get_attributes (match, E_CONTACT_EMAIL);
	num_of_email = g_list_length (email_attr_list);

	for (field = E_CONTACT_FULL_NAME; res && field != (E_CONTACT_LAST_SIMPLE_STRING -1); field++) {

		if ((field == E_CONTACT_EMAIL_1 || field == E_CONTACT_EMAIL_2
		     || field == E_CONTACT_EMAIL_3 || field == E_CONTACT_EMAIL_4) && (num_of_email < 4)) {
			str = (gchar *) e_contact_get_const (contact, field);
			switch (num_of_email)
			{
			case 0:
				res = FALSE;
				break;
			case 1:
				if ((str && *str) && (g_ascii_strcasecmp (e_contact_get_const (match, E_CONTACT_EMAIL_1),str)))
					res = FALSE;
				break;
			case 2:
				if ((str && *str) && (g_ascii_strcasecmp (str,e_contact_get_const (match, E_CONTACT_EMAIL_1))) &&
						(g_ascii_strcasecmp (e_contact_get_const (match, E_CONTACT_EMAIL_2),str)))
					res = FALSE;
				break;
			case 3:
				if ((str && *str) && (g_ascii_strcasecmp (e_contact_get_const (match, E_CONTACT_EMAIL_1),str)) &&
						(g_ascii_strcasecmp (e_contact_get_const (match, E_CONTACT_EMAIL_2),str)) &&
						(g_ascii_strcasecmp (e_contact_get_const (match, E_CONTACT_EMAIL_3),str)))
					res = FALSE;
				break;
			}
		}
		else {
			string = (gchar *) e_contact_get_const (contact, field);
			string1 = (gchar *) e_contact_get_const (match, field);
			if ((string && *string) && (string1 && *string1) && (g_ascii_strcasecmp (string1, string))) {
				res = FALSE;
				break;
			/*if the field entry exist in either of the contacts,we'll have to give the choice and thus merge button should be sensitive*/
			} else if ((string && *string) && !(string1 && *string1)) {
				res = FALSE;
				break;
			}
		}
	}

	g_list_free_full (email_attr_list, (GDestroyNotify) e_vcard_attribute_free);

	return res;
}

static GtkWidget *
create_duplicate_contact_detected_dialog (EContact *old_contact,
                                          EContact *new_contact,
                                          gboolean disable_merge,
                                          gboolean is_for_commit)
{
	GtkWidget *widget, *scrolled;
	GtkContainer *container;
	GtkDialog *dialog;
	const gchar *text;

	widget = gtk_dialog_new ();
	dialog = GTK_DIALOG (widget);

	g_object_set (
		G_OBJECT (dialog),
		"title", _("Duplicate Contact Detected"),
		"default-width", 500,
		"default-height", 400,
		NULL);

	gtk_dialog_add_action_widget (dialog, e_dialog_button_new_with_icon ("process-stop", _("_Cancel")), GTK_RESPONSE_CANCEL);

	if (is_for_commit) {
		gtk_dialog_add_action_widget (dialog, e_dialog_button_new_with_icon ("document-save", _("_Save")), GTK_RESPONSE_OK);
	} else {
		gtk_dialog_add_action_widget (dialog, e_dialog_button_new_with_icon ("list-add", _("_Add")), GTK_RESPONSE_OK);
		gtk_dialog_add_action_widget (dialog, e_dialog_button_new_with_icon (NULL, _("_Merge")), GTK_RESPONSE_APPLY);
	}

	if (disable_merge)
		gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_APPLY, FALSE);

	container = GTK_CONTAINER (gtk_dialog_get_content_area (dialog));

	widget = gtk_grid_new ();
	g_object_set (
		G_OBJECT (widget),
		"orientation", GTK_ORIENTATION_HORIZONTAL,
		"hexpand", TRUE,
		"halign", GTK_ALIGN_FILL,
		"vexpand", TRUE,
		"valign", GTK_ALIGN_FILL,
		"margin", 12,
		NULL);

	gtk_container_add (container, widget);
	container = GTK_CONTAINER (widget);

	widget = gtk_image_new_from_icon_name ("avatar-default", GTK_ICON_SIZE_BUTTON);
	g_object_set (
		G_OBJECT (widget),
		"hexpand", FALSE,
		"halign", GTK_ALIGN_START,
		"vexpand", FALSE,
		"valign", GTK_ALIGN_START,
		"margin-right", 12,
		NULL);
	gtk_container_add (container, widget);

	widget = gtk_grid_new ();
	g_object_set (
		G_OBJECT (widget),
		"orientation", GTK_ORIENTATION_VERTICAL,
		"hexpand", TRUE,
		"halign", GTK_ALIGN_FILL,
		"vexpand", TRUE,
		"valign", GTK_ALIGN_FILL,
		NULL);

	gtk_container_add (container, widget);
	container = GTK_CONTAINER (widget);

	if (is_for_commit)
		text = _("The name or email address of this contact already exists\n"
			 "in this folder. Would you like to save the changes anyway?");
	else
		text = _("The name or email address of this contact already exists\n"
			 "in this folder. Would you like to add it anyway?");

	widget = gtk_label_new (text);
	g_object_set (
		G_OBJECT (widget),
		"hexpand", FALSE,
		"halign", GTK_ALIGN_START,
		"vexpand", FALSE,
		"valign", GTK_ALIGN_START,
		"margin-bottom", 6,
		NULL);
	gtk_container_add (container, widget);

	if (is_for_commit)
		text = _("Changed Contact:");
	else
		text = _("New Contact:");

	widget = gtk_label_new (text);
	g_object_set (
		G_OBJECT (widget),
		"hexpand", FALSE,
		"halign", GTK_ALIGN_START,
		"vexpand", FALSE,
		"valign", GTK_ALIGN_START,
		"margin-bottom", 6,
		NULL);
	gtk_container_add (container, widget);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	g_object_set (
		G_OBJECT (scrolled),
		"hexpand", TRUE,
		"halign", GTK_ALIGN_FILL,
		"hscrollbar-policy", GTK_POLICY_AUTOMATIC,
		"vexpand", TRUE,
		"valign", GTK_ALIGN_FILL,
		"vscrollbar-policy", GTK_POLICY_AUTOMATIC,
		"margin-bottom", 6,
		NULL);
	gtk_container_add (container, scrolled);

	widget = eab_contact_display_new ();
	g_object_set (
		G_OBJECT (widget),
		"hexpand", TRUE,
		"halign", GTK_ALIGN_FILL,
		"vexpand", TRUE,
		"valign", GTK_ALIGN_FILL,
		"contact", new_contact,
		"mode", EAB_CONTACT_DISPLAY_RENDER_COMPACT,
		NULL);
	gtk_container_add (GTK_CONTAINER (scrolled), widget);

	if (is_for_commit)
		text = _("Conflicting Contact:");
	else
		text = _("Old Contact:");

	widget = gtk_label_new (text);
	g_object_set (
		G_OBJECT (widget),
		"hexpand", FALSE,
		"halign", GTK_ALIGN_START,
		"vexpand", FALSE,
		"valign", GTK_ALIGN_START,
		"margin-bottom", 6,
		NULL);
	gtk_container_add (container, widget);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	g_object_set (
		G_OBJECT (scrolled),
		"hexpand", TRUE,
		"halign", GTK_ALIGN_FILL,
		"hscrollbar-policy", GTK_POLICY_AUTOMATIC,
		"vexpand", TRUE,
		"valign", GTK_ALIGN_FILL,
		"vscrollbar-policy", GTK_POLICY_AUTOMATIC,
		NULL);
	gtk_container_add (container, scrolled);

	widget = eab_contact_display_new ();
	g_object_set (
		G_OBJECT (widget),
		"hexpand", TRUE,
		"halign", GTK_ALIGN_FILL,
		"vexpand", TRUE,
		"valign", GTK_ALIGN_FILL,
		"contact", old_contact,
		"mode", EAB_CONTACT_DISPLAY_RENDER_COMPACT,
		NULL);
	gtk_container_add (GTK_CONTAINER (scrolled), widget);

	gtk_widget_show_all (gtk_dialog_get_content_area (dialog));

	return GTK_WIDGET (dialog);
}

static void
response (GtkWidget *dialog,
          gint response,
          EContactMergingLookup *lookup)
{
	switch (response) {
	case GTK_RESPONSE_OK:
		doit (lookup, FALSE);
		break;
	case GTK_RESPONSE_CANCEL:
		cancelit (lookup);
		break;
	case GTK_RESPONSE_APPLY:
		if (mergeit (lookup))
			break;
		return;
	case GTK_RESPONSE_DELETE_EVENT:
		cancelit (lookup);
		break;
	default:
		g_warn_if_reached ();
		break;
	}

	gtk_widget_destroy (dialog);
}

static void
match_query_callback (EContact *contact,
                      EContact *match,
                      EABContactMatchType type,
                      gpointer closure)
{
	EContactMergingLookup *lookup = closure;
	gboolean flag;
	gboolean same_uids;

	if (lookup->op == E_CONTACT_MERGING_FIND) {
		if (lookup->c_cb)
			lookup->c_cb (
				lookup->book_client, NULL,
				(gint) type <= (gint)
				EAB_CONTACT_MATCH_VAGUE ? NULL : match,
				lookup->closure);

		free_lookup (lookup);
		finished_lookup ();
		return;
	}

	/* if had same UID, then we are editing old contact, thus force commit change to it */
	same_uids = contact && match
		&& e_contact_get_const (contact, E_CONTACT_UID)
		&& e_contact_get_const (match, E_CONTACT_UID)
		&& g_str_equal (e_contact_get_const (contact, E_CONTACT_UID), e_contact_get_const (match, E_CONTACT_UID));

	if ((gint) type <= (gint) EAB_CONTACT_MATCH_VAGUE || same_uids) {
		doit (lookup, same_uids);
	} else {
		GtkWidget *dialog;

		lookup->match = g_object_ref (match);
		if (lookup->op == E_CONTACT_MERGING_ADD) {
			/* Compares all the values of contacts and return true, if they match */
			flag = check_if_same (contact, match);
			dialog = create_duplicate_contact_detected_dialog (match, contact, flag, FALSE);
		} else if (lookup->op == E_CONTACT_MERGING_COMMIT) {
			dialog = create_duplicate_contact_detected_dialog (match, contact, FALSE, TRUE);
		} else {
			doit (lookup, FALSE);
			return;
		}

		g_signal_connect (
			dialog, "response",
			G_CALLBACK (response), lookup);

		gtk_widget_show_all (dialog);
	}
}

gboolean
eab_merging_book_add_contact (ESourceRegistry *registry,
                              EBookClient *book_client,
                              EContact *contact,
                              EABMergingIdAsyncCallback cb,
                              gpointer closure)
{
	EContactMergingLookup *lookup;

	g_return_val_if_fail (E_IS_SOURCE_REGISTRY (registry), FALSE);

	lookup = g_new (EContactMergingLookup, 1);

	lookup->op = E_CONTACT_MERGING_ADD;
	lookup->registry = g_object_ref (registry);
	lookup->book_client = g_object_ref (book_client);
	lookup->contact = g_object_ref (contact);
	lookup->id_cb = cb;
	lookup->closure = closure;
	lookup->avoid = NULL;
	lookup->match = NULL;

	add_lookup (lookup);

	return TRUE;
}

gboolean
eab_merging_book_modify_contact (ESourceRegistry *registry,
                                 EBookClient *book_client,
                                 EContact *contact,
                                 EABMergingAsyncCallback cb,
                                 gpointer closure)
{
	EContactMergingLookup *lookup;

	g_return_val_if_fail (E_IS_SOURCE_REGISTRY (registry), FALSE);

	lookup = g_new (EContactMergingLookup, 1);

	lookup->op = E_CONTACT_MERGING_COMMIT;
	lookup->registry = g_object_ref (registry);
	lookup->book_client = g_object_ref (book_client);
	lookup->contact = g_object_ref (contact);
	lookup->cb = cb;
	lookup->closure = closure;
	lookup->avoid = g_list_append (NULL, contact);
	lookup->match = NULL;

	add_lookup (lookup);

	return TRUE;
}

gboolean
eab_merging_book_find_contact (ESourceRegistry *registry,
                               EBookClient *book_client,
                               EContact *contact,
                               EABMergingContactAsyncCallback cb,
                               gpointer closure)
{
	EContactMergingLookup *lookup;

	lookup = g_new (EContactMergingLookup, 1);

	lookup->op = E_CONTACT_MERGING_FIND;
	lookup->registry = g_object_ref (registry);
	lookup->book_client = g_object_ref (book_client);
	lookup->contact = g_object_ref (contact);
	lookup->c_cb = cb;
	lookup->closure = closure;
	lookup->avoid = g_list_append (NULL, contact);
	lookup->match = NULL;

	add_lookup (lookup);

	return TRUE;
}
