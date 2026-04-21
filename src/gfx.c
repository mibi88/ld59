#include "gfx.h"

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <math.h>

#if __EMSCRIPTEN__
#include <emscripten.h>
EMSCRIPTEN_RESULT emscripten_get_canvas_element_size(const char *target,
                                                     int *width, int *height);
#endif

static unsigned char keys[SDLK_LAST];
static int mx, my;

#if !__EMSCRIPTEN__
static int avoid_closing = 0;
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define B_LUT_SIZE \
    (MAX( \
        MAX( \
            MAX( \
                MAX(SDL_BUTTON_LEFT, SDL_BUTTON_MIDDLE), \
                SDL_BUTTON_RIGHT \
            ), \
            SDL_BUTTON_WHEELUP \
        ), \
        SDL_BUTTON_WHEELDOWN \
    )+1)

#define FLAGS (SDL_OPENGL | SDL_RESIZABLE)

static unsigned char button_lut[B_LUT_SIZE];

static unsigned char buttons[B_AMOUNT];

static unsigned int width, height;
static unsigned char bpp;

static unsigned char c;

static const int sdl_keys[K_AMOUNT] = {
    SDLK_ESCAPE,
    SDLK_F1,
    SDLK_F2,
    SDLK_F3,
    SDLK_F4,
    SDLK_F5,
    SDLK_F6,
    SDLK_F7,
    SDLK_F8,
    SDLK_F9,
    SDLK_F10,
    SDLK_F11,
    SDLK_F12,
    SDLK_SPACE,
    SDLK_EXCLAIM,
    SDLK_QUOTEDBL,
    SDLK_HASH,
    SDLK_DOLLAR,
    SDLK_UNKNOWN,
    SDLK_AMPERSAND,
    SDLK_QUOTE,
    SDLK_LEFTPAREN,
    SDLK_RIGHTPAREN,
    SDLK_ASTERISK,
    SDLK_PLUS,
    SDLK_COMMA,
    SDLK_UNKNOWN,
    SDLK_PERIOD,
    SDLK_SLASH,
    SDLK_0,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_4,
    SDLK_5,
    SDLK_6,
    SDLK_7,
    SDLK_8,
    SDLK_9,
    SDLK_COLON,
    SDLK_SEMICOLON,
    SDLK_LESS,
    SDLK_EQUALS,
    SDLK_GREATER,
    SDLK_QUESTION,
    SDLK_AT,
    SDLK_a,
    SDLK_b,
    SDLK_c,
    SDLK_d,
    SDLK_e,
    SDLK_f,
    SDLK_g,
    SDLK_h,
    SDLK_i,
    SDLK_j,
    SDLK_k,
    SDLK_l,
    SDLK_m,
    SDLK_n,
    SDLK_o,
    SDLK_p,
    SDLK_q,
    SDLK_r,
    SDLK_s,
    SDLK_t,
    SDLK_u,
    SDLK_v,
    SDLK_w,
    SDLK_x,
    SDLK_y,
    SDLK_z,
    SDLK_LEFTBRACKET,
    SDLK_BACKSLASH,
    SDLK_RIGHTBRACKET,
    SDLK_CARET,
    SDLK_UNDERSCORE,
    SDLK_BACKQUOTE,
    SDLK_a,
    SDLK_b,
    SDLK_c,
    SDLK_d,
    SDLK_e,
    SDLK_f,
    SDLK_g,
    SDLK_h,
    SDLK_i,
    SDLK_j,
    SDLK_k,
    SDLK_l,
    SDLK_m,
    SDLK_n,
    SDLK_o,
    SDLK_p,
    SDLK_q,
    SDLK_r,
    SDLK_s,
    SDLK_t,
    SDLK_u,
    SDLK_v,
    SDLK_w,
    SDLK_x,
    SDLK_y,
    SDLK_z,
    SDLK_UNKNOWN,
    SDLK_UNKNOWN,
    SDLK_UNKNOWN,
    SDLK_UNKNOWN,
    SDLK_BACKSPACE,
    SDLK_TAB,
    SDLK_CAPSLOCK,
    SDLK_LSHIFT,
    SDLK_LCTRL,
    SDLK_LSUPER,
    SDLK_LALT,
    SDLK_RALT,
    SDLK_RCTRL,
    SDLK_MENU,
    SDLK_RSHIFT,
    SDLK_RETURN,
    SDLK_DELETE,
    SDLK_PRINT,
    SDLK_NUMLOCK,
    SDLK_UP,
    SDLK_DOWN,
    SDLK_LEFT,
    SDLK_RIGHT,
};

static void handle_resize(unsigned int w, unsigned int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);

    width = w;
    height = h;
}

int gfx_init(unsigned int w, unsigned int h, char *title) {
    const SDL_VideoInfo *info;

    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        fputs("Failed to initialize SDL!\n", stderr);

        return 1;
    }

    info = SDL_GetVideoInfo();
    if(info == NULL){
        fputs("Failed to get SDL video info!\n", stderr);

        return 2;
    }

    bpp = info->vfmt->BitsPerPixel;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if(!SDL_SetVideoMode(w, h, bpp, FLAGS)){
        fputs("Failed set video mode!\n", stderr);

        return 3;
    }

    SDL_WM_SetCaption(title, NULL);

    glClearColor(1, 1, 1, 1);

    glShadeModel(GL_FLAT);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    handle_resize(w, h);

    memset(keys, 0, SDLK_LAST);
    memset(buttons, 0, B_AMOUNT);
    mx = 0;
    my = 0;
    c = '\0';

    memset(button_lut, 0xFF, B_LUT_SIZE);

    button_lut[SDL_BUTTON_LEFT] = B_LEFT;
    button_lut[SDL_BUTTON_MIDDLE] = B_MIDDLE;
    button_lut[SDL_BUTTON_RIGHT] = B_RIGHT;
    button_lut[SDL_BUTTON_WHEELUP] = B_SCROLLUP;
    button_lut[SDL_BUTTON_WHEELDOWN] = B_SCROLLDOWN;

    return 0;
}

static unsigned int findpot(unsigned int n) {
    if(n&(n-1)){
        unsigned int pot;

        for(pot=1;pot<n;pot<<=1);

        return pot;
    }

    return n;
}

void gfx_clear_color(unsigned char r, unsigned char g, unsigned char b) {
    glClearColor((float)r/255, (float)g/255, (float)b/255, 1);
}

int gfx_load_image(struct image *img, char *file) {
    int y;

    GLuint id;

    unsigned int wpot, hpot;
    unsigned int pot;

    unsigned char *data;

    SDL_Surface *surface;

    unsigned char *row;
    unsigned char *target_row;
    size_t row_size;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &id);

    img->id = id;
    glBindTexture(GL_TEXTURE_2D, img->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    surface = IMG_Load(file);
    if(surface == NULL){
        fprintf(stderr, "Failed to load `%s'\n", file);

        return 1;
    }
    if(surface->format->BitsPerPixel != 32){
        fprintf(stderr, "Failed to load `%s': image needs to have 32 bpp\n",
                file);
        SDL_FreeSurface(surface);

        return 2;
    }

    wpot = findpot(surface->w);
    hpot = findpot(surface->h);

    pot = wpot > hpot ? wpot : hpot;

    /* NOTE: We can use malloc as we don't care if the rest of the texture
     *       contains garbage as it won't be rendered. */
    data = malloc(pot*pot*4);
    if(data == NULL){
        SDL_FreeSurface(surface);

        return 2;
    }

    row = surface->pixels;
    target_row = data;
    row_size = surface->w*4;
    for(y=0;y<surface->h;y++){
        memcpy(target_row, row, row_size);

        row += row_size;
        target_row += pot*4;
    }

    img->w = surface->w;
    img->h = surface->h;
    img->pot = pot;

    SDL_FreeSurface(surface);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pot, pot, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);

    free(data);

    return 0;
}

void gfx_subimage(struct image *img,
                  int x, int y, unsigned int w, unsigned int h,
                  int ix, int iy, unsigned int iw, unsigned int ih,
                  unsigned char r, unsigned char g, unsigned char b,
                  unsigned char a) {
    float u1 = (float)ix/img->pot;
    float v1 = (float)iy/img->pot;
    float u2 = u1+(float)iw/img->pot;
    float v2 = v1+(float)ih/img->pot;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, img->id);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glColor4f((float)r/255, (float)g/255, (float)b/255, (float)a/255);

    glBegin(GL_QUADS);
    glTexCoord2f(u1, v1);
    glVertex2i(x, y);
    glTexCoord2f(u2, v1);
    glVertex2i(x+w, y);
    glTexCoord2f(u2, v2);
    glVertex2i(x+w, y+h);
    glTexCoord2f(u1, v2);
    glVertex2i(x, y+h);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glColor4f(1, 1, 1, 1);
}

void gfx_image(struct image *img, int x, int y) {
    float u = (float)img->w/img->pot;
    float v = (float)img->h/img->pot;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, img->id);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2i(x, y);
    glTexCoord2f(u, 0);
    glVertex2i(x+img->w, y);
    glTexCoord2f(u, v);
    glVertex2i(x+img->w, y+img->h);
    glTexCoord2f(0, v);
    glVertex2i(x, y+img->h);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void gfx_text(struct image *font, int x, int y, char *str,
              unsigned char r, unsigned char g, unsigned char b,
              unsigned char a) {
    size_t i;
    char c;

    int cx = x;

    unsigned int gw = font->w/16;
    unsigned int gh = font->h/6;

    for(i=0;(c=str[i]);i++){
        unsigned int ix, iy;

        if(c == '\n'){
            cx = x;
            y += gh;

            continue;
        }

        if(c >= 0x20){
            c -= 0x20;

            ix = (c%16)*gw;
            iy = (c/16)*gh;

            gfx_subimage(font, cx, y, gw, gh, ix, iy, gw, gh, r, g, b, a);
        }

        cx += gw;
    }
}

void gfx_free_image(struct image *img) {
    GLuint id = img->id;

    glDeleteTextures(1, &id);
}

void gfx_line(int x1, int y1, int x2, int y2,
              unsigned char r, unsigned char g, unsigned char b,
              unsigned char a,
              unsigned int thickness) {
    float len = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
    float s = len ? thickness/(2*len) : 0;

    float dir_x, dir_y;

    dir_x = (y2-y1)*s;
    dir_y = -(x2-x1)*s;

    glColor4f((float)r/255, (float)g/255, (float)b/255, (float)a/255);
    glBegin(GL_QUADS);
    glVertex2i(x1-dir_x, y1-dir_y);
    glVertex2i(x2-dir_x, y2-dir_y);
    glVertex2i(x2+dir_x, y2+dir_y);
    glVertex2i(x1+dir_x, y1+dir_y);
    glEnd();

    glColor4f(1, 1, 1, 1);
}

void gfx_rect(int x, int y, unsigned int w, unsigned int h,
              unsigned char r, unsigned char g, unsigned char b,
              unsigned char a) {
    glColor4f((float)r/255, (float)g/255, (float)b/255, (float)a/255);

    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x+w, y);
    glVertex2i(x+w, y+h);
    glVertex2i(x, y+h);
    glEnd();

    glColor4f(1, 1, 1, 1);
}

void gfx_get_mouse_position(int *x, int *y) {
    *x = mx;
    *y = my;
}

void gfx_get_size(unsigned int *w, unsigned int *h) {
    *w = width;
    *h = height;
}

int gfx_buttondown(int b) {
    return buttons[b];
}

int gfx_keydown(int key) {
    return keys[sdl_keys[key]];
}

unsigned char gfx_char(void) {
    unsigned char r = c;

    c = '\0';

    return r;
}

#if __EMSCRIPTEN__
static void (*render)(unsigned long ms, void *data);
void *data;

static void loop(void) {
#else
void gfx_mainloop(void render(unsigned long ms, void *data), void *data) {
    do{
#endif
        unsigned long int last, new;
        unsigned int w, h;

        int resize = 0;

        /* Ugly fix to avoid scrolling to never be noticed by render. */
        int scrolled = 0;

        SDL_Event event;

        last = SDL_GetTicks();

        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
#if !__EMSCRIPTEN__
                    if(!avoid_closing) goto END;
#endif
                    break;

                case SDL_VIDEORESIZE:
                    w = event.resize.w;
                    h = event.resize.h;
                    resize = 1;
                    break;

                case SDL_KEYDOWN:
                    keys[event.key.keysym.sym] = 1;
                    if(event.key.keysym.unicode < 0xFF){
                        c = event.key.keysym.unicode;
                    }
                    break;

                case SDL_KEYUP:
                    keys[event.key.keysym.sym] = 0;
                    break;

                case SDL_MOUSEMOTION:
                    mx = event.motion.x;
                    my = event.motion.y;

                    break;

                case SDL_MOUSEBUTTONDOWN:
                    {
                        unsigned char id;

                        if(event.button.button < B_LUT_SIZE){
#if 0
                            printf("button down: %d\n", event.button.button);
#endif
                            id = button_lut[event.button.button];
                            if(id != 0xFF){
                                buttons[id] = 1;
                                if(id == B_SCROLLUP || id == B_SCROLLDOWN){
                                    scrolled = 1;
                                }
                            }
                        }
                    }

                    break;

                case SDL_MOUSEBUTTONUP:
                    {
                        unsigned char id;

                        if(event.button.button < B_LUT_SIZE){
                            id = button_lut[event.button.button];
                            if(id != 0xFF){
#if 0
                                printf("button up: %d\n", event.button.button);
#endif
                                if((id != B_SCROLLUP && id != B_SCROLLDOWN) ||
                                   !scrolled){
                                    buttons[id] = 0;
                                }
                            }
                        }
                    }

                    break;
            }
        }

#if __EMSCRIPTEN__
        emscripten_get_canvas_element_size("#canvas", (int*)&w, (int*)&h);

        printf("%u, %u\n", w, h);

        resize = w != width || h != height;
#endif

        if(resize){
            if(SDL_SetVideoMode(w, h, bpp, FLAGS)){
                handle_resize(w, h);
            }else{
                fputs("Resizing failed!\n", stderr);
            }
        }

        new = SDL_GetTicks();
        glClear(GL_COLOR_BUFFER_BIT);
        render(new-last, data);
        SDL_GL_SwapBuffers();

        last = new;

        buttons[B_SCROLLUP] = 0;
        buttons[B_SCROLLDOWN] = 0;
#if __EMSCRIPTEN__
}

void gfx_mainloop(void r(unsigned long ms, void *data), void *d) {
    render = r;
    data = d;

    emscripten_set_main_loop(loop, 0, 0);
}
#else
    }while(1);

END:
    SDL_Quit();
}
#endif

void gfx_free(void) {
    /**/
}
