// tested in VC6 (1998) and VS 2019
#define WIN32_MEAN_AND_LEAN
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
    u32 idx;
    u32 size;
} FunctionString;

enum TokenType
{
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_CONSTANT,
    TOKEN_STRING,
    TOKEN_SYMBOL,
    TOKEN_OPERATOR
};

typedef struct
{
    enum TokenType type;
    u32 idx;
    u32 size;
} Token;

const u32 SCREEN_WIDTH  = 1024;
const u32 SCREEN_HEIGHT = 768;

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
                LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
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
    WNDCLASSEX  wndclass = {0};
    wndclass.cbSize        = sizeof(wndclass);
    wndclass.lpfnWndProc   = (WNDPROC) win_proc;
    wndclass.hInstance     = hInstance;
    wndclass.lpszClassName = "truetype-test";
    HINSTANCE app = hInstance;
    if (!RegisterClassEx(&wndclass)) return 0;

    HWND window = CreateWindow(
            "truetype-test",
            "truetype test",
            //WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            WS_POPUP | WS_VISIBLE,
            CW_USEDEFAULT,
            0,
            SCREEN_WIDTH,
            SCREEN_HEIGHT,
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
    {
        u8* ttf_buffer = (u8*)malloc(1<<20);
        u8* temp_bitmap = (u8*)malloc(1024*1024);

        //FILE* font_file = fopen("c:/windows/fonts/times.ttf", "rb");
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
        
        FILE* f = fopen("main.c", "r");
        fseek(f, 0, SEEK_END);
        text_size = ftell(f);
        rewind(f);
        // Over allocate to allow for overscan.
        text = (char*)malloc(text_size + 64);
        fread(text, 1, text_size, f);
        for(u32 i = text_size; i < text_size + 64; i++)
        {
            text[i] = 0;
        }
        fclose(f);

        const char* text_end = text + text_size;
        for(char* it = text; it < text_end;)
        {
            // Skip whitespace
            if(*it == ' ' || *it == '\n' || *it == '\r' || *it == '\t')
            {
                continue;
            }

            // Skip single line comment
            if(text_end - it >= 2)
            {
                if(it[0] == '/' && it[1] == '/')
                {
                    while(it < text_end && *it != '\n')
                    {
                        it++;
                    }
                }
            }

            // Skip preprocessor directives
            if(*it == '#')
            {
                while(it < text_end && *it != '\n')
                {
                    it++;
                }
            }

            if(it >= text_end)
            {
                break;
            }


#define KEYWORD(K) \
            if(strncmp(it, K, sizeof(K) - 1) == 0) \
            { \
                Token token; \
                token.type = TOKEN_KEYWORD; \
                token.idx = it - text; \
                token.size = sizeof(K) - 1; \
                tokens[num_tokens++] = token; \
                it += sizeof(K) - 1; \
                continue;
            }

            KEYWORD("auto")
            KEYWORD("double")
            KEYWORD("int")
            KEYWORD("struct")
            KEYWORD("break")
            KEYWORD("else")
            KEYWORD("long")
            KEYWORD("switch")
            KEYWORD("case")
            KEYWORD("enum")
            KEYWORD("register")
            KEYWORD("typedef")
            KEYWORD("char")
            KEYWORD("extern")
            KEYWORD("return")
            KEYWORD("union")
            KEYWORD("const")
            KEYWORD("float")
            KEYWORD("short")
            KEYWORD("unsigned")
            KEYWORD("continue")
            KEYWORD("for")
            KEYWORD("signed")
            KEYWORD("void")
            KEYWORD("default")
            KEYWORD("goto")
            KEYWORD("sizeof")
            KEYWORD("volatile")
            KEYWORD("do")
            KEYWORD("if")
            KEYWORD("static")
            KEYWORD("while")
#undef KEYWORD
        }

        {
            FILE* f = fopen("out.txt", "w");
            for(u32 i = 0; i < num_tokens; i++)
            {
                char buf[256];
                snprintf(buf, tokens[i].size + 1, "%s\n", text + tokens[i].idx);
                fprintf(f, "%s\n", buf);
            }
            fclose(f);
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
            glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
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
            glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT - camera_pos, -camera_pos, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();

            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor3f(0.8f, 0.8f, 0.8f);

            print(&g_font_16, 8, g_font_16.height * 2.0f, text);

            //char frame_ms_text[64];
            //snprintf(frame_ms_text, sizeof(frame_ms_text), "%f\n", (double)(get_time_ns() - time_frame_start_ns) / 1000000.0);
            //print(&g_font_16, 8, g_font_16.height, frame_ms_text);
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

