// Fonts are linked into the harness so it needs no file access at run time. The paths come from
// build.sh (-DRML_FONT_REGULAR / -DRML_FONT_BOLD) and point at upstream's Samples/assets.
#define RML_EMBED(sym, path)               \
	__asm__(".section .rodata\n"           \
			".balign 16\n"                 \
			".global " #sym "\n" #sym ":\n" \
			".incbin \"" path "\"\n"       \
			".global " #sym "_end\n" #sym "_end:\n" \
			".byte 0\n"                    \
			".previous\n")

RML_EMBED(rml_font_regular, RML_FONT_REGULAR);
RML_EMBED(rml_font_bold, RML_FONT_BOLD);
