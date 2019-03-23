#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef _Bool bool;

struct match_results {
    const char* top;
    size_t len;
};
typedef struct match_results match_results_t;
match_results_t match(const char *regexp, const char *text);
void match_each_row(const char* pattern, FILE* file);

static const size_t text_max_n = 256;
static struct {
    const char* pattern;
    const char* filename;
    FILE* file;
} args;
static int validation_args(int argc, char *argv[]);

int 
main(int argc, char *argv[])
{
    /* 引数の正常性を検査 */
    if (validation_args(argc, argv) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
    
    /* 行毎に正規表現パターンを照合 */
    match_each_row(args.pattern, args.file);

    return EXIT_SUCCESS;
}

/* check_args: コマンドライン引数を検証 */
static int
validation_args(int argc, char *argv[])
{
    char *arg_cmd = argv[0];
    if (argc < 3) {
        fprintf(stderr, "Usage: %s PATTERN FILE\n", arg_cmd);
        return EXIT_FAILURE;
    }
    args.pattern  = argv[1];
    args.filename = argv[2];

    args.file = fopen(args.filename, "r");
    if (args.file == NULL) {
        fprintf(stderr, "Error: '%s' file not open\n", args.filename);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void nputs(const char* s, int n);

/* match_each_row: fileポインタから1行ずつパターン照合する */
void 
match_each_row(const char* pattern, FILE* file)
{
    char fetch_text[text_max_n];
    if (file == NULL) {
        exit(EXIT_FAILURE);
    }

    while(fgets(fetch_text, sizeof(fetch_text), file) != NULL) {
        match_results_t results;
        results = match(pattern, fetch_text);
        if (results.top != NULL) {
            nputs(fetch_text, results.top - fetch_text);
            printf("\e[31m");
            nputs(results.top, results.len);
            printf("\e[m");
            printf("%s", results.top + results.len);
        }
    }
}

/* nputs: sからnバイトまでの文字を表示する */
static void 
nputs(const char* s, int n)
{
    for (int i = 0; i < n; i++) {
        putchar(s[i]);
    }
}

static const char* match_here(const char *regexp, const char *text);
static const char* match_star(int c, const char *regexp, const char *text);
static const char* match_plus(int c, const char *regexp, const char *text);

/* match: textの任意の位置にある正規表現を探索 */
match_results_t
match(const char *regexp, const char *text)
{
    match_results_t results;
    const char* match_end = NULL;

    if(regexp[0] == '^'){
        /* 「^」パターンはtext先頭からの照合のみで良い */
        match_end = match_here(regexp+1, text);
    }
    else {
        /* 照合成功するまで、textの位置を順次送る */
        do {
            match_end = match_here(regexp, text);
            if(match_end != NULL) {
                break;
            }
        } while (*text++ != '\0');
    }

    /* 照合結果から、先頭位置と長さを求める */
    if (match_end == NULL) {
        results.top = NULL;
        results.len = 0u;
    }
    else {
        results.top = text;
        results.len = match_end - text;
    }

    return results;
}

/* match_here: textの先頭位置にある正規表現のマッチ */
static const char* 
match_here(const char *regexp, const char *text)
{
    if (regexp[0] == '\0') {
        /* 正規表現が終端ならば、照合成功 */
        return text;
    }
    else if (regexp[0] == '?') {
        /* 「?」パターンの直前は照合済み */
        return match_here(regexp+1, text);
    }
    else if (regexp[0] == '$' && regexp[1] == '\0') {
        /* 「$」で終わるパターンでtextが終端ならば、照合成功 */
        return (*text == '\0') ? text : NULL;
    }
    else if (regexp[1] == '*') {
        /* 「c*」パターンを照合 */
        return match_star(regexp[0], regexp+2, text);
    }
    else if (regexp[1] == '+') {
        /* 「c+」パターンを照合 */
        return match_plus(regexp[0], regexp+2, text);
    }

    if (*text != '\0') {
        /* 「.」「c」パターンを照合 */
        if ((regexp[0] == '.') || (regexp[0] == *text)) {
            return match_here(regexp+1, text+1);
        }
        /* 「?」パターンならば不一致でも次へ */
        else if (regexp[1] == '?') {
            return match_here(regexp+2, text+1);
        }
    }

    return NULL;
}

static const char*
matchstar_shortest(int c, const char *regexp, const char *text);
static const char*
matchstar_longest(int c, const char *regexp, const char *text);

/* match_star: 「c*」パターンの正規表現をテキストの先頭位置からマッチ */
static const char*
match_star(int c, const char *regexp, const char *text)
{
    //return matchstar_shortest(c, regexp, text);
    return matchstar_longest(c, regexp, text);
}

/* matchstar_shortest: 「c*」パターンの最短マッチ */
static const char*
matchstar_shortest(int c, const char *regexp, const char *text)
{
    do {
        /* 「*」は0回以上の繰り返しであるため、先に残りの正規表現を照合 */
        const char* match_end;
        match_end = match_here(regexp, text);
        if (match_end != NULL) {
            return match_end;
        }
    /* 「c*」「.*」パターンを照合 */
    } while (*text != '\0' && (*text++ == c || c == '.'));

    return NULL;
}

/* matchstar_shortest: 「c*」パターンの最長マッチ */
static const char*
matchstar_longest(int c, const char *regexp, const char *text)
{
    /* 「*」の最長マッチを照合 */
    const char *t = text;
    for (; *t != '\0' && (*t == c || c == '.'); t++)
        ;
    
    do {
        /* 「*」は0回以上の繰り返しであるため、先に残りの正規表現を照合 */
        const char* match_end;
        match_end = match_here(regexp, t);
        if(match_end != NULL) {
            return match_end;
        }
    /* textの先頭までバックトラックさせる */
    } while (t-- > text);

    return NULL;
}

/* match_plus: 「c+」パターンの照合 */
static const char*
match_plus(int c, const char *regexp, const char *text)
{
    /* 「c」パターンが最低1個照合できたら、残りは「*」パターンで照合 */
    if (*text != '\0' && (*text == c || c == '.')) {
        return match_star(c, regexp, text++);
    }

    return NULL;
}

