// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<vector.c>>

#include <string.h>

#include "utils/vector.h"

struct __vector {
	void	(*free)(void *);
	size_t	element_size;
	size_t	capacity;
	size_t	elements;
	u8		*data;
};

#define index(vec, i)	((void *)((uintptr_t)vec->data + (vec->element_size * i)))

static inline void	_set_element(vector vec, const size_t i, const void *val);

vector	__vec_new(const size_t size, const size_t count, void (*_free)(void *)) {
	vector	out;

	out = (size) ? malloc(sizeof(*out)) : NULL;
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

u8	__vec_psh(vector vec, const void *val) {
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
			vec->free(*(void **)index(vec, vec->elements));
	}
}

void	*__vec_get(const vector vec, const size_t i) {
	return (i < vec->elements) ? index(vec, i) : VECTOR_INDEX_OUT_OF_BOUNDS;
}

u8	__vec_set(vector vec, const size_t i, const void *val) {
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
			vec->free(*(void **)index(vec, --vec->elements));
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
			vec->free(*(void **)index(vec, i));
	}
	vec->elements = 0;
}

u8	__vec_ins(vector vec, const size_t i, const void *val) {
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
		_set_element(vec, i, val);
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
				vec->free(*(void **)index(vec, i));
			for (_i = i; _i < vec->elements - 1; _i++)
				_set_element(vec, _i, __vec_get(vec, _i + 1));
			vec->elements--;
		}
		return 1;
	}
	return 0;
}

static inline void	_set_element(vector vec, const size_t i, const void *val) {
	memmove(index(vec, i), val, vec->element_size);
}
