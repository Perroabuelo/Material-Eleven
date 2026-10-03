#include <stddef.h>
#include <strings.h>

#include "track_meta.h"

// Mas que esto no es un numero de pista sino basura; se corta para no
// desbordar un int con un tag de cien digitos.
#define TRACK_META_MAX 9999

int TrackMeta_ParseNumber(const char *text) {
	if (text == NULL)
		return 0;

	// Algunos programas rellenan con espacios por delante.
	while (*text == ' ')
		text++;

	int n = 0;

	for (const char *p = text; *p >= '0' && *p <= '9'; p++) {
		n = n * 10 + (*p - '0');

		if (n > TRACK_META_MAX)
			return 0;
	}

	return n;
}

int TrackMeta_Id3v1Track(const unsigned char comment[30]) {
	if (comment == NULL)
		return 0;

	return (comment[28] == 0 && comment[29] != 0) ? comment[29] : 0;
}

static const char *TrackMeta_Str(const char *s) {
	return (s != NULL) ? s : "";
}

int TrackMeta_CompareDisc(const TrackMeta_Key *a, const TrackMeta_Key *b) {
	int disc_a = (a->disc > 0) ? a->disc : 1;
	int disc_b = (b->disc > 0) ? b->disc : 1;

	if (disc_a != disc_b)
		return (disc_a < disc_b) ? -1 : 1;

	// Las sin numero van detras de las numeradas de su disco.
	if ((a->track == 0) != (b->track == 0))
		return (a->track == 0) ? 1 : -1;

	if (a->track != b->track)
		return (a->track < b->track) ? -1 : 1;

	int by_title = strcasecmp(TrackMeta_Str(a->title), TrackMeta_Str(b->title));

	if (by_title != 0)
		return by_title;

	return strcasecmp(TrackMeta_Str(a->path), TrackMeta_Str(b->path));
}

int TrackMeta_CompareGrouped(const TrackMeta_Key *a, const TrackMeta_Key *b) {
	const char *group_a = TrackMeta_Str(a->group);
	const char *group_b = TrackMeta_Str(b->group);
	int empty_a = (group_a[0] == '\0');
	int empty_b = (group_b[0] == '\0');

	// El grupo vacio va al final, como el cubo "Desconocido" de las vistas.
	if (empty_a != empty_b)
		return empty_a ? 1 : -1;

	int by_group = strcasecmp(group_a, group_b);

	if (by_group != 0)
		return by_group;

	return TrackMeta_CompareDisc(a, b);
}
