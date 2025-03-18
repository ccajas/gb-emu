#include "graphics.h"

const char * default_fs_source =
"#version 330\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D indexed;\n"
"uniform vec2 screenSize;\n"

"void main()\n"
"{\n"
"    vec3 sampled = texture2D(indexed, TexCoords).rgb;\n"
"    gl_FragColor = vec4(sampled, 1.0);\n"
"}\n";

const char * ppu_fs_source =
"#version 330\n"
"varying vec2 TexCoords;\n"
"uniform sampler2D colorPalette;\n"
"uniform sampler2D indexed;\n"
"uniform vec3 textColor;\n"
"uniform vec2 screenSize;\n"

"#define PI_2          3.1415926538 * 2\n"

// Define main shader used
"#define SCREEN_SHADER dot_matrix\n"

"uniform float rr = 0.640000;\n"
"uniform float rg = 0.660000;\n"
"uniform float rb = 0.720000;\n"
"uniform float colorP = 1.2;\n"
"uniform float brightness = 3.0;\n"

"vec3 dot_matrix(vec3 color, vec3 tint)\n"
"{\n"
"    vec2 position = (TexCoords.xy);\n"
"    float dotTint = 0.15;\n"
"    if (color.r + color.g + color.b > 7.3) dotTint = 0;\n"
"    //color = vec3(0.05) + (color * vec3(0.95)) - 0.05;\n"
"    if (fract(position.x * screenSize.x) <= 0.25) color = mix(color, vec3(1), dotTint * 1.25);\n"
"    if (fract(position.y * screenSize.y) <= 0.25) color = mix(color, vec3(1), dotTint);\n"
"    if (fract(position.x * screenSize.x) >= 0.75) color = mix(color, vec3(1), dotTint * 1.25);\n"
"    if (fract(position.y * screenSize.y) >= 0.75) color = mix(color, vec3(1), dotTint);\n"
"    color = vec3(pow(color.r, 1.2), pow(color.g, 1.2), pow(color.b, 1.2));\n"
"    return color;\n"
"}\n"

"#define COMPAT_PRECISION mediump\n"

// ### Magic Numbers...
"#define GRID_INTENSITY 1.0\n"
"#define GRID_WIDTH 1.0\n"
"#define GRID_BIAS 1.0\n"
"#define DARKEN_GRID 0.0\n"
"#define DARKEN_COLOR 0.0\n"

// Grid pattern
// > Line weighting equation:
//   y = a * (x^4 - b * x^6)
"const COMPAT_PRECISION float LINE_WEIGHT_A = 48.0;\n"
"const COMPAT_PRECISION float LINE_WEIGHT_B = 8.0 / 3.0;\n"

// RGB -> Luminosity conversion
// > Photometric/digital ITU BT.709
"#define LUMA_R 0.2126\n"
"#define LUMA_G 0.7152\n"
"#define LUMA_B 0.0722\n"

// Background texture size
"#define BG_TEXTURE_SIZE 2048.0\n"
"const COMPAT_PRECISION float INV_BG_TEXTURE_SIZE = 1.0 / BG_TEXTURE_SIZE;\n"

"vec3 simpletex_lcd(vec3 color, vec3 tint)\n"
"{\n"
	// Get current texture coordinate
"	COMPAT_PRECISION vec2 imgPixelCoord = (TexCoords.xy * screenSize.xy);\n"
"	COMPAT_PRECISION vec2 imgCenterCoord = floor(imgPixelCoord.xy) + vec2(0.5, 0.5);\n"
	
	// Darken colors (if required...)
"	color.rgb = pow(color.rgb, vec3(1.0 + DARKEN_COLOR));\n"
	
	// Generate grid pattern...
"	COMPAT_PRECISION vec2 distFromCenter = abs(imgCenterCoord.xy - imgPixelCoord.xy);\n"
	
"	COMPAT_PRECISION float xSquared = max(distFromCenter.x, distFromCenter.y);\n"
"	xSquared = xSquared * xSquared;\n"
	
"	COMPAT_PRECISION float xQuarted = xSquared * xSquared;\n"
	
	// > Line weighting equation:
	//   y = 48 * (x^4 - (8/3) * x^6)
"	COMPAT_PRECISION float lineWeight = LINE_WEIGHT_A * (xQuarted - (LINE_WEIGHT_B * xQuarted * xSquared));\n"
	
	// > Apply grid adjustments (phase 1)
	//   - GRID_WIDTH:
	//        1.0: Use raw lineWeight value
	//        0.0: Use lineWeight ^ 2 (makes lines thinner - realistically, this is
	//                                 the thinnest we can go before the grid effect
	//                                 becomes pointless, particularly with 'high resolution'
	//                                 systems like the GBA)
	//   - GRID_INTENSITY:
	//        1.0: Grid lines are white
	//        0.0: Grid lines are invisible
"	lineWeight = lineWeight * (lineWeight + ((1.0 - lineWeight) * GRID_WIDTH)) * GRID_INTENSITY;\n"
	
	// > Apply grid adjustments (phase 2)
	//   - GRID_BIAS:
	//        0.0: Use 'unbiased' lineWeight value calculated above
	//        1.0: Scale lineWeight by current pixel luminosity
	//             > i.e. the darker the pixel, the lower the intensity of the grid
"	COMPAT_PRECISION float luma = (LUMA_R * color.r) + (LUMA_G * color.g) + (LUMA_B * color.b);\n"
"	lineWeight = lineWeight * (luma + ((1.0 - luma) * (1.0 - GRID_BIAS)));\n"
	
	// Apply grid pattern
	// (lineWeight == 1 -> set color to value specified by DARKEN_GRID)
"	color.rgb = mix(color.rgb, vec3(1.0 - DARKEN_GRID), lineWeight);\n"

	// Sample background texture and 'colorise' according to current pixel color
	// (NB: the 'colorisation' here is lame, but the proper method is slow...)
"	COMPAT_PRECISION vec3 bgTexture = vec3(0.9, 0.9, 0.9) * color.rgb;\n"
	
	// Blend current pixel with background according to luminosity
	// (lighter color == more transparent, more visible background)
	// Note: Have to calculate luminosity a second time... tiresome, but
	// it's not a particulary expensive operation...
"	luma = (LUMA_R * color.r) + (LUMA_G * color.g) + (LUMA_B * color.b);\n"
"	color.rgb = mix(color.rgb, bgTexture.rgb, luma);\n"
"	return color;\n"
"}\n"

"vec3 lcd_subpixel(vec3 color, vec3 tint)\n"
"{\n"
"    vec2 position = (TexCoords.xy);\n"
"    if (fract(position.x * screenSize.x) > 0.75) color = mix(color, vec3(0), 0.08);\n"
"    float tr = sin(position.x * screenSize.x * PI_2) + color.r * 2.0;\n"
"    float tg = sin((position.x + 0.33) * screenSize.x * PI_2) + color.g * 2.0;\n"
"    float tb = sin((position.x + 0.67) * screenSize.x * PI_2) + color.b * 2.0;\n"
"    color.r *= tr * rr; color.g *= tg * rg; color.b *= tb * rb;\n"
     // Gamma adjustment
"    color.r = pow(color.r, colorP);\n"
"    color.g = pow(color.g, colorP);\n"
"    color.b = pow(color.b, colorP);\n"
     // Scanline separation
"    if (fract(position.y * screenSize.y) > 0.75) color = mix(color, vec3(0), 0.7);\n"
"    return color * brightness;\n"
"}\n"

"vec3 passthru(vec3 color, vec3 tint)\n"
"{\n"
"    color = vec3(pow(color.r, 1.2), pow(color.g, 1.2), pow(color.b, 1.2));\n"
"    return color;\n"
"}\n"

"void main()\n"
"{\n"
"    vec3 tint = vec3(0.37, 0.84, 0.87);\n"
"    vec3 tint2 = vec3(0.61, 0.74, 0.03);\n"
"    vec3 sampled = texture2D(indexed, TexCoords).rgb;\n"
"    gl_FragColor = vec4(SCREEN_SHADER(sampled, tint2), 1.0);\n"
"}\n";

const char * ppu_vs_source =
"#version 330\n"
"layout (location = 0) in vec4 vertex;\n"
"layout (location = 1) in vec2 texture;"
"out vec2 TexCoords;\n"
"uniform mat4 projection;\n"
"uniform mat4 model;\n"
"uniform vec2 screenSize;\n"

"void main()\n"
"{\n"
"    vec4 localPos = model * vec4(vertex.xy, 1.0, 1.0);"
"    gl_Position = projection * vec4(localPos.xy, 0.0, 1.0);\n"
"    TexCoords = texture.xy;\n"
"}\n";

uint32_t quadVAO[2] = { 0, 0 };

void draw_lazy_quad(const float width, const float height, const int i)
{
    while (quadVAO[i] == 0)
    {
        uint32_t quadVBO = 0;
        const float quadVertices[] = {

            0.0f,  height, 0.0f, 0.0f, 0.0f,
            0.0f,  0.0f,   0.0f, 0.0f, 1.0f,
            width, height, 0.0f, 1.0f, 0.0f,
            width, 0.0f,   0.0f, 1.0f, 1.0f,
        };

        glGenVertexArrays(1, &quadVAO[i]);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO[i]);

        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0); 

        break;
    }

    glBindVertexArray(quadVAO[i]);
    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void graphics_init (Scene * const scene)
{
    gladLoadGL();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    /* Create shaders */
    scene->fbufferShader = shader_init_source (ppu_vs_source, ppu_fs_source);
    scene->debugShader   = shader_init_source (ppu_vs_source, default_fs_source);

    scene->activeShader  = &scene->debugShader;
    scene->activeTexture = &scene->debugTexture;

    /* Create main textures */
    texture_setup (&scene->fbufferTexture, 160, 144, GL_NEAREST, NULL);
    texture_setup (&scene->debugTexture,   320, 288, GL_LINEAR, NULL);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDepthFunc(GL_LEQUAL);
}

/* Draw a textured quad on the display */
void draw_quad (Scene * const scene, 
    struct Texture * const pixels, 
    const int xpos, const int ypos, const float scale)
{
    /* Setup matrix and send data to shader */
    glUseProgram(scene->activeShader->program);
    mat4x4 model;
    mat4x4 projection;

    mat4x4_ortho (projection, 0, (pixels->width * scale), 0, (pixels->height * scale), 0, 0.1f);
    mat4x4_identity (model);

    /* Translated as if the top left corner is x:0 y:0 */
    mat4x4_translate (model, 0, 0, 0);
    mat4x4_scale_aniso (model, model, pixels->width * scale, pixels->height * scale, 1.0f);

    glUniform2f (glGetUniformLocation(scene->activeShader->program, "screenSize"), (GLfloat) pixels->width, (GLfloat) pixels->height);
    glUniformMatrix4fv (glGetUniformLocation(scene->activeShader->program, "model"),      1, GL_FALSE, (const GLfloat*) model);
    glUniformMatrix4fv (glGetUniformLocation(scene->activeShader->program, "projection"), 1, GL_FALSE, (const GLfloat*) projection);

    glActiveTexture (GL_TEXTURE0);

    /* Draw quad */
    glBindTexture (GL_TEXTURE_2D, *scene->activeTexture);
    glTexImage2D  (GL_TEXTURE_2D, 0, GL_RGBA, pixels->width, pixels->height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels->imgData);
	draw_lazy_quad(1.0f, 1.0f, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void draw_screen_quad (Scene * const scene, struct Texture * const pixels, const float scale)
{
    draw_quad (scene, pixels, 0, 0, scale);
}