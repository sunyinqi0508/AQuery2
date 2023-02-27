// Non-standard Extensions for MonetDBe, may break concurrency control!

#include "monetdbe.h"
#include <stdint.h>
#include "mal_client.h"
#include "sql_mvc.h"
#include "sql_semantic.h"
#include "mal_exception.h"

typedef struct column_storage {
	int refcnt;
	int bid;
	int ebid;		/* extra bid */
	int uibid;		/* bat with positions of updates */
	int uvbid;		/* bat with values of updates */
	storage_type st; /* ST_DEFAULT, ST_DICT, ST_FOR */
	bool cleared;
	bool merged;	/* only merge changes once */
	size_t ucnt;	/* number of updates */
	ulng ts;		/* version timestamp */
} column_storage;

typedef struct segment {
	BUN start;
	BUN end;
	bool deleted;	/* we need to keep a dense segment set, 0 - end of last segemnt,
					   some segments maybe deleted */
	ulng ts;		/* timestamp on this segment, ie tid of some active transaction or commit time of append/delete or
					   rollback time, ie ready for reuse */
	ulng oldts;		/* keep previous ts, for rollbacks */
	struct segment *next;	/* usualy one should be enough */
	struct segment *prev;	/* used in destruction list */
} segment;

/* container structure to allow sharing this structure */
typedef struct segments {
	sql_ref r;
	struct segment *h;
	struct segment *t;
} segments;

typedef struct storage {
	column_storage cs;	/* storage on disk */
	segments *segs;	/* local used segements */
	struct storage *next;
} storage;

typedef struct {
	char 	language;		/* 'S' or 's' or 'X' */
	char	depth;			/* depth >= 1 means no output for trans/schema statements */
	int 	remote;			/* counter to make remote function names unique */
	mvc 	*mvc;
    char others[];
} backend;

typedef struct {
	Client c;
	char *msg;
	monetdbe_data_blob blob_null;
	monetdbe_data_date date_null;
	monetdbe_data_time time_null;
	monetdbe_data_timestamp timestamp_null;
	str mid;
} monetdbe_database_internal;

size_t
monetdbe_get_size(monetdbe_database dbhdl, const char *table_name)
{
	monetdbe_database_internal* hdl = (monetdbe_database_internal*)dbhdl;
	backend* be = ((backend *)(((monetdbe_database_internal*)dbhdl)->c->sqlcontext));
	mvc *m = be->mvc;
    //mvc_trans(m);
	sql_table *t = find_table_or_view_on_scope(m, NULL, "sys", table_name, "CATALOG", false);
	if (!t) return 0;
	sql_column *col = ol_first_node(t->columns)->data;
	sqlstore* store = m->store;
	size_t sz = store->storage_api.count_col(m->session->tr, col, QUICK);
	//mvc_cancel_session(m); 
	return sz; 
}

void* 
monetdbe_get_col(monetdbe_database dbhdl, const char *table_name, uint32_t col_id) {
    monetdbe_database_internal* hdl = (monetdbe_database_internal*)dbhdl;
	backend* be = ((backend *)(((monetdbe_database_internal*)dbhdl)->c->sqlcontext));
	mvc *m = be->mvc;
    //mvc_trans(m);
	sql_table *t = find_table_or_view_on_scope(m, NULL, "sys", table_name, "CATALOG", false);
	if (!t) return NULL;
	sql_column *col = ol_fetch(t->columns, col_id);
	sqlstore* store = m->store;
	BAT *b = store->storage_api.bind_col(m->session->tr, col, QUICK);
	BATiter iter = bat_iterator(b);
	//mvc_cancel_session(m); 
    return iter.base;
}
