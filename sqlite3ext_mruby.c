#include "mruby.h"
#include "mruby/proc.h"
#include "mruby/array.h"
#include "mruby/string.h"
#include "mruby/compile.h"
#include "mruby/dump.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3ext.h>

SQLITE_EXTENSION_INIT1;

static mrb_value
sv2mv(sqlite3_value *sv, mrb_state *mrb)
{
    switch (sqlite3_value_type(sv)) {
        case SQLITE_INTEGER:
              return mrb_fixnum_value(sqlite3_value_int64(sv));
        case SQLITE_FLOAT:
              return mrb_float_value(sqlite3_value_double(sv));
        case SQLITE_TEXT:
        case SQLITE_BLOB:
              return mrb_str_new(mrb, (const char*) sqlite3_value_text(sv),
                      sqlite3_value_bytes(sv));
    }
    return mrb_nil_value();
}

static void
result_exception(sqlite3_context *ctx, mrb_state *mrb)
{
    mrb_value e = mrb_obj_value(mrb->exc);
    mrb_value msg = mrb_funcall(mrb, e, "to_s", 0);
    sqlite3_result_error(ctx, RSTRING_PTR(msg), RSTRING_LEN(msg));
    mrb->exc = 0;
}

static void
result_value(sqlite3_context *ctx, mrb_state *mrb, mrb_value v)
{
    if (mrb->exc) {
        result_exception(ctx, mrb);
        return;
    }
    if (mrb_nil_p(v)) {
        sqlite3_result_null(ctx);
        return;
    }
    switch (mrb_type(v)) {
        case MRB_TT_FALSE:
            sqlite3_result_int(ctx, 0);
            break;
        case MRB_TT_TRUE:
            sqlite3_result_int(ctx, 1);
            break;
        case MRB_TT_UNDEF:
            sqlite3_result_null(ctx);
            break;
        case MRB_TT_FIXNUM:
            sqlite3_result_int64(ctx, mrb_fixnum(v));
            break;
        case MRB_TT_FLOAT:
            sqlite3_result_double(ctx, mrb_float(v));
            break;
        default:
            if (mrb_type(v) != MRB_TT_STRING) {
                v = mrb_funcall(mrb, v, "to_s", 0);
            }
            sqlite3_result_text(ctx, RSTRING_PTR(v), RSTRING_LEN(v), NULL);
            break;
    }
}

static void
load(sqlite3_context *ctx, int argc, sqlite3_value *argv[], int rb)
{
    mrb_state *mrb = (mrb_state *) sqlite3_user_data(ctx);
    const char *filename = (const char*) sqlite3_value_text(argv[0]);
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        sqlite3_result_error(ctx, "no such file to load", -1);
        return;
    }
    if (rb) {
        int n = mrb_load_irep(mrb, fp);
        if (n > 0)
            mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_top_self(mrb));
    } else {
        mrb_load_file(mrb, fp);
    }
    fclose(fp);
    result_value(ctx, mrb, mrb_nil_value());
}

static void
f_mrb_load(sqlite3_context *ctx, int argc, sqlite3_value *argv[]) {
    load(ctx, argc, argv, 0);
}

static void
f_mrb_load_irep(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    load(ctx, argc, argv, 1);
}

static void
define_argv(mrb_state *mrb, int argc, sqlite3_value **argv)
{
    int i;
    mrb_value cargv = mrb_ary_new(mrb);
    for (i = 1; i < argc; i++) {
        mrb_ary_push(mrb, cargv, sv2mv(argv[i], mrb));
    }
    mrb_define_global_const(mrb, "ARGV", cargv);
}

static void
f_mrb_eval(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    if (argc > 0) {
        mrb_state *mrb = (mrb_state *) sqlite3_user_data(ctx);
        const char *src = (const char *) sqlite3_value_text(argv[0]);
        int src_len = sqlite3_value_bytes(argv[0]);
        mrb_value res;
        define_argv(mrb, argc, argv);
        res = mrb_load_nstring(mrb, src, src_len);
        result_value(ctx, mrb, res);
    } else {
        sqlite3_result_error(ctx, "wrong number of arguments to function mrb_eval()", -1);
    }
}

struct func_data {
    mrb_state *mrb;
    mrb_value blk;
};

static void
f_ruby_function(sqlite3_context *ctx, int argc, sqlite3_value *argv[])
{
    struct func_data *fd = sqlite3_user_data(ctx);
    mrb_state *mrb = fd->mrb;
    mrb_value res;
    if (argc > 0) {
        int i;
        mrb_value ary = mrb_ary_new_capa(mrb, argc);
        for (i = 0; i < argc; i++) {
            mrb_ary_push(mrb, ary, sv2mv(argv[i], mrb));
        }
        res = mrb_funcall_argv(mrb, fd->blk, "call", RARRAY_LEN(ary), RARRAY_PTR(ary));
    } else {
        res = mrb_funcall(mrb, fd->blk, "call", 0);
    }
    result_value(ctx, mrb, res);
}

static mrb_value
kernel_create_function(mrb_state *mrb, mrb_value self)
{
    sqlite3 *db = mrb->ud;
    mrb_value name;
    mrb_value blk;
    struct func_data *fd = malloc(sizeof(struct func_data));
    int rc;
    mrb_get_args(mrb, "o&", &name, &blk);
    fd->mrb = mrb;
    fd->blk = blk;
    name = mrb_funcall(mrb, name, "to_s", 0);
    rc = sqlite3_create_function_v2(db, RSTRING_PTR(name), -1,
            SQLITE_UTF8, fd, f_ruby_function, NULL, NULL, free);
    if (rc != SQLITE_OK) {
        mrb_raise(mrb, E_RUNTIME_ERROR, "Can't create the function %s", name);
        return mrb_nil_value(); /* not reached */
    }
    return blk;
}

static void
destroy_mrb(void *p)
{
    mrb_close((mrb_state *) p);
}

int
sqlite3_extension_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
    SQLITE_EXTENSION_INIT2(pApi);
    mrb_state *mrb = mrb_open();
    int rc;
    mrb->ud = db;
    mrb_define_module_function(mrb, mrb->kernel_module, "create_function",
            kernel_create_function, ARGS_REQ(1));
    rc = sqlite3_create_function_v2(db, "mrb_eval", -1, SQLITE_UTF8,
            mrb, f_mrb_eval, NULL, NULL, destroy_mrb);
    if (rc != SQLITE_OK) {
        *pzErrMsg = sqlite3_mprintf("Can't create mrb_eval.");
        return 1;
    }
    rc = sqlite3_create_function_v2(db, "mrb_load", 1, SQLITE_UTF8,
            mrb, f_mrb_load, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        *pzErrMsg = sqlite3_mprintf("Can't create mrb_load.");
        return 1;
    }
    rc = sqlite3_create_function_v2(db, "mrb_load_irep", 1, SQLITE_UTF8,
            mrb, f_mrb_load_irep, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        *pzErrMsg = sqlite3_mprintf("Can't create mrb_load_mrb.");
        return 1;
    }
    return 0;
}
