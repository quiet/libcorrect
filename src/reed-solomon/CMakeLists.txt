set(SRCFILES polynomial.c reed-solomon.c rsencode.c rsdecode.c)
add_library(correct-reed-solomon OBJECT ${SRCFILES})
