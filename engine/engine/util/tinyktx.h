#ifndef TINYKTX_H
#define TINYKTX_H

#include <stddef.h>
#include <stdint.h>

typedef enum ktx_result_t {
  KTX_SUCCESS,
  KTX_FAILED_TO_OPEN_FILE,
  KTX_WRONG_IDENTIFIER,
} ktx_result_t;

#define KTX_ZERO 0

// Type
// Table 8.2
#define KTX_UNSIGNED_BYTE 0x1401
#define KTX_BYTE 0x1400
#define KTX_UNSIGNED_SHORT 0x1403
#define KTX_SHORT 0x1402
#define KTX_UNSIGNED_INT 0x1405
#define KTX_INT 0x1404
#define KTX_HALF_FLOAT 0x140B
#define KTX_FLOAT 0x1406
#define KTX_UNSIGNED_BYTE_3_3_2 0x8032
#define KTX_UNSIGNED_BYTE_2_3_3_REV 0x8362
#define KTX_UNSIGNED_SHORT_5_6_5 0x8363
#define KTX_UNSIGNED_SHORT_5_6_5_REV 0x8364
#define KTX_UNSIGNED_SHORT_4_4_4_4 0x8033
#define KTX_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define KTX_UNSIGNED_SHORT_5_5_5_1 0x8034
#define KTX_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define KTX_UNSIGNED_INT_8_8_8_8 0x8035
#define KTX_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define KTX_UNSIGNED_INT_10_10_10_2 0x8036
#define KTX_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define KTX_UNSIGNED_INT_24_8 0x84FA
#define KTX_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define KTX_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define KTX_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD

// Format (done)
// Table 8.3
#define KTX_STENCIL_INDEX 0x1901
#define KTX_DEPTH_COMPONENT 0x1902
#define KTX_DEPTH_STENCIL 0x84F9
#define KTX_RED 0x1903
#define KTX_GREEN 0x1904
#define KTX_BLUE 0x1905
#define KTX_RG 0x8227
#define KTX_RGB 0x1907
#define KTX_RGBA 0x1908
#define KTX_BGR 0x80E0
#define KTX_BGRA 0x80E1
#define KTX_RED_INTEGER 0x8D94
#define KTX_GREEN_INTEGER 0x8D95
#define KTX_BLUE_INTEGER 0x8D96
#define KTX_RG_INTEGER 0x8228
#define KTX_RGB_INTEGER 0x8D98
#define KTX_RGBA_INTEGER 0x8D99
#define KTX_BGR_INTEGER 0x8D9A
#define KTX_BGRA_INTEGER 0x8D9B

// Internal format
// Table 8.14
#define KTX_COMPRESSED_RED 0x8225
#define KTX_COMPRESSED_RG 0x8226
#define KTX_COMPRESSED_RGB 0x84ED
#define KTX_COMPRESSED_RGBA 0x84EE
#define KTX_COMPRESSED_SRGB 0x8C48
#define KTX_COMPRESSED_SRGB_ALPHA 0x8C49
#define KTX_COMPRESSED_RED_RGTC1 0x8DBB
#define KTX_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#define KTX_COMPRESSED_RG_RGTC2 0x8DBD
#define KTX_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#define KTX_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#define KTX_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#define KTX_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#define KTX_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#define KTX_COMPRESSED_RGB8_ETC2 0x9274
#define KTX_COMPRESSED_SRGB8_ETC2 0x9275
#define KTX_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define KTX_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define KTX_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#define KTX_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#define KTX_COMPRESSED_R11_EAC 0x9270
#define KTX_COMPRESSED_SIGNED_R11_EAC 0x9271
#define KTX_COMPRESSED_RG11_EAC 0x9272
#define KTX_COMPRESSED_SIGNED_RG11_EAC 0x9273

// Table 8.12
#define KTX_R8 0x8229
#define KTX_R8_SNORM 0x8F94
#define KTX_R16 0x822A
#define KTX_R16_SNORM 0x8F98
#define KTX_RG8 0x822B
#define KTX_RG8_SNORM 0x8F95
#define KTX_RG16 0x822C
#define KTX_RG16_SNORM 0x8F99
#define KTX_R3_G3_B2 0x2A10
#define KTX_RGB4 0x804F
#define KTX_RGB5 0x8050
#define KTX_RGB565 0x8D62
#define KTX_RGB8 0x8051
#define KTX_RGB8_SNORM 0x8F96
#define KTX_RGB10 0x8052
#define KTX_RGB12 0x8053
#define KTX_RGB16 0x8054
#define KTX_RGB16_SNORM 0x8F9A
#define KTX_RGBA2 0x8055
#define KTX_RGBA4 0x8056
#define KTX_RGB5_A1 0x8057
#define KTX_RGBA8 0x8058
#define KTX_RGBA8_SNORM 0x8F97
#define KTX_RGB10_A2 0x8059
#define KTX_RGB10_A2UI 0x906F
#define KTX_RGBA12 0x805A
#define KTX_RGBA16 0x805B
#define KTX_RGBA16_SNORM 0x8F9B
#define KTX_SRGB8 0x8C41
#define KTX_SRGB8_ALPHA8 0x8C43
#define KTX_R16F 0x822D
#define KTX_RG16F 0x822F
#define KTX_RGB16F 0x881B
#define KTX_RGBA16F 0x881A
#define KTX_R32F 0x822E
#define KTX_RG32F 0x8230
#define KTX_RGB32F 0x8815
#define KTX_RGBA32F 0x8814
#define KTX_R11F_G11F_B10F 0x8C3A
#define KTX_RGB9_E5 0x8C3D
#define KTX_R8I 0x8231
#define KTX_R8UI 0x8232
#define KTX_R16I 0x8233
#define KTX_R16UI 0x8234
#define KTX_R32I 0x8235
#define KTX_R32UI 0x8236
#define KTX_RG8I 0x8237
#define KTX_RG8UI 0x8238
#define KTX_RG16I 0x8239
#define KTX_RG16UI 0x823A
#define KTX_RG32I 0x823B
#define KTX_RG32UI 0x823C
#define KTX_RGB8I 0x8D8F
#define KTX_RGB8UI 0x8D7D
#define KTX_RGB16I 0x8D89
#define KTX_RGB16UI 0x8D77
#define KTX_RGB32I 0x8D83
#define KTX_RGB32UI 0x8D71
#define KTX_RGBA8I 0x8D8E
#define KTX_RGBA8UI 0x8D7C
#define KTX_RGBA16I 0x8D88
#define KTX_RGBA16UI 0x8D76
#define KTX_RGBA32I 0x8D82

// Table 8.13
#define KTX_DEPTH_COMPONENT16 0x81A5
#define KTX_DEPTH_COMPONENT24 0x81A6
#define KTX_DEPTH_COMPONENT32 0x81A7
#define KTX_DEPTH_COMPONENT32F 0x8CAC
#define KTX_DEPTH24_STENCIL8 0x88F0
#define KTX_DEPTH32F_STENCIL8 0x8CAD
#define KTX_STENCIL_INDEX1 0x8D46
#define KTX_STENCIL_INDEX4 0x8D47
#define KTX_STENCIL_INDEX8 0x8D48
#define KTX_STENCIL_INDEX16 0x8D49

// Base internal format
// Table 8.11
#define KTX_DEPTH_COMPONENT 0x1902
#define KTX_DEPTH_STENCIL 0x84F9
#define KTX_RED 0x1903
#define KTX_RG 0x8227
#define KTX_RGB 0x1907
#define KTX_RGBA 0x1908
#define KTX_STENCIL_INDEX 0x1901

typedef struct ktx_slice_t {
  uint8_t *data;
} ktx_slice_t;

typedef struct ktx_face_t {
  ktx_slice_t *slices;
} ktx_face_t;

typedef struct ktx_array_element_t {
  ktx_face_t faces[6];
} ktx_array_element_t;

typedef struct ktx_mip_level_t {
  ktx_array_element_t *array_elements;
} ktx_mip_level_t;

typedef struct ktx_data_t {
  uint8_t *raw_data;

  uint32_t pixel_width;
  uint32_t pixel_height;
  uint32_t pixel_depth;

  uint32_t array_element_count;
  uint32_t face_count;
  uint32_t mipmap_level_count;

  uint32_t type;
  uint32_t type_size;

  uint32_t format;
  uint32_t internal_format;
  uint32_t base_internal_format;

  ktx_mip_level_t *mip_levels;
} ktx_data_t;

ktx_result_t ktx_read_from_file(const char *filename, ktx_data_t *data);

ktx_result_t
ktx_read(uint8_t *raw_data, size_t raw_data_size, ktx_data_t *data);

void ktx_data_destroy(ktx_data_t *data);

#endif
