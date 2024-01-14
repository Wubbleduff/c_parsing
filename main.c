// tested in VC6 (1998) and VS 2019
#define WIN32_MEAN_AND_LEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#include <stdio.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "glad/glad.h"
#include <gl/glu.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <stdint.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "Glu32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "lib/glfw/glfw.lib")

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;



#define MAX_TEXT_SIZE 50 * 1024 * 1024
#define MAX_QUADS 1024 * 1024
#define SSBO_STRIDE 10 * 1024 * 4
#define NUM_SSBO_FIELDS 11
typedef struct
{
    u32 num_text;
    float text_x[MAX_TEXT_SIZE];
    float text_y[MAX_TEXT_SIZE];
    float text_w[MAX_TEXT_SIZE];
    float text_h[MAX_TEXT_SIZE];
    float text_r[MAX_TEXT_SIZE];
    float text_g[MAX_TEXT_SIZE];
    float text_b[MAX_TEXT_SIZE];
    float text_tex_l[MAX_TEXT_SIZE];
    float text_tex_r[MAX_TEXT_SIZE];
    float text_tex_b[MAX_TEXT_SIZE];
    float text_tex_t[MAX_TEXT_SIZE];

    u32 num_quads;
    float quad_x[MAX_QUADS];
    float quad_y[MAX_QUADS];
    float quad_w[MAX_QUADS];
    float quad_h[MAX_QUADS];
    float quad_r[MAX_QUADS];
    float quad_g[MAX_QUADS];
    float quad_b[MAX_QUADS];
} GraphicsData;

typedef struct
{
    float x;
    float y;

    float transform_x;
    float transform_y;
    float transform_scale;
} Cursor;

typedef struct
{
    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
    GLuint tex;
    float height;
} Font;

typedef struct
{
    u64 token_idx_start;
    u64 token_idx_end;

    u64 name_token_idx;
} Function;

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
    TOKEN_BACK_SLASH,
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
    TOKEN_QUESTION_MARK,
    TOKEN_SEMICOLON,
    TOKEN_PERIOD,
    TOKEN_SYMBOLS_END,

    TOKEN_IDENTIFIER,
    TOKEN_CONSTANT,

    TOKEN_COMMENT,
    TOKEN_PREPROCESSOR,

    NUM_TOKEN_TYPES
};

typedef struct
{
    enum TokenType type;
    u64 idx;
    u64 size;
} Token;

u32 g_screen_width  = 0;
u32 g_screen_height = 0;

LARGE_INTEGER g_freq;

FILE* g_log_file;

HGLRC g_rc;
u8 g_key_state[512] = {0};
u8 g_key_state_last[512] = {0};
u8 g_mouse_key_state[512] = {0};
u8 g_mouse_key_state_last[512] = {0};
float g_mouse_wheel_state = 0;
float g_mouse_window_x = 0.0f;
float g_mouse_window_y = 0.0f;
float g_mouse_window_x_last = 0.0f;
float g_mouse_window_y_last = 0.0f;

const float CAMERA_SCROLL_SPEED = 0.1f;

float g_quad_data[] = {
    // X
    0.0f, 1.0f, 1.0f, 0.0f,
    // Y
    0.0f, 0.0f, 1.0f, 1.0f,
};

Font g_font_8;
Font g_font_16;
Font g_font_32;
Font g_font_64;

enum ColorId
{
    WHITE,
    BLACK,
    RED,
    GREEN,
    BLUE,
    YELLOW,
    ORANGE,
    GRAY,

    BLUE_0,
    BLUE_1,
    BLUE_2,
    BLUE_3,

    NUM_COLOR_ID
};
float g_color_data[NUM_COLOR_ID * 3] = {
    1.0f, 1.0f, 1.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,
    0.8f, 0.7f, 0.0f,
    0.8f, 0.4f, 0.2f,
    0.4f, 0.4f, 0.4f,

    0.05f, 0.025f, 0.2f,
    0.1f, 0.05f, 0.2f,
    0.05f, 0.05f, 0.4f,
    0.3f, 0.05f, 0.4f,
};

enum ColorId g_token_colors[NUM_TOKEN_TYPES] = { 0 };

u32 g_background_colors[] = {
    BLUE_0,
    BLUE_1,
    BLUE_2,
    BLUE_3,
};

#define ARRAY_COUNT(N) (sizeof(N) / sizeof(*(N)))

static u32 min_u32(u32 a, u32 b)
{
    return a < b ? a : b;
}

static u8 is_identifier(const enum TokenType t)
{
    if(t >= TOKEN_KEYWORDS_START && t <= TOKEN_KEYWORDS_END)
    {
        return 1;
    }
    if(t == TOKEN_IDENTIFIER)
    {
        return 1;
    }
    return 0;
}

static float print_height(
        const char* const text,
        const u64 text_len,
        const Font* font)
{
    float result = 0.0f;
    for(u32 i = 0; i < text_len; i++)
    {
        // TODO(mfritz) compat
        result += text[i] == '\n' ? font->height : 0.0f;
    }
    return result;
}

static void print_cursor(
        GraphicsData* graphics_data,
        Cursor* cursor,
        const char* const in_text,
        const u64 text_len,
        const Font* font,
        const enum ColorId color_id)
{
    const char* text = in_text;

    for(u64 i = 0; i < text_len; i++, text++)
    {
        if(*text >= 32 && *text < 128)
        {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->cdata, 1024, 1024, *text - 32, &cursor->x, &cursor->y, &q, 1);

            u32 num = graphics_data->num_text;

            graphics_data->text_x[num] = q.x0 * cursor->transform_scale + cursor->transform_x;
            graphics_data->text_y[num] = q.y0 * cursor->transform_scale + cursor->transform_y;

            graphics_data->text_w[num] = (q.x1 - q.x0) * cursor->transform_scale;
            graphics_data->text_h[num] = (q.y1 - q.y0) * cursor->transform_scale;

            graphics_data->text_r[num] = g_color_data[color_id*3 + 0];
            graphics_data->text_g[num] = g_color_data[color_id*3 + 1];
            graphics_data->text_b[num] = g_color_data[color_id*3 + 2];

            graphics_data->text_tex_l[num] = q.s0;
            graphics_data->text_tex_r[num] = q.s1;
            graphics_data->text_tex_b[num] = q.t0;
            graphics_data->text_tex_t[num] = q.t1;

            num++;
            assert(num < MAX_TEXT_SIZE);

            graphics_data->num_text = num;
        }
    }
}

static void print(
        GraphicsData* graphics_data,
        float* x,
        float* y,
        const char* const in_text,
        const u64 text_len,
        const Font* font,
        const enum ColorId color_id)
{
    Cursor c = {
        .x = *x,
        .y = *y,
        .transform_x = 0.0f,
        .transform_y = 0.0f,
        .transform_scale = 1.0f,
    };
    print_cursor(graphics_data, &c, in_text, text_len, font, color_id);
}

static void quad(
        GraphicsData* graphics_data,
        const float x,
        const float y,
        const float w,
        const float h,
        const enum ColorId color_id)
{
    const u32 num = graphics_data->num_quads;
    graphics_data->quad_x[num] = x;
    graphics_data->quad_y[num] = y;
    graphics_data->quad_w[num] = w;
    graphics_data->quad_h[num] = h;
    graphics_data->quad_r[num] = g_color_data[color_id*3 + 0];
    graphics_data->quad_g[num] = g_color_data[color_id*3 + 1];
    graphics_data->quad_b[num] = g_color_data[color_id*3 + 2];
    graphics_data->num_quads++;
}

static void space(Cursor* cursor, const u32 count, const Font* font)
{
    float tmp_x = 0.0f;
    float tmp_y = 0.0f;
    stbtt_aligned_quad q;
    stbtt_GetBakedQuad(font->cdata, 1024, 1024, ' ' - 32, &tmp_x, &tmp_y, &q, 1);
    for(u32 i = 0; i < count; i++) cursor->x += tmp_x;
}

static void newline(Cursor* cursor, const Font* font)
{
    cursor->x = 0.0f;
    cursor->y += font->height;
}

static u8 same_token_name(const Token a, const Token b, const char* const text)
{
    if(a.size == b.size && strncmp(text + a.idx, text + b.idx, a.size) == 0)
    {
        return 1;
    }
    return 0;
}



static void render_function(
        GraphicsData* graphics_data,
        Cursor* cursor,
        const u32 recurse_depth,
        const char* text,
        const Font* font,
        const Function fn,
        const Function* all_fns,
        const u32 num_fns,
        const Token* tokens,
        const u32 num_tokens,
        const u32* text_to_token_idx)
{
    u64 fn_to_draw_idx = ~0ULL;
    float cursor_save_x = 0.0f;
    const u64 text_start_idx = tokens[fn.token_idx_start].idx;
    const u64 text_end_idx = tokens[fn.token_idx_end].idx + tokens[fn.token_idx_end].size;
    for(u64 i = text_start_idx; i < text_end_idx;)
    {
        const u64 token_idx = text_to_token_idx[i];
        if(token_idx < num_tokens)
        {
            const Token token = tokens[token_idx];
            enum ColorId color = g_token_colors[token.type];

            // TODO(mfritz) allow overscan
            if(token_idx + 1 < num_tokens &&
               token.type == TOKEN_IDENTIFIER &&
               tokens[token_idx + 1].type == TOKEN_OPEN_PAREN)
            {

                for(u64 fn_idx = 0; fn_idx < num_fns; fn_idx++)
                {
                    const Function cur_fn = all_fns[fn_idx];
                    const Token fn_name_token = tokens[cur_fn.name_token_idx];
                    if(strncmp(text + fn_name_token.idx, text + token.idx, token.size) == 0)
                    {
                        fn_to_draw_idx = fn_idx;
                    }
                }

                if(fn_to_draw_idx != ~0ULL)
                {
                    color = GREEN;
                    cursor_save_x = cursor->x;
                }
                else 
                {
                    color = RED;
                }
            }

            print_cursor(graphics_data, cursor, text + token.idx, token.size, font, color);

            if(fn_to_draw_idx != ~0ULL &&
               token.type == TOKEN_CLOSE_PAREN &&
               !same_token_name(tokens[fn.name_token_idx], tokens[all_fns[fn_to_draw_idx].name_token_idx], text))
            {
                newline(cursor, font);
                cursor->x = cursor_save_x;

                Cursor sub_cursor = {
                    .x = 0.0f,
                    .y = 0.0f,
                    .transform_x = cursor->transform_x + cursor->x * cursor->transform_scale,
                    .transform_y = cursor->transform_y + cursor->y * cursor->transform_scale,
                    .transform_scale = cursor->transform_scale * 0.5f,
                };
                render_function(
                    graphics_data,
                    &sub_cursor,
                    recurse_depth + 1,
                    text,
                    font,
                    all_fns[fn_to_draw_idx],
                    all_fns,
                    num_fns,
                    tokens,
                    num_tokens,
                    text_to_token_idx);

                cursor->x = 0.0f;
                cursor->y += sub_cursor.y * 0.5f;

                quad(graphics_data,
                        sub_cursor.transform_x,
                        sub_cursor.transform_y - cursor->transform_scale * 0.5f * font->height,
                        10000.0f,
                        sub_cursor.y * cursor->transform_scale * 0.5f + cursor->transform_scale * 0.5f * font->height,
                        g_background_colors[recurse_depth % ARRAY_COUNT(g_background_colors)]);

                newline(cursor, font);

                fn_to_draw_idx = ~0ULL;
            }

            if(fn_to_draw_idx < num_fns && same_token_name(tokens[fn.name_token_idx], tokens[all_fns[fn_to_draw_idx].name_token_idx], text))
            {
                fn_to_draw_idx = ~0ULL;
            }


            i += token.size;
        }
        else if(text[i] == '\r' && text[i + 1] == '\n')
        {
            newline(cursor, font);
            i += 2;
        }
        else if(text[i] == '\r')
        {
            newline(cursor, font);
            i++;
        }
        else if(text[i] == '\n')
        {
            newline(cursor, font);
            i++;
        }
        else if(text[i] == ' ')
        {
            space(cursor, 1, font);
            i++;
        }
        else if(text[i] == '\t')
        {
            space(cursor, 4, font);
            i++;
        }
        else
        {
            print_cursor(graphics_data, cursor, text + i, 1, font, WHITE);
            i++;
        }
    }
}

static s64 get_time_ms()
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    const s64 ns = (s64)((t.QuadPart * 1'000LL) / g_freq.QuadPart);
    return ns;
}

void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)window;
    (void)mods;
    (void)scancode;
    if(action == GLFW_PRESS)
    {
        g_key_state[key] = 1;
    }
    else if(action == GLFW_RELEASE)
    {
        g_key_state[key] = 0;
    }
}

void glfw_mouse_key_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)window;
    (void)mods;
    if(action == GLFW_PRESS)
    {
        g_mouse_key_state[button] = 1;
    }
    else if(action == GLFW_RELEASE)
    {
        g_mouse_key_state[button] = 0;
    }
}

void glfw_mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;
    g_mouse_wheel_state = (float)yoffset;
}

void glfw_error_fn(s32 error, const char *message)
{
    (void)error;
    (void)message;
    fprintf(g_log_file, "%s:%i GLFW error: %i \"%s\"\n", __FILE__, __LINE__, error, message);
    fflush(g_log_file);
    assert(0);
}

static void check_gl_errors_fn(const char* file, int line)
{
    // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetError.xhtml
    GLint error;
    u8 any_error = 0;
    do
    {
        error = glGetError();
        if(error)
        {
            fprintf(g_log_file, "%s:%i GL error: %i \"%s\"\n", file, line, error, gluErrorString(error));
            any_error = 1;
        }
    }
    while(error);

    if(any_error)
    {
        fflush(g_log_file);
        assert(0);
    }
}
#define GL_ERR() check_gl_errors_fn(__FILE__, __LINE__);

static GLuint make_shader_from_string(const char* vert_str, const char* frag_str)
{
    static char error_buf[1024];
    GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER); GL_ERR();
    const GLchar* const* p_vert_source_v = &vert_str;
    glShaderSource(vert_shader, 1, p_vert_source_v, NULL); GL_ERR();
    glCompileShader(vert_shader); GL_ERR();
    GLint success;
    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success); GL_ERR();
    if(!success)
    {
        glGetShaderInfoLog(vert_shader, sizeof(error_buf), NULL, error_buf);GL_ERR();
        fprintf(g_log_file, "%s:%i GL error compiling shader \"%s\"\n", __FILE__, __LINE__, error_buf);
        fflush(g_log_file);
        assert(0);
    }
    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER); GL_ERR();
    const GLchar* const* p_frag_source_v = &frag_str;
    glShaderSource(frag_shader, 1, p_frag_source_v, NULL); GL_ERR();
    glCompileShader(frag_shader); GL_ERR();
    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success); GL_ERR();
    if(!success)
    {
        glGetShaderInfoLog(frag_shader, sizeof(error_buf), NULL, error_buf);GL_ERR();
        fprintf(g_log_file, "%s:%i GL error compiling shader \"%s\"\n", __FILE__, __LINE__, error_buf);
        fflush(g_log_file);
        assert(0);
    }
    GLuint shader_program = glCreateProgram(); GL_ERR();
    glAttachShader(shader_program, vert_shader); GL_ERR();
    glAttachShader(shader_program, frag_shader); GL_ERR();
    glLinkProgram(shader_program); GL_ERR();
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success); GL_ERR();
    assert(success);
    glDeleteShader(vert_shader); GL_ERR();
    glDeleteShader(frag_shader);  GL_ERR();

    return shader_program;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)nCmdShow;
    (void)lpCmdLine;
    (void)hPrevInstance;
    (void)hInstance;

    g_log_file = fopen("log.txt", "w");

    const u32 monitor_width = GetSystemMetrics(SM_CXSCREEN);
    const u32 monitor_height = GetSystemMetrics(SM_CYSCREEN);
    g_screen_width = (u32)(monitor_width * 0.9f);
    g_screen_height = (u32)(monitor_height * 0.9f);
    //const u32 window_x = monitor_width / 2 - g_screen_width / 2;
    //const u32 window_y = monitor_height / 2 - g_screen_height / 2;
    glfwSetErrorCallback(glfw_error_fn);
    if(!glfwInit())
    {
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* glfw_window = glfwCreateWindow(g_screen_width, g_screen_height, "c_parsing", NULL, NULL);
    glfwMakeContextCurrent(glfw_window);
    if(!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        return 1;
    }
    glfwSetKeyCallback(glfw_window, glfw_key_callback);
    glfwSetMouseButtonCallback(glfw_window, glfw_mouse_key_callback);
    glfwSetScrollCallback(glfw_window, glfw_mouse_scroll_callback);
    glfwSwapInterval(1);
    QueryPerformanceFrequency(&g_freq);

    GraphicsData* graphics_data = (GraphicsData*)malloc(sizeof(GraphicsData));
    assert(graphics_data);

    // init
    u32 text_size;
    char* text = NULL;
    u32 num_tokens = 0;
    Token* tokens = (Token*)malloc(sizeof(Token) * 1024*1024);
    assert(tokens);

    // TODO(mfritz) enforce max size
    u32* text_to_token_idx;

    u32 num_functions = 0;
    Function functions[1024];

    GLuint shader_program;
    GLuint color_shader_program;
    GLuint vao;
    GLuint vbo;
    GLuint ssbo;
    {
        // VBO
        glGenBuffers(1, &vbo); GL_ERR();
        glBindBuffer(GL_ARRAY_BUFFER, vbo); GL_ERR();
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_data), g_quad_data, GL_STATIC_DRAW); GL_ERR();

        // VAO
        glGenVertexArrays(1, &vao); GL_ERR();
        glBindVertexArray(vao); GL_ERR();
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, (void *)(0*sizeof(float))); GL_ERR();
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, (void *)(4*sizeof(float))); GL_ERR();
        glEnableVertexAttribArray(0); GL_ERR();
        glEnableVertexAttribArray(1); GL_ERR();

        // SSBO
        // https://www.khronos.org/opengl/wiki/Shader_Storage_Buffer_Object
        glGenBuffers(1, &ssbo); GL_ERR();
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo); GL_ERR();
        glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_SSBO_FIELDS * SSBO_STRIDE, 0, GL_DYNAMIC_DRAW); GL_ERR();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo); GL_ERR();
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); GL_ERR();

        // Shader
        assert(NUM_SSBO_FIELDS == 11);
        shader_program = make_shader_from_string(
            "#version 440 core\n"
            "layout (location = 0) in float a_x;"
            "layout (location = 1) in float a_y;"
            "layout(std430, binding = 0) buffer voxel_data"
            "{"
            "    float ssbo_pos_x[10*1024];"
            "    float ssbo_pos_y[10*1024];"
            "    float ssbo_scale_x[10*1024];"
            "    float ssbo_scale_y[10*1024];"
            "    float ssbo_r[10*1024];"
            "    float ssbo_g[10*1024];"
            "    float ssbo_b[10*1024];"
            "    float ssbo_tex_l[10*1024];"
            "    float ssbo_tex_r[10*1024];"
            "    float ssbo_tex_b[10*1024];"
            "    float ssbo_tex_t[10*1024];"
            "};"
            "uniform mat4 u_view_proj_mat;"
            "out float frag_u;"
            "out float frag_v;"
            "out vec4 frag_color;"
            "void main()"
            "{"
            "    frag_u = mix(ssbo_tex_l[gl_InstanceID], ssbo_tex_r[gl_InstanceID], a_x);"
            "    frag_v = mix(ssbo_tex_b[gl_InstanceID], ssbo_tex_t[gl_InstanceID], a_y);"
            "    frag_color = vec4(ssbo_r[gl_InstanceID], ssbo_g[gl_InstanceID], ssbo_b[gl_InstanceID], 1.0f);"
            "    vec4 world_position = vec4("
            "            a_x*ssbo_scale_x[gl_InstanceID] + ssbo_pos_x[gl_InstanceID],"
            "            a_y*ssbo_scale_y[gl_InstanceID] + ssbo_pos_y[gl_InstanceID],"
            "            0.0f,"
            "            1.0f);"
            "    gl_Position = u_view_proj_mat * world_position;"
            "};",

            "#version 440 core\n"
            "in float frag_u;"
            "in float frag_v;"
            "in vec4 frag_color;"
            "uniform sampler2D u_tex_sampler;"
            "out vec4 frag_result;"
            "void main()"
            "{"
            "    float tex = texture(u_tex_sampler, vec2(frag_u, frag_v)).r;"
            "    vec4 tex_color = vec4(tex, tex, tex, tex);"
            "    frag_result = frag_color * tex_color;"
            "}"
            );

        color_shader_program = make_shader_from_string(
            "#version 440 core\n"
            "layout (location = 0) in float a_x;"
            "layout (location = 1) in float a_y;"
            "layout(std430, binding = 0) buffer voxel_data"
            "{"
            "    float ssbo_pos_x[10*1024];"
            "    float ssbo_pos_y[10*1024];"
            "    float ssbo_scale_x[10*1024];"
            "    float ssbo_scale_y[10*1024];"
            "    float ssbo_r[10*1024];"
            "    float ssbo_g[10*1024];"
            "    float ssbo_b[10*1024];"
            "    float ssbo_tex_l[10*1024];"
            "    float ssbo_tex_r[10*1024];"
            "    float ssbo_tex_b[10*1024];"
            "    float ssbo_tex_t[10*1024];"
            "};"
            "uniform mat4 u_view_proj_mat;"
            "out vec4 frag_color;"
            "void main()"
            "{"
            "    frag_color = vec4(ssbo_r[gl_InstanceID], ssbo_g[gl_InstanceID], ssbo_b[gl_InstanceID], 1.0f);"
            "    vec4 world_position = vec4("
            "            a_x*ssbo_scale_x[gl_InstanceID] + ssbo_pos_x[gl_InstanceID],"
            "            a_y*ssbo_scale_y[gl_InstanceID] + ssbo_pos_y[gl_InstanceID],"
            "            0.0f,"
            "            1.0f);"
            "    gl_Position = u_view_proj_mat * world_position;"
            "};",

            "#version 440 core\n"
            "in vec4 frag_color;"
            "out vec4 frag_result;"
            "void main()"
            "{"
            "    frag_result = frag_color;"
            "}"
            );

        u8* ttf_buffer = (u8*)malloc(1<<20);
        assert(ttf_buffer);
        u8* temp_bitmap = (u8*)malloc(1024*1024);
        assert(temp_bitmap);

        FILE* font_file = fopen("Cousine-Regular.ttf", "rb");
        fread(ttf_buffer, 1, 1<<20, font_file);
        fclose(font_file);

        Font* font = &g_font_8;
        font->height = 8.0f;
        stbtt_BakeFontBitmap(ttf_buffer,0, font->height, temp_bitmap,1024,1024, 32,96, font->cdata);
        glGenTextures(1, &font->tex); GL_ERR();
        glBindTexture(GL_TEXTURE_2D, font->tex); GL_ERR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024,1024,0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap); GL_ERR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GL_ERR();

        font = &g_font_16;
        font->height = 16.0f;
        stbtt_BakeFontBitmap(ttf_buffer,0, font->height, temp_bitmap,1024,1024, 32,96, font->cdata);
        glGenTextures(1, &font->tex); GL_ERR();
        glBindTexture(GL_TEXTURE_2D, font->tex); GL_ERR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024,1024,0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap); GL_ERR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GL_ERR();

        font = &g_font_32;
        font->height = 32.0f;
        stbtt_BakeFontBitmap(ttf_buffer,0, font->height, temp_bitmap,1024,1024, 32,96, font->cdata);
        glGenTextures(1, &font->tex); GL_ERR();
        glBindTexture(GL_TEXTURE_2D, font->tex); GL_ERR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024,1024,0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap); GL_ERR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GL_ERR();

        font = &g_font_64;
        font->height = 64.0f;
        stbtt_BakeFontBitmap(ttf_buffer,0, font->height, temp_bitmap,1024,1024, 32,96, font->cdata);
        glGenTextures(1, &font->tex); GL_ERR();
        glBindTexture(GL_TEXTURE_2D, font->tex); GL_ERR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1024,1024,0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap); GL_ERR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); GL_ERR();

        free(ttf_buffer);
        free(temp_bitmap);

        //FILE* f = fopen("main.c", "rb");
        FILE* f = fopen("C:/Users/mfritz/projects/game0/main.cpp", "rb");
        fseek(f, 0L, SEEK_END);
        text_size = ftell(f);
        rewind(f);
        // Over allocate to allow for overscan.
        text = (char*)malloc(text_size + 64);
        assert(text);
        u64 bytes_read = fread(text, 1, text_size, f);
        assert(bytes_read == text_size);
        for(u32 i = text_size; i < text_size + 64; i++)
        {
            text[i] = 0;
        }
        fclose(f);

        text_to_token_idx = (u32*)malloc(sizeof(u32) * text_size);
        assert(text_to_token_idx);
        for(u32 i = 0; i < text_size; i++)
        {
            text_to_token_idx[i] = ~0U;
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
                type = TOKEN_COMMENT;
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
                type = TOKEN_PREPROCESSOR;
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
            else if(*it == '+')  { type = TOKEN_PLUS;          it++; }
            else if(*it == '-')  { type = TOKEN_MINUS;         it++; }
            else if(*it == '*')  { type = TOKEN_ASTERISK;      it++; }
            else if(*it == '/')  { type = TOKEN_FORWARD_SLASH; it++; }
            else if(*it == '\\') { type = TOKEN_BACK_SLASH;    it++; }
            else if(*it == '~')  { type = TOKEN_TILDE;         it++; }
            else if(*it == '!')  { type = TOKEN_BANG;          it++; }
            else if(*it == '&')  { type = TOKEN_AMPERSAND;     it++; }
            else if(*it == '%')  { type = TOKEN_PERCENT;       it++; }
            else if(*it == '<')  { type = TOKEN_LESS_THAN;     it++; }
            else if(*it == '>')  { type = TOKEN_GREATER_THAN;  it++; }
            else if(*it == '=')  { type = TOKEN_EQUALS;        it++; }
            else if(*it == '|')  { type = TOKEN_BAR;           it++; }
            else if(*it == '^')  { type = TOKEN_CARET;         it++; }
            else if(*it == ',')  { type = TOKEN_COMMA;         it++; }
            else if(*it == '(')  { type = TOKEN_OPEN_PAREN;    it++; }
            else if(*it == ')')  { type = TOKEN_CLOSE_PAREN;   it++; }
            else if(*it == '[')  { type = TOKEN_OPEN_BRACKET;  it++; }
            else if(*it == ']')  { type = TOKEN_CLOSE_BRACKET; it++; }
            else if(*it == '{')  { type = TOKEN_OPEN_BRACE;    it++; }
            else if(*it == '}')  { type = TOKEN_CLOSE_BRACE;   it++; }
            else if(*it == ',')  { type = TOKEN_COMMA;         it++; }
            else if(*it == ':')  { type = TOKEN_COLON;         it++; }
            else if(*it == ';')  { type = TOKEN_SEMICOLON;     it++; }
            else if(*it == '.')  { type = TOKEN_PERIOD;        it++; }
            else if(*it == '?')  { type = TOKEN_QUESTION_MARK; it++; }
            else
            {
                DebugBreak();
                assert(0);
            }

            // Add the token.
            const u64 E = it - text_start;
            tokens[num_tokens].type = type;
            tokens[num_tokens].idx = S;
            tokens[num_tokens].size = E - S;

            for(u64 text_i = S; text_i < E; text_i++)
            {
                text_to_token_idx[text_i] = num_tokens;
            }

            num_tokens++;


            if(it >= text_end)
            {
                break;
            }
        }

        for(u32 token_idx = 0; token_idx < num_tokens;)
        {
            u64 fn_token_idx_start = token_idx;

            const u32 fn_name_token_idx = token_idx;

            // Read fn name
            if(tokens[token_idx].type != TOKEN_IDENTIFIER)
            {
                token_idx++;
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

            Function fn;
            fn.token_idx_start = fn_token_idx_start;
            fn.token_idx_end = token_idx;
            fn.name_token_idx = fn_name_token_idx;
            functions[num_functions++] = fn;
        }

        // Init color data for tokens.
        for(u32 i = 0; i < NUM_TOKEN_TYPES; i++)
        {
            enum ColorId color = WHITE;

            if(i >= TOKEN_KEYWORDS_START && i < TOKEN_KEYWORDS_END) { color = YELLOW; }
            else if(i >= TOKEN_SYMBOLS_START && i < TOKEN_SYMBOLS_END) { color = WHITE; }
            else if(i >= TOKEN_OPERATORS_START && i < TOKEN_OPERATORS_END) { color = WHITE; }
            else if(i == TOKEN_CONSTANT) { color = ORANGE; }
            else if(i == TOKEN_COMMENT) { color = GRAY; }
            else if(i == TOKEN_PREPROCESSOR) { color = GRAY; }

            g_token_colors[i] = color;
        }

    }

    const s64 frame_time_ms = 16LL;
    s64 last_time_ms = get_time_ms();

    float camera_vel_x = 0.0f;
    float camera_vel_y = 0.0f;

    //float camera_pos_x = (float)g_screen_width * 0.25f;
    //float camera_pos_y = (float)g_screen_height * 0.25f;
    float camera_pos_x = 0.0f;
    float camera_pos_y = 0.0f;
    float camera_width = (float)g_screen_width;

    while(1)
    {
        while(get_time_ms() - last_time_ms < frame_time_ms)
        {
            _mm_pause();
        }
        const s64 time_frame_start_ms = get_time_ms();
        last_time_ms = time_frame_start_ms;

        glfwPollEvents();

        if(glfwWindowShouldClose(glfw_window))
        {
            return 0;
        }

        if(g_key_state[GLFW_KEY_ESCAPE])
        {
            return 0;
        }

        // Get mouse position.
        {
            double xpos;
            double ypos;
            glfwGetCursorPos(glfw_window, &xpos, &ypos);
            g_mouse_window_x = (float)xpos;
            g_mouse_window_y = (float)ypos;
        }


        camera_width -= g_mouse_wheel_state * CAMERA_SCROLL_SPEED * camera_width;
        camera_width = max(camera_width, 0.001f);

        float camera_accel_x = 0.0f;
        float camera_accel_y = 0.0f;

        if(g_key_state[GLFW_KEY_W]) camera_accel_y -= 10.0f;
        if(g_key_state[GLFW_KEY_S]) camera_accel_y += 10.0f;

        if(g_key_state[GLFW_KEY_A]) camera_accel_x -= 10.0f;
        if(g_key_state[GLFW_KEY_D]) camera_accel_x += 10.0f;

        camera_vel_x += camera_accel_x;
        camera_vel_y += camera_accel_y;
        camera_pos_x += camera_vel_x;
        camera_pos_y += camera_vel_y;
        camera_vel_x *= 0.825f;
        camera_vel_y *= 0.825f;

        const float aspect_ratio = (float)g_screen_width / (float)g_screen_height;
        const float camera_height = camera_width / aspect_ratio;
        float mouse_camera_x = ((g_mouse_window_x / g_screen_width) - 0.5f);
        float mouse_camera_y = ((g_mouse_window_y / g_screen_height) - 0.5f);

        static float mouse_camera_x_last = 0.0f;
        static float mouse_camera_y_last = 0.0f;
        float mouse_camera_dx = mouse_camera_x - mouse_camera_x_last;
        float mouse_camera_dy = mouse_camera_y - mouse_camera_y_last;
        mouse_camera_x_last = mouse_camera_x;
        mouse_camera_y_last = mouse_camera_y;

        float mouse_world_x = camera_pos_x + mouse_camera_x * camera_width;
        float mouse_world_y = camera_pos_y + mouse_camera_y * camera_height;

        if(g_mouse_key_state[GLFW_MOUSE_BUTTON_LEFT])
        {
            camera_pos_x -= mouse_camera_dx * camera_width;
            camera_pos_y -= mouse_camera_dy * camera_height;
        }

        // Draw
        {
            glViewport(0, 0, g_screen_width, g_screen_height); GL_ERR();
            glClearColor(
                    15.0f / 256.0f,
                    5.0f / 256.0f,
                    40.0f / 256.0f,
                    0); GL_ERR();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GL_ERR();
            glDisable(GL_CULL_FACE); GL_ERR();
            glDisable(GL_DEPTH_TEST); GL_ERR();
            glEnable(GL_BLEND); GL_ERR();
            glEnable(GL_MULTISAMPLE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GL_ERR();

            const Font* font = &g_font_32;

            //const char* entry = "WinMain";
            const char* entry = "wWinMain";
            Function entry_function = {0};
            u8 found_entry = 0;
            for(u32 fn_idx = 0; fn_idx < num_functions; fn_idx++)
            {
                const Function fn = functions[fn_idx];
                const Token fn_name_token = tokens[fn.name_token_idx];
                if(strncmp(text + fn_name_token.idx, entry, strlen(entry)) == 0)
                {
                    entry_function = fn;
                    found_entry = 1;
                }
            }
            assert(found_entry);

            graphics_data->num_text = 0;
            graphics_data->num_quads = 0;

            {
                Cursor cursor = {
                    .x = 0.0f,
                    .y = 0.0f,
                    .transform_x = 0.0f,
                    .transform_y = 0.0f,
                    .transform_scale = 1.0f,
                };
                u32 recurse_depth = 0;
                render_function(
                    graphics_data,
                    &cursor,
                    recurse_depth,
                    text,
                    font,
                    entry_function,
                    functions,
                    num_functions,
                    tokens,
                    num_tokens,
                    text_to_token_idx);
            }

            (void)mouse_world_x;
            (void)mouse_world_y;
            /*
            {
                const float orig_x = 0.0f;
                float x = orig_x;
                float y = 0.0f;

                char buf[512];

                x = orig_x;
                y += 32.0f;
                snprintf(buf, sizeof(buf), "camera_width %f\n", (double)camera_width);
                print(graphics_data, &x, &y, buf, strlen(buf), font, WHITE);

                x = orig_x;
                y += 32.0f;
                snprintf(buf, sizeof(buf), "camera_pos (%f, %f)\n", (double)camera_pos_x, (double)camera_pos_y);
                print(graphics_data, &x, &y, buf, strlen(buf), font, WHITE);

                x = orig_x;
                y += 32.0f;
                snprintf(buf, sizeof(buf), "mouse_window (%f, %f)\n", (double)g_mouse_window_x, (double)g_mouse_window_y);
                print(graphics_data, &x, &y, buf, strlen(buf), font, WHITE);

                x = orig_x;
                y += 32.0f;
                snprintf(buf, sizeof(buf), "mouse_camera (%f, %f)\n", (double)mouse_camera_x, (double)mouse_camera_y);
                print(graphics_data, &x, &y, buf, strlen(buf), font, WHITE);

                x = orig_x;
                y += 32.0f;
                snprintf(buf, sizeof(buf), "mouse_world (%f, %f)\n", (double)mouse_world_x, (double)mouse_world_y);
                print(graphics_data, &x, &y, buf, strlen(buf), font, WHITE);
            }
            */


            // TODO(mfritz) remove
            /*
            (void)mouse_world_x;
            (void)mouse_world_y;
            {
                Cursor c = {
                    .x = mouse_world_x,
                    .y = mouse_world_y,
                    .transform_x = 0.0f,
                    .transform_y = 0.0f,
                    .transform_scale = 1.0f,
                };

                u32 num = graphics_data->num;

                for(u32 i = 0; i < 10; i++)
                {
                    graphics_data->x[num] = c.x * c.transform_scale + c.transform_x;
                    graphics_data->y[num] = c.y * c.transform_scale + c.transform_y;
                    graphics_data->w[num] = 100.0f * c.transform_scale;
                    graphics_data->h[num] = 100.0f * c.transform_scale;
                    graphics_data->r[num] = g_color_data[WHITE*3 + 0];
                    graphics_data->g[num] = g_color_data[WHITE*3 + 1];
                    graphics_data->b[num] = g_color_data[WHITE*3 + 2];
                    num++;

                    c.transform_x += 100.0f * c.transform_scale;
                    c.transform_y += 100.0f * c.transform_scale;
                    c.transform_scale *= 0.5f;
                }

                graphics_data->num = num;
            }
            */




            static const u32 BATCH_SIZE = 1024;
#if 1
            //for(u32 batch_i = 0; batch_i < graphics_data->num_quads; batch_i += BATCH_SIZE)
            for(s32 batch_i = (s32)graphics_data->num_quads; batch_i >= 0; batch_i -= 1)
            {
                //u32 batch_size = min_u32(graphics_data->num_quads - batch_i, BATCH_SIZE);
                u32 batch_size = 1;

                glUseProgram(color_shader_program); GL_ERR();
                const float view_proj_mat[] = {
                    2.0f / camera_width, 0.0f, 0.0f, -camera_pos_x / (camera_width*0.5f),
                    0.0f, -2.0f / camera_height, 0.0f, camera_pos_y / (camera_height*0.5f),
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f,
                };
                GLint loc = glGetUniformLocation(color_shader_program, "u_view_proj_mat"); GL_ERR();
                assert(loc != -1);
                glUniformMatrix4fv(loc, 1, 1, &(view_proj_mat[0])); GL_ERR();
                
                glBindVertexArray(vao); GL_ERR();

                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo); GL_ERR();

                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  0 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->quad_x + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  1 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->quad_y + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  2 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->quad_w + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  3 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->quad_h + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  4 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->quad_r + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  5 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->quad_g + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  6 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->quad_b + batch_i); GL_ERR();

                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo); GL_ERR();
                glBindBuffer(GL_ARRAY_BUFFER, vbo); GL_ERR();
                glBindTexture(GL_TEXTURE_2D, font->tex); GL_ERR();

                glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, batch_size); GL_ERR();
            }
#endif

            for(u32 batch_i = 0; batch_i < graphics_data->num_text; batch_i += BATCH_SIZE)
            {
                u32 batch_size = min_u32(graphics_data->num_text - batch_i, BATCH_SIZE);

                glUseProgram(shader_program); GL_ERR();
                const float view_proj_mat[] = {
                    2.0f / camera_width, 0.0f, 0.0f, -camera_pos_x / (camera_width*0.5f),
                    0.0f, -2.0f / camera_height, 0.0f, camera_pos_y / (camera_height*0.5f),
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f,
                };
                GLint loc = glGetUniformLocation(shader_program, "u_view_proj_mat"); GL_ERR();
                assert(loc != -1);
                glUniformMatrix4fv(loc, 1, 1, &(view_proj_mat[0])); GL_ERR();
                
                glBindVertexArray(vao); GL_ERR();

                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo); GL_ERR();

                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  0 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_x + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  1 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_y + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  2 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_w + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  3 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_h + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  4 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_r + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  5 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_g + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  6 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_b + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  7 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_tex_l + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  8 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_tex_r + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER,  9 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_tex_b + batch_i); GL_ERR();
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, 10 * SSBO_STRIDE, sizeof(float) * batch_size, graphics_data->text_tex_t + batch_i); GL_ERR();

                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo); GL_ERR();
                glBindBuffer(GL_ARRAY_BUFFER, vbo); GL_ERR();
                glBindTexture(GL_TEXTURE_2D, font->tex); GL_ERR();

                glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, batch_size); GL_ERR();

                //glUseProgram(color_shader_program); GL_ERR();
                //loc = glGetUniformLocation(color_shader_program, "u_view_proj_mat"); GL_ERR();
                //assert(loc != -1);
                //glUniformMatrix4fv(loc, 1, 1, &(view_proj_mat[0])); GL_ERR();
                //glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, batch_size); GL_ERR();
            }

            glFlush(); GL_ERR();
            glFinish(); GL_ERR();
        }

        {
            memcpy(g_key_state_last, g_key_state, sizeof(g_key_state));
            memcpy(g_mouse_key_state_last, g_mouse_key_state, sizeof(g_mouse_key_state));
            g_mouse_wheel_state = 0;

            g_mouse_window_x_last = g_mouse_window_x;
            g_mouse_window_y_last = g_mouse_window_y;
        }

        glfwSwapBuffers(glfw_window);

    }
    return 0;
}

