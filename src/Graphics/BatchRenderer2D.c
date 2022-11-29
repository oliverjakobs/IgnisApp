#include "BatchRenderer2D.h"

#include "Renderer.h"

typedef struct
{
	IgnisVertexArray vao;
	IgnisShader shader;

	float* vertices;
	size_t vertex_index;

	size_t quad_count;

	GLuint* texture_slots;
	size_t texture_slot_index;

	GLint uniform_location_view_proj;
} _BatchRenderer2DStorage;

static _BatchRenderer2DStorage _render_data;

void BatchRenderer2DInit(const char* vert, const char* frag)
{
	ignisGenerateVertexArray(&_render_data.vao);

	IgnisBufferElement layout[] =
	{
		{GL_FLOAT, 3, GL_FALSE},	/* position */
		{GL_FLOAT, 2, GL_FALSE},	/* Texture coords*/
		{GL_FLOAT, 1, GL_FALSE}		/* Texture index */
	};

	ignisAddArrayBufferLayout(&_render_data.vao, BATCHRENDERER2D_BUFFER_SIZE * sizeof(float), NULL, GL_DYNAMIC_DRAW, 0, layout, 3);

	GLuint indices[BATCHRENDERER2D_INDEX_COUNT];
	GenerateIndices(indices, BATCHRENDERER2D_INDEX_COUNT, BATCHRENDERER2D_INDICES_PER_QUAD);

	ignisLoadElementBuffer(&_render_data.vao, indices, BATCHRENDERER2D_INDEX_COUNT, GL_STATIC_DRAW);

	ignisCreateShadervf(&_render_data.shader, vert, frag);

	ignisUseShader(&_render_data.shader);
	int samplers[BATCHRENDERER2D_TEXTURES];
	for (int i = 0; i < BATCHRENDERER2D_TEXTURES; i++)
	{
		samplers[i] = i;
	}
	ignisSetUniform1iv(&_render_data.shader, "u_Textures", BATCHRENDERER2D_TEXTURES, samplers);

	_render_data.uniform_location_view_proj = ignisGetUniformLocation(&_render_data.shader, "u_ViewProjection");

	_render_data.vertices = ignisMalloc(BATCHRENDERER2D_BUFFER_SIZE * sizeof(float));
	_render_data.vertex_index = 0;
	_render_data.quad_count = 0;

	_render_data.texture_slots = ignisMalloc(BATCHRENDERER2D_TEXTURES * sizeof(GLuint));
	if (_render_data.texture_slots) memset(_render_data.texture_slots, 0, BATCHRENDERER2D_TEXTURES * sizeof(GLuint));
	_render_data.texture_slot_index = 0;
}

void BatchRenderer2DDestroy()
{
	ignisFree(_render_data.vertices);
	ignisFree(_render_data.texture_slots);

	ignisDeleteShader(&_render_data.shader);
	ignisDeleteVertexArray(&_render_data.vao);
}

void BatchRenderer2DStart(const float* mat_view_proj)
{
	ignisSetUniformMat4l(&_render_data.shader, _render_data.uniform_location_view_proj, mat_view_proj);
}

void BatchRenderer2DFlush()
{
	if (_render_data.vertex_index == 0) return;

	ignisBindVertexArray(&_render_data.vao);
	ignisBufferSubData(&_render_data.vao.array_buffers[0], 0, _render_data.vertex_index * sizeof(float), _render_data.vertices);

	ignisUseShader(&_render_data.shader);

	/* bind textures */
	for (size_t i = 0; i < _render_data.texture_slot_index; i++)
	{
		glActiveTexture(GL_TEXTURE0 + (GLenum)i);
		glBindTexture(GL_TEXTURE_2D, _render_data.texture_slots[i]);
	}

	glDrawElements(GL_TRIANGLES, BATCHRENDERER2D_INDICES_PER_QUAD * (GLsizei)_render_data.quad_count, GL_UNSIGNED_INT, NULL);

	_render_data.vertex_index = 0;
	_render_data.quad_count = 0;

	/* textures */
	memset(_render_data.texture_slots, 0, BATCHRENDERER2D_TEXTURES * sizeof(GLuint));
	_render_data.texture_slot_index = 0;
}

static void _BatchRenderer2DPushValue(float value)
{
	if (_render_data.vertex_index >= BATCHRENDERER2D_BUFFER_SIZE)
	{
		_ignisErrorCallback(IGNIS_WARN, "[BatchRenderer2D] Vertex overflow");
		return;
	}

	_render_data.vertices[_render_data.vertex_index++] = value;
}

void BatchRenderer2DRenderTexture(const IgnisTexture2D* texture, float x, float y, float w, float h)
{
	BatchRenderer2DRenderTextureSrc(texture, x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);
}

void BatchRenderer2DRenderTextureFrame(const IgnisTexture2D* texture, float x, float y, float w, float h, size_t frame)
{
	float src_x, src_y, src_w, src_h;
	GetTexture2DSrcRect(texture, frame, &src_x, &src_y, &src_w, &src_h);

	BatchRenderer2DRenderTextureSrc(texture, x, y, w, h, src_x, src_y, src_w, src_h);
}

void BatchRenderer2DRenderTextureSrc(const IgnisTexture2D* texture, float x, float y, float w, float h, float src_x, float src_y, float src_w, float src_h)
{
	if (_render_data.quad_count >= BATCHRENDERER2D_MAX_QUADS || _render_data.texture_slot_index >= BATCHRENDERER2D_TEXTURES)
		BatchRenderer2DFlush();

	float texture_index = -1.0f;
	for (size_t i = 0; i < _render_data.texture_slot_index; i++)
	{
		if (_render_data.texture_slots[i] == texture->name)
		{
			texture_index = (float)i;
			break;
		}
	}

	if (texture_index < 0.0f)
	{
		texture_index = (float)_render_data.texture_slot_index;
		_render_data.texture_slots[_render_data.texture_slot_index++] = texture->name;
	}

	/* BOTTOM LEFT */
	_BatchRenderer2DPushValue(x);
	_BatchRenderer2DPushValue(y);
	_BatchRenderer2DPushValue(0.0f);

	_BatchRenderer2DPushValue(src_x);
	_BatchRenderer2DPushValue(src_y);

	_BatchRenderer2DPushValue(texture_index);

	/* BOTTOM RIGHT */
	_BatchRenderer2DPushValue(x + w);
	_BatchRenderer2DPushValue(y);
	_BatchRenderer2DPushValue(0.0f);

	_BatchRenderer2DPushValue(src_x + src_w);
	_BatchRenderer2DPushValue(src_y);

	_BatchRenderer2DPushValue(texture_index);

	/* TOP RIGHT */
	_BatchRenderer2DPushValue(x + w);
	_BatchRenderer2DPushValue(y + h);
	_BatchRenderer2DPushValue(0.0f);

	_BatchRenderer2DPushValue(src_x + src_w);
	_BatchRenderer2DPushValue(src_y + src_h);

	_BatchRenderer2DPushValue(texture_index);

	/* TOP LEFT */
	_BatchRenderer2DPushValue(x);
	_BatchRenderer2DPushValue(y + h);
	_BatchRenderer2DPushValue(0.0f);

	_BatchRenderer2DPushValue(src_x);
	_BatchRenderer2DPushValue(src_y + src_h);

	_BatchRenderer2DPushValue(texture_index);

	_render_data.quad_count++;
}
