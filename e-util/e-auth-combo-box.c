/*
 * e-auth-combo-box.c
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

#include "e-auth-combo-box.h"

#define E_AUTH_COMBO_BOX_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), E_TYPE_AUTH_COMBO_BOX, EAuthComboBoxPrivate))

struct _EAuthComboBoxPrivate {
	CamelProvider *provider;
};

enum {
	PROP_0,
	PROP_PROVIDER
};

enum {
	COLUMN_MECHANISM,
	COLUMN_DISPLAY_NAME,
	COLUMN_STRIKETHROUGH,
	COLUMN_AUTHTYPE,
	NUM_COLUMNS
};

G_DEFINE_TYPE (
	EAuthComboBox,
	e_auth_combo_box,
	GTK_TYPE_COMBO_BOX)

static void
auth_combo_box_rebuild_model (EAuthComboBox *combo_box)
{
	GtkComboBox *gtk_combo_box;
	CamelProvider *provider;
	GtkTreeModel *model;
	GList *link;
	const gchar *active_id;

	provider = e_auth_combo_box_get_provider (combo_box);

	gtk_combo_box = GTK_COMBO_BOX (combo_box);
	model = gtk_combo_box_get_model (gtk_combo_box);
	active_id = gtk_combo_box_get_active_id (gtk_combo_box);

	gtk_list_store_clear (GTK_LIST_STORE (model));

	if (provider == NULL)
		return;

	for (link = provider->authtypes; link != NULL; link = link->next) {
		CamelServiceAuthType *authtype = link->data;
		GtkTreeIter iter;

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);

		gtk_list_store_set (
			GTK_LIST_STORE (model), &iter,
			COLUMN_MECHANISM, authtype->authproto,
			COLUMN_DISPLAY_NAME, authtype->name,
			COLUMN_AUTHTYPE, authtype,
			-1);
	}

	/* Try selecting the previous mechanism. */
	if (active_id != NULL)
		gtk_combo_box_set_active_id (gtk_combo_box, active_id);

	/* Or else fall back to the first mechanism. */
	if (gtk_combo_box_get_active (gtk_combo_box) == -1)
		gtk_combo_box_set_active (gtk_combo_box, 0);
}

static void
auth_combo_box_set_property (GObject *object,
                             guint property_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_PROVIDER:
			e_auth_combo_box_set_provider (
				E_AUTH_COMBO_BOX (object),
				g_value_get_pointer (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
auth_combo_box_get_property (GObject *object,
                             guint property_id,
                             GValue *value,
                             GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_PROVIDER:
			g_value_set_pointer (
				value,
				e_auth_combo_box_get_provider (
				E_AUTH_COMBO_BOX (object)));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
auth_combo_box_constructed (GObject *object)
{
	GtkComboBox *combo_box;
	GtkListStore *list_store;
	GtkCellLayout *cell_layout;
	GtkCellRenderer *cell_renderer;

	/* Chain up to parent's constructed() method. */
	G_OBJECT_CLASS (e_auth_combo_box_parent_class)->constructed (object);

	list_store = gtk_list_store_new (
		NUM_COLUMNS,
		G_TYPE_STRING,		/* COLUMN_MECHANISM */
		G_TYPE_STRING,		/* COLUMN_DISPLAY_NAME */
		G_TYPE_BOOLEAN,		/* COLUMN_STRIKETHROUGH */
		G_TYPE_POINTER);	/* COLUMN_AUTHTYPE */

	combo_box = GTK_COMBO_BOX (object);
	gtk_combo_box_set_model (combo_box, GTK_TREE_MODEL (list_store));
	gtk_combo_box_set_id_column (combo_box, COLUMN_MECHANISM);
	g_object_unref (list_store);

	cell_layout = GTK_CELL_LAYOUT (object);
	cell_renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (cell_layout, cell_renderer, TRUE);

	gtk_cell_layout_set_attributes (
		cell_layout, cell_renderer,
		"text", COLUMN_DISPLAY_NAME,
		"strikethrough", COLUMN_STRIKETHROUGH,
		NULL);
}

static void
e_auth_combo_box_class_init (EAuthComboBoxClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (EAuthComboBoxPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = auth_combo_box_set_property;
	object_class->get_property = auth_combo_box_get_property;
	object_class->constructed = auth_combo_box_constructed;

	g_object_class_install_property (
		object_class,
		PROP_PROVIDER,
		g_param_spec_pointer (
			"provider",
			"Provider",
			"The provider to query for auth mechanisms",
			G_PARAM_READWRITE |
			G_PARAM_STATIC_STRINGS));
}

static void
e_auth_combo_box_init (EAuthComboBox *combo_box)
{
	combo_box->priv = E_AUTH_COMBO_BOX_GET_PRIVATE (combo_box);
}

GtkWidget *
e_auth_combo_box_new (void)
{
	return g_object_new (E_TYPE_AUTH_COMBO_BOX, NULL);
}

CamelProvider *
e_auth_combo_box_get_provider (EAuthComboBox *combo_box)
{
	g_return_val_if_fail (E_IS_AUTH_COMBO_BOX (combo_box), NULL);

	return combo_box->priv->provider;
}

void
e_auth_combo_box_set_provider (EAuthComboBox *combo_box,
                               CamelProvider *provider)
{
	g_return_if_fail (E_IS_AUTH_COMBO_BOX (combo_box));

	if (provider == combo_box->priv->provider)
		return;

	combo_box->priv->provider = provider;

	g_object_notify (G_OBJECT (combo_box), "provider");

	auth_combo_box_rebuild_model (combo_box);
}

void
e_auth_combo_box_update_available (EAuthComboBox *combo_box,
                                   GList *available_authtypes)
{
	GtkComboBox *gtk_combo_box;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gint active_index;
	gint available_index = -1;
	gint index = 0;
	gboolean iter_set;

	g_return_if_fail (E_IS_AUTH_COMBO_BOX (combo_box));

	gtk_combo_box = GTK_COMBO_BOX (combo_box);
	model = gtk_combo_box_get_model (gtk_combo_box);
	active_index = gtk_combo_box_get_active (gtk_combo_box);

	iter_set = gtk_tree_model_get_iter_first (model, &iter);

	while (iter_set) {
		CamelServiceAuthType *authtype;
		gboolean available;

		gtk_tree_model_get (
			model, &iter, COLUMN_AUTHTYPE, &authtype, -1);

		available = (g_list_find (
			available_authtypes, authtype) != NULL);

		gtk_list_store_set (
			GTK_LIST_STORE (model), &iter,
			COLUMN_STRIKETHROUGH, !available, -1);

		if (index == active_index && !available)
			active_index = -1;

		if (available && available_index == -1)
			available_index = index;

		iter_set = gtk_tree_model_iter_next (model, &iter);
		index++;
	}

	/* If the active combo_box item turned out to be unavailable
	 * (or there was no active item), select the first available. */
	if (active_index == -1 && available_index != -1)
		gtk_combo_box_set_active (gtk_combo_box, available_index);
}
