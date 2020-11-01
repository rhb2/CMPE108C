#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <curl/curl.h>

#include <sched.h>
#include <jsmn.h>


#define	MAXNAME	24
#define	NUM_WORKERS	10
#define	QUEUE_DEPTH	10

static char *base_url = "https://query1.finance.yahoo.com:443/v7/finance/quote?"
    "laS&corsDomain=finance.yahoo.com&symbols=";

typedef struct stock_stat {
	char company[64];
	char symbol[16];
	char mktcap[16];
	char price[16];
	char chg[16];
} stock_stat_t;

typedef struct buf {
	char *memory;
	size_t size;
} buf_t;

void stock_func(void *, int);
void print_stat(stock_stat_t *);

void
usage(void)
{
	printf("Usage: ./stockwatch <config file>\n");
}

static stock_stat_t *portfolio = NULL;

void main(int argc, char **argv)
{
	int i;
	int total;
	FILE *fp;
	char *fname;
	char symbol[20];
	sched_t scheduler;
	stock_stat_t header;
	task_t *tasks;

	if (argc == 1) {
		usage();
		exit(-1);
	}

	fname = argv[1];
	if ((fp = fopen(fname, "r")) == NULL) {
		printf("Failed to open file: %s\n", fname);
		exit(1);
	}

	/* Figure out how many entries there are in the portfolio. */
	bzero(symbol, sizeof (symbol));
	while (fgets(symbol, MAXNAME, fp) != NULL)
		total++;

	/* Allocate the storage for our portfolio. */
	if ((portfolio = malloc(sizeof (stock_stat_t) * total)) == NULL) {
		printf("Failed to allocate stock portfolio.\n");
		exit(-1);
	}
	bzero(portfolio, sizeof (stock_stat_t) * total);

	/* Allocate storage for tasks. */
	if ((tasks = malloc(sizeof (task_t) * total)) == NULL) {
		printf("Failed to allocate storage for tasks.\n");
		exit (-1);
	}
	bzero(tasks, sizeof (task_t) * total);

	/* Start the scheduler. */
	(void) sched_init(&scheduler, total, total);

	fseek(fp, 0L, SEEK_SET);

	for (i = 0; fgets(portfolio[i].symbol, MAXNAME, fp) != NULL; i++) {
		portfolio[i].symbol[strlen(portfolio[i].symbol) - 1] = 0;
		task_init(&tasks[i], 1, stock_func, (void *)&portfolio[i]);

		(void) sched_post(&scheduler, &tasks[i], false);
	}

	/* Block until the scheduler has finished processing all tasks. */
	sched_execute(&scheduler);

	bzero(&header, sizeof (stock_stat_t));
	(void) strcat(header.symbol, "SYMBOL");
	(void) strcat(header.company, "COMPANY");
	(void) strcat(header.price, "PRICE");
	(void) strcat(header.chg, "CHANGE");
	(void) strcat(header.mktcap, "MARKET CAP");
	print_stat(&header);

	for (i = 0; i < total; i++)
		print_stat(&portfolio[i]);

	sched_fini(&scheduler);
}

static int
get_token_with_key(jsmntok_t *objs, int len, int start, char *key, char *buf)
{
	int i;
	jsmntok_t *tp;

	assert(objs != NULL && key != NULL && buf != NULL);

	for (i = start; i < len; i++) {
		if (strncmp(key, &buf[objs[i].start], strlen(key)) == 0)
			return (i);
	}
	return (-1);
}

void
copy_value_to_buf(jsmntok_t *tokens, int num_tokens, char *key, char *src,
    char *dst)
{
	int index;
	jsmntok_t *tok;

	assert(tokens != NULL && key != NULL && src != NULL && dst != NULL);

	if ((index = get_token_with_key(tokens, num_tokens, 0, key,
	    src)) == -1) {
		(void) strcpy(dst, "unknown");
		return;
	}

	tok = &tokens[index + 1];
	(void) strncpy(dst, src + tok->start, tok->end - tok->start);
}

void
extract_stock_stat(jsmntok_t *tokens, int num_tokens, char *buf,
    stock_stat_t *sp)
{
	assert(tokens != NULL && sp != NULL);

	copy_value_to_buf(tokens, num_tokens, "shortName", buf, sp->company);
	copy_value_to_buf(tokens, num_tokens, "symbol", buf, sp->symbol);
	copy_value_to_buf(tokens, num_tokens, "marketCap", buf, sp->mktcap);
	copy_value_to_buf(tokens, num_tokens, "regularMarketPrice", buf,
	    sp->price);
	copy_value_to_buf(tokens, num_tokens, "regularMarketChange", buf,
	    sp->chg);
}

void
print_stat(stock_stat_t *stat)
{
	if (stat == NULL)
		return;

	printf("%-25s%-15s%-15s%-10s%-10s\n", stat->company, stat->symbol,
	    stat->mktcap, stat->price, stat->chg);
}

static size_t
callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	buf_t *mem = userp;
	char *ptr;
	
	if ((ptr = realloc(mem->memory, mem->size + realsize + 1)) == NULL) {
		printf("Not enough memory (realloc returned NULL).\n");
		return (0);
	}

	mem->memory = ptr;
	(void) memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return (realsize);
}

void
stock_func(void *arg, int thread_num)
{
	CURL *ctx;
	CURLcode res;
	int num_tokens;
	jsmn_parser p;
	jsmntok_t tokens[256];
	char url[256];
	stock_stat_t header;
	stock_stat_t *stock = arg;
	buf_t chunk;

	assert(stock != NULL);

	jsmn_init(&p);

	bzero(url, sizeof (url));
	bzero(&header, sizeof (stock_stat_t));
	(void) strcat(url, base_url);
	(void) strcat(url, stock->symbol);

	/* Will be grown as needed by the realloc above */
	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);

	/* Init the curl session */
	ctx = curl_easy_init();

	/* Specify URL to get */
	curl_easy_setopt(ctx, CURLOPT_URL, url);

	/* Configure our callback to process response. */
	curl_easy_setopt(ctx, CURLOPT_WRITEFUNCTION, callback);

	/* Specify that our callback takes a `chunk' as its argument */
	curl_easy_setopt(ctx, CURLOPT_WRITEDATA, (void *)&chunk);

	/*
	 * Some servers don't like requests that are made without a user-agent
	 * field, so we provide one
	 */
	curl_easy_setopt(ctx, CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* Send the request */
	if ((res = curl_easy_perform(ctx)) != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
		    curl_easy_strerror(res));
	}

	/* Cleanup curl stuff. */
	curl_easy_cleanup(ctx);

	num_tokens = jsmn_parse(&p, chunk.memory, chunk.size, tokens,
	    sizeof (tokens) / sizeof (tokens[0]));

	extract_stock_stat(tokens, num_tokens, chunk.memory, stock);
	free(chunk.memory);
	curl_global_cleanup();
}
