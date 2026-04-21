#ifndef GFX_H
#define GFX_H

struct image {
    unsigned int id;
    unsigned int w;
    unsigned int h;
    unsigned int pot;
};

enum {
    B_LEFT,
    B_MIDDLE,
    B_RIGHT,
    B_SCROLLUP,
    B_SCROLLDOWN,
    B_AMOUNT
};

enum {
    K_ESC,
    K_F1,
    K_F2,
    K_F3,
    K_F4,
    K_F5,
    K_F6,
    K_F7,
    K_F8,
    K_F9,
    K_F10,
    K_F11,
    K_F12,
    K_SPACE,
    K_EXCLAMATION_MARK,
    K_QUOTE,
    K_HASHTAG,
    K_DOLLAR,
    K_PERCENT,
    K_AMPERSAND,
    K_APOSTROPHY,
    K_LPAREN,
    K_RPAREN,
    K_ASTERISK,
    K_PLUS,
    K_COMMA,
    K_HYPHEN,
    K_DOT,
    K_SLASH,
    K_0,
    K_1,
    K_2,
    K_3,
    K_4,
    K_5,
    K_6,
    K_7,
    K_8,
    K_9,
    K_COLON,
    K_SEMICOLON,
    K_LESS_THAN,
    K_EQUAL,
    K_GREATER_THAN,
    K_QUESTION_MARK,
    K_AT,
    K_A,
    K_B,
    K_C,
    K_D,
    K_E,
    K_F,
    K_G,
    K_H,
    K_I,
    K_J,
    K_K,
    K_L,
    K_M,
    K_N,
    K_O,
    K_P,
    K_Q,
    K_R,
    K_S,
    K_T,
    K_U,
    K_V,
    K_W,
    K_X,
    K_Y,
    K_Z,
    K_LBRACKET,
    K_BACKSLASH,
    K_RBRACKET,
    K_CARET,
    K_UNDERSCORE,
    K_BACKTICK,
    K_LOWER_A,
    K_LOWER_B,
    K_LOWER_C,
    K_LOWER_D,
    K_LOWER_E,
    K_LOWER_F,
    K_LOWER_G,
    K_LOWER_H,
    K_LOWER_I,
    K_LOWER_J,
    K_LOWER_K,
    K_LOWER_L,
    K_LOWER_M,
    K_LOWER_N,
    K_LOWER_O,
    K_LOWER_P,
    K_LOWER_Q,
    K_LOWER_R,
    K_LOWER_S,
    K_LOWER_T,
    K_LOWER_U,
    K_LOWER_V,
    K_LOWER_W,
    K_LOWER_X,
    K_LOWER_Y,
    K_LOWER_Z,
    K_LCURLY_BRACE,
    K_VERTICAL_BAR,
    K_RCURLY_BRACE,
    K_TILDE,
    K_BACKSPACE,
    K_TAB,
    K_CAPS_LOCK,
    K_LSHIFT,
    K_LCTRL,
    K_SUPER,
    K_LALT,
    K_RALT,
    K_RCTRL,
    K_MENU,
    K_RSHIFT,
    K_RETURN,
    K_DELETE,
    K_PRINTSCREEN,
    K_NUMLOCK,
    K_UP,
    K_DOWN,
    K_LEFT,
    K_RIGHT,
    K_AMOUNT
};

int gfx_init(unsigned int w, unsigned int h, char *title);

void gfx_clear_color(unsigned char r, unsigned char g, unsigned char b);

int gfx_load_image(struct image *img, char *file);
void gfx_subimage(struct image *img,
                  int x, int y, unsigned int w, unsigned int h,
                  int ix, int iy, unsigned int iw, unsigned int ih,
                  unsigned char r, unsigned char g, unsigned char b,
                  unsigned char a);
void gfx_image(struct image *img, int x, int y);
void gfx_text(struct image *font, int x, int y, char *str,
              unsigned char r, unsigned char g, unsigned char b,
              unsigned char a);
void gfx_free_image(struct image *img);

void gfx_line(int x1, int y1, int x2, int y2,
              unsigned char r, unsigned char g, unsigned char b,
              unsigned char a,
              unsigned int thickness);
void gfx_rect(int x, int y, unsigned int w, unsigned int h,
              unsigned char r, unsigned char g, unsigned char b,
              unsigned char a);

void gfx_get_mouse_position(int *x, int *y);
void gfx_get_size(unsigned int *w, unsigned int *h);
int gfx_buttondown(int b);
int gfx_keydown(int key);
unsigned char gfx_char(void);

void gfx_mainloop(void render(unsigned long ms, void *data), void *data);

void gfx_free(void);

#endif
