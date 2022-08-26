#ifndef MGF_H_
#define MGF_H_

#include <postgres.h>

struct Parser;
struct LargeObjectDesc;
struct AttInMetadata;

struct Parser* parser_init_lob(struct LargeObjectDesc*, struct AttInMetadata*);
struct Parser* parser_init_varchar(VarChar*, struct AttInMetadata*);
void parser_close(struct Parser*);
bool parser_next(struct Parser*);
void parser_globals(struct Parser*);
Datum parser_get_tuple(struct Parser*);

#endif /* MGF_H_ */
