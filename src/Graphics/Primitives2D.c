#include "Primitives2D.h"

#include "Renderer.h"

#include <math.h>

/* ---------------------| Lines |----------------------------------------------*/
typedef struct
{
	IgnisVertexArray vao;
	IgnisShader shader;

	float* vertices;
	GLsizei vertex_count;
} Primitives2DLines;

static int Primitives2DLinesCreate(Primitives2DLines* lines, const char* vert, const char* frag, IgnisBufferElement* layout, size_t layout_size)
{
	ignisGenerateVertexArray(&lines->vao);
	ignisAddArrayBufferLayout(&lines->vao, PRIMITIVES2D_LINES_BUFFER_SIZE * sizeof(float), NULL, GL_DYNAMIC_DRAW, 0, layout, layout_size);

	lines->vertices = ignisMalloc(PRIMITIVES2D_LINES_BUFFER_SIZE * sizeof(float));
	lines->vertex_count = 0;

	ignisCreateShadervf(&lines->shader, vert, frag);

	return 1;
}

static void Primitives2DLinesDestroy(Primitives2DLines* lines)
{
	ignisDeleteShader(&lines->shader);
	ignisDeleteVertexArray(&lines->vao);

	ignisFree(lines->vertices);
}

static void Primitives2DLinesStart(Primitives2DLines* lines, const float* mat_view_proj)
{
	ignisSetUniformMat4(&lines->shader, "u_ViewProjection", mat_view_proj);
}

static void Primitives2DLinesFlush(Primitives2DLines* lines)
{
	if (lines->vertex_count == 0) return;

	ignisBindVertexArray(&lines->vao);
	ignisBufferSubData(&lines->vao.array_buffers[0], 0, lines->vertex_count * sizeof(float), lines->vertices);

	ignisUseShader(&lines->shader);

	glDrawArrays(GL_LINES, 0, lines->vertex_count / PRIMITIVES2D_VERTEX_SIZE);

	lines->vertex_count = 0;
}

static void Primitives2DLinesVertex(Primitives2DLines* lines, float x, float y, const IgnisColorRGBA color)
{
	if (lines->vertex_count >= PRIMITIVES2D_LINES_BUFFER_SIZE) Primitives2DLinesFlush(lines);

	lines->vertices[lines->vertex_count++] = x;
	lines->vertices[lines->vertex_count++] = y;

	lines->vertices[lines->vertex_count++] = color.r;
	lines->vertices[lines->vertex_count++] = color.g;
	lines->vertices[lines->vertex_count++] = color.b;
	lines->vertices[lines->vertex_count++] = color.a;
}
/* ---------------------| !Lines |---------------------------------------------*/

/* ---------------------| Triangles |----------------------------------------------*/
typedef struct
{
	IgnisVertexArray vao;
	IgnisShader shader;

	float* vertices;
	GLsizei vertex_count;
} Primitives2DTriangles;

static int Primitives2DTrianglesCreate(Primitives2DTriangles* triangles, const char* vert, const char* frag, IgnisBufferElement* layout, size_t layout_size)
{
	ignisGenerateVertexArray(&triangles->vao);
	ignisAddArrayBufferLayout(&triangles->vao, PRIMITIVES2D_TRIANGLES_BUFFER_SIZE * sizeof(float), NULL, GL_DYNAMIC_DRAW, 0, layout, layout_size);

	triangles->vertices = ignisMalloc(PRIMITIVES2D_TRIANGLES_BUFFER_SIZE * sizeof(float));
	triangles->vertex_count = 0;

	ignisCreateShadervf(&triangles->shader, vert, frag);

	return 1;
}

static void Primitives2DTrianglesDestroy(Primitives2DTriangles* triangles)
{
	ignisDeleteShader(&triangles->shader);

	ignisDeleteVertexArray(&triangles->vao);

	ignisFree(triangles->vertices);
}

static void Primitives2DTrianglesStart(Primitives2DTriangles* triangles, const float* mat_view_proj)
{
	ignisSetUniformMat4(&triangles->shader, "u_ViewProjection", mat_view_proj);
}

static void Primitives2DTrianglesFlush(Primitives2DTriangles* triangles)
{
	if (triangles->vertex_count == 0) return;

	ignisBindVertexArray(&triangles->vao);
	ignisBufferSubData(&triangles->vao.array_buffers[0], 0, triangles->vertex_count * sizeof(float), triangles->vertices);

	ignisUseShader(&triangles->shader);

	glDrawArrays(GL_TRIANGLES, 0, triangles->vertex_count / PRIMITIVES2D_VERTEX_SIZE);

	triangles->vertex_count = 0;
}

static void Primitives2DTrianglesVertex(Primitives2DTriangles* triangles, float x, float y, const IgnisColorRGBA color)
{
	if (triangles->vertex_count >= PRIMITIVES2D_TRIANGLES_BUFFER_SIZE) Primitives2DTrianglesFlush(triangles);

	triangles->vertices[triangles->vertex_count++] = x;
	triangles->vertices[triangles->vertex_count++] = y;

	triangles->vertices[triangles->vertex_count++] = color.r;
	triangles->vertices[triangles->vertex_count++] = color.g;
	triangles->vertices[triangles->vertex_count++] = color.b;
	triangles->vertices[triangles->vertex_count++] = color.a;
}
/* ---------------------| !Triangles |---------------------------------------------*/

static Primitives2DLines _lines;
static Primitives2DTriangles _triangles;

void Primitives2DInit(const char* vert, const char* frag)
{
	IgnisBufferElement layout[] = { {GL_FLOAT, 2, GL_FALSE}, {GL_FLOAT, 4, GL_FALSE} };
	size_t layout_size = 2;

	Primitives2DLinesCreate(&_lines, vert, frag, layout, layout_size);
	Primitives2DTrianglesCreate(&_triangles, vert, frag, layout, layout_size);
}

void Primitives2DDestroy()
{
	Primitives2DLinesDestroy(&_lines);
	Primitives2DTrianglesDestroy(&_triangles);
}

void Primitives2DStart(const float* mat_view_proj)
{
	Primitives2DLinesStart(&_lines, mat_view_proj);
	Primitives2DTrianglesStart(&_triangles, mat_view_proj);
}

void Primitives2DFlush()
{
	Primitives2DLinesFlush(&_lines);
	Primitives2DTrianglesFlush(&_triangles);
}

void Primitives2DRenderLine(float x1, float y1, float x2, float y2, IgnisColorRGBA color)
{
	Primitives2DLinesVertex(&_lines, x1, y1, color);
	Primitives2DLinesVertex(&_lines, x2, y2, color);
}

void Primitives2DRenderRect(float x, float y, float w, float h, IgnisColorRGBA color)
{
	float vertices[] =
	{
		x + 0, y + 0,
		x + w, y + 0,
		x + w, y + h,
		x + 0, y + h
	};

	Primitives2DRenderPolygon(vertices, 8, color);
}

void Primitives2DRenderPolygon(float* vertices, size_t count, IgnisColorRGBA color)
{
	if (!vertices || count < 2) return;

	float p1x = vertices[count - 2];
	float p1y = vertices[count - 1];

	size_t i = 0;
	while (i < count - 1)
	{
		float p2x = vertices[i++];
		float p2y = vertices[i++];

		Primitives2DLinesVertex(&_lines, p1x, p1y, color);
		Primitives2DLinesVertex(&_lines, p2x, p2y, color);

		p1x = p2x;
		p1y = p2y;
	}
}

void Primitives2DRenderCircle(float x, float y, float radius, IgnisColorRGBA color)
{
	float sinInc = sinf(PRIMITIVES2D_K_INCREMENT);
	float cosInc = cosf(PRIMITIVES2D_K_INCREMENT);

	float r1x = 1.0f;
	float r1y = 0.0f;

	float v1x = x + radius * r1x;
	float v1y = y + radius * r1y;

	for (int i = 0; i < PRIMITIVES2D_K_SEGMENTS; ++i)
	{
		/* Perform rotation to avoid additional trigonometry. */
		float r2x = cosInc * r1x - sinInc * r1y;
		float r2y = sinInc * r1x + cosInc * r1y;

		float v2x = x + radius * r2x;
		float v2y = y + radius * r2y;

		Primitives2DLinesVertex(&_lines, v1x, v1y, color);
		Primitives2DLinesVertex(&_lines, v2x, v2y, color);

		r1x = r2x;
		r1y = r2y;
		v1x = v2x;
		v1y = v2y;
	}
}

void Primitives2DFillRect(float x, float y, float w, float h, IgnisColorRGBA color)
{
	Primitives2DTrianglesVertex(&_triangles, x + 0, y + 0, color);
	Primitives2DTrianglesVertex(&_triangles, x + w, y + 0, color);
	Primitives2DTrianglesVertex(&_triangles, x + w, y + h, color);

	Primitives2DTrianglesVertex(&_triangles, x + w, y + h, color);
	Primitives2DTrianglesVertex(&_triangles, x + 0, y + h, color);
	Primitives2DTrianglesVertex(&_triangles, x + 0, y + 0, color);
}

void Primitives2DFillPolygon(float* vertices, size_t count, IgnisColorRGBA color)
{
	if (!vertices || count < 2) return;

	for (size_t i = 2; i < count - 3; ++i)
	{
		Primitives2DTrianglesVertex(&_triangles, vertices[0], vertices[1], color);
		Primitives2DTrianglesVertex(&_triangles, vertices[i + 0], vertices[i + 1], color);
		Primitives2DTrianglesVertex(&_triangles, vertices[i + 2], vertices[i + 3], color);
	}
}

void Primitives2DFillCircle(float x, float y, float radius, IgnisColorRGBA color)
{
	float sinInc = sinf(PRIMITIVES2D_K_INCREMENT);
	float cosInc = cosf(PRIMITIVES2D_K_INCREMENT);

	float r1x = 1.0f;
	float r1y = 0.0f;

	float v1x = x + radius * r1x;
	float v1y = y + radius * r1y;

	for (int i = 0; i < PRIMITIVES2D_K_SEGMENTS; ++i)
	{
		/* Perform rotation to avoid additional trigonometry. */
		float r2x = cosInc * r1x - sinInc * r1y;
		float r2y = sinInc * r1x + cosInc * r1y;

		float v2x = x + radius * r2x;
		float v2y = y + radius * r2y;

		Primitives2DTrianglesVertex(&_triangles, x, y, color);
		Primitives2DTrianglesVertex(&_triangles, v1x, v1y, color);
		Primitives2DTrianglesVertex(&_triangles, v2x, v2y, color);

		r1x = r2x;
		r1y = r2y;
		v1x = v2x;
		v1y = v2y;
	}
}
