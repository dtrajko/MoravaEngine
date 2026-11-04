BC7 and BC5 encoders and decoders from https://github.com/richgel999/bc7enc_rdo (Richard Geldreich, Jr.; MIT or public domain, see LICENSE),
commit b9438627eef73a1157e84201b6fa6eb2ffd6d9f0: bc7enc, bc7decomp and rgbcx only, unchanged. The RDO tools, the ISPC encoder and rgbcx_table4.h
(the larger of two BC1 tables: RGBCX_USE_SMALLER_TABLES is on) are left out. CMakeLists.txt is the engine's own.
