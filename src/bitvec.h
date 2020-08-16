#ifndef BITVEC_H
#define BITVEC_H

typedef struct bitvec bitvec;

struct bitvec {
	size_t *array;
	size_t len;
	_Bool rev;
};

#define BITVEC_INIT ((bitvec) { NULL, 0, 0 })

static
_Bool
bitvec_isset(bitvec *self, size_t bit)
{
	size_t oarr, obit;
	_Bool res;

	oarr = bit / sizeof(size_t) * 8;
	obit = bit % sizeof(size_t) * 8;

	if (oarr >= self->len)
		return 0;

	res = self->array[oarr] & (1 << obit);
	return res != self->rev;
}


static
void
bitvec_clear(bitvec *self, size_t bit);

static
void
bitvec_set(bitvec *self, size_t bit);

void
bitvec_clear(bitvec *self, size_t bit)
{
	size_t oarr, obit;

	if (self->rev) {
		bitvec_set(self, bit);
		return;
	}

	oarr = bit / sizeof(size_t) * 8;
	obit = bit % sizeof(size_t) * 8;

	if (oarr < self->len)
		self->array[oarr] &= ~(1 << obit);
}

void
bitvec_set(bitvec *self, size_t bit)
{
	size_t oarr, obit;

	if (self->rev) {
		bitvec_clear(self, bit);
		return;
	}

	oarr = bit / sizeof(size_t) * 8;
	obit = bit % sizeof(size_t) * 8;

	if (oarr >= self->len) {
		self->array = realloc(self->array, (oarr + 1) * sizeof(size_t));
		for (; self->len < oarr + 1; ++self->len)
			self->array[self->len] = 0;
	}

	self->array[oarr] |= (1 << obit);
}

#endif
