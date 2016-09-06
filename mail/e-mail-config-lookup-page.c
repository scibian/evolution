/*
 * e-mail-config-lookup-page.c
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

#include <glib/gi18n-lib.h>

#include "e-util/e-util.h"

#include "e-mail-config-lookup-page.h"

/* Forward Declarations */
static void	e_mail_config_lookup_page_interface_init
					(EMailConfigPageInterface *iface);

G_DEFINE_TYPE_WITH_CODE (
	EMailConfigLookupPage,
	e_mail_config_lookup_page,
	GTK_TYPE_BOX,
	G_IMPLEMENT_INTERFACE (
		E_TYPE_MAIL_CONFIG_PAGE,
		e_mail_config_lookup_page_interface_init))

static void
mail_config_lookup_page_constructed (GObject *object)
{
	EMailConfigLookupPage *page;
	GtkWidget *container;
	GtkWidget *widget;
	const gchar *text;

	page = E_MAIL_CONFIG_LOOKUP_PAGE (object);

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (e_mail_config_lookup_page_parent_class)->constructed (object);

	gtk_orientable_set_orientation (
		GTK_ORIENTABLE (page), GTK_ORIENTATION_VERTICAL);

	gtk_box_set_spacing (GTK_BOX (page), 12);

	gtk_widget_set_valign (GTK_WIDGET (page), GTK_ALIGN_FILL);

	widget = gtk_alignment_new (0.5, 0.5, 0.5, 0.5);
	gtk_box_pack_start (GTK_BOX (page), widget, TRUE, TRUE, 0);
	gtk_widget_show (widget);

	container = widget;

	widget = e_spinner_new ();
	e_spinner_start (E_SPINNER (widget));
	gtk_container_add (GTK_CONTAINER (container), widget);
	gtk_widget_show (widget);

	text = _("Looking up account details...");
	widget = gtk_label_new (text);
	gtk_box_pack_start (GTK_BOX (page), widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);
}

static gboolean
mail_config_lookup_page_check_complete (EMailConfigPage *page)
{
	return FALSE;
}

static void
e_mail_config_lookup_page_class_init (EMailConfigLookupPageClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->constructed = mail_config_lookup_page_constructed;
}

static void
e_mail_config_lookup_page_interface_init (EMailConfigPageInterface *iface)
{
	/* Do not set a title.  We don't want this
	 * page listed in a GtkAssistant sidebar. */
	iface->title = "";
	iface->sort_order = E_MAIL_CONFIG_LOOKUP_PAGE_SORT_ORDER;
	iface->page_type = GTK_ASSISTANT_PAGE_CUSTOM;
	iface->check_complete = mail_config_lookup_page_check_complete;
}

static void
e_mail_config_lookup_page_init (EMailConfigLookupPage *page)
{
}

EMailConfigPage *
e_mail_config_lookup_page_new ()
{
	return g_object_new (E_TYPE_MAIL_CONFIG_LOOKUP_PAGE, NULL);
}
