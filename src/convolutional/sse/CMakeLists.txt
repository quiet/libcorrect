set(SRCFILES lookup_sse.c convolutional_sse.c encode_sse.c decode_sse.c)
add_library(correct-convolutional-sse OBJECT ${SRCFILES})
