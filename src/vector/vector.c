// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<vector.c>>

#include "vector/vector.h"

typedef u8	data_1B;
typedef u16	data_2B;
typedef u32	data_4B;
typedef u64	data_8B;

typedef struct [[gnu::packed]] __data_3B {
	data_2B	_2B;
	data_1B	_1B;
}	data_3B;

typedef struct [[gnu::packed]] __data_5B {
	data_4B	_4B;
	data_1B	_1B;
}	data_5B;

typedef struct [[gnu::packed]] __data_6B {
	data_4B	_4B;
	data_2B	_2B;
}	data_6B;

typedef struct [[gnu::packed]] __data_7B {
	data_6B	_6B;
	data_1B	_1B;
}	data_7B;

struct __vector {
	void	(*free)(void *);
	size_t	element_size;
	size_t	capacity;
	size_t	elements;
	data_1B	*data;
};

#define _MAX_ELEM_SIZE_T	data_8B

#define _u64_to_data_3B(val)	((data_3B){._2B = val >> 8 & 0xFFFF, ._1B = val & 0xFF})
#define _u64_to_data_5B(val)	((data_5B){._4B = val >> 8 & 0xFFFFFFFF, ._1B = val & 0xFF})
#define _u64_to_data_6B(val)	((data_6B){._4B = val >> 16 & 0xFFFFFFFF, ._2B = val & 0xFFFF})
#define _u64_to_data_7B(val)	((data_7B){._6B = _u64_to_data_6B(val >> 8), ._1B = val & 0xFF})

#define _data_3B_to_u64(d)	((d)._2B << 8 | (d)._1B)
#define _data_5B_to_u64(d)	((d)._4B << 8 | (d)._1B)
#define _data_6B_to_u64(d)	((d)._4B << 16 | (d)._2B)
#define _data_7B_to_u64(d)	((_data_6B_to_u64((d)._6B) << 8) | (d)._1B)

static inline void	*_get_element(const vector vec, const size_t i);
static inline void	_set_element(vector vec, const size_t i, const u64 val);
static inline u64	_get_value(const vector vec, const void *elem);

vector	__vec_new(const size_t size, const size_t count, void (*_free)(void *)) {
	vector	out;

	out = (size && size <= sizeof(_MAX_ELEM_SIZE_T)) ? malloc(sizeof(*out)) : NULL;
	if (out) {
		out->data = malloc(size * count);
		if (!out->data) {
			free(out);
			return NULL;
		}
		out->element_size = size;
		out->capacity = count;
		out->elements = 0;
		out->free = _free;
	}
	return out;
}

void	__vec_del(vector vec) {
	if (vec) {
		__vec_clr(vec);
		free(vec->data);
		free(vec);
	}
}

u8	__vec_psh(vector vec, const u64 val) {
	if (vec->elements == vec->capacity) {
		vec->capacity *= 2;
		vec->data = realloc(vec->data, vec->capacity * vec->element_size);
	}
	if (vec->data)
		_set_element(vec, vec->elements++, val);
	return (vec->data) ? 1 : 0;
}

void	__vec_pop(vector vec) {
	if (vec->elements) {
		vec->elements--;
		if (vec->free)
			vec->free(*(void **)_get_element(vec, vec->elements));
	}
}

u64	__vec_get(const vector vec, const size_t i) {
	return (i < vec->elements) ? _get_value(vec, _get_element(vec, i)) : VECTOR_INDEX_OUT_OF_BOUNDS;
}

u8	__vec_set(vector vec, const size_t i, const u64 val) {
	if (i < vec->elements)
		_set_element(vec, i, val);
	return (i < vec->elements) ? 1 : 0;
}

size_t	__vec_sze(const vector vec) {
	return vec->elements;
}

size_t	__vec_cap(const vector vec) {
	return vec->capacity;
}

u8	__vec_rsz(vector vec, const size_t size) {
	size_t	i;

	if (size < vec->elements && vec->free) {
		for (i = vec->elements; i > size; i--)
			vec->free(*(void **)_get_element(vec, --vec->elements));
	}
	vec->capacity = size;
	vec->data = realloc(vec->data, vec->capacity * vec->element_size);
	return (vec->data) ? 1 : 0;
}

u8	__vec_stf(vector vec) {
	return __vec_rsz(vec, vec->elements);
}

void	__vec_clr(vector vec) {
	size_t	i;

	if (vec->free) {
		for (i = 0; i < vec->elements; i++)
			vec->free(*(void **)_get_element(vec, i));
	}
	vec->elements = 0;
}

u8	__vec_ins(vector vec, const size_t i, const u64 val) {
	size_t	_i;

	if (i >= vec->elements)
		return __vec_psh(vec, val);
	if (vec->elements == vec->capacity) {
		vec->capacity *= 2;
		vec->data = realloc(vec->data, vec->capacity * vec->element_size);
	}
	if (vec->data) {
		for (_i = vec->elements; _i > i; _i--)
			_set_element(vec, _i, __vec_get(vec, _i - 1));
		__vec_set(vec, i, val);
		vec->elements++;
	}
	return (vec->data) ? 1 : 0;
}

u8	__vec_ers(vector vec, const size_t i) {
	size_t	_i;

	if (i < vec->elements) {
		if (i == vec->elements - 1)
			__vec_pop(vec);
		else {
			if (vec->free)
				vec->free(*(void **)_get_element(vec, i));
			for (_i = i; _i < vec->elements - 1; _i++)
				_set_element(vec, _i, __vec_get(vec, _i + 1));
			vec->elements--;
		}
		return 1;
	}
	return 0;
}

static inline void	*_get_element(const vector vec, const size_t i) {
	switch (vec->element_size) {
		case sizeof(data_1B):
			return &((data_1B *)vec->data)[i];
		case sizeof(data_2B):
			return &((data_2B *)vec->data)[i];
		case sizeof(data_3B):
			return &((data_3B *)vec->data)[i];
		case sizeof(data_4B):
			return &((data_4B *)vec->data)[i];
		case sizeof(data_5B):
			return &((data_5B *)vec->data)[i];
		case sizeof(data_6B):
			return &((data_6B *)vec->data)[i];
		case sizeof(data_7B):
			return &((data_7B *)vec->data)[i];
		case sizeof(data_8B):
			return &((data_8B *)vec->data)[i];
	}
	return NULL;
}

static inline void	_set_element(vector vec, const size_t i, const u64 val) {
	switch (vec->element_size) {
		case sizeof(data_1B):
			*(data_1B *)_get_element(vec, i) = (data_1B)val;
			break ;
		case sizeof(data_2B):
			*(data_2B *)_get_element(vec, i) = (data_2B)val;
			break ;
		case sizeof(data_3B):
			*(data_3B *)_get_element(vec, i) = _u64_to_data_3B(val);
			break ;
		case sizeof(data_4B):
			*(data_4B *)_get_element(vec, i) = (data_4B)val;
			break ;
		case sizeof(data_5B):
			*(data_5B *)_get_element(vec, i) = _u64_to_data_5B(val);
			break ;
		case sizeof(data_6B):
			*(data_6B *)_get_element(vec, i) = _u64_to_data_6B(val);
			break ;
		case sizeof(data_7B):
			*(data_7B *)_get_element(vec, i) = _u64_to_data_7B(val);
			break ;
		case sizeof(data_8B):
			*(data_8B *)_get_element(vec, i) = (data_8B)val;
			break ;
	}
}

static inline u64	_get_value(const vector vec, const void *elem) {
	switch (vec->element_size) {
		case sizeof(data_1B):
			return *(data_1B *)elem;
		case sizeof(data_2B):
			return *(data_2B *)elem;
		case sizeof(data_3B):
			return _data_3B_to_u64(*(data_3B *)elem);
		case sizeof(data_4B):
			return *(data_4B *)elem;
		case sizeof(data_5B):
			return _data_5B_to_u64(*(data_5B *)elem);
		case sizeof(data_6B):
			return _data_6B_to_u64(*(data_6B *)elem);
		case sizeof(data_7B):
			return _data_7B_to_u64(*(data_7B *)elem);
		case sizeof(data_8B):
			return *(data_8B *)elem;
	}
	return 0;
}
