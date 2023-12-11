// tested in VC6 (1998) and VS 2019
#define WIN32_MEAN_AND_LEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <gl/gl.h>
#include <gl/glu.h>

#include <stdint.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;


typedef struct
{
    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
    GLuint tex;
    float height;
} Font;

typedef struct
{
    u64 idx;
    u64 size;
} FunctionString;

enum TokenType
{
    // Keywords
    TOKEN_KEYWORDS_START,
    TOKEN_auto = TOKEN_KEYWORDS_START,
    TOKEN_double,
    TOKEN_int,
    TOKEN_struct,
    TOKEN_break,
    TOKEN_else,
    TOKEN_long,
    TOKEN_switch,
    TOKEN_case,
    TOKEN_enum,
    TOKEN_register,
    TOKEN_typedef,
    TOKEN_char,
    TOKEN_extern,
    TOKEN_return,
    TOKEN_union,
    TOKEN_const,
    TOKEN_float,
    TOKEN_short,
    TOKEN_unsigned,
    TOKEN_continue,
    TOKEN_for,
    TOKEN_signed,
    TOKEN_void,
    TOKEN_default,
    TOKEN_goto,
    TOKEN_sizeof,
    TOKEN_volatile,
    TOKEN_do,
    TOKEN_if,
    TOKEN_static,
    TOKEN_while,
    TOKEN_u8,
    TOKEN_u16,
    TOKEN_u32,
    TOKEN_u64,
    TOKEN_s8,
    TOKEN_s16,
    TOKEN_s32,
    TOKEN_s64,
    TOKEN_uint8_t,
    TOKEN_uint16_t,
    TOKEN_uint32_t,
    TOKEN_uint64_t,
    TOKEN_int8_t,
    TOKEN_int16_t,
    TOKEN_int32_t,
    TOKEN_int64_t,
    TOKEN_KEYWORDS_END,

    // Operators
    TOKEN_OPERATORS_START,
    TOKEN_PLUS = TOKEN_OPERATORS_START,
    TOKEN_MINUS,
    TOKEN_ASTERISK,
    TOKEN_FORWARD_SLASH,
    TOKEN_TILDE,
    TOKEN_BANG,
    TOKEN_AMPERSAND,
    TOKEN_PERCENT,
    TOKEN_LESS_THAN,
    TOKEN_GREATER_THAN,
    TOKEN_EQUALS,
    TOKEN_BAR,
    TOKEN_CARET,
    TOKEN_COMMA,
    TOKEN_OPERATORS_END,

    // Symbols
    TOKEN_SYMBOLS_START,
    TOKEN_OPEN_PAREN = TOKEN_SYMBOLS_START,
    TOKEN_CLOSE_PAREN,
    TOKEN_OPEN_BRACKET,
    TOKEN_CLOSE_BRACKET,
    TOKEN_OPEN_BRACE,
    TOKEN_CLOSE_BRACE,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_PERIOD,
    TOKEN_SYMBOLS_END,

    TOKEN_IDENTIFIER,
    TOKEN_CONSTANT,
};

typedef struct
{
    enum TokenType type;
    u64 idx;
    u64 size;
} Token;

u32 g_screen_width  = 0;
u32 g_screen_height = 0;

HGLRC g_rc;
u8 g_key_state[256] = {0};
u8 g_key_state_last[256] = {0};
LARGE_INTEGER g_freq;

Font g_font_8;
Font g_font_16;
Font g_font_32;
Font g_font_64;

void print(const Font* font, const float in_x, const float in_y, const char* const in_text)
{
    const char* text = in_text;
    float x = in_x;
    float y = in_y;

    // assume orthographic projection with units = screen pixels, origin at top left
    glBindTexture(GL_TEXTURE_2D, font->tex);
    glBegin(GL_QUADS);
    while (*text)
    {
        if(*text >= 32 && *text < 128)
        {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->cdata, 1024, 1024, *text-32, &x, &y, &q, 1); //1=opengl
            glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
            glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
            glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
            glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
        }
        if(*text == '\n')
        {
            y += font->height;
            x = in_x;
        }
        ++text;
    }
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);

#if 0
    text = in_text;
    x = in_x;
    y = in_y;

    glBegin(GL_LINES);
    glColor3f(0.0f, 1.0f, 0.0f);
    while (*text)
    {
        if (*text >= 32 && *text < 128)
        {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->cdata, 1024, 1024, *text-32, &x, &y, &q, 1); //1=opengl
            glVertex2f(q.x0,q.y0);
            glVertex2f(q.x1,q.y0);

            glVertex2f(q.x1,q.y0);
            glVertex2f(q.x1,q.y1);

            glVertex2f(q.x1,q.y1);
            glVertex2f(q.x0,q.y1);

            glVertex2f(q.x0,q.y1);
            glVertex2f(q.x0,q.y0);
        }
        if(*text == '\n')
        {
            y += font->height;
            x = in_x;
        }
        ++text;
    }
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);
#endif
}

LRESULT CALLBACK win_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_CREATE:
            {
                HDC dc = GetDC(window);

                PIXELFORMATDESCRIPTOR pfd = { sizeof(pfd), 1, PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA };
                int pixel_format;
                pfd.dwLayerMask  = PFD_MAIN_PLANE;
                pfd.cColorBits   = 24;
                pfd.cAlphaBits   = 8;
                pfd.cDepthBits   = 24;
                pfd.cStencilBits = 8;
                pixel_format = ChoosePixelFormat(dc, &pfd);
                if (pixel_format)
                {
                    s32 success = DescribePixelFormat(dc, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pfd);
                    if(success)
                    {
                        SetPixelFormat(dc, pixel_format, &pfd);
                        g_rc = wglCreateContext(dc);
                        if (g_rc)
                        {
                            wglMakeCurrent(dc, g_rc);
                            return 0;
                        }
                    }
                }
                return -1;
            }

        case WM_DESTROY:
            {
                wglMakeCurrent(NULL, NULL); 
                if (g_rc) wglDeleteContext(g_rc);
                PostQuitMessage (0);
                return 0;
            }
        case WM_CLOSE: { DestroyWindow(window); return 0; } break; 
        case WM_PAINT: { ValidateRect(window, 0); } break;
        case WM_KEYDOWN: { g_key_state[wParam] = 1; } break;
        case WM_KEYUP:   { g_key_state[wParam] = 0; } break;
        default: { result = DefWindowProc(window, message, wParam, lParam); } break;
    }

    return result;
}

static s64 get_time_ns()
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    const s64 ns = (s64)((t.QuadPart * 1'000'000'000LL) / g_freq.QuadPart);
    return ns;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)nCmdShow;
    (void)lpCmdLine;
    (void)hPrevInstance;

    WNDCLASSEX  wndclass = {0};
    wndclass.cbSize        = sizeof(wndclass);
    wndclass.lpfnWndProc   = (WNDPROC) win_proc;
    wndclass.hInstance     = hInstance;
    wndclass.lpszClassName = "truetype-test";
    HINSTANCE app = hInstance;
    if (!RegisterClassEx(&wndclass)) return 0;

    const u32 monitor_width = GetSystemMetrics(SM_CXSCREEN);
    const u32 monitor_height = GetSystemMetrics(SM_CYSCREEN);
    g_screen_width = (u32)(monitor_width * 0.9f);
    g_screen_height = (u32)(monitor_height * 0.9f);
    const u32 window_x = monitor_width / 2 - g_screen_width / 2;
    const u32 window_y = monitor_height / 2 - g_screen_height / 2;
    HWND window = CreateWindow(
            "truetype-test",
            "truetype test",
            WS_POPUP | WS_VISIBLE,
            window_x,
            window_y,
            g_screen_width,
            g_screen_height,
            NULL,
            NULL,
            app,
            NULL);
    ShowWindow(window, SW_SHOWNORMAL);

    QueryPerformanceFrequency(&g_freq);

    // init
    u32 text_size;
    char* text = NULL;
    u32 num_tokens = 0;
    Token* tokens = (Token*)malloc(sizeof(Token) * 1024*1024);
    float* text_colors = NULL;

    u32 num_function_strings = 0;
    FunctionString function_strings[1024];

    {
        u8* ttf_buffer = (u8*)malloc(1<<20);
        u8* temp_bitmap = (u8*)malloc(1024*1024);

        FILE* font_file = fopen("Cousine-Regular.ttf", "rb");
        fread(ttf_buffer, 1, 1<<20, font_file);
        fclose(font_file);

        Font* font = &g_font_8;
        font->height = 8.0f;
        stbtt_BakeFontBitmap(ttf_buffer,0, font->height, temp_bitmap,1024,1024, 32,96, font->cdata);
        glGenTextures(1, &font->tex);
        glBindTexture(GL_TEXTURE_2D, font->tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1024,1024,0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        font = &g_font_16;
        font->height = 16.0f;
        stbtt_BakeFontBitmap(ttf_buffer,0, font->height, temp_bitmap,1024,1024, 32,96, font->cdata);
        glGenTextures(1, &font->tex);
        glBindTexture(GL_TEXTURE_2D, font->tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1024,1024,0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        font = &g_font_32;
        font->height = 32.0f;
        stbtt_BakeFontBitmap(ttf_buffer,0, font->height, temp_bitmap,1024,1024, 32,96, font->cdata);
        glGenTextures(1, &font->tex);
        glBindTexture(GL_TEXTURE_2D, font->tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1024,1024,0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        font = &g_font_64;
        font->height = 64.0f;
        stbtt_BakeFontBitmap(ttf_buffer,0, font->height, temp_bitmap,1024,1024, 32,96, font->cdata);
        glGenTextures(1, &font->tex);
        glBindTexture(GL_TEXTURE_2D, font->tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1024,1024,0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        free(ttf_buffer);
        free(temp_bitmap);
        
        FILE* f = fopen("main.c", "rb");
        fseek(f, 0L, SEEK_END);
        text_size = ftell(f);
        rewind(f);
        // Over allocate to allow for overscan.
        text = (char*)malloc(text_size + 64);
        u64 bytes_read = fread(text, 1, text_size, f);
        assert(bytes_read == text_size);
        for(u32 i = text_size; i < text_size + 64; i++)
        {
            text[i] = 0;
        }
        fclose(f);

        text_colors = (float*)malloc(sizeof(float) * 3 * text_size);
        for(u32 i = 0; i < text_size; i++)
        {
            text_colors[i * 3 + 0] = 0.8f;
            text_colors[i * 3 + 1] = 0.8f;
            text_colors[i * 3 + 2] = 0.8f;
        }

        const char* text_start = text;
        const char* text_end = text + text_size;
        for(char* it = text; it < text_end;)
        {
            const u64 S = it - text_start;
            enum TokenType type = TOKEN_KEYWORDS_START;

            // TODO(mfritz) Skip block comments
            if(*it == ' ' || *it == '\n' || *it == '\r' || *it == '\t')
            {
                // Skip whitespace
                it++;
                continue;
            }
            else if(it[0] == '/' && it[1] == '/')
            {
                // Skip single line comment
                while(it < text_end && *it != '\n')
                {
                    it++;
                }
                it++;
            }
            else if(*it == '#')
            {
                // Skip preprocessor directives
                while(it < text_end && *it != '\n')
                {
                    if(*it == '\\')
                    {
                        it++;
                        it += *it == '\r';
                        it += *it == '\n';
                    }
                    else
                    {
                        it++;
                    }
                }
                it++;
            }
            else if((*it >= 'a' && *it <= 'z') ||
                    (*it >= 'A' && *it <= 'Z') ||
                    (*it == '_'))
            {
                // Keyword or identifier
                do
                {
                    it++;
                }
                while((*it >= 'a' && *it <= 'z') ||
                      (*it >= 'A' && *it <= 'Z') ||
                      (*it == '_') ||
                      (*it >= '0' && *it <= '9'));

            type = TOKEN_IDENTIFIER;
#define KEYWORD(K) \
            if(strncmp(text_start + S, #K, sizeof(#K) - 1) == 0) \
            { \
                type = TOKEN_ ## K; \
            }
            KEYWORD(auto)
            KEYWORD(double)
            KEYWORD(int)
            KEYWORD(struct)
            KEYWORD(break)
            KEYWORD(else)
            KEYWORD(long)
            KEYWORD(switch)
            KEYWORD(case)
            KEYWORD(enum)
            KEYWORD(register)
            KEYWORD(typedef)
            KEYWORD(char)
            KEYWORD(extern)
            KEYWORD(return)
            KEYWORD(union)
            KEYWORD(const)
            KEYWORD(float)
            KEYWORD(short)
            KEYWORD(unsigned)
            KEYWORD(continue)
            KEYWORD(for)
            KEYWORD(signed)
            KEYWORD(void)
            KEYWORD(default)
            KEYWORD(goto)
            KEYWORD(sizeof)
            KEYWORD(volatile)
            KEYWORD(do)
            KEYWORD(if)
            KEYWORD(static)
            KEYWORD(while)
            KEYWORD(u8)
            KEYWORD(u16)
            KEYWORD(u32)
            KEYWORD(u64)
            KEYWORD(s8)
            KEYWORD(s16)
            KEYWORD(s32)
            KEYWORD(s64)
            KEYWORD(uint8_t)
            KEYWORD(uint16_t)
            KEYWORD(uint32_t)
            KEYWORD(uint64_t)
            KEYWORD(int8_t)
            KEYWORD(int16_t)
            KEYWORD(int32_t)
            KEYWORD(int64_t)
#undef KEYWORD
            }
            else if(*it >= '0' && *it <= '9')
            {
                // Numeric literal
                do
                {
                    it++;
                }
                // TODO(mfritz) This is wrong. Just trying to get past numbers for now.
                while((*it >= '0' && *it <= '9') ||
                      (*it == '.') ||
                      (*it == '\'') ||
                      (*it == 'x') || (*it == 'X') ||
                      (*it == 'f') || (*it == 'F') ||
                      (*it == 'e') || (*it == 'E') ||
                      (*it == 'l') || (*it == 'L') ||
                      (*it == 'u') || (*it == 'U'));
                type = TOKEN_CONSTANT;
            }
            else if(*it == '\'')
            {
                // Character literal
                it++;
                it += *it == '\\';
                it++;
                assert(*it == '\'');
                it++;
                type = TOKEN_CONSTANT;
            }
            else if(*it == '"')
            {
                // String literal
                do
                {
                    it++;
                }
                // TODO(mfritz) Handle escape quote
                while(*it != '"');
                it++;
                type = TOKEN_CONSTANT;
            }
            else if(*it == '+') { type = TOKEN_PLUS;          it++; }
            else if(*it == '-') { type = TOKEN_MINUS;         it++; }
            else if(*it == '*') { type = TOKEN_ASTERISK;      it++; }
            else if(*it == '/') { type = TOKEN_FORWARD_SLASH; it++; }
            else if(*it == '~') { type = TOKEN_TILDE;         it++; }
            else if(*it == '!') { type = TOKEN_BANG;          it++; }
            else if(*it == '&') { type = TOKEN_AMPERSAND;     it++; }
            else if(*it == '%') { type = TOKEN_PERCENT;       it++; }
            else if(*it == '<') { type = TOKEN_LESS_THAN;     it++; }
            else if(*it == '>') { type = TOKEN_GREATER_THAN;  it++; }
            else if(*it == '=') { type = TOKEN_EQUALS;        it++; }
            else if(*it == '|') { type = TOKEN_BAR;           it++; }
            else if(*it == '^') { type = TOKEN_CARET;         it++; }
            else if(*it == ',') { type = TOKEN_COMMA;         it++; }
            else if(*it == '(') { type = TOKEN_OPEN_PAREN;    it++; }
            else if(*it == ')') { type = TOKEN_CLOSE_PAREN;   it++; }
            else if(*it == '[') { type = TOKEN_OPEN_BRACKET;  it++; }
            else if(*it == ']') { type = TOKEN_CLOSE_BRACKET; it++; }
            else if(*it == '{') { type = TOKEN_OPEN_BRACE;    it++; }
            else if(*it == '}') { type = TOKEN_CLOSE_BRACE;   it++; }
            else if(*it == ',') { type = TOKEN_COMMA;         it++; }
            else if(*it == ':') { type = TOKEN_COLON;         it++; }
            else if(*it == ';') { type = TOKEN_SEMICOLON;     it++; }
            else if(*it == '.') { type = TOKEN_PERIOD;        it++; }
            else
            {
                assert(0);
            }

            // Add the token.
            const u64 E = it - text_start;
            tokens[num_tokens].type = type;
            tokens[num_tokens].idx = S;
            tokens[num_tokens].size = E - S;
            num_tokens++;

            if(it >= text_end)
            {
                break;
            }
        }

        for(u32 i = 0; i < num_tokens; i++)
        {
            if(tokens[i].type >= TOKEN_KEYWORDS_START && tokens[i].type < TOKEN_KEYWORDS_END)
            {
                const u64 idx = tokens[i].idx;
                const u64 size = tokens[i].size;
                for(u64 j = idx; j < idx + size; j++)
                {
                    text_colors[j * 3 + 0] = 0.8f;
                    text_colors[j * 3 + 1] = 0.7f;
                    text_colors[j * 3 + 2] = 0.0f;
                }
            }
            else if(tokens[i].type >= TOKEN_SYMBOLS_START && tokens[i].type < TOKEN_SYMBOLS_END)
            {
                const u64 idx = tokens[i].idx;
                const u64 size = tokens[i].size;
                for(u64 j = idx; j < idx + size; j++)
                {
                    text_colors[j * 3 + 0] = 1.0f;
                    text_colors[j * 3 + 1] = 1.0f;
                    text_colors[j * 3 + 2] = 1.0f;
                }
            }
            else if(tokens[i].type >= TOKEN_OPERATORS_START && tokens[i].type < TOKEN_OPERATORS_END)
            {
                const u64 idx = tokens[i].idx;
                const u64 size = tokens[i].size;
                for(u64 j = idx; j < idx + size; j++)
                {
                    text_colors[j * 3 + 0] = 0.8f;
                    text_colors[j * 3 + 1] = 0.6f;
                    text_colors[j * 3 + 2] = 0.4f;
                }
            }
            else if(tokens[i].type == TOKEN_CONSTANT)
            {
                const u64 idx = tokens[i].idx;
                const u64 size = tokens[i].size;
                for(u64 j = idx; j < idx + size; j++)
                {
                    text_colors[j * 3 + 0] = 0.8f;
                    text_colors[j * 3 + 1] = 0.4f;
                    text_colors[j * 3 + 2] = 0.2f;
                }
            }
        }

        for(u32 token_idx = 0; token_idx < num_tokens; token_idx++)
        {
            u64 fn_token_idx_start = token_idx;

            // Read return type
            if(!(tokens[token_idx].type >= TOKEN_KEYWORDS_START && tokens[token_idx].type < TOKEN_KEYWORDS_END))
            {
                continue;
            }
            token_idx++;

            // Read fn name
            if(tokens[token_idx].type != TOKEN_IDENTIFIER)
            {
                continue;
            }
            token_idx++;

            // Read open paren
            if(tokens[token_idx].type != TOKEN_OPEN_PAREN)
            {
                continue;
            }
            token_idx++;

            // Skip all args
            while(token_idx < num_tokens && tokens[token_idx].type != TOKEN_CLOSE_PAREN)
            {
                token_idx++;
            }
            if(token_idx == num_tokens)
            {
                break;
            }
            // Move past close paren
            token_idx++;

            // Read open brace
            if(tokens[token_idx].type != TOKEN_OPEN_BRACE)
            {
                continue;
            }
            token_idx++;

            // Skip all fn body
            u32 brace_depth = 1;
            while(token_idx < num_tokens &&
                  (tokens[token_idx].type != TOKEN_CLOSE_BRACE || brace_depth > 0))
            {
                token_idx++;

                brace_depth += tokens[token_idx].type == TOKEN_OPEN_BRACE;
                brace_depth -= tokens[token_idx].type == TOKEN_CLOSE_BRACE;
            }
            if(token_idx == num_tokens)
            {
                break;
            }

            // Token idx should be on the close brace.

            FunctionString fn_string;
            fn_string.idx = tokens[fn_token_idx_start].idx;
            fn_string.size = tokens[token_idx].idx + tokens[token_idx].size - tokens[fn_token_idx_start].idx;
            function_strings[num_function_strings++] = fn_string;
        }

    }

    const s64 frame_time_ns = 16'666'666LL;
    s64 last_time_ns = get_time_ns();

    float camera_vel = 0.0f;
    float camera_pos = 0.0f;

    while(1)
    {
        while(get_time_ns() - last_time_ns < frame_time_ns)
        {
            _mm_pause();
        }
        const s64 time_frame_start_ns = get_time_ns();
        last_time_ns = time_frame_start_ns;

        MSG msg;
        while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT)
            {
                return 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(g_key_state[VK_ESCAPE])
        {
            return 0;
        }

        const HDC dc = GetDC(window);
        wglMakeCurrent(dc, g_rc);

        // Draw
        {
            glViewport(0, 0, g_screen_width, g_screen_height);
            glClearColor(
                    15.0f / 256.0f,
                     5.0f / 256.0f,
                    40.0f / 256.0f,
                    0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_CULL_FACE);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_BLEND);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, g_screen_width, g_screen_height - camera_pos, -camera_pos, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor3f(0.8f, 0.8f, 0.8f);

            //print(&g_font_16, 8, g_font_16.height * 2.0f, text);

#if 0
            float text_x = 8;
            float text_y = g_font_16.height * 2.0f;
            for(char* it = text; *it != 0; it++)
            {
                const Font* font = &g_font_16;
                const u32 idx = it - text;

                float r = text_colors[idx * 3 + 0];
                float g = text_colors[idx * 3 + 1];
                float b = text_colors[idx * 3 + 2];

                glColor3f(r, g, b);

                glBindTexture(GL_TEXTURE_2D, font->tex);
                glBegin(GL_QUADS);
                if(*it >= 32 && *it < 128)
                {
                    stbtt_aligned_quad q;
                    stbtt_GetBakedQuad(font->cdata, 1024, 1024, *it-32, &text_x, &text_y, &q, 1); //1=opengl
                    glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
                    glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
                    glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
                    glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
                }
                if(*it == '\n')
                {
                    text_y += font->height;
                    text_x = 8;
                }
                glEnd();
                glBindTexture(GL_TEXTURE_2D, 0);
#endif

#if 1
                const Font* font = &g_font_16;
                glBindTexture(GL_TEXTURE_2D, font->tex);
                const float start_x = 8.0f;
                float text_x = start_x;
                float text_y = g_font_16.height * 2.0f;
                for(u32 fn_idx = 0; fn_idx < num_function_strings; fn_idx++)
                {
                    for(char* it = text + function_strings[fn_idx].idx;
                            it < text + function_strings[fn_idx].idx + function_strings[fn_idx].size;
                            it++)
                    {
                        const u64 idx = it - text;

                        float r = text_colors[idx * 3 + 0];
                        float g = text_colors[idx * 3 + 1];
                        float b = text_colors[idx * 3 + 2];

                        glColor3f(r, g, b);

                        glBegin(GL_QUADS);
                        if(*it >= 32 && *it < 128)
                        {
                            stbtt_aligned_quad q;
                            stbtt_GetBakedQuad(font->cdata, 1024, 1024, *it-32, &text_x, &text_y, &q, 1); //1=opengl
                            glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
                            glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
                            glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
                            glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
                        }
                        if(*it == '\n')
                        {
                            text_y += font->height;
                            text_x = start_x;
                        }
                        glEnd();
                    }
#endif
                    text_x = start_x;
                    text_y += font->height;
                    text_y += font->height;
                    text_y += font->height;
                    text_y += font->height;

                }
                glBindTexture(GL_TEXTURE_2D, 0);
        }

        float camera_accel = 0.0f;
        if(g_key_state[VK_UP])   camera_accel += 1.0f;
        if(g_key_state[VK_DOWN]) camera_accel -= 1.0f;

        if(g_key_state[VK_SPACE]) camera_accel *= 4.0f;

        camera_vel += camera_accel;
        camera_pos += camera_vel;
        camera_vel *= 0.825f;

        {
            memcpy(g_key_state_last, g_key_state, sizeof(g_key_state));
        }

        SwapBuffers(dc);

    }
    return 0;
}

