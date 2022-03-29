#ifndef PGMS_H_
#define PGMS_H_

#include <postgres.h>

#define NEGATIVE_SIGN           '-'
#define POSITIVE_SIGN           '+'

extern Oid spectrumOid;

char* reverse_postfix_sign(char*);

#endif /* PGMS_H_ */
