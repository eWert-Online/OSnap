#define CAML_NAME_SPACE

#include <spng.h>

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/fail.h>
#include <caml/bigarray.h>

CAMLprim value
read_png_buffer(value buffer, value length)
{
  CAMLparam2(buffer, length);
  CAMLlocal2(res, ba);

  int result = 0;
  spng_ctx *ctx = NULL;
  unsigned char *out = NULL;
  uint8_t *buf = (uint8_t *)String_val(buffer);
  size_t buf_len = (size_t)Int_val(length);

  ctx = spng_ctx_new(0);

  if (ctx == NULL)
  {
    caml_failwith("spng_ctx_new() failed");
    spng_ctx_free(ctx);
  }

  /* Ignore and don't calculate chunk CRC's */
  spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

  /* Set memory usage limits for storing standard and unknown chunks,
      this is important when reading untrusted files! */
  size_t limit = 1024 * 1024 * 64;
  spng_set_chunk_limits(ctx, limit, limit);

  /* Set source PNG Buffer */
  spng_set_png_buffer(ctx, buf, buf_len);

  struct spng_ihdr ihdr;
  result = spng_get_ihdr(ctx, &ihdr);

  if (result)
  {
    caml_failwith("spng_get_ihdr() error!");
    spng_ctx_free(ctx);
  }

  size_t out_size;
  result = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &out_size);
  if (result)
  {
    spng_ctx_free(ctx);
  };

  out = malloc(out_size);
  if (out == NULL)
  {
    spng_ctx_free(ctx);
    free(out);
  };

  result = spng_decode_image(ctx, out, out_size, SPNG_FMT_RGBA8, 0);
  if (result)
  {
    spng_ctx_free(ctx);
    free(out);
    caml_failwith(spng_strerror(result));
  }

  res = caml_alloc(3, 0);
  ba = caml_ba_alloc_dims(CAML_BA_INT32 | CAML_BA_C_LAYOUT, 1, out, out_size);

  Store_field(res, 0, Val_int(ihdr.width));
  Store_field(res, 1, Val_int(ihdr.height));
  Store_field(res, 2, ba);

  spng_ctx_free(ctx);

  CAMLreturn(res);
}

void free_png_buffer(value buffer)
{
  struct caml_ba_array *b = Caml_ba_array_val(buffer);
  free(b->data);
}