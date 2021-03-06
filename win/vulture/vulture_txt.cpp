/* NetHack may be freely redistributed under the Nethack General Public License
* See nethack/dat/license for details */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <SDL.h>
#include "SDL_ttf.h"

#include "vulture_txt.h"

#define VULTURE_MAX_FONTS  2

/* the rest of vulture gets these defines indirectly from nethack code.
* but I'd prefer not to pull all that in here */
#define TRUE 1
#define FALSE 0

static struct vulture_font {
	TTF_Font *fontptr;
	char lineheight;
} *vulture_fonts = NULL;


/* load a font from a ttf file with the given fontindex and pointsize as font font_id */
int vulture_load_font (int font_id, const char * ttf_filename, int fontindex, int pointsize)
{
	TTF_Font *newfont;
	if(!TTF_WasInit()) {
		if(TTF_Init()==-1)
			return FALSE;

		if (!vulture_fonts)
			vulture_fonts = (struct vulture_font *)calloc(VULTURE_MAX_FONTS, sizeof(struct vulture_font));
	}

	/* font_id should always be passed as a #define, which is always <  VULTURE_MAX_FONTS */
	if (font_id >= VULTURE_MAX_FONTS)
		return FALSE;

	newfont = TTF_OpenFontIndex(ttf_filename, pointsize, fontindex);

	if (!newfont)
		return FALSE;

	if (vulture_fonts[font_id].fontptr)
		TTF_CloseFont(vulture_fonts[font_id].fontptr);

	vulture_fonts[font_id].fontptr = newfont;
	vulture_fonts[font_id].lineheight = TTF_FontAscent(newfont) + 2;

	return TRUE;
}


int vulture_put_text (int font_id, std::string str, SDL_Surface *dest, int x, int y, Uint32 color)
{
	SDL_Surface *textsurface;
	SDL_Color fontcolor;
	SDL_Rect dstrect;
  std::string cleaned_str;
	unsigned int i;

	if (font_id >= VULTURE_MAX_FONTS || (!vulture_fonts[font_id].fontptr) || str.empty())
		return FALSE;

	/* sanitize the input string of unprintable characters */
	cleaned_str = str;
	for (i = 0; i < cleaned_str.length(); i++)
		if (!isprint(cleaned_str[i]))
			cleaned_str[i] = ' ';


	/* extract components from the 32-bit color value */
	fontcolor.r = (color & dest->format->Rmask) >> dest->format->Rshift;
	fontcolor.g = (color & dest->format->Gmask) >> dest->format->Gshift;
	fontcolor.b = (color & dest->format->Bmask) >> dest->format->Bshift;
	fontcolor.unused = 0;

	textsurface = TTF_RenderText_Blended(vulture_fonts[font_id].fontptr,
	                                     cleaned_str.c_str(), fontcolor);
	if (!textsurface)
		return FALSE;

	dstrect.x = x;
	dstrect.y = y - 1;
	dstrect.w = textsurface->w;
	dstrect.h = textsurface->h;

	SDL_BlitSurface(textsurface, NULL, dest, &dstrect);
	SDL_FreeSurface(textsurface);
	
	return TRUE;
}



int vulture_put_text_shadow (int font_id, std::string str, SDL_Surface *dest,
							int x, int y, Uint32 textcolor, Uint32 shadowcolor)
{
	/* draw the shadow first */
	vulture_put_text(font_id, str, dest, x+1, y+1, shadowcolor);
	/* we only truly care that the actual text got drawn, so only retrun that status */
	return vulture_put_text(font_id, str, dest, x, y, textcolor);
}


/* draw text over multiple lines if its length exceeds maxlen */
void vulture_put_text_multiline(int font_id, std::string str, SDL_Surface *dest,
								int x, int y, Uint32 color, Uint32 shadowcolor, int maxlen)
{
  std::string str_copy;
	int lastfit, txtlen, lineno, text_height;
	size_t endpos, startpos;

	lineno = 0;
	lastfit = 0;
	startpos = endpos = 0;
	text_height = vulture_text_height(font_id, str);

	/* lastfit is true when the last segment has been fit onto the surface (drawn) */
	while (!lastfit) {
		lastfit = 1;

		startpos = startpos + endpos;
		str_copy = str.substr(startpos, std::string::npos);
		endpos = str_copy.length() - 1;

		txtlen = vulture_text_length(font_id, str_copy);

		/* split word off the end of the string until it is short enough to fit */
		while (txtlen > maxlen) {
			/* a piece will be split off the end, so this is not the last piece */
			lastfit = 0;

			/* find a suitable place to split */
			endpos = str_copy.find_last_of(" \t\n");

			/* prevent infinite loops if a long word doesn't fit */
			if (endpos == std::string::npos) {
				endpos = str_copy.length() - 1;
				lastfit = 1;
				break;
			}

			/* truncate the text */
			str_copy.erase(endpos++);
			txtlen = vulture_text_length(font_id, str_copy);
		}
		vulture_put_text_shadow(font_id, str_copy, dest, x, y + lineno * text_height, color, shadowcolor);

		lineno++;
	}
}



int vulture_text_length (int font_id, std::string str)
{
	int width = 0;

	if (!vulture_fonts[font_id].fontptr || str.empty())
		return 0;

	TTF_SizeText(vulture_fonts[font_id].fontptr, str.c_str(), &width, NULL);

	return width;
}



int vulture_text_height (int font_id, std::string str)
{
	int height = 0;

	if (!vulture_fonts[font_id].fontptr || str.empty())
		return 0;

	TTF_SizeText(vulture_fonts[font_id].fontptr, str.c_str(), NULL, &height);

	return height;
}



int vulture_get_lineheight(int font_id)
{
	if (font_id < VULTURE_MAX_FONTS && vulture_fonts[font_id].fontptr)
		return vulture_fonts[font_id].lineheight;
	return 0;
}



void vulture_free_fonts()
{
	int font_id;

	for (font_id = 0; font_id < VULTURE_MAX_FONTS; font_id++)
	{
		if (vulture_fonts[font_id].fontptr)
			TTF_CloseFont(vulture_fonts[font_id].fontptr);
		vulture_fonts[font_id].fontptr = NULL;
	}

	free(vulture_fonts);
	vulture_fonts = NULL;

	TTF_Quit();
}
