#include "mpc.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/*
** State Type
*/

typedef struct {
  char last;
  char next;
  int pos;
  int row;
  int col;
} mpc_state_t;

static mpc_state_t mpc_state_null(void) {
  mpc_state_t s;
  s.last = '\0';
  s.next = '\0';
  s.pos = 0;
  s.row = 0;
  s.col = 0;
  return s;
}

/*
** Error Type
*/

struct mpc_err_t {
  char* filename;
  mpc_state_t state;
  int expected_num;
  char** expected;
  char* failure;
};

static mpc_err_t* mpc_err_new(const char* filename, mpc_state_t s, const char* expected) {
  mpc_err_t* x = malloc(sizeof(mpc_err_t));
  x->filename = malloc(strlen(filename) + 1);
  strcpy(x->filename, filename);
  x->state = s;
  x->expected_num = 1;
  x->expected = malloc(sizeof(char*));
  x->expected[0] = malloc(strlen(expected) + 1);
  strcpy(x->expected[0], expected);
  x->failure = NULL;
  return x;
}

static mpc_err_t* mpc_err_new_fail(const char* filename, mpc_state_t s, const char* failure) {
  mpc_err_t* x = malloc(sizeof(mpc_err_t));
  x->filename = malloc(strlen(filename) + 1);
  strcpy(x->filename, filename);
  x->state = s;
  x->expected_num = 0;
  x->expected = NULL;
  x->failure = malloc(strlen(failure) + 1);
  strcpy(x->failure, failure);
  return x;
}

void mpc_err_delete(mpc_err_t* x) {

  int i;
  for (i = 0; i < x->expected_num; i++) {
    free(x->expected[i]);
  }
  
  free(x->expected);
  free(x->filename);
  free(x->failure);
  free(x);
}

static bool mpc_err_contains_expected(mpc_err_t* x, char* expected) {
  
  int i;
  for (i = 0; i < x->expected_num; i++) {
    if (strcmp(x->expected[i], expected) == 0) { return true; }
  }
  
  return false;
  
}

static void mpc_err_add_expected(mpc_err_t* x, char* expected) {
  
  x->expected_num++;
  x->expected = realloc(x->expected, sizeof(char*) * x->expected_num);
  x->expected[x->expected_num-1] = malloc(strlen(expected) + 1);
  strcpy(x->expected[x->expected_num-1], expected);
  
}

static void mpc_err_clear_expected(mpc_err_t* x, char* expected) {
  
  int i;
  for (i = 0; i < x->expected_num; i++) {
    free(x->expected[i]);
  }
  x->expected_num = 1;
  x->expected = realloc(x->expected, sizeof(char*) * x->expected_num);
  x->expected[0] = malloc(strlen(expected) + 1);
  strcpy(x->expected[0], expected);
  
}

void mpc_err_print(mpc_err_t* x) {
  mpc_err_print_to(x, stdout);
}

void mpc_err_print_to(mpc_err_t* x, FILE* f) {
  
  if (x->failure) {
    fprintf(f, "%s:%i:%i: error: %s\n", 
      x->filename, x->state.row, 
      x->state.col, x->failure);
    return;
  }
  
  fprintf(f, "%s:%i:%i: error: expected ", x->filename, x->state.row, x->state.col);
  
  if (x->expected_num == 0) {
    
    fprintf(f, "ERROR: NOTHING EXPECTED");
    
  } else if (x->expected_num == 1) {
    
    fprintf(f, "%s", x->expected[0]);
    
  } else {
  
    int i;
    for (i = 0; i < x->expected_num-2; i++) {
      fprintf(f, "%s, ", x->expected[i]);
    } 
    
    fprintf(f, "%s or %s", 
      x->expected[x->expected_num-2], 
      x->expected[x->expected_num-1]);
    
  }
  
  fprintf(f, " at ");
  if (x->state.next == '\a') { fprintf(f, "bell"); }
  else if (x->state.next == '\b') { fprintf(f, "backspace"); }
  else if (x->state.next == '\f') { fprintf(f, "formfeed"); }
  else if (x->state.next == '\r') { fprintf(f, "carriage return"); }
  else if (x->state.next == '\v') { fprintf(f, "vertical tab"); }
  else if (x->state.next == '\0') { fprintf(f, "end of input"); }
  else if (x->state.next == '\n') { fprintf(f, "newline"); }
  else if (x->state.next == '\t') { fprintf(f, "tab"); }
  else { fprintf(f, "'%c'", x->state.next); }
  fprintf(f, "\n");
  
}

char* mpc_err_string_new(mpc_err_t* x) {
  
  char* buffer = malloc(1024);
  
  if (x->failure) {
    snprintf(buffer, 1023, "%s:%i:%i: error: %s\n", 
      x->filename, x->state.row, 
      x->state.col, x->failure);
    return buffer;
  }
  
  int left = 1023;
  left -= snprintf(buffer, left, "%s:%i:%i: error: expected ", x->filename, x->state.row, x->state.col);
  
  if (x->expected_num == 0) {
    
    left -= snprintf(buffer, left, "ERROR: NOTHING EXPECTED");
    
  } else if (x->expected_num == 1) {
    
    left -= snprintf(buffer, left, "%s", x->expected[0]);
    
  } else {
  
    int i;
    for (i = 0; i < x->expected_num-2; i++) {
      left -= snprintf(buffer, left, "%s, ", x->expected[i]);
    } 
    
    left -= snprintf(buffer, left, "%s or %s", 
      x->expected[x->expected_num-2], 
      x->expected[x->expected_num-1]);
    
  }
  
  left -= snprintf(buffer, left, " at ");
  if (x->state.next == '\a') { left -= snprintf(buffer, left, "bell"); }
  else if (x->state.next == '\b') { left -= snprintf(buffer, left, "backspace"); }
  else if (x->state.next == '\f') { left -= snprintf(buffer, left, "formfeed"); }
  else if (x->state.next == '\r') { left -= snprintf(buffer, left, "carriage return"); }
  else if (x->state.next == '\v') { left -= snprintf(buffer, left, "vertical tab"); }
  else if (x->state.next == '\0') { left -= snprintf(buffer, left, "end of input"); }
  else if (x->state.next == '\n') { left -= snprintf(buffer, left, "newline"); }
  else if (x->state.next == '\t') { left -= snprintf(buffer, left, "tab"); }
  else { left -= snprintf(buffer, left, "'%c'", x->state.next); }
  left -= snprintf(buffer, left, "\n");
  
  buffer = realloc(buffer, strlen(buffer) + 1);
  
  return buffer;
  
}

static mpc_err_t* mpc_err_either(mpc_err_t* x, mpc_err_t* y) {
  
  if (x->state.pos > y->state.pos) {
    mpc_err_delete(y);
    return x;
  }
  
  if (x->state.pos < y->state.pos) {
    mpc_err_delete(x);
    return y;
  }
  
  if (x->state.pos == y->state.pos) {  
    
    int i;
    for (i = 0; i < y->expected_num; i++) {
      if (mpc_err_contains_expected(x, y->expected[i])) { continue; }
      else { mpc_err_add_expected(x, y->expected[i]); }      
    }
    
    mpc_err_delete(y);
    return x;
  }
  
  return NULL;
  
}

static mpc_err_t* mpc_err_or(mpc_err_t** x, int n) {
  mpc_err_t* e = x[0];
  
  int i;
  for (i = 1; i < n; i++) {
    e = mpc_err_either(e, x[i]);
  }
  
  return e;
}

static mpc_err_t* mpc_err_many1(mpc_err_t* x) {
  
  char* expect = malloc(strlen("one or more of ") + 1);
  strcpy(expect, "one or more of ");
  
  int i;
  for (i = 0; i < x->expected_num - 1; i++) {
    expect = realloc(expect, strlen(expect) + strlen(x->expected[i]) + strlen(", ") + 1);
    strcat(expect, x->expected[i]);
    strcat(expect, ", ");
  }
  
  expect = realloc(expect, strlen(expect) + strlen(x->expected[x->expected_num-1]) + 1);
  strcat(expect, x->expected[x->expected_num-1]);
  
  mpc_err_clear_expected(x, expect);
  free(expect);
  
  return x;
}

static mpc_err_t* mpc_err_count(mpc_err_t* x, int n) {

  int digits = n/10 + 1;
  char* expect = malloc(digits + strlen(" of ") + 1);
  sprintf(expect, "%i of ", n);
  
  int i;
  for (i = 0; i < x->expected_num - 1; i++) {
    expect = realloc(expect, strlen(expect) + strlen(x->expected[i]) + strlen(", ") + 1);
    strcat(expect, x->expected[i]);
    strcat(expect, ", ");
  }
  
  expect = realloc(expect, strlen(expect) + strlen(x->expected[x->expected_num-1]) + 1);
  strcat(expect, x->expected[x->expected_num-1]);
  
  mpc_err_clear_expected(x, expect);
  free(expect);
  
  return x;
}

char* mpc_err_filename(mpc_err_t* x) {
  return x->filename;
}

char** mpc_err_expected(mpc_err_t* x, int* num) {
  *num = x->expected_num;
  return x->expected;
}

int mpc_err_line(mpc_err_t* x) {
  return x->state.row;
}

int mpc_err_column(mpc_err_t* x) {
  return x->state.col;
}

char mpc_err_unexpected(mpc_err_t* x) {
  return x->state.next;
}

/*
** Input Type
*/

typedef struct {

  char* filename;
  char* str;
  mpc_state_t state;
  
  bool backtrack;
  int marks_num;
  mpc_state_t* marks;
  
} mpc_input_t;

static mpc_input_t* mpc_input_new(const char* filename, const char* str) {

  mpc_input_t* i = malloc(sizeof(mpc_input_t));
  i->str = malloc(strlen(str) + 1);
  strcpy(i->str, str);
  
  i->filename = malloc(strlen(filename) + 1);
  strcpy(i->filename, filename);
  
  i->state.next = i->str[0];
  i->state.last = '\0';
  i->state.pos = 0;
  i->state.row = 0;
  i->state.col = 0;
  
  i->backtrack = true;
  i->marks_num = 0;
  i->marks = NULL;
  
  return i;
}

static void mpc_input_delete(mpc_input_t* i) {
  free(i->filename);
  free(i->str);
  free(i->marks);
  free(i);
}

static void mpc_input_backtrack_disable(mpc_input_t* i) {
  i->backtrack = false;
}

static void mpc_input_backtrack_enable(mpc_input_t* i) {
  i->backtrack = true;
}

static void mpc_input_mark(mpc_input_t* i) {
  if (!i->backtrack) { return; }
  i->marks_num++;
  i->marks = realloc(i->marks, sizeof(mpc_state_t) * i->marks_num);
  i->marks[i->marks_num-1] = i->state;
}

static void mpc_input_unmark(mpc_input_t* i) {
  if (!i->backtrack) { return; }
  i->marks_num--;
  i->marks = realloc(i->marks, sizeof(mpc_state_t) * i->marks_num);
}

static void mpc_input_rewind(mpc_input_t* i) {
  if (!i->backtrack) { return; }
  i->state = i->marks[i->marks_num-1];
  mpc_input_unmark(i);
}

static bool mpc_input_next(mpc_input_t* i, char** o) {

  i->state.last = i->str[i->state.pos];
  i->state.pos++;
  i->state.col++;
  
  if (i->state.last == '\n') {
    i->state.col = 0;
    i->state.row++;
  }
  
  (*o) = malloc(2);
  (*o)[0] = i->state.last;
  (*o)[1] = '\0';
  return true;

}

static bool mpc_input_any(mpc_input_t* i, char** o) {

  if (i->state.pos >= strlen(i->str)) { i->state.next = '\0'; return false; }
  if (i->str[i->state.pos] == '\0') {
    i->state.next = i->str[i->state.pos];
    return false;
  }

  return mpc_input_next(i, o);
  
}

static bool mpc_input_char(mpc_input_t* i, char c, char** o) {
  
  if (i->state.pos >= strlen(i->str)) { i->state.next = '\0'; return false; }
  if (i->str[i->state.pos] != c) {
    i->state.next = i->str[i->state.pos];
    return false; 
  }
  
  return mpc_input_next(i, o);

}

static bool mpc_input_range(mpc_input_t* i, char c, char d, char** o) {

  if (i->state.pos >= strlen(i->str)) { i->state.next = '\0'; return false; }
  if (i->str[i->state.pos] < c ||
      i->str[i->state.pos] > d) {
    i->state.next = i->str[i->state.pos];
    return false;
  }
  
  return mpc_input_next(i, o);

}

static bool char_in_string(char c, const char* x) {
  
  while (*x) {
    if (*x == c) { return true; }
    x++;
  }
  
  return false;
}

static bool mpc_input_oneof(mpc_input_t* i, const char* c, char** o) {
  
  if (i->state.pos >= strlen(i->str)) { i->state.next = '\0'; return false; }
  if (!char_in_string(i->str[i->state.pos], c)) {
    i->state.next = i->str[i->state.pos];
    return false;
  }
  
  return mpc_input_next(i, o);

}

static bool mpc_input_noneof(mpc_input_t* i, const char* c, char** o) {
  
  if (i->state.pos >= strlen(i->str)) { i->state.next = '\0'; return false; }
  if (char_in_string(i->str[i->state.pos], c)	|| (i->str[i->state.pos] == '\0')) {
    i->state.next = i->str[i->state.pos]; 
    return false;
  }
  
  return mpc_input_next(i, o);

}

static bool mpc_input_satisfy(mpc_input_t* i, bool(*cond)(char), char** o) {
  
  if (i->state.pos >= strlen(i->str)) { i->state.next = '\0'; return false; }
  if (!cond(i->str[i->state.pos])) { i->state.next = i->str[i->state.pos]; return false; }
  
  return mpc_input_next(i, o);
  
}

static bool mpc_input_eoi(mpc_input_t* i) {
  return i->state.pos == strlen(i->str);
}

static bool mpc_input_soi(mpc_input_t* i) {
  return i->state.pos == 0;
}

static bool mpc_input_string(mpc_input_t* i, const char* c, char** o) {
  
  mpc_input_mark(i);
  char* co = NULL;
  const char* x = c;
  while (*x) {
    if (mpc_input_char(i, *x, &co)) {
      free(co);
    } else {
      mpc_input_rewind(i);
      return false;
    }
    x++;
  }
  mpc_input_unmark(i);
  
  *o = malloc(strlen(c) + 1);
  strcpy(*o, c);
  return true;
}

/*
** Parser Type
*/

enum {
  MPC_TYPE_UNDEFINED = 0,
  MPC_TYPE_PASS      = 1,
  MPC_TYPE_FAIL      = 2,
  MPC_TYPE_LIFT      = 3,
  MPC_TYPE_LIFT_VAL  = 4,
  MPC_TYPE_EXPECT    = 5,
  
  MPC_TYPE_SOI       = 6,
  MPC_TYPE_EOI       = 7,
  MPC_TYPE_ANY       = 8,
  MPC_TYPE_SINGLE    = 9,
  MPC_TYPE_ONEOF     = 10,
  MPC_TYPE_NONEOF    = 11,
  MPC_TYPE_RANGE     = 12,
  MPC_TYPE_SATISFY   = 13,
  MPC_TYPE_STRING    = 14,
  
  MPC_TYPE_APPLY     = 15,
  MPC_TYPE_APPLY_TO  = 16,
  MPC_TYPE_PREDICT   = 17,
  MPC_TYPE_NOT       = 18,
  MPC_TYPE_MAYBE     = 19,
  MPC_TYPE_MANY      = 20,
  MPC_TYPE_MANY1     = 21,
  MPC_TYPE_COUNT     = 22,
  
  MPC_TYPE_ELSE      = 23,
  MPC_TYPE_ALSO      = 24,
  MPC_TYPE_OR        = 25,
  MPC_TYPE_AND       = 26,
};

typedef struct { char* m; } mpc_pdata_fail_t;
typedef struct { mpc_lift_t lf; void* x; } mpc_pdata_lift_t;
typedef struct { mpc_parser_t* x; char* m; } mpc_pdata_expect_t;
typedef struct { char x; } mpc_pdata_single_t;
typedef struct { char x; char y; } mpc_pdata_range_t;
typedef struct { bool(*f)(char); } mpc_pdata_satisfy_t;
typedef struct { char* x; } mpc_pdata_string_t;
typedef struct { mpc_parser_t* x; mpc_apply_t f; } mpc_pdata_apply_t;
typedef struct { mpc_parser_t* x; mpc_apply_to_t f; void* d; } mpc_pdata_apply_to_t;
typedef struct { mpc_parser_t* x; } mpc_pdata_predict_t;
typedef struct { mpc_parser_t* x; mpc_dtor_t dx; mpc_lift_t lf; } mpc_pdata_not_t;
typedef struct { mpc_parser_t* x; mpc_fold_t f; int n; mpc_dtor_t dx; mpc_lift_t lf; } mpc_pdata_repeat_t;
typedef struct { mpc_parser_t* x; mpc_parser_t* y; } mpc_pdata_else_t;
typedef struct { mpc_parser_t* x; mpc_parser_t* y; mpc_dtor_t dx; mpc_fold_t f; } mpc_pdata_also_t;
typedef struct { int n; mpc_parser_t** xs; } mpc_pdata_or_t;
typedef struct { int n; mpc_parser_t** xs; mpc_dtor_t* dxs; mpc_afold_t f; } mpc_pdata_and_t;

typedef union {
  mpc_pdata_fail_t fail;
  mpc_pdata_lift_t lift;
  mpc_pdata_expect_t expect;
  mpc_pdata_single_t single;
  mpc_pdata_range_t range;
  mpc_pdata_satisfy_t satisfy;
  mpc_pdata_string_t string;
  mpc_pdata_apply_t apply;
  mpc_pdata_apply_to_t apply_to;
  mpc_pdata_predict_t predict;
  mpc_pdata_not_t not;
  mpc_pdata_repeat_t repeat;
  mpc_pdata_else_t orelse;
  mpc_pdata_also_t also;
  mpc_pdata_and_t and;
  mpc_pdata_or_t or;
} mpc_pdata_t;

struct mpc_parser_t {
  bool retained;
  char* name;
  uint8_t type;
  mpc_pdata_t data;
};

/*
** This is rather pleasant. The core parsing routine
** is written in about 500 lines of C.
**
** I also love the way in which each parsing type
** concisely matches some construct or idiom in C.
** Particularly nice is are the `either` and `also`
** types which have a broken but mirrored structure
** with return value and error reflected.
**
*/

#define MPC_SUCCESS(x) r->output = x; return true;
#define MPC_FAILURE(x) r->error = x; return false;
#define MPC_TRY(x, f) if (f) { MPC_SUCCESS(x) } else { MPC_FAILURE(mpc_err_new_fail(i->filename, i->state, "Incorrect Input")); }

bool mpc_parse_input(mpc_input_t* i, mpc_parser_t* p, mpc_result_t* r) {
  
  memset(r, 0, sizeof(mpc_result_t)); 
  
  if (p->type == MPC_TYPE_UNDEFINED) { MPC_FAILURE(mpc_err_new_fail(i->filename, i->state, "Parser Undefined!")); }
  
  /* Trivial Parsers */
  
  if (p->type == MPC_TYPE_PASS) { MPC_SUCCESS(NULL); }
  if (p->type == MPC_TYPE_FAIL) { MPC_FAILURE(mpc_err_new_fail(i->filename, i->state, p->data.fail.m)); }
  if (p->type == MPC_TYPE_LIFT) { MPC_SUCCESS(p->data.lift.lf()); }
  if (p->type == MPC_TYPE_LIFT_VAL) { MPC_SUCCESS(p->data.lift.x); }
  
  /* Basic Parsers */

  if (p->type == MPC_TYPE_SOI)     { if (mpc_input_soi(i)) { MPC_SUCCESS(NULL) } else { MPC_FAILURE(mpc_err_new(i->filename, i->state, "start of input")); } }  
  if (p->type == MPC_TYPE_EOI)     { if (mpc_input_eoi(i)) { MPC_SUCCESS(NULL) } else { MPC_FAILURE(mpc_err_new(i->filename, i->state, "end of input")); } }
  
  char* s = NULL;  

  if (p->type == MPC_TYPE_ANY)     { MPC_TRY(s, mpc_input_any(i, &s)); }
  if (p->type == MPC_TYPE_SINGLE)  { MPC_TRY(s, mpc_input_char(i, p->data.single.x, &s)); }
  if (p->type == MPC_TYPE_RANGE)   { MPC_TRY(s, mpc_input_range(i, p->data.range.x, p->data.range.y, &s)); }
  if (p->type == MPC_TYPE_ONEOF)   { MPC_TRY(s, mpc_input_oneof(i, p->data.string.x, &s)); }
  if (p->type == MPC_TYPE_NONEOF)  { MPC_TRY(s, mpc_input_noneof(i, p->data.string.x, &s)); }
  if (p->type == MPC_TYPE_SATISFY) { MPC_TRY(s, mpc_input_satisfy(i, p->data.satisfy.f, &s)); }
  if (p->type == MPC_TYPE_STRING)  { MPC_TRY(s, mpc_input_string(i, p->data.string.x, &s)); }
  
  /* Advanced Parsers */
  
  int c = 0; 
  mpc_val_t* t = NULL;
  mpc_result_t x, y;
  memset(&x, 0, sizeof(mpc_result_t));
  memset(&y, 0, sizeof(mpc_result_t));

  if (p->type == MPC_TYPE_EXPECT) {
    if (mpc_parse_input(i, p->data.expect.x, &x)) {
      MPC_SUCCESS(x.output);
    } else {
      mpc_err_delete(x.error);
      MPC_FAILURE(mpc_err_new(i->filename, i->state, p->data.expect.m));
    }
  } 
  
  if (p->type == MPC_TYPE_APPLY) {
    if (mpc_parse_input(i, p->data.apply.x, &x)) {
      MPC_SUCCESS(p->data.apply.f(x.output));
    } else {
      MPC_FAILURE(x.error);
    }
  }
  
  if (p->type == MPC_TYPE_APPLY_TO) {
    if (mpc_parse_input(i, p->data.apply_to.x, &x)) {
      MPC_SUCCESS(p->data.apply_to.f(x.output, p->data.apply_to.d));
    } else {
      MPC_FAILURE(x.error);
    }
  }
  
  if (p->type == MPC_TYPE_PREDICT) {
    mpc_input_backtrack_disable(i);
    bool res = mpc_parse_input(i, p->data.predict.x, r);
    mpc_input_backtrack_enable(i);
    return res;    
  }
  
  if (p->type == MPC_TYPE_NOT) {
    mpc_input_mark(i);
    if (mpc_parse_input(i, p->data.not.x, &x)) {
      mpc_input_rewind(i);
      p->data.not.dx(x.output);
      MPC_FAILURE(mpc_err_new(i->filename, i->state, "opposite"));
    } else {
      mpc_input_unmark(i);
      mpc_err_delete(x.error);
      MPC_SUCCESS(p->data.not.lf());
    }
  }
  
  if (p->type == MPC_TYPE_MAYBE) {
    if (mpc_parse_input(i, p->data.repeat.x, &x)) { MPC_SUCCESS(x.output); }
    mpc_err_delete(x.error);
    MPC_SUCCESS(p->data.repeat.lf());
  }
  
  if (p->type == MPC_TYPE_MANY) {
    while (mpc_parse_input(i, p->data.repeat.x, &x)) { t = p->data.repeat.f(t, x.output); }
    mpc_err_delete(x.error);
    MPC_SUCCESS(t ? t : p->data.repeat.lf());
  }
  
  if (p->type == MPC_TYPE_MANY1) {
    
    while (mpc_parse_input(i, p->data.repeat.x, &x)) { t = p->data.repeat.f(t, x.output); c++; }
    
    if (c >= 1) {
      mpc_err_delete(x.error);
      MPC_SUCCESS(t);
    } else {
      MPC_FAILURE(mpc_err_many1(x.error));
    }
    
  }
  
  if (p->type == MPC_TYPE_COUNT) {
    
    mpc_input_mark(i);
    while (mpc_parse_input(i, p->data.repeat.x, &x)) {
      t = p->data.repeat.f(t, x.output);
      c++;
      if (c == p->data.repeat.n) { break; }
    }
    
    if (c == p->data.repeat.n) {
      mpc_input_unmark(i);
      MPC_SUCCESS(t ? t : p->data.repeat.lf());
    } else {
      p->data.repeat.dx(t);
      mpc_input_rewind(i);
      MPC_FAILURE(mpc_err_count(x.error, p->data.repeat.n));
    }
    
  }
  
  /* Combinatory Parsers */
  
  if (p->type == MPC_TYPE_ELSE) {
    if (mpc_parse_input(i, p->data.orelse.x, &x)) { MPC_SUCCESS(x.output); }
    if (mpc_parse_input(i, p->data.orelse.y, &y)) { mpc_err_delete(x.error); MPC_SUCCESS(y.output); }
    MPC_FAILURE(mpc_err_either(x.error, y.error));
  }
  
  if (p->type == MPC_TYPE_ALSO) {
    mpc_input_mark(i);
    if (!mpc_parse_input(i, p->data.also.x, &x)) { mpc_input_rewind(i); MPC_FAILURE(x.error); }
    if (!mpc_parse_input(i, p->data.also.y, &y)) { mpc_input_rewind(i); p->data.also.dx(x.output); MPC_FAILURE(y.error); }
    mpc_input_unmark(i);
    MPC_SUCCESS(p->data.also.f(x.output, y.output));
  }
  
  if (p->type == MPC_TYPE_OR) {
    
    mpc_result_t* rs = malloc(sizeof(mpc_result_t) * p->data.or.n);
    int ri, pri;
    
    for (ri = 0; ri < p->data.or.n; ri++) {
      if (mpc_parse_input(i, p->data.or.xs[ri], &rs[ri])) {
        
        for (pri = 0; pri < ri; pri++) { mpc_err_delete(rs[pri].error); }
        r->output = rs[ri].output;
        free(rs);
        return true;
        
      }
    }
    
    mpc_err_t** vals = malloc(sizeof(mpc_err_t*) * p->data.or.n);
    
    for (ri = 0; ri < p->data.and.n; ri++) {
      vals[ri] = rs[ri].error;
    }
    
    r->error = mpc_err_or(vals, p->data.or.n);
    
    free(vals);
    free(rs);
    return false;
    
  }
  
  if (p->type == MPC_TYPE_AND) {
    
    mpc_input_mark(i);

    mpc_result_t* rs = malloc(sizeof(mpc_result_t) * p->data.and.n);
    int ri, pri;
    
    for (ri = 0; ri < p->data.and.n; ri++) {
      if (!mpc_parse_input(i, p->data.and.xs[ri], &rs[ri])) {
        
        for (pri = 0; pri < ri; pri++) { p->data.and.dxs[pri](rs[pri].output); }
        r->error = rs[ri].error;
        free(rs);
        mpc_input_rewind(i);
        return false;
        
      }
    }
    
    mpc_val_t** vals = malloc(sizeof(mpc_val_t*) * p->data.and.n);
    
    for (ri = 0; ri < p->data.and.n; ri++) {
      vals[ri] = rs[ri].output;
    }
    
    r->output = p->data.and.f(p->data.and.n, vals);
    
    free(rs);
    free(vals);
    mpc_input_unmark(i);
    return true;
  
  }
  
  MPC_FAILURE(mpc_err_new_fail(i->filename, i->state, "Unknown Parser Type Id!"));
  
}

#undef MPC_SUCCESS
#undef MPC_FAILURE
#undef MPC_TRY

bool mpc_parse(const char* filename, const char* s, mpc_parser_t* p, mpc_result_t* r) {
  mpc_input_t* i = mpc_input_new(filename, s);
  bool x = mpc_parse_input(i, p, r);
  mpc_input_delete(i);
  return x;
}

bool mpc_parse_file(const char* filename, FILE* f, mpc_parser_t* p, mpc_result_t* r) {
  fseek(f, 0, SEEK_END);
  int len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buff = malloc(len + 1);
  fread(buff, 1, len, f);
  buff[len] = '\0';
  
  bool x = mpc_parse(filename, buff, p, r);
  
  free(buff);
  return x;
}

bool mpc_parse_filename(const char* filename, mpc_parser_t* p, mpc_result_t* r) {
  
  FILE* f = fopen(filename, "r");
  
  if (f == NULL) {
    r->output = NULL;
    r->error = mpc_err_new_fail(filename, mpc_state_null(), "Unable to open file!");
    return false;
  }
  
  bool res = mpc_parse_file(filename, f, p, r);
  fclose(f);
  return res;
}

/*
** Building a Parser
*/

static void mpc_undefine_unretained(mpc_parser_t* p, bool force);

static void mpc_undefine_or(mpc_parser_t* p) {
  
  int i;
  for (i = 0; i < p->data.or.n; i++) {
    mpc_undefine_unretained(p->data.or.xs[i], false);
  }
  free(p->data.or.xs);
  
}

static void mpc_undefine_and(mpc_parser_t* p) {
  
  int i;
  for (i = 0; i < p->data.and.n; i++) {
    mpc_undefine_unretained(p->data.and.xs[i], false);
  }
  free(p->data.and.xs);
  free(p->data.and.dxs);
  
}

static void mpc_undefine_unretained(mpc_parser_t* p, bool force) {
  
  if (p->retained && !force) { return; }
  
  switch (p->type) {
    
    case MPC_TYPE_FAIL:
      free(p->data.fail.m);
      break;
    
    case MPC_TYPE_ONEOF: 
    case MPC_TYPE_NONEOF:
    case MPC_TYPE_STRING:
      free(p->data.string.x); 
      break;
    
    case MPC_TYPE_APPLY:
      mpc_undefine_unretained(p->data.apply.x, false);
      break;
    
    case MPC_TYPE_APPLY_TO:
      mpc_undefine_unretained(p->data.apply_to.x, false);
      break;
    
    case MPC_TYPE_PREDICT:
      mpc_undefine_unretained(p->data.predict.x, false);
      break;
    
    case MPC_TYPE_NOT:
      mpc_undefine_unretained(p->data.not.x, false);
      break;
    
    case MPC_TYPE_EXPECT:
      mpc_undefine_unretained(p->data.expect.x, false);
      free(p->data.expect.m);
      break;

    case MPC_TYPE_MAYBE:
    case MPC_TYPE_MANY:
    case MPC_TYPE_MANY1:
    case MPC_TYPE_COUNT:
      mpc_undefine_unretained(p->data.repeat.x, false);
      break;
    
    case MPC_TYPE_ELSE: 
      mpc_undefine_unretained(p->data.orelse.x, false); 
      mpc_undefine_unretained(p->data.orelse.y, false); 
      break;
    
    case MPC_TYPE_ALSO:
      mpc_undefine_unretained(p->data.also.x, false); 
      mpc_undefine_unretained(p->data.also.y, false); 
      break;
    
    case MPC_TYPE_OR:
      mpc_undefine_or(p);
      break;
    
    case MPC_TYPE_AND:
      mpc_undefine_and(p);
      break;
    
    default: break;
  }
  
  if (!force) {
    free(p->name);
    free(p);
  }
  
}

void mpc_delete(mpc_parser_t* p) {
  if (p->retained) {

    if (p->type != MPC_TYPE_UNDEFINED) {
      mpc_undefine_unretained(p, false);
    } 
    
    free(p->name);
    free(p);
  
  } else {
    mpc_undefine_unretained(p, false);  
  }
}

static void mpc_soft_delete(mpc_val_t* x) {
  mpc_undefine_unretained(x, false);
}

static mpc_parser_t* mpc_undefined(void) {
  mpc_parser_t* p = calloc(1, sizeof(mpc_parser_t));
  p->retained = false;
  p->type = MPC_TYPE_UNDEFINED;
  p->name = NULL;
  return p;
}

mpc_parser_t* mpc_new(const char* name) {
  mpc_parser_t* p = mpc_undefined();
  p->retained = true;
  p->name = realloc(p->name, strlen(name) + 1);
  strcpy(p->name, name);
  return p;
}

mpc_parser_t* mpc_undefine(mpc_parser_t* p) {
  mpc_undefine_unretained(p, true);
  p->type = MPC_TYPE_UNDEFINED;
  return p;
}

mpc_parser_t* mpc_define(mpc_parser_t* p, mpc_parser_t* a) {
  
  if (p->retained) {
    p->type = a->type;
    p->data = a->data;
  } else {
    mpc_parser_t* a2 = mpc_failf("Attempt to assign to Unretained Parser!");
    p->type = a2->type;
    p->data = a2->data;
    free(a2);
  }
  
  free(a);
  return p;  
}

void mpc_cleanup(int n, ...) {
  va_list va;
  va_start(va, n);  
  mpc_cleanup_va(n, va);
  va_end(va);  
}

void mpc_cleanup_va(int n, va_list va) {
  
  int i;
  mpc_parser_t** list = malloc(sizeof(mpc_parser_t*) * n);
  
  for (i = 0; i < n; i++) {
    list[i] = va_arg(va, mpc_parser_t*);
  }
  
  for (i = 0; i < n; i++) {
    mpc_undefine(list[i]);
  }
  
  for (i = 0; i < n; i++) {
    mpc_delete(list[i]);
  }
  
  free(list);
}

mpc_parser_t* mpc_pass(void) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_PASS;
  return p;
}

mpc_parser_t* mpc_fail(const char* m) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_FAIL;
  p->data.fail.m = malloc(strlen(m) + 1);
  strcpy(p->data.fail.m, m);
  return p;
}

mpc_parser_t* mpc_failf(const char* fmt, ...) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_FAIL;
  
  va_list va;
  va_start(va, fmt);
  char* buffer = malloc(1024);
  vsnprintf(buffer, 1023, fmt, va);
  va_end(va);
  
  buffer = realloc(buffer, strlen(buffer) + 1);
  
  return p;

}

mpc_parser_t* mpc_lift_val(mpc_val_t* x) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_LIFT_VAL;
  p->data.lift.x = x;
  return p;
}

mpc_parser_t* mpc_lift(mpc_lift_t lf) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_LIFT;
  p->data.lift.lf = lf;
  return p;
}

mpc_parser_t* mpc_expect(mpc_parser_t* a, const char* expected) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_EXPECT;
  p->data.expect.x = a;
  p->data.expect.m = malloc(strlen(expected) + 1);
  strcpy(p->data.expect.m, expected);
  return p;
}


/*
** Basic Parsers
*/

mpc_parser_t* mpc_any(void) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_ANY;
  return mpc_expect(p, "any character");
}

mpc_parser_t* mpc_char(char c) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_SINGLE;
  p->data.single.x = c;
  return mpc_expect(p, (char[]){ '\'', c, '\'', '\0' } );
}

mpc_parser_t* mpc_range(char s, char e) {
  
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_RANGE;
  p->data.range.x = s;
  p->data.range.y = e;
  
  char expected[30];
  strcpy(expected, "character between '");
  strcat(expected, (char[]){ s, '\0' });
  strcat(expected, "' and '");
  strcat(expected, (char[]){ e, '\0' });
  strcat(expected, "'");
  
  return mpc_expect(p, expected);
}

mpc_parser_t* mpc_oneof(const char* s) {
  
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_ONEOF;
  p->data.string.x = malloc(strlen(s) + 1);
  strcpy(p->data.string.x, s);
  
  char* expected = malloc(strlen(s) + 10);
  strcpy(expected, "one of '");
  strcat(expected, s);
  strcat(expected, "'");
  
  p = mpc_expect(p, expected);
  
  free(expected);
  
  return p;
}

mpc_parser_t* mpc_noneof(const char* s) {

  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_NONEOF;
  p->data.string.x = malloc(strlen(s) + 1);
  strcpy(p->data.string.x, s);
  
  char* expected = malloc(strlen(s) + 11);
  strcpy(expected, "none of '");
  strcat(expected, s);
  strcat(expected, "'");
  
  p = mpc_expect(p, expected);
  
  free(expected);
  
  return p;

}

mpc_parser_t* mpc_satisfy(bool(*f)(char)) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_SATISFY;
  p->data.satisfy.f = f;
  
  return p;
}

mpc_parser_t* mpc_string(const char* s) {

  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_STRING;
  p->data.string.x = malloc(strlen(s) + 1);
  strcpy(p->data.string.x, s);
  
  char* expected = malloc(strlen(s) + 3);
  strcpy(expected, "\"");
  strcat(expected, s);
  strcat(expected, "\"");
  
  p = mpc_expect(p, expected);
  
  free(expected);
  
  return p;

}

/*
** Core Parsers
*/

mpc_parser_t* mpc_apply(mpc_parser_t* a, mpc_apply_t f) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_APPLY;
  p->data.apply.x = a;
  p->data.apply.f = f;
  return p;
}

mpc_parser_t* mpc_apply_to(mpc_parser_t* a, mpc_apply_to_t f, void* x) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_APPLY_TO;
  p->data.apply_to.x = a;
  p->data.apply_to.f = f;
  p->data.apply_to.d = x;
  return p;
}

mpc_parser_t* mpc_predict(mpc_parser_t* a) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_PREDICT;
  p->data.predict.x = a;
  return p;
}

mpc_parser_t* mpc_not_else(mpc_parser_t* a, mpc_dtor_t da, mpc_lift_t lf) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_NOT;
  p->data.not.x = a;
  p->data.not.dx = da;
  p->data.not.lf = lf;
  return p;
}

mpc_parser_t* mpc_not(mpc_parser_t* a, mpc_dtor_t da) {
  return mpc_not_else(a, da, mpcf_lift_null);
}

mpc_parser_t* mpc_maybe_else(mpc_parser_t* a, mpc_lift_t lf) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_MAYBE;
  p->data.repeat.x = a;
  p->data.repeat.lf = lf;
  return p;
}

mpc_parser_t* mpc_maybe(mpc_parser_t* a) {
  return mpc_maybe_else(a, mpcf_lift_null);
}

mpc_parser_t* mpc_many(mpc_parser_t* a, mpc_fold_t f) {
  return mpc_many_else(a, f, mpcf_lift_null);
}

mpc_parser_t* mpc_many_else(mpc_parser_t* a, mpc_fold_t f, mpc_lift_t lf) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_MANY;
  p->data.repeat.x = a;
  p->data.repeat.f = f;
  p->data.repeat.lf = lf;
  return p;
}

mpc_parser_t* mpc_many1(mpc_parser_t* a, mpc_fold_t f) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_MANY1;
  p->data.repeat.x = a;
  p->data.repeat.f = f;
  return p;
}

mpc_parser_t* mpc_count_else(mpc_parser_t* a, mpc_dtor_t da, mpc_fold_t f, int n, mpc_lift_t lf) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_COUNT;
  p->data.repeat.x = a;
  p->data.repeat.dx = da;
  p->data.repeat.f = f;
  p->data.repeat.n = n;
  p->data.repeat.lf = lf;
  return p;
}

mpc_parser_t* mpc_count(mpc_parser_t* a, mpc_dtor_t da, mpc_fold_t f, int n) {
  return mpc_count_else(a, da, f, n, mpcf_lift_null);
}

mpc_parser_t* mpc_else(mpc_parser_t* a, mpc_parser_t* b) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_ELSE;
  p->data.orelse.x = a;
  p->data.orelse.y = b;
  return p;
}

mpc_parser_t* mpc_also(mpc_parser_t* a, mpc_parser_t* b, mpc_dtor_t da, mpc_fold_t f) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_ALSO;
  p->data.also.x = a;
  p->data.also.y = b;
  p->data.also.dx = da;
  p->data.also.f = f;
  return p;
}

mpc_parser_t* mpc_bind(mpc_parser_t* a, mpc_parser_t* b, mpc_dtor_t da, mpc_fold_t f) {
  return mpc_also(a, b, da, f);
}

mpc_parser_t* mpc_or(int n, ...) {

  mpc_parser_t* p = mpc_undefined();
  
  p->type = MPC_TYPE_OR;
  p->data.or.n = n;
  p->data.or.xs = malloc(sizeof(mpc_parser_t*) * n);
  
  va_list va;
  va_start(va, n);  
  int i;
  for (i = 0; i < n; i++) {
    p->data.or.xs[i] = va_arg(va, mpc_parser_t*);
  }
  va_end(va);
  
  return p;
}

mpc_parser_t* mpc_and(int n, mpc_afold_t f, ...) {

  mpc_parser_t* p = mpc_undefined();
  
  p->type = MPC_TYPE_AND;
  p->data.and.n = n;
  p->data.and.f = f;
  p->data.and.xs = malloc(sizeof(mpc_parser_t*) * n);
  p->data.and.dxs = malloc(sizeof(mpc_dtor_t) * (n-1));

  va_list va;
  va_start(va, f);  
  int i;
  for (i = 0; i < n; i++) {
    p->data.and.xs[i] = va_arg(va, mpc_parser_t*);
  }
  for (i = 0; i < (n-1); i++) {
    p->data.and.dxs[i] = va_arg(va, mpc_dtor_t);
  }  
  va_end(va);
  
  return p;
}

/*
** Common Parsers
*/

mpc_parser_t* mpc_eoi(void) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_EOI;
  return p;
}

mpc_parser_t* mpc_soi(void) {
  mpc_parser_t* p = mpc_undefined();
  p->type = MPC_TYPE_SOI;
  return p;
}

mpc_parser_t* mpc_space(void) { return mpc_expect(mpc_oneof(" \f\n\r\t\v"), "space"); }
mpc_parser_t* mpc_spaces(void) { return mpc_expect(mpc_many(mpc_space(), mpcf_strfold), "spaces"); }
mpc_parser_t* mpc_whitespace(void) { return mpc_expect(mpc_apply(mpc_spaces(), mpcf_free), "whitespace"); }

mpc_parser_t* mpc_newline(void) { return mpc_expect(mpc_char('\n'), "newline"); }
mpc_parser_t* mpc_tab(void) { return mpc_expect(mpc_char('\t'), "tab"); }
mpc_parser_t* mpc_escape(void) { return mpc_also(mpc_char('\\'), mpc_any(), free, mpcf_strfold); }

mpc_parser_t* mpc_digit(void) { return mpc_expect(mpc_oneof("012345689"), "digit"); }
mpc_parser_t* mpc_hexdigit(void) { return mpc_expect(mpc_oneof("0123456789ABCDEFabcdef"), "hex digit"); }
mpc_parser_t* mpc_octdigit(void) { return mpc_expect(mpc_oneof("01234567"), "oct digit"); }
mpc_parser_t* mpc_digits(void) { return mpc_expect(mpc_many1(mpc_digit(), mpcf_strfold), "digits"); }
mpc_parser_t* mpc_hexdigits(void) { return mpc_expect(mpc_many1(mpc_hexdigit(), mpcf_strfold), "hex digits"); }
mpc_parser_t* mpc_octdigits(void) { return mpc_expect(mpc_many1(mpc_octdigit(), mpcf_strfold), "oct digits"); }

mpc_parser_t* mpc_lower(void) { return mpc_expect(mpc_oneof("abcdefghijklmnopqrstuvwxyz"), "lowercase letter"); }
mpc_parser_t* mpc_upper(void) { return mpc_expect(mpc_oneof("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), "uppercase letter"); }
mpc_parser_t* mpc_alpha(void) { return mpc_expect(mpc_oneof("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"), "letter"); }
mpc_parser_t* mpc_underscore(void) { return mpc_expect(mpc_char('_'), "underscore"); }
mpc_parser_t* mpc_alphanum(void) { return mpc_expect(mpc_or(3, mpc_alpha(), mpc_digit(), mpc_underscore()), "alphanumeric"); }

mpc_parser_t* mpc_int(void) { return mpc_expect(mpc_apply(mpc_digits(), mpcf_int), "integer"); }
mpc_parser_t* mpc_hex(void) { return mpc_expect(mpc_apply(mpc_hexdigits(), mpcf_hex), "hexadecimal"); }
mpc_parser_t* mpc_oct(void) { return mpc_expect(mpc_apply(mpc_octdigits(), mpcf_oct), "octadecimal"); }
mpc_parser_t* mpc_number(void) { return mpc_expect(mpc_or(3, mpc_int(), mpc_hex(), mpc_oct()), "number"); }

mpc_parser_t* mpc_real(void) {

  /* [+-]?\d+(\.\d+)?([eE][+-]?[0-9]+)? */
  
  mpc_parser_t* p0 = mpc_maybe_else(mpc_oneof("+-"), mpcf_lift_emptystr);
  mpc_parser_t* p1 = mpc_digits();
  mpc_parser_t* p2 = mpc_maybe_else(mpc_also(mpc_char('.'), mpc_digits(), free, mpcf_strfold), mpcf_lift_emptystr);
  mpc_parser_t* p30 = mpc_oneof("eE");
  mpc_parser_t* p31 = mpc_maybe_else(mpc_oneof("+-"), mpcf_lift_emptystr);
  mpc_parser_t* p32 = mpc_digits();
  mpc_parser_t* p3 = mpc_maybe_else(mpc_and(3, mpcf_astrfold, p30, p31, p32, free, free), mpcf_lift_emptystr);
  
  return mpc_expect(mpc_and(4, mpcf_astrfold, p0, p1, p2, p3, free, free, free), "real");

}

mpc_parser_t* mpc_float(void) {
  return mpc_expect(mpc_apply(mpc_real(), mpcf_float), "float");
}

mpc_parser_t* mpc_char_lit(void) {
  return mpc_expect(mpc_between(mpc_else(mpc_escape(), mpc_any()), free, "'", "'"), "char");
}

mpc_parser_t* mpc_string_lit(void) {
  mpc_parser_t* strchar = mpc_else(mpc_escape(), mpc_noneof("\""));
  return mpc_expect(mpc_between(mpc_many_else(strchar, mpcf_strfold, mpcf_lift_emptystr), free, "\"", "\""), "string");
}

mpc_parser_t* mpc_regex_lit(void) {  
  mpc_parser_t* regexchar = mpc_else(mpc_escape(), mpc_noneof("/"));
  return mpc_expect(mpc_between(mpc_many_else(regexchar, mpcf_strfold, mpcf_lift_emptystr), free, "/", "/"), "regex");
}

mpc_parser_t* mpc_ident(void) {
  mpc_parser_t* p0 = mpc_else(mpc_alpha(), mpc_underscore());
  mpc_parser_t* p1 = mpc_many_else(mpc_alphanum(), mpcf_strfold, mpcf_lift_emptystr); 
  return mpc_also(p0, p1, free, mpcf_strfold);
}

/*
** Useful Parsers
*/

mpc_parser_t* mpc_start(mpc_parser_t* a) { return mpc_also(mpc_soi(), a, mpcf_dtor_null, mpcf_snd); }
mpc_parser_t* mpc_end(mpc_parser_t* a, mpc_dtor_t da) { return mpc_also(a, mpc_eoi(), da, mpcf_fst); }
mpc_parser_t* mpc_enclose(mpc_parser_t* a, mpc_dtor_t da) { return mpc_and(3, mpcf_asnd, mpc_soi(), a, mpc_eoi(), mpcf_dtor_null, da); }

mpc_parser_t* mpc_strip(mpc_parser_t* a) { return mpc_and(3, mpcf_asnd, mpc_whitespace(), a, mpc_whitespace(), mpcf_dtor_null, mpcf_dtor_null); }
mpc_parser_t* mpc_tok(mpc_parser_t* a) { return mpc_also(a, mpc_whitespace(), mpcf_dtor_null, mpcf_fst); }
mpc_parser_t* mpc_sym(const char* s) { return mpc_tok(mpc_string(s)); }
mpc_parser_t* mpc_total(mpc_parser_t* a, mpc_dtor_t da) { return mpc_enclose(mpc_strip(a), da); }

mpc_parser_t* mpc_between(mpc_parser_t* a, mpc_dtor_t ad, const char* o, const char* c) {
  return mpc_and(3, mpcf_between_free,
    mpc_string(o), a, mpc_string(c),
    free, ad);
}

mpc_parser_t* mpc_parens(mpc_parser_t* a, mpc_dtor_t ad)   { return mpc_between(a, ad, "(", ")"); }
mpc_parser_t* mpc_braces(mpc_parser_t* a, mpc_dtor_t ad)   { return mpc_between(a, ad, "<", ">"); }
mpc_parser_t* mpc_brackets(mpc_parser_t* a, mpc_dtor_t ad) { return mpc_between(a, ad, "{", "}"); }
mpc_parser_t* mpc_squares(mpc_parser_t* a, mpc_dtor_t ad)  { return mpc_between(a, ad, "[", "]"); }

mpc_parser_t* mpc_tok_between(mpc_parser_t* a, mpc_dtor_t ad, const char* o, const char* c) {
  return mpc_and(3, mpcf_between_free,
    mpc_sym(o), a, mpc_sym(c),
    free, ad);
}

mpc_parser_t* mpc_tok_parens(mpc_parser_t* a, mpc_dtor_t ad)   { return mpc_tok_between(a, ad, "(", ")"); }
mpc_parser_t* mpc_tok_braces(mpc_parser_t* a, mpc_dtor_t ad)   { return mpc_tok_between(a, ad, "<", ">"); }
mpc_parser_t* mpc_tok_brackets(mpc_parser_t* a, mpc_dtor_t ad) { return mpc_tok_between(a, ad, "{", "}"); }
mpc_parser_t* mpc_tok_squares(mpc_parser_t* a, mpc_dtor_t ad)  { return mpc_tok_between(a, ad, "[", "]"); }

/*
** Regular Expression Parsers
*/

/*
** So here is a cute bootstrapping.
**
** I'm using the previously defined
** mpc constructs and functions to
** parse the user regex string and
** construct a parser from it.
**
** As it turns out lots of the standard
** mpc functions look a lot like `fold`
** functions and so can be used indirectly
** by many of the parsing functions to build
** a parser directly - as we are parsing.
**
** This is certainly something that
** would be less elegant/interesting 
** in a two-phase parser which first
** builds an AST and then traverses it
** to generate the object.
**
** This whole thing acts as a great
** case study for how trivial it can be
** to write a great parser in a few
** lines of code using mpc.
*/

/*
**
**  ### Regular Expression Grammar
**
**      <regex> : (<term> "|" <regex>) | <term>
**     
**      <term> : <factor>*
**
**      <factor> : <base>
**               | <base> "*"
**               | <base> "+"
**               | <base> "?"
**               | <base> "{" <digits> "}"
**           
**      <base> : <char>
**             | "\" <char>
**             | "(" <regex> ")"
**             | "[" <range> "]"
*/

static mpc_val_t* mpc_re_afold_or(int n, mpc_val_t** xs) {
  free(xs[1]);
  return mpc_else(xs[0], xs[2]);
}

static mpc_val_t* mpc_re_fold_repeat(mpc_val_t* x, mpc_val_t* y) {
  if (strcmp(y, "*") == 0) { free(y); return mpc_many_else(x, mpcf_strfold, mpcf_lift_emptystr); }
  if (strcmp(y, "+") == 0) { free(y); return mpc_many1(x, mpcf_strfold); }
  if (strcmp(y, "?") == 0) { free(y); return mpc_maybe_else(x, mpcf_lift_emptystr); }
  int n = *(int*)y;
  free(y);
  return mpc_count_else(x, free, mpcf_strfold, n, mpcf_lift_emptystr);
}

static mpc_val_t* mpc_re_fold_many(mpc_val_t* t, mpc_val_t* x) {
  if (t == NULL) { return x; }
  if (x == NULL) { return t; }
  return mpc_also(t, x, free, mpcf_strfold);
}

static mpc_val_t* mpc_re_escape(mpc_val_t* x) {
  
  char* s = x;
  
  if (s[0] == '.') { free(x); return mpc_any(); }
  if (s[0] == '$') { free(x); return mpc_eoi(); }
  if (s[0] == '^') { free(x); return mpc_soi(); }
  
  if (s[0] == '\\') {
  
    if (s[1] == 'd') { free(x); return mpc_digit(); }
    if (s[1] == 'D') { free(x); return mpc_not_else(mpc_digit(), free, mpcf_lift_emptystr); }
    if (s[1] == 's') { free(x); return mpc_space(); }
    if (s[1] == 'S') { free(x); return mpc_not_else(mpc_space(), free, mpcf_lift_emptystr); }
    if (s[1] == 'w') { free(x); return mpc_alphanum(); }
    if (s[1] == 'W') { free(x); return mpc_not_else(mpc_alphanum(), free, mpcf_lift_emptystr); }
    if (s[1] == 'Z') { free(x); return mpc_eoi(); }
    
    mpc_parser_t* p = mpc_char(s[1]);
    free(x); return p;
  } else {
    mpc_parser_t* p = mpc_char(s[0]);
    free(x); return p;
  }
  
}

static mpc_val_t* mpc_re_range(mpc_val_t* x) {
  
  char* s = x;
  bool comp = false;
  
  if (*s == '\0') { free(x); return mpc_failf("Invalid Regex Range Specifier '%s'", x); } 
  
  if (*s == '^') {
    comp = true;
    s++;
  }
  
  if (*s == '\0') { free(x); return mpc_failf("Invalid Regex Range Specifier '%s'", x); }
  
  char* range = calloc(1, 1);
  
  while (*s) {
    
    /* TODO: Deal Properly with Escape characters */
    if (*s == '\\') {
      if (*(s+1) == '\0') { break; }
      range = realloc(range, strlen(range) + 2);
      strcat(range, (char[]){ *(s+1), '\0' });      
      s++;
    }
        
    else if (*s == '-') {
      
      char start = *(s-1);
      char end = *(s+1);
      
      if (end == '\0') { break; }
      if (end < start) { s++; continue; }
      
      range = realloc(range, strlen(range) + 1 + (end-start));
      
      int i;
      for (i = 0; i < (end-start); i++) {
        strcat(range, (char[]){start+i+1, '\0'});
      }
      
      s++;
    }
    
    else {
      range = realloc(range, strlen(range) + 2);
      strcat(range, (char[]){*s, '\0'});
    }
  
    s++;
  }
  
  mpc_parser_t* p = (comp ? mpc_noneof(range) : mpc_oneof(range));
  
  free(range);
  free(x);
  return p;
}

static mpc_val_t* mpc_re_lift(void) {
  return mpc_pass();
}

mpc_parser_t* mpc_re(const char* re) {
  
  mpc_parser_t* Regex  = mpc_new("regex");
  mpc_parser_t* Term   = mpc_new("term");
  mpc_parser_t* Factor = mpc_new("factor");
  mpc_parser_t* Base   = mpc_new("base");
  mpc_parser_t* Range  = mpc_new("range");
  
  mpc_define(Regex, mpc_else(
    mpc_and(3, mpc_re_afold_or, Term, mpc_char('|'), Regex, mpc_delete, free),
    Term
  ));
  
  mpc_define(Term, mpc_many_else(Factor, mpc_re_fold_many, mpc_re_lift));
  
  mpc_define(Factor, mpc_or(5,
    mpc_also(Base, mpc_char('*'), (mpc_dtor_t)mpc_delete, mpc_re_fold_repeat),
    mpc_also(Base, mpc_char('+'), (mpc_dtor_t)mpc_delete, mpc_re_fold_repeat),
    mpc_also(Base, mpc_char('?'), (mpc_dtor_t)mpc_delete, mpc_re_fold_repeat),
    mpc_also(Base, mpc_brackets(mpc_int(), free), (mpc_dtor_t)mpc_delete, mpc_re_fold_repeat),
    Base
  ));
  
  mpc_define(Base, mpc_or(4,
    mpc_parens(Regex, (mpc_dtor_t)mpc_delete),
    mpc_squares(Range, (mpc_dtor_t)mpc_delete),
    mpc_apply(mpc_escape(), mpc_re_escape),
    mpc_apply(mpc_noneof(")|"), mpc_re_escape)
  ));
  
  mpc_define(Range, mpc_apply(
    mpc_many_else(mpc_else(mpc_escape(), mpc_noneof("]")), mpcf_strfold, mpcf_lift_emptystr),
    mpc_re_range
  ));
  
  mpc_parser_t* RegexEnclose = mpc_enclose(Regex, (mpc_dtor_t)mpc_delete);
  
  mpc_result_t r;
  if(!mpc_parse("<mpc_re_compiler>", re, RegexEnclose, &r)) {
    char* err_msg = mpc_err_string_new(r.error);
    r.output = mpc_failf("Invalid Regex: %s", err_msg);
    free(err_msg);
    mpc_err_delete(r.error);  
  }
  
  mpc_delete(RegexEnclose);
  mpc_cleanup(5, Regex, Term, Factor, Base, Range);
  
  return r.output;
  
}

/*
** Common Fold Functions
*/

void mpcf_dtor_null(mpc_val_t* x) {
  return;
}

mpc_val_t* mpcf_lift_null(void) {
  return NULL;
}

mpc_val_t* mpcf_lift_emptystr(void) {
  return calloc(1, 1);
}

mpc_val_t* mpcf_free(mpc_val_t* x) {
  free(x);
  return NULL;
}

mpc_val_t* mpcf_int(mpc_val_t* x) {
  int* y = malloc(sizeof(int));
  *y = strtol(x, NULL, 10);
  free(x);
  return y;
}

mpc_val_t* mpcf_hex(mpc_val_t* x) {
  int* y = malloc(sizeof(int));
  *y = strtol(x, NULL, 16);
  free(x);
  return y;
}

mpc_val_t* mpcf_oct(mpc_val_t* x) {
  int* y = malloc(sizeof(int));
  *y = strtol(x, NULL, 8);
  free(x);
  return y;
}

mpc_val_t* mpcf_float(mpc_val_t* x) {
  float* y = malloc(sizeof(float));
  *y = strtod(x, NULL);
  free(x);
  return y;
}

static char* mpc_escape_input_c  = (char[]) {
  '\a', '\b', '\f', '\n', '\r',
  '\t', '\v', '\\', '\'', '\"', '\0'};
    
static char** mpc_escape_output_c = (char*[]) {
  "\\a", "\\b", "\\f", "\\n", "\\r",  "\\t", 
  "\\v", "\\\\", "\\'", "\\\"", "\\0", NULL};

static char* mpc_escape_input_raw_re = (char[]) { '/' };
static char** mpc_escape_output_raw_re = (char*[]) { "\\/", NULL };

static char* mpc_escape_input_raw_cstr = (char[]) { '"' };
static char** mpc_escape_output_raw_cstr = (char*[]) { "\\\"", NULL };

static char* mpc_escape_input_raw_cchar = (char[]) { '\'' };
static char** mpc_escape_output_raw_cchar = (char*[]) { "\\'", NULL };

static mpc_val_t* mpcf_escape_new(mpc_val_t* x, char* input, char** output) {
  
  int i;
  bool found;
  char* s = x;
  char* y = calloc(1, 1);
  
  while (*s) {
    
    i = 0;
    found = false;

    while (output[i]) {
      if (*s == input[i]) {
        y = realloc(y, strlen(y) + strlen(output[i]) + 1);
        strcat(y, output[i]);
        found = true;
        break;
      }
      i++;
    }
    
    if (!found) {
      y = realloc(y, strlen(y) + 2);
      strcat(y, (char[]){*s, '\0'});
    }
    
    s++;
  }
  
  
  return y;
}

static mpc_val_t* mpcf_unescape_new(mpc_val_t* x, char* input, char** output) {
  
  int i;
  bool found = false;
  char* s = x;
  char* y = calloc(1, 1);
  
  while (*s) {
    
    i = 0;
    found = false;
    
    while (output[i]) {
      if (((*s) == output[i][0]) && (*(s+1) == output[i][1])) {
        y = realloc(y, strlen(y) + 2);
        strcat(y, (char[]){ input[i], '\0' });
        found = true;
        break;
      }
      i++;
    }
      
    if (!found) {
      y = realloc(y, strlen(y) + 2);
      strcat(y, (char[]){ *s, '\0' });
    }
    
    s++;
  }
  
  return y;
  
}

mpc_val_t* mpcf_escape(mpc_val_t* x) {
  mpc_val_t* y = mpcf_escape_new(x, mpc_escape_input_c, mpc_escape_output_c);
  free(x);
  return y;
}

mpc_val_t* mpcf_unescape(mpc_val_t* x) {
  mpc_val_t* y = mpcf_unescape_new(x, mpc_escape_input_c, mpc_escape_output_c);
  free(x);
  return y;
}

mpc_val_t* mpcf_unescape_regex(mpc_val_t* x) {
  mpc_val_t* y = mpcf_unescape_new(x, mpc_escape_input_raw_re, mpc_escape_output_raw_re);
  free(x);
  return y;  
}

mpc_val_t* mpcf_fst(mpc_val_t* x, mpc_val_t* y) {
  return x;
}

mpc_val_t* mpcf_snd(mpc_val_t* x, mpc_val_t* y) {
  return y;
}

mpc_val_t* mpcf_fst_free(mpc_val_t* x, mpc_val_t* y) {
  free(y);
  return x;
}

mpc_val_t* mpcf_snd_free(mpc_val_t* x, mpc_val_t* y) {
  free(x);
  return y;
}

mpc_val_t* mpcf_freefold(mpc_val_t* t, mpc_val_t* x) {
  free(x);
  return NULL;
}

mpc_val_t* mpcf_strfold(mpc_val_t* t, mpc_val_t* x) {
  
  if (t == NULL) { return x; }
  if (x == NULL) { return t; }
  
  t = realloc(t, strlen(t) + strlen(x) + 1);
  strcat(t, x);
  free(x);
  
  return t;
}

mpc_val_t* mpcf_afst(int n, mpc_val_t** xs) { return xs[0]; }
mpc_val_t* mpcf_asnd(int n, mpc_val_t** xs) { return xs[1]; }
mpc_val_t* mpcf_atrd(int n, mpc_val_t** xs) { return xs[2]; }

mpc_val_t* mpcf_astrfold(int n, mpc_val_t** xs) {
  mpc_val_t* t = NULL;
  int i;
  for (i = 0; i < n; i++) {
    t = mpcf_strfold(t, xs[i]);
  }
  return t;
}

mpc_val_t* mpcf_between_free(int n, mpc_val_t** xs) {
  free(xs[0]);
  free(xs[2]);
  return xs[1];
}

mpc_val_t* mpcf_maths(int n, mpc_val_t** xs) {
  
  int** vs = (int**)xs;
    
  if (strcmp(xs[1], "*") == 0) { *vs[0] *= *vs[2]; }
  if (strcmp(xs[1], "/") == 0) { *vs[0] /= *vs[2]; }
  if (strcmp(xs[1], "%") == 0) { *vs[0] %= *vs[2]; }
  if (strcmp(xs[1], "+") == 0) { *vs[0] += *vs[2]; }
  if (strcmp(xs[1], "-") == 0) { *vs[0] -= *vs[2]; }
  
  free(xs[1]); free(xs[2]);
  
  return xs[0];
}

/*
** Printing
*/

static void mpc_print_unretained(mpc_parser_t* p, bool force) {
  
  if (p->retained && !force) {
    if (p->name) { printf("<%s>", p->name); }
    else { printf("<anon>"); }
    return;
  }
  
  if (p->type == MPC_TYPE_UNDEFINED) { printf("<undefined>"); }
  if (p->type == MPC_TYPE_PASS)   { printf("<pass>"); }
  if (p->type == MPC_TYPE_FAIL)   { printf("<fail>"); }
  if (p->type == MPC_TYPE_LIFT)   { printf("<lift>"); }
  if (p->type == MPC_TYPE_EXPECT) {
    printf("%s", p->data.expect.m);
    /*mpc_print_unretained(p->data.expect.x, false);*/
  }
  
  if (p->type == MPC_TYPE_SOI) { printf("<soi>"); }
  if (p->type == MPC_TYPE_EOI) { printf("<eoi>"); }
  
  if (p->type == MPC_TYPE_ANY) { printf("<any>"); }
  if (p->type == MPC_TYPE_SATISFY) { printf("<satisfy %p>", p->data.satisfy.f); }

  if (p->type == MPC_TYPE_SINGLE) {
    char* s = mpcf_escape_new(
      (char[]){ p->data.single.x, '\0' },
      mpc_escape_input_c,
      mpc_escape_output_c);
    printf("'%s'", s);
    free(s);
  }
  
  if (p->type == MPC_TYPE_RANGE) {
    char* s = mpcf_escape_new(
      (char[]){ p->data.range.x, '\0' },
      mpc_escape_input_c,
      mpc_escape_output_c);
    char* e = mpcf_escape_new(
      (char[]){ p->data.range.y, '\0' },
      mpc_escape_input_c,
      mpc_escape_output_c);
    printf("[%s-%s]", s, e);
    free(s);
    free(e);
  }
  
  if (p->type == MPC_TYPE_ONEOF) {
    char* s = mpcf_escape_new(
      p->data.string.x,
      mpc_escape_input_c,
      mpc_escape_output_c);
    printf("[%s]", s);
    free(s);
  }
  
  if (p->type == MPC_TYPE_NONEOF) {
    char* s = mpcf_escape_new(
      p->data.string.x,
      mpc_escape_input_c,
      mpc_escape_output_c);
    printf("[^%s]", s);
    free(s);
  }
  
  if (p->type == MPC_TYPE_STRING) {
    char* s = mpcf_escape_new(
      p->data.string.x,
      mpc_escape_input_c,
      mpc_escape_output_c);
    printf("\"%s\"", s);
    free(s);
  }
  
  if (p->type == MPC_TYPE_APPLY)    { mpc_print_unretained(p->data.apply.x, false); }
  if (p->type == MPC_TYPE_APPLY_TO) { mpc_print_unretained(p->data.apply_to.x, false); }
  if (p->type == MPC_TYPE_PREDICT)  { mpc_print_unretained(p->data.predict.x, false); }
  if (p->type == MPC_TYPE_NOT)   { printf("!"); mpc_print_unretained(p->data.not.x, false); }
  if (p->type == MPC_TYPE_MAYBE) { printf("("); mpc_print_unretained(p->data.repeat.x, false); printf(")?"); }
  if (p->type == MPC_TYPE_MANY)  { printf("("); mpc_print_unretained(p->data.repeat.x, false); printf(")*"); }
  if (p->type == MPC_TYPE_MANY1) { printf("("); mpc_print_unretained(p->data.repeat.x, false); printf(")+"); }
  if (p->type == MPC_TYPE_COUNT) { printf("("); mpc_print_unretained(p->data.repeat.x, false); printf("){%i}", p->data.repeat.n); }
  
  if (p->type == MPC_TYPE_ELSE) {
    printf("(");
    mpc_print_unretained(p->data.orelse.x, false);
    printf(" | ");
    mpc_print_unretained(p->data.orelse.y, false);
    printf(")");
  }
  
  if (p->type == MPC_TYPE_ALSO) {
    printf("(");
    mpc_print_unretained(p->data.also.x, false);
    printf(" ");
    mpc_print_unretained(p->data.also.y, false);
    printf(")");
   }
  
  if (p->type == MPC_TYPE_OR) {
    printf("(");
    int i;
    for(i = 0; i < p->data.or.n-1; i++) {
      mpc_print_unretained(p->data.or.xs[i], false);
      printf(" | ");
    }
    mpc_print_unretained(p->data.or.xs[p->data.or.n-1], false);
    printf(")");
  }
  
  if (p->type == MPC_TYPE_AND) {
    printf("(");
    int i;
    for(i = 0; i < p->data.and.n-1; i++) {
      mpc_print_unretained(p->data.and.xs[i], false);
      printf(" ");
    }
    mpc_print_unretained(p->data.and.xs[p->data.and.n-1], false);
    printf(")");
  }
  
}

void mpc_print(mpc_parser_t* p) {
  mpc_print_unretained(p, true);
  printf("\n");
}

/*
** Testing
*/


bool mpc_unmatch(mpc_parser_t* p, const char* s, void* d,
  bool(*tester)(void*, void*),
  mpc_dtor_t destructor,
  void(*printer)(void*)) {

  mpc_result_t r;  
  if (mpc_parse("<test>", s, p, &r)) {

    if (tester(r.output, d)) {
      destructor(r.output);
      return false;
    } else {
      destructor(r.output);
      return true;
    }
  
  } else {
    mpc_err_delete(r.error);
    return true;
  }
  
}

bool mpc_match(mpc_parser_t* p, const char* s, void* d,
  bool(*tester)(void*, void*), 
  mpc_dtor_t destructor, 
  void(*printer)(void*)) {

  mpc_result_t r;  
  if (mpc_parse("<test>", s, p, &r)) {
    
    if (tester(r.output, d)) {
      destructor(r.output);
      return true;
    } else {
      printf("Got "); printer(r.output); printf("\n");
      printf("Expected "); printer(d); printf("\n");
      destructor(r.output);
      return false;
    }
    
  } else {    
    mpc_err_print(r.error);
    mpc_err_delete(r.error);
    return false;
    
  }
  
}


/*
** AST
*/

void mpc_ast_delete(mpc_ast_t* a) {
  
  int i;
  for (i = 0; i < a->children_num; i++) {
    mpc_ast_delete(a->children[i]);
  }
  
  free(a->children);
  free(a->tag);
  free(a->contents);
  free(a);
  
}

static void mpc_ast_delete_no_children(mpc_ast_t* a) {
  free(a->children);
  free(a->tag);
  free(a->contents);
  free(a);
}

mpc_ast_t* mpc_ast_new(const char* tag, const char* contents) {
  
  mpc_ast_t* a = malloc(sizeof(mpc_ast_t));
  
  a->tag = malloc(strlen(tag) + 1);
  strcpy(a->tag, tag);
  
  a->contents = malloc(strlen(contents) + 1);
  strcpy(a->contents, contents);
  
  a->children_num = 0;
  a->children = NULL;
  return a;
  
}

mpc_ast_t* mpc_ast_build(int n, const char* tag, ...) {
  
  mpc_ast_t* a = mpc_ast_new(tag, "");
  
  va_list va;
  va_start(va, tag);
  
  int i;
  for (i = 0; i < n; i++) {
    mpc_ast_add_child(a, va_arg(va, mpc_ast_t*));
  }
  
  va_end(va);
  
  return a;
  
}

mpc_ast_t* mpc_ast_insert_root(mpc_ast_t* a) {

  if (a == NULL) { return a; }
  if (a->children_num == 0) { return a; }
  if (a->children_num == 1) { return a; }

  mpc_ast_t* r = mpc_ast_new("root", "");
  mpc_ast_add_child(r, a);
  return r;
}

bool mpc_ast_eq(mpc_ast_t* a, mpc_ast_t* b) {
  
  if (strcmp(a->tag, b->tag) != 0) { return false; }
  if (strcmp(a->contents, b->contents) != 0) { return false; }
  if (a->children_num != b->children_num) { return false; }
  
  int i;
  for (i = 0; i < a->children_num; i++) {
    if (!mpc_ast_eq(a->children[i], b->children[i])) { return false; }
  }
  
  return true;
}

void mpc_ast_add_child(mpc_ast_t* r, mpc_ast_t* a) {
  
  if (a == NULL || r == NULL) { return; }
  
  r->children_num++;
  r->children = realloc(r->children, sizeof(mpc_ast_t*) * r->children_num);
  r->children[r->children_num-1] = a;
  
}

void mpc_ast_tag(mpc_ast_t* a, const char* t) {
  a->tag = realloc(a->tag, strlen(t) + 1);
  strcpy(a->tag, t);
}

static void mpc_ast_print_depth(mpc_ast_t* a, int d) {
  
  int i;
  for (i = 0; i < d; i++) { printf("\t"); }
  
  if (strlen(a->contents)) {
    printf("%s: '%s'\n", a->tag, a->contents);
  } else {
    printf("%s:\n", a->tag);
  }
  
  
  for (i = 0; i < a->children_num; i++) {
    mpc_ast_print_depth(a->children[i], d+1);
  }
  
}

void mpc_ast_print(mpc_ast_t* a) {
  mpc_ast_print_depth(a, 0);
}

mpc_val_t* mpcf_fold_ast(mpc_val_t* a, mpc_val_t* b) {
  
  int i;
  mpc_ast_t* r = mpc_ast_new("", "");
  mpc_ast_t* x = a;
  mpc_ast_t* y = b;
  
  if (x && x->children_num > 0) {
    for (i = 0; i < x->children_num; i++) {
      mpc_ast_add_child(r, x->children[i]);
    }
    mpc_ast_delete_no_children(x);
  } else if (x && x->children_num == 0) { mpc_ast_add_child(r, x); }
  
  if (y && y->children_num > 0) {
    for (i = 0; i < y->children_num; i++) {
      mpc_ast_add_child(r, y->children[i]);
    }
    mpc_ast_delete_no_children(y);
  } else if (y && y->children_num == 0) { mpc_ast_add_child(r, y); }
  
  return r;
}

mpc_val_t* mpcf_afold_ast(int n, mpc_val_t** as) {
  
  mpc_val_t* t = NULL;
  
  int i;
  for (i = 0; i < n; i++) {
    mpcf_fold_ast(t, as[i]);
  }
  
  return t;  
}

mpc_val_t* mpcf_apply_str_ast(mpc_val_t* c) {
  mpc_ast_t* a = mpc_ast_new("", c);
  free(c);
  return a;
}

static mpc_val_t* mpcf_apply_tag(mpc_val_t* x, void* d) {
  mpc_ast_tag(x, d);
  return x;
}

mpc_parser_t* mpca_tag(mpc_parser_t* a, const char* t) {
  return mpc_apply_to(a, mpcf_apply_tag, (void*)t);
}

mpc_parser_t* mpca_not(mpc_parser_t* a) { return mpc_not(a, (mpc_dtor_t)mpc_ast_delete); }
mpc_parser_t* mpca_maybe(mpc_parser_t* a) { return mpc_maybe(a); }
mpc_parser_t* mpca_many(mpc_parser_t* a) { return mpc_many(a, mpcf_fold_ast); }
mpc_parser_t* mpca_many1(mpc_parser_t* a) { return mpc_many1(a, mpcf_fold_ast); }
mpc_parser_t* mpca_count(mpc_parser_t* a, int n) { return mpc_count(a, (mpc_dtor_t)mpc_ast_delete, mpcf_fold_ast, n); }
mpc_parser_t* mpca_else(mpc_parser_t* a, mpc_parser_t* b) { return mpc_else(a, b); }
mpc_parser_t* mpca_also(mpc_parser_t* a, mpc_parser_t* b) { return mpc_also(a, b, (mpc_dtor_t)mpc_ast_delete, mpcf_fold_ast); }
mpc_parser_t* mpca_bind(mpc_parser_t* a, mpc_parser_t* b) { return mpca_also(a, b); }

mpc_parser_t* mpca_or(int n, ...) {

  mpc_parser_t* p = mpc_undefined();
  
  p->type = MPC_TYPE_OR;
  p->data.or.n = n;
  p->data.or.xs = malloc(sizeof(mpc_parser_t*) * n);
  
  va_list va;
  va_start(va, n);  
  int i;
  for (i = 0; i < n; i++) {
    p->data.or.xs[i] = va_arg(va, mpc_parser_t*);
  }
  va_end(va);
  
  return p;
  
}

mpc_parser_t* mpca_and(int n, ...) {
  
  va_list va;
  va_start(va, n);
  
  mpc_parser_t* p = mpc_undefined();
  
  p->type = MPC_TYPE_AND;
  p->data.and.n = n;
  p->data.and.f = mpcf_afold_ast;
  p->data.and.xs = malloc(sizeof(mpc_parser_t*) * n);
  p->data.and.dxs = malloc(sizeof(mpc_dtor_t) * (n-1));
  
  int i;
  for (i = 0; i < n; i++) {
    p->data.and.xs[i] = va_arg(va, mpc_parser_t*);
  }
  for (i = 0; i < (n-1); i++) {
    p->data.and.dxs[i] = (mpc_dtor_t)mpc_ast_delete;
  }
    
  va_end(va);
  
  return p;  
}

mpc_parser_t* mpca_total(mpc_parser_t* a) { return mpc_total(a, (mpc_dtor_t)mpc_ast_delete); }

/*
** Grammar Parser
*/

/*
** This is another interesting bootstrapping.
**
** Having a general purpose AST type allows
** users to specify the grammar alone and
** let all fold rules be automatically taken
** care of by existing functions.
**
** You don't get to control the type spat
** out but this means you can make a nice
** parser to take in some grammar in nice
** syntax and spit out a parser that works.
**
** The grammar for this looks surprisingly
** like regex but the main difference is that
** it is now whitespace insensitive and the
** base type takes literals of some form.
*/

/*
**
**  ### Grammar Grammar
**
**      <grammar> : (<term> "|" <grammar>) | <term>
**     
**      <term> : <factor>*
**
**      <factor> : <base>
**               | <base> "*"
**               | <base> "+"
**               | <base> "?"
**               | <base> "{" <digits> "}"
**           
**      <base> : "<" (<digits> | <ident>) ">"
**             | <string_lit>
**             | <char_lit>
**             | <regex_lit>
**             | "(" <grammar> ")"
*/

static mpc_val_t* mpca_grammar_afold_or(int n, mpc_val_t** xs) {
  free(xs[1]);
  return mpca_else(xs[0], xs[2]);
}

static mpc_val_t* mpca_grammar_fold_many(mpc_val_t* x, mpc_val_t* y) {
  if (x == NULL) { return y; }
  if (y == NULL) { return x; }
  return mpca_also(x, y);
}

static mpc_val_t* mpca_grammar_lift(void) {
  return mpc_pass();
}

static mpc_val_t* mpca_grammar_fold_repeat(mpc_val_t* x, mpc_val_t* y) {
  if (strcmp(y, "*") == 0) { free(y); return mpca_many(x); }
  if (strcmp(y, "+") == 0) { free(y); return mpca_many1(x); }
  if (strcmp(y, "?") == 0) { free(y); return mpca_maybe(x); }
  int n = *((int*)y);
  free(y);
  return mpca_count(x, n);
}

static mpc_val_t* mpca_grammar_apply_string(mpc_val_t* x) {
  char* y = mpcf_unescape(x);
  mpc_parser_t* p = mpc_tok(mpc_string(y));
  free(y);
  return mpca_tag(mpc_apply(p, mpcf_apply_str_ast), "string");
}

static mpc_val_t* mpca_grammar_apply_char(mpc_val_t* x) {
  char* y = mpcf_unescape(x);
  mpc_parser_t* p = mpc_tok(mpc_char(y[0]));
  free(y);
  return mpca_tag(mpc_apply(p, mpcf_apply_str_ast), "char");
}

static mpc_val_t* mpca_grammar_apply_regex(mpc_val_t* x) {
  char* y = mpcf_unescape_regex(x);
  mpc_parser_t* p = mpc_tok(mpc_re(y));
  free(y);
  return mpca_tag(mpc_apply(p, mpcf_apply_str_ast), "regex");
}

typedef struct {
  va_list* va;
  int parsers_num;
  mpc_parser_t** parsers; 
} mpca_grammar_st_t;

static mpc_parser_t* mpca_grammar_find_parser(char* x, mpca_grammar_st_t* st) {
  
  /* Case of Number */
  if (strstr("0123456789", x)) {

    int i = strtol(x, NULL, 10);
    
    while (st->parsers_num <= i) {
      st->parsers_num++;
      st->parsers = realloc(st->parsers, sizeof(mpc_parser_t*) * st->parsers_num);
      st->parsers[st->parsers_num-1] = va_arg(*st->va, mpc_parser_t*);
      if (st->parsers[st->parsers_num-1] == NULL) {
        return mpc_failf("No Parser in position %i! Only supplied %i Parsers!", i, st->parsers_num);
      }
    }
    
    return st->parsers[st->parsers_num-1];
  
  /* Case of Identifier */
  } else {
    
    int i;
    
    /* Search Existing Parsers */
    for (i = 0; i < st->parsers_num; i++) {
      mpc_parser_t* p = st->parsers[i];
      if (p->name && strcmp(p->name, x) == 0) { return p; }
    }
    
    /* Search New Parsers */
    while (true) {
    
      mpc_parser_t* p = va_arg(*st->va, mpc_parser_t*);
      
      st->parsers_num++;
      st->parsers = realloc(st->parsers, sizeof(mpc_parser_t*) * st->parsers_num);
      st->parsers[st->parsers_num-1] = p;
      
      if (p == NULL) {
        return mpc_failf("Unknown Parser '%s'!", x);
      }
      
      if (p->name && strcmp(p->name, x) == 0) { return p; }
      
    }
  
  }  
  
}

static mpc_val_t* mpca_grammar_apply_id(mpc_val_t* x, void* y) {
  
  mpc_parser_t* p = mpca_grammar_find_parser(x, y);
  
  free(x);
  
  if (p->name) {
    return mpc_apply(mpca_tag(p, p->name), (mpc_apply_t)mpc_ast_insert_root);
  } else {
    return mpc_apply(p, (mpc_apply_t)mpc_ast_insert_root);
  }

}

static mpc_val_t* mpcf_make_root(mpc_val_t* x) {
  return mpca_tag(x, "root");
}

mpc_parser_t* mpca_grammar_st(const char* grammar, mpca_grammar_st_t* st) {
  
  mpc_parser_t* Grammar = mpc_new("grammar");
  mpc_parser_t* Term = mpc_new("term");
  mpc_parser_t* Factor = mpc_new("factor");
  mpc_parser_t* Base = mpc_new("base");
  
  mpc_define(Grammar, mpc_else(
    mpc_and(3, mpca_grammar_afold_or, Term, mpc_sym("|"), Grammar, mpc_soft_delete, free),
    Term
  ));
  
  mpc_define(Term, mpc_many_else(Factor, mpca_grammar_fold_many, mpca_grammar_lift));
  
  mpc_define(Factor, mpc_or(5,
    mpc_also(Base, mpc_sym("*"), mpc_soft_delete, mpca_grammar_fold_repeat),
    mpc_also(Base, mpc_sym("+"), mpc_soft_delete, mpca_grammar_fold_repeat),
    mpc_also(Base, mpc_sym("?"), mpc_soft_delete, mpca_grammar_fold_repeat),
    mpc_also(Base, mpc_tok_brackets(mpc_tok(mpc_int()), free), mpc_soft_delete, mpca_grammar_fold_repeat),
    Base
  ));
  
  mpc_define(Base, mpc_or(5,
    mpc_apply(mpc_tok(mpc_string_lit()), mpca_grammar_apply_string),
    mpc_apply(mpc_tok(mpc_char_lit()),   mpca_grammar_apply_char),
    mpc_apply(mpc_tok(mpc_regex_lit()),  mpca_grammar_apply_regex),
    mpc_apply_to(mpc_tok_braces(mpc_tok(mpc_else(mpc_digits(), mpc_ident())), free), mpca_grammar_apply_id, st),
    mpc_tok_parens(Grammar, mpc_soft_delete)
  ));
  
  mpc_parser_t* GrammarTotal = mpc_apply(mpc_total(Grammar, mpc_soft_delete), mpcf_make_root);
  
  mpc_result_t r;
  if(!mpc_parse("<mpc_grammar_compiler>", grammar, GrammarTotal, &r)) {
    char* err_msg = mpc_err_string_new(r.error);
    r.output = mpc_failf("Invalid Grammar: %s", err_msg);
    free(err_msg);
    mpc_err_delete(r.error);
  }
  
  mpc_delete(GrammarTotal);
  mpc_cleanup(4, Grammar, Term, Factor, Base);
  
  return r.output;
  
}

mpc_parser_t* mpca_grammar(const char* grammar, ...) {
  va_list va;
  va_start(va, grammar);
  mpca_grammar_st_t st = { &va, 0, NULL };
  mpc_parser_t* res = mpca_grammar_st(grammar, &st);  
  free(st.parsers);
  va_end(va);
  return res;
}

typedef struct {
  char* ident;
  mpc_parser_t* grammar;
} mpca_stmt_t;

static mpc_val_t* mpca_stmt_afold(int n, mpc_val_t** xs) {
  
  mpca_stmt_t* stmt = malloc(sizeof(mpca_stmt_t));
  stmt->ident = ((char**)xs)[0];
  stmt->grammar = ((mpc_parser_t**)xs)[2];
  
  free(((char**)xs)[1]);
  free(((char**)xs)[3]);
  
  return stmt;
}

static int mpca_stmt_list_count(mpc_val_t* x) {
  int i = 0;
  mpca_stmt_t** stmts = x;
  while(*stmts) { i++; stmts++;  }
  return i;
}

static mpc_val_t* mpca_stmt_fold(mpc_val_t* t, mpc_val_t* x) {
  
  mpca_stmt_t** stmts = t;

  if (stmts == NULL) {
    stmts = calloc(1, sizeof(mpca_stmt_t*));
  }
  
  int count = mpca_stmt_list_count(stmts);
  stmts = realloc(stmts, (count + 2) * sizeof(mpca_stmt_t*));
  stmts[count] = x;
  stmts[count+1] = NULL;
  
  return stmts;
  
}

static void mpca_stmt_list_delete(mpc_val_t* x) {

  mpca_stmt_t** stmts = x;

  while(*stmts) {
    mpca_stmt_t* stmt = *stmts; 
    free(stmt->ident);
    mpc_soft_delete(stmt->grammar);
    free(stmt);  
    stmts++;
  }
  free(x);

}

static mpc_val_t* mpca_stmt_list_apply_to(mpc_val_t* x, void* st) {

  mpca_stmt_t** stmts = x;

  while(*stmts) {
    mpca_stmt_t* stmt = *stmts;
    mpc_parser_t* left = mpca_grammar_find_parser(stmt->ident, st);
    mpc_define(left, stmt->grammar);
    free(stmt->ident);
    free(stmt);
    stmts++;
  }
  free(x);
  
  return NULL;
}

static mpc_err_t* mpca_lang_st(const char* language, mpca_grammar_st_t* st) {
  
  mpc_parser_t* Lang = mpc_new("lang");
  mpc_parser_t* Stmt = mpc_new("stmt");
  mpc_parser_t* Grammar = mpc_new("grammar");
  mpc_parser_t* Term = mpc_new("term");
  mpc_parser_t* Factor = mpc_new("factor");
  mpc_parser_t* Base = mpc_new("base");
  
  mpc_define(Grammar, mpc_else(
    mpc_and(3, mpca_grammar_afold_or, Term, mpc_sym("|"), Grammar, mpc_soft_delete, free),
    Term
  ));
  
  mpc_define(Term, mpc_many_else(Factor, mpca_grammar_fold_many, mpca_grammar_lift));
  
  mpc_define(Factor, mpc_or(5,
    mpc_also(Base, mpc_sym("*"), mpc_soft_delete, mpca_grammar_fold_repeat),
    mpc_also(Base, mpc_sym("+"), mpc_soft_delete, mpca_grammar_fold_repeat),
    mpc_also(Base, mpc_sym("?"), mpc_soft_delete, mpca_grammar_fold_repeat),
    mpc_also(Base, mpc_tok_brackets(mpc_tok(mpc_int()), free), mpc_soft_delete, mpca_grammar_fold_repeat),
    Base
  ));
  
  mpc_define(Base, mpc_predict(mpc_or(5,
    mpc_apply(mpc_tok(mpc_string_lit()), mpca_grammar_apply_string),
    mpc_apply(mpc_tok(mpc_char_lit()),   mpca_grammar_apply_char),
    mpc_apply(mpc_tok(mpc_regex_lit()),  mpca_grammar_apply_regex),
    mpc_apply_to(mpc_tok_braces(mpc_tok(mpc_else(mpc_digits(), mpc_ident())), free), mpca_grammar_apply_id, st),
    mpc_tok_parens(Grammar, mpc_soft_delete)
  )));
  
  mpc_define(Lang, mpc_apply_to(
    mpc_total(mpc_many(Stmt, mpca_stmt_fold), mpca_stmt_list_delete),
    mpca_stmt_list_apply_to,
    st
  ));
  
  mpc_define(Stmt, mpc_and(4,
    mpca_stmt_afold,
    mpc_tok(mpc_ident()), mpc_sym(":"), Grammar, mpc_sym(";"),
    free, free, mpc_soft_delete
  ));
  
  mpc_result_t r = {0};
  mpc_parse("<mpc_lang_compiler>", language, Lang, &r);
  
  mpc_cleanup(6, Lang, Stmt, Grammar, Term, Factor, Base);
  
  return r.error;
}

mpc_err_t* mpca_lang(const char* language, ...) {
  va_list va;
  va_start(va, language);
  mpca_grammar_st_t st = { (va_list*)&va, 0, NULL };
  mpc_err_t* err = mpca_lang_st(language, &st);
  free(st.parsers);
  va_end(va);
  return err;
}

mpc_err_t* mpca_lang_file(const char* filename, ...) {
  
  FILE* f = fopen(filename, "r");
  
  if (f == NULL) {
    return mpc_err_new_fail(filename, mpc_state_null(), "Unable to open file!");
  }
  
  fseek(f, 0, SEEK_END);
  int len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buff = malloc(len + 1);
  fread(buff, 1, len, f);
  buff[len] = '\0';
  fclose(f);
  
  va_list va;
  va_start(va, filename);
  mpca_grammar_st_t st = { (va_list*)&va, 0, NULL };
  mpc_err_t* err = mpca_lang_st(buff, &st);
  free(st.parsers);
  va_end(va);  
  
  free(buff);
  
  return err;
}
