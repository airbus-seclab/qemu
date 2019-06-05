/*
 * QEMU American Fuzzy Lop board
 * configuration
 *
 * Copyright (c) 2019 S. Duverger Airbus
 * GPLv2
 */
#include "qemu/afl.h"

#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qjson.h"
#include "qapi/qmp/qnum.h"
#include "qapi/qmp/qnull.h"
#include "qapi/qmp/qstring.h"
#include "qemu/cutils.h"
#include "qemu/option.h"
#include "qemu/option_int.h"
#include "qemu/config-file.h"

static QDict *parse_json(const char *filename)
{
   QIOChannel  *ioc;
   QEMUFile    *f;
   QObject     *obj;
   QDict       *dict;
   uint8_t     *data;
   struct stat st;

   if (stat(filename, &st) < 0) {
      error_report("stat(json) failed");
      return NULL;
   }

   ioc = QIO_CHANNEL(qio_channel_file_new_path(filename, O_RDONLY, 0, NULL));
   f   = qemu_fopen_channel_input(ioc);

   if (!f) {
      error_report("qemu_fopen failed");
      return NULL;
   }

   object_unref(OBJECT(ioc));

   data = g_malloc(st.st_size);
   qemu_get_buffer(f, data, st.st_size);
   qemu_fclose(f);

   obj = qobject_from_json((const char*)data, NULL);
   if (!obj) {
      error_report("Could not parse JSON: ");
      return NULL;
   }

   dict = qobject_to(QDict, obj);
   if (!dict) {
      qobject_unref(obj);
      error_report("Invalid JSON object given");
      return NULL;
   }

   return dict;
}

#define __afl_obj_to_conf(field, dict, key, type, cast, getter) \
   ({                                                           \
      QObject *obj = qdict_get(dict, key);                      \
                                                                \
      if (obj == NULL) {                                        \
         printf("afl conf qdict get failed\n");                 \
         exit(-1);                                              \
      }                                                         \
                                                                \
      if (qobject_type(obj) != type) {                          \
         printf("afl conf bad type for: \'"key"\'\n");          \
         exit(-1);                                              \
      }                                                         \
                                                                \
      field = getter(qobject_to(cast, obj));                    \
   })

static void afl_map_obj_conf(afl_t *afl, QDict *json)
{
   __afl_obj_to_conf(afl->config.qemu.timeout, json,"user-timeout",
                     QTYPE_QNUM, QNum, qnum_get_int);
   __afl_obj_to_conf(afl->config.qemu.overhead, json,"qemu-overhead",
                     QTYPE_QNUM, QNum, qnum_get_int);
   __afl_obj_to_conf(afl->config.qemu.vms_tpl, json,"vm-state-template",
                     QTYPE_QSTRING, QString, qstring_get_str);

   __afl_obj_to_conf(afl->config.afl.ctl_fd, json,"afl-control-fd",
                     QTYPE_QNUM, QNum, qnum_get_int);
   __afl_obj_to_conf(afl->config.afl.sts_fd, json,"afl-status-fd",
                     QTYPE_QNUM, QNum, qnum_get_int);
   __afl_obj_to_conf(afl->config.afl.trace_size, json,"afl-trace-size",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.afl.trace_addr, json,"afl-trace-addr",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.afl.trace_env, json,"afl-trace-env",
                     QTYPE_QSTRING, QString, qstring_get_str);
   __afl_obj_to_conf(afl->config.tgt.part_base, json,"vm-part-base",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.part_size, json,"vm-part-size",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.part_kstack, json,"vm-part-kstack",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.part_kstack_size,json,"vm-part-kstack-size",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.nop_size, json,"vm-nop-size",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.part_off, json,"vm-part-off",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.fuzz_inj, json,"vm-fuzz-inj",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.fuzz_ep, json,"vm-fuzz-ep",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.fuzz_ep_next, json,"vm-fuzz-ep-next",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.size, json,"vm-size",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.panic, json,"vm-panic",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.cswitch, json,"vm-cswitch",
                     QTYPE_QNUM, QNum, qnum_get_uint);
   __afl_obj_to_conf(afl->config.tgt.cswitch_next, json,"vm-cswitch-next",
                     QTYPE_QNUM, QNum, qnum_get_uint);
}

void afl_init_conf(afl_t *afl)
{
   const char *fname;
   QDict      *json;

   fname = qemu_opt_get(qemu_find_opts_singleton("gustave"), "gustave");
   if (!fname) {
      error_report("need GUSTAVE configuration file");
      exit(EXIT_FAILURE);
   }

   json = parse_json(fname);
   if (!json) {
      error_report("can't parse GUSTAVE json file");
      exit(EXIT_FAILURE);
   }

   afl_map_obj_conf(afl, json);

   /* XXX: should unref object, but copy retrieved strings first */
   /* qobject_unref(json); */
}
