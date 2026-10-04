// 5x5 block font. Every glyph is five rows of five characters; any
// non-space character is drawn as one block.

global const char *digit_glyphs[10][5] = {
    {" 000 ", "0   0", "0   0", "0   0", " 000 "},
    {"  1  ", " 11  ", "  1  ", "  1  ", " 111 "},
    {" 222 ", "2   2", "   2 ", "  2  ", "22222"},
    {"3333 ", "    3", "  333", "    3", "3333 "},
    {"4  4 ", "4  4 ", "44444", "   4 ", "   4 "},
    {"55555", "5    ", "5555 ", "    5", "5555 "},
    {" 666 ", "6    ", "6666 ", "6   6", " 666 "},
    {"77777", "   7 ", "  7  ", " 7   ", "7    "},
    {" 888 ", "8   8", " 888 ", "8   8", " 888 "},
    {" 999 ", "9   9", " 9999", "    9", " 999 "},
};

global const char *letter_glyphs[26][5] = {
    {"  A  ", " A A ", "AAAAA", "A   A", "A   A"},
    {"BBBB ", "B   B", "BBBB ", "B   B", "BBBB "},
    {" CCC ", "C   C", "C    ", "C   C", " CCC "},
    {"DDD  ", "D  D ", "D   D", "D  D ", "DDD  "},
    {"EEEEE", "E    ", "EEE  ", "E    ", "EEEEE"},
    {"FFFFF", "F    ", "FFF  ", "F    ", "F    "},
    {" GGG ", "G    ", "G  GG", "G   G", " GGG "},
    {"H   H", "H   H", "HHHHH", "H   H", "H   H"},
    {"IIIII", "  I  ", "  I  ", "  I  ", "IIIII"},
    {"JJJJJ", "   J ", "   J ", "J  J ", " JJ  "},
    {"K   K", "K  K ", "KKK  ", "K  K ", "K   K"},
    {"L    ", "L    ", "L    ", "L    ", "LLLLL"},
    {"M   M", "MM MM", "M M M", "M   M", "M   M"},
    {"N   N", "NN  N", "N N N", "N  NN", "N   N"},
    {" OOO ", "O   O", "O   O", "O   O", " OOO "},
    {"PPPP ", "P   P", "PPPP ", "P    ", "P    "},
    {" QQQ ", "Q   Q", "Q   Q", "Q  QQ", " QQQQ"},
    {"RRRR ", "R   R", "RRRR ", "R  R ", "R   R"},
    {" SSS ", "S    ", " SSS ", "    S", " SSS "},
    {"TTTTT", "  T  ", "  T  ", "  T  ", "  T  "},
    {"U   U", "U   U", "U   U", "U   U", " UUU "},
    {"V   V", "V   V", "V   V", " V V ", "  V  "},
    {"W   W", "W   W", "W W W", "WW WW", "W   W"},
    {"X   X", " X X ", "  X  ", " X X ", "X   X"},
    {"Y   Y", " Y Y ", "  Y  ", "  Y  ", "  Y  "},
    {"ZZZZZ", "   Z ", "  Z  ", " Z   ", "ZZZZZ"},
};

internal void
draw_glyph(v2 p, u32 color, const char *const *rows) {
    const f32 block_half_size = .15f;
    p.x -= block_half_size * 5;
    f32 original_x = p.x;

    for (int r = 0; r < 5; r++) {
        for (const char *at = rows[r]; *at; at++) {
            if (*at != ' ') {
                draw(p, (v2){block_half_size, block_half_size}, color, SHAPE_RECT);
            }
            p.x += block_half_size * 2.f;
        }
        p.y -= block_half_size * 2.f;
        p.x = original_x;
    }
}

internal void
draw_digit(v2 p, u32 color, int digit) {
    if (digit < 0 || digit > 9) return;
    draw_glyph(p, color, digit_glyphs[digit]);
}

internal void
draw_alphabet(v2 p, u32 color, char letter) {
    if (letter < 'A' || letter > 'Z') return;
    draw_glyph(p, color, letter_glyphs[letter - 'A']);
}

#define MAX_DIGITS 10

internal void
draw_score(v2 p, u32 color, u32 num) {
    int digits[MAX_DIGITS];
    int count = 0;

    if (num == 0) {
        draw_digit(p, color, 0);
        return;
    }
    while (num && count < MAX_DIGITS) {
        digits[count++] = (int)(num % 10);
        num /= 10;
    }
    for (int j = count - 1; j >= 0; j--) {
        draw_digit(p, color, digits[j]);
        p.x += 2.f;
    }
}

internal void
draw_words(v2 p, u32 color, const char *word) {
    for (int i = 0; word[i] != '\0'; i++) {
        char c = word[i];
        if (c >= '0' && c <= '9')      draw_digit(p, color, c - '0');
        else if (c >= 'A' && c <= 'Z') draw_alphabet(p, color, c);
        else if (c >= 'a' && c <= 'z') draw_alphabet(p, color, (char)(c - 'a' + 'A'));
        p.x += 2.f;
    }
}
