#include "tinyktx.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ALIGN_MASK(_value, _mask) (((_value) + (_mask)) & ((~0) & (~(_mask))))

const static uint8_t ktx_identifier[12] = {
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A,
};

typedef struct ktx_header_t {
  uint8_t identifier[12];
  uint32_t endianness;
  uint32_t gl_type;
  uint32_t gl_type_size;
  uint32_t gl_format;
  uint32_t gl_internal_format;
  uint32_t gl_base_internal_format;
  uint32_t pixel_width;
  uint32_t pixel_height;
  uint32_t pixel_depth;
  uint32_t number_of_array_elements;
  uint32_t number_of_faces;
  uint32_t number_of_mipmap_levels;
  uint32_t bytes_of_key_value_data;
} ktx_header_t;

static inline uint32_t get_block_size(uint32_t internal_format) {
  switch (internal_format) {
  case KTX_R8: return 1;
  case KTX_R8_SNORM: return 1;
  case KTX_R16: return 2;
  case KTX_R16_SNORM: return 2;
  case KTX_RG8: return 2;
  case KTX_RG8_SNORM: return 2;
  case KTX_RG16: return 4;
  case KTX_RG16_SNORM: return 4;
  case KTX_R3_G3_B2: return 1;
  case KTX_RGB8: return 3;
  case KTX_RGB8_SNORM: return 3;
  case KTX_RGB16: return 6;
  case KTX_RGB16_SNORM: return 6;
  case KTX_RGBA8: return 4;
  case KTX_RGBA8_SNORM: return 4;
  case KTX_RGB10_A2: return 4;
  case KTX_RGB10_A2UI: return 4;
  case KTX_RGBA16: return 8;
  case KTX_RGBA16_SNORM: return 8;
  case KTX_SRGB8: return 3;
  case KTX_R16F: return 2;
  case KTX_RG16F: return 4;
  case KTX_RGB16F: return 6;
  case KTX_RGBA16F: return 8;
  case KTX_R32F: return 4;
  case KTX_RG32F: return 8;
  case KTX_RGB32F: return 12;
  case KTX_RGBA32F: return 16;
  case KTX_R8I: return 1;
  case KTX_R8UI: return 1;
  case KTX_R16I: return 2;
  case KTX_R16UI: return 2;
  case KTX_R32I: return 4;
  case KTX_R32UI: return 4;
  case KTX_RG8I: return 2;
  case KTX_RG8UI: return 2;
  case KTX_RG16I: return 4;
  case KTX_RG16UI: return 4;
  case KTX_RG32I: return 8;
  case KTX_RG32UI: return 8;
  case KTX_RGB8I: return 3;
  case KTX_RGB8UI: return 3;
  case KTX_RGB16I: return 6;
  case KTX_RGB16UI: return 6;
  case KTX_RGB32I: return 12;
  case KTX_RGB32UI: return 12;
  case KTX_RGBA8I: return 4;
  case KTX_RGBA8UI: return 4;
  case KTX_RGBA16I: return 8;
  case KTX_RGBA16UI: return 8;
  case KTX_RGBA32I: return 16;

  case KTX_DEPTH_COMPONENT16: return 2;
  case KTX_DEPTH_COMPONENT24: return 3;
  case KTX_DEPTH_COMPONENT32: return 4;
  case KTX_DEPTH_COMPONENT32F: return 4;
  case KTX_DEPTH24_STENCIL8: return 4;
  case KTX_DEPTH32F_STENCIL8: return 5;
  default: return 0;
  }
}

ktx_result_t ktx_read_from_file(const char *filename, ktx_data_t *data) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    return KTX_FAILED_TO_OPEN_FILE;
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  uint8_t *ktx_data = malloc(size);
  fread(ktx_data, size, 1, file);

  ktx_result_t result = ktx_read(ktx_data, size, data);

  fclose(file);

  return result;
}

ktx_result_t
ktx_read(uint8_t *raw_data, size_t raw_data_size, ktx_data_t *data) {
  size_t pos = 0;

  ktx_header_t header;
  memcpy(&header, raw_data, sizeof(ktx_header_t));

  pos += sizeof(ktx_header_t);

  if (memcmp(header.identifier, ktx_identifier, sizeof(ktx_identifier)) != 0) {
    return KTX_WRONG_IDENTIFIER;
  }

  // TODO: handle endianness conversion
  /* bool convert_endianness = (header.endianness != 0x04030201); */
  assert(header.endianness == 0x04030201);

  *data = (ktx_data_t){
      .raw_data = raw_data,

      .pixel_width = header.pixel_width,
      .pixel_height = header.pixel_height,
      .pixel_depth = MAX(header.pixel_depth, 1),

      .array_element_count = MAX(header.number_of_array_elements, 1),
      .face_count = MAX(header.number_of_faces, 1),
      .mipmap_level_count = MAX(header.number_of_mipmap_levels, 1),

      .type = header.gl_type,
      .type_size = header.gl_type_size,

      .format = header.gl_format,
      .internal_format = header.gl_internal_format,
      .base_internal_format = header.gl_base_internal_format,
  };

  uint32_t kv_bytes_left = header.bytes_of_key_value_data;

  while (kv_bytes_left > 0) {
    uint32_t key_and_value_byte_size = *((uint32_t *)(raw_data + pos));
    pos += sizeof(key_and_value_byte_size);
    kv_bytes_left -= sizeof(key_and_value_byte_size);

    pos += key_and_value_byte_size;
    kv_bytes_left -= key_and_value_byte_size;

    uint32_t padding = 3 - ((key_and_value_byte_size + 3) % 4);
    pos += padding;
    kv_bytes_left -= padding;
  }

  uint32_t block_size = get_block_size(header.gl_internal_format);
  assert(block_size > 0);

  data->mip_levels = malloc(sizeof(ktx_mip_level_t) * data->mipmap_level_count);

  for (uint32_t mip_level = 0; mip_level < data->mipmap_level_count;
       mip_level++) {
    data->mip_levels[mip_level].array_elements =
        malloc(sizeof(ktx_array_element_t) * data->array_element_count);

    uint32_t image_size = *((uint32_t *)(raw_data + pos));
    pos += sizeof(image_size);

    uint32_t mip_width = header.pixel_width / (1 << mip_level);
    uint32_t mip_height = header.pixel_height / (1 << mip_level);

    for (uint32_t layer = 0; layer < data->array_element_count; layer++) {
      for (uint32_t face = 0; face < data->face_count; face++) {
        data->mip_levels[mip_level].array_elements[layer].faces[face].slices =
            malloc(sizeof(ktx_slice_t) * data->pixel_depth);
        for (uint32_t z_slice = 0; z_slice < data->pixel_depth; z_slice++) {
          data->mip_levels[mip_level]
              .array_elements[layer]
              .faces[face]
              .slices[z_slice]
              .data = &data->raw_data[pos];

          uint32_t slice_size = mip_width * mip_height * block_size;
          pos += slice_size;
        }

        // cube padding
        pos = ALIGN_MASK(pos, 3);
      }
    }

    // mip padding
    pos = ALIGN_MASK(pos, 3);
  }

  return KTX_SUCCESS;
}

void ktx_data_destroy(ktx_data_t *data) {
  for (uint32_t mip_level = 0; mip_level < data->mipmap_level_count;
       mip_level++) {
    for (uint32_t layer = 0; layer < data->array_element_count; layer++) {
      for (uint32_t face = 0; face < data->face_count; face++) {
        free(data->mip_levels[mip_level]
                 .array_elements[layer]
                 .faces[face]
                 .slices);
      }
    }

    free(data->mip_levels[mip_level].array_elements);
  }

  free(data->mip_levels);

  free(data->raw_data);
}
