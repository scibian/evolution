
/* Generated data (by glib-mkenums) */

#include "e-mail-formatter-enumtypes.h"
/* enumerations from "./e-mail-formatter-enums.h" */
#include "./e-mail-formatter-enums.h"

GType
e_mail_formatter_color_get_type (void)
{
	static volatile gsize the_type__volatile = 0;

	if (g_once_init_enter (&the_type__volatile)) {
		static const GEnumValue values[] = {
			{ E_MAIL_FORMATTER_COLOR_BODY,
			  "E_MAIL_FORMATTER_COLOR_BODY",
			  "body" },
			{ E_MAIL_FORMATTER_COLOR_CITATION,
			  "E_MAIL_FORMATTER_COLOR_CITATION",
			  "citation" },
			{ E_MAIL_FORMATTER_COLOR_CONTENT,
			  "E_MAIL_FORMATTER_COLOR_CONTENT",
			  "content" },
			{ E_MAIL_FORMATTER_COLOR_FRAME,
			  "E_MAIL_FORMATTER_COLOR_FRAME",
			  "frame" },
			{ E_MAIL_FORMATTER_COLOR_HEADER,
			  "E_MAIL_FORMATTER_COLOR_HEADER",
			  "header" },
			{ E_MAIL_FORMATTER_COLOR_TEXT,
			  "E_MAIL_FORMATTER_COLOR_TEXT",
			  "text" },
			{ 0, NULL, NULL }
		};
		GType the_type = g_enum_register_static (
			g_intern_static_string ("EMailFormatterColor"),
			values);
		g_once_init_leave (&the_type__volatile, the_type);
	}
	return the_type__volatile;
}

GType
e_mail_formatter_header_flags_get_type (void)
{
	static volatile gsize the_type__volatile = 0;

	if (g_once_init_enter (&the_type__volatile)) {
		static const GFlagsValue values[] = {
			{ E_MAIL_FORMATTER_HEADER_FLAG_COLLAPSABLE,
			  "E_MAIL_FORMATTER_HEADER_FLAG_COLLAPSABLE",
			  "collapsable" },
			{ E_MAIL_FORMATTER_HEADER_FLAG_COLLAPSED,
			  "E_MAIL_FORMATTER_HEADER_FLAG_COLLAPSED",
			  "collapsed" },
			{ E_MAIL_FORMATTER_HEADER_FLAG_HTML,
			  "E_MAIL_FORMATTER_HEADER_FLAG_HTML",
			  "html" },
			{ E_MAIL_FORMATTER_HEADER_FLAG_NOCOLUMNS,
			  "E_MAIL_FORMATTER_HEADER_FLAG_NOCOLUMNS",
			  "nocolumns" },
			{ E_MAIL_FORMATTER_HEADER_FLAG_BOLD,
			  "E_MAIL_FORMATTER_HEADER_FLAG_BOLD",
			  "bold" },
			{ E_MAIL_FORMATTER_HEADER_FLAG_NODEC,
			  "E_MAIL_FORMATTER_HEADER_FLAG_NODEC",
			  "nodec" },
			{ E_MAIL_FORMATTER_HEADER_FLAG_HIDDEN,
			  "E_MAIL_FORMATTER_HEADER_FLAG_HIDDEN",
			  "hidden" },
			{ E_MAIL_FORMATTER_HEADER_FLAG_NOLINKS,
			  "E_MAIL_FORMATTER_HEADER_FLAG_NOLINKS",
			  "nolinks" },
			{ E_MAIL_FORMATTER_HEADER_FLAG_NOELIPSIZE,
			  "E_MAIL_FORMATTER_HEADER_FLAG_NOELIPSIZE",
			  "noelipsize" },
			{ 0, NULL, NULL }
		};
		GType the_type = g_flags_register_static (
			g_intern_static_string ("EMailFormatterHeaderFlags"),
			values);
		g_once_init_leave (&the_type__volatile, the_type);
	}
	return the_type__volatile;
}

GType
e_mail_formatter_mode_get_type (void)
{
	static volatile gsize the_type__volatile = 0;

	if (g_once_init_enter (&the_type__volatile)) {
		static const GEnumValue values[] = {
			{ E_MAIL_FORMATTER_MODE_INVALID,
			  "E_MAIL_FORMATTER_MODE_INVALID",
			  "invalid" },
			{ E_MAIL_FORMATTER_MODE_NORMAL,
			  "E_MAIL_FORMATTER_MODE_NORMAL",
			  "normal" },
			{ E_MAIL_FORMATTER_MODE_SOURCE,
			  "E_MAIL_FORMATTER_MODE_SOURCE",
			  "source" },
			{ E_MAIL_FORMATTER_MODE_RAW,
			  "E_MAIL_FORMATTER_MODE_RAW",
			  "raw" },
			{ E_MAIL_FORMATTER_MODE_CID,
			  "E_MAIL_FORMATTER_MODE_CID",
			  "cid" },
			{ E_MAIL_FORMATTER_MODE_PRINTING,
			  "E_MAIL_FORMATTER_MODE_PRINTING",
			  "printing" },
			{ E_MAIL_FORMATTER_MODE_ALL_HEADERS,
			  "E_MAIL_FORMATTER_MODE_ALL_HEADERS",
			  "all-headers" },
			{ 0, NULL, NULL }
		};
		GType the_type = g_enum_register_static (
			g_intern_static_string ("EMailFormatterMode"),
			values);
		g_once_init_leave (&the_type__volatile, the_type);
	}
	return the_type__volatile;
}

GType
e_mail_formatter_quote_flags_get_type (void)
{
	static volatile gsize the_type__volatile = 0;

	if (g_once_init_enter (&the_type__volatile)) {
		static const GFlagsValue values[] = {
			{ E_MAIL_FORMATTER_QUOTE_FLAG_CITE,
			  "E_MAIL_FORMATTER_QUOTE_FLAG_CITE",
			  "cite" },
			{ E_MAIL_FORMATTER_QUOTE_FLAG_HEADERS,
			  "E_MAIL_FORMATTER_QUOTE_FLAG_HEADERS",
			  "headers" },
			{ E_MAIL_FORMATTER_QUOTE_FLAG_KEEP_SIG,
			  "E_MAIL_FORMATTER_QUOTE_FLAG_KEEP_SIG",
			  "keep-sig" },
			{ 0, NULL, NULL }
		};
		GType the_type = g_flags_register_static (
			g_intern_static_string ("EMailFormatterQuoteFlags"),
			values);
		g_once_init_leave (&the_type__volatile, the_type);
	}
	return the_type__volatile;
}

GType
e_mail_image_loading_policy_get_type (void)
{
	static volatile gsize the_type__volatile = 0;

	if (g_once_init_enter (&the_type__volatile)) {
		static const GEnumValue values[] = {
			{ E_MAIL_IMAGE_LOADING_POLICY_NEVER,
			  "E_MAIL_IMAGE_LOADING_POLICY_NEVER",
			  "never" },
			{ E_MAIL_IMAGE_LOADING_POLICY_SOMETIMES,
			  "E_MAIL_IMAGE_LOADING_POLICY_SOMETIMES",
			  "sometimes" },
			{ E_MAIL_IMAGE_LOADING_POLICY_ALWAYS,
			  "E_MAIL_IMAGE_LOADING_POLICY_ALWAYS",
			  "always" },
			{ 0, NULL, NULL }
		};
		GType the_type = g_enum_register_static (
			g_intern_static_string ("EMailImageLoadingPolicy"),
			values);
		g_once_init_leave (&the_type__volatile, the_type);
	}
	return the_type__volatile;
}

GType
e_mail_parser_extension_flags_get_type (void)
{
	static volatile gsize the_type__volatile = 0;

	if (g_once_init_enter (&the_type__volatile)) {
		static const GFlagsValue values[] = {
			{ E_MAIL_PARSER_EXTENSION_INLINE,
			  "E_MAIL_PARSER_EXTENSION_INLINE",
			  "inline" },
			{ E_MAIL_PARSER_EXTENSION_INLINE_DISPOSITION,
			  "E_MAIL_PARSER_EXTENSION_INLINE_DISPOSITION",
			  "inline-disposition" },
			{ E_MAIL_PARSER_EXTENSION_COMPOUND_TYPE,
			  "E_MAIL_PARSER_EXTENSION_COMPOUND_TYPE",
			  "compound-type" },
			{ 0, NULL, NULL }
		};
		GType the_type = g_flags_register_static (
			g_intern_static_string ("EMailParserExtensionFlags"),
			values);
		g_once_init_leave (&the_type__volatile, the_type);
	}
	return the_type__volatile;
}

GType
e_mail_part_validity_flags_get_type (void)
{
	static volatile gsize the_type__volatile = 0;

	if (g_once_init_enter (&the_type__volatile)) {
		static const GFlagsValue values[] = {
			{ E_MAIL_PART_VALIDITY_NONE,
			  "E_MAIL_PART_VALIDITY_NONE",
			  "none" },
			{ E_MAIL_PART_VALIDITY_PGP,
			  "E_MAIL_PART_VALIDITY_PGP",
			  "pgp" },
			{ E_MAIL_PART_VALIDITY_SMIME,
			  "E_MAIL_PART_VALIDITY_SMIME",
			  "smime" },
			{ E_MAIL_PART_VALIDITY_SIGNED,
			  "E_MAIL_PART_VALIDITY_SIGNED",
			  "signed" },
			{ E_MAIL_PART_VALIDITY_ENCRYPTED,
			  "E_MAIL_PART_VALIDITY_ENCRYPTED",
			  "encrypted" },
			{ 0, NULL, NULL }
		};
		GType the_type = g_flags_register_static (
			g_intern_static_string ("EMailPartValidityFlags"),
			values);
		g_once_init_leave (&the_type__volatile, the_type);
	}
	return the_type__volatile;
}


/* Generated data ends here */

