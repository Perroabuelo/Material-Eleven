#include <string.h>

#include "text_encoding.h"
#include "text_fold.h"

// U+00C0 to U+00FF. An empty entry (× and ÷) keeps the character as it is.
static const char fold_latin1[64][3] = {
	"a", "a", "a", "a", "a", "a", "ae", "c", // À Á Â Ã Ä Å Æ Ç
	"e", "e", "e", "e", "i", "i", "i", "i",  // È É Ê Ë Ì Í Î Ï
	"d", "n", "o", "o", "o", "o", "o", "",   // Ð Ñ Ò Ó Ô Õ Ö ×
	"o", "u", "u", "u", "u", "y", "th", "ss", // Ø Ù Ú Û Ü Ý Þ ß
	"a", "a", "a", "a", "a", "a", "ae", "c", // à á â ã ä å æ ç
	"e", "e", "e", "e", "i", "i", "i", "i",  // è é ê ë ì í î ï
	"d", "n", "o", "o", "o", "o", "o", "",   // ð ñ ò ó ô õ ö ÷
	"o", "u", "u", "u", "u", "y", "th", "y", // ø ù ú û ü ý þ ÿ
};

// U+0100 to U+017F, one base letter per code point, upper and lower case
// alternating. The two ligatures, Ĳ ĳ and Œ œ, are marked '2' and spelled out
// by TextFold_Base.
static const char fold_ext_a[129] =
	"aaaaaa"       // Ā ā Ă ă Ą ą
	"cccccccc"     // Ć ć Ĉ ĉ Ċ ċ Č č
	"dddd"         // Ď ď Đ đ
	"eeeeeeeeee"   // Ē ē Ĕ ĕ Ė ė Ę ę Ě ě
	"gggggggg"     // Ĝ ĝ Ğ ğ Ġ ġ Ģ ģ
	"hhhh"         // Ĥ ĥ Ħ ħ
	"iiiiiiiiii"   // Ĩ ĩ Ī ī Ĭ ĭ Į į İ ı
	"22"           // Ĳ ĳ
	"jj"           // Ĵ ĵ
	"kkk"          // Ķ ķ ĸ
	"llllllllll"   // Ĺ ĺ Ļ ļ Ľ ľ Ŀ ŀ Ł ł
	"nnnnnnnnn"    // Ń ń Ņ ņ Ň ň ŉ Ŋ ŋ
	"oooooo"       // Ō ō Ŏ ŏ Ő ő
	"22"           // Œ œ
	"rrrrrr"       // Ŕ ŕ Ŗ ŗ Ř ř
	"ssssssss"     // Ś ś Ŝ ŝ Ş ş Š š
	"tttttt"       // Ţ ţ Ť ť Ŧ ŧ
	"uuuuuuuuuuuu" // Ũ ũ Ū ū Ŭ ŭ Ů ů Ű ű Ų ų
	"ww"           // Ŵ ŵ
	"yyy"          // Ŷ ŷ Ÿ
	"zzzzzz"       // Ź ź Ż ż Ž ž
	"s";           // ſ

// Writes into `dst` (room for 2 bytes) the folded spelling of the code point
// `cp`, and returns its length, or 0 when the character is copied as is.
static size_t TextFold_Base(unsigned int cp, char *dst) {
	if (cp >= 'A' && cp <= 'Z') {
		dst[0] = (char)(cp - 'A' + 'a');
		return 1;
	}

	if (cp >= 0xC0 && cp <= 0xFF) {
		const char *base = fold_latin1[cp - 0xC0];
		size_t len = strlen(base);

		memcpy(dst, base, len);
		return len;
	}

	if (cp >= 0x100 && cp <= 0x17F) {
		char c = fold_ext_a[cp - 0x100];

		if (c != '2') {
			dst[0] = c;
			return 1;
		}

		memcpy(dst, (cp == 0x132 || cp == 0x133) ? "ij" : "oe", 2);
		return 2;
	}

	return 0;
}

size_t TextFold_Fold(const char *utf8, char *out, size_t cap) {
	if (cap == 0)
		return 0;

	const unsigned char *p = (const unsigned char *)utf8;
	size_t avail = strlen(utf8), used = 0;

	while (avail > 0) {
		size_t n = TextEncoding_Utf8SeqLen(p, avail);
		char base[2];
		size_t base_len = 0;

		if (n == 1)
			base_len = TextFold_Base(p[0], base);
		else if (n == 2)
			base_len = TextFold_Base(((p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu), base);

		// Invalid bytes go through one at a time, like the rest of what is
		// not folded.
		size_t take = (n == 0) ? 1 : n;
		size_t len = base_len > 0 ? base_len : take;

		if (used + len > cap - 1)
			break;

		memcpy(out + used, base_len > 0 ? base : (const char *)p, len);
		used += len;
		p += take;
		avail -= take;
	}

	out[used] = '\0';
	return used;
}

int TextFold_Contains(const char *haystack_utf8, const char *folded_needle) {
	if (folded_needle[0] == '\0')
		return 1;

	char folded[TEXT_FOLD_MAX];
	TextFold_Fold(haystack_utf8, folded, sizeof(folded));

	return strstr(folded, folded_needle) != NULL;
}
