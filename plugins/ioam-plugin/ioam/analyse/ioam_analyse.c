/*
 * Copyright (c) 2016 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ioam/analyse/ioam_analyse.h>
#include <ioam/export/ioam_export.h>
#include <ioam/analyse/ip6_ioam_analyse_node.h>
#include <ioam/analyse/ioam_summary_export.h>
#include <vnet/ip/ip.h>
#include <vnet/ip/udp.h>

static clib_error_t *
set_ioam_analyse_command_fn (vlib_main_t *vm, unformat_input_t *input,
                             vlib_cli_command_t *cmd)
{
  //int rv;
  int is_export = 0;
  int is_add = 1;
  ip4_address_t collector, src;
  int remote_listen = 0;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat(input, "export-ipfix-collector %U src %U",
                    unformat_ip4_address, &collector, unformat_ip4_address, &src))
        is_export = 1;
      else if (unformat(input, "disable"))
        is_add = 0;
      else if (unformat(input, "listen-ipfix"))
        remote_listen = 1;
      else
        break;
    }

  if (is_add)
    {
      if (is_export)
        (void) ioam_flow_create (collector, src, 0);

      ioam_export_set_next_node((u8 *) "ip6-hbh-analyse");
      ip6_ioam_analyse_register_handlers();
      if (remote_listen)
        udp_register_dst_port (vm, UDP_DST_PORT_ipfix,
                               analyse_node_remote.index, 1 /* is_ip4 */ );
    }
  else
    {
      (void) ioam_flow_create (collector, src, 1);

      //memset((void *) &ioam_analyser_main, 0, sizeof(ioam_analyser_main));
      ioam_export_reset_next_node();
      ip6_ioam_analyse_unregister_handlers();
    }
  return 0;
}

VLIB_CLI_COMMAND (set_ioam_analyse_command, static) = {
    .path = "set ioam analyse",
    .short_help = "set ioam analyse [export-ipfix-collector] [disable] [listen-ipfix]",
    .function = set_ioam_analyse_command_fn,
};

static clib_error_t *
show_ioam_analyse_cmd_fn (vlib_main_t * vm, unformat_input_t * input,
                          vlib_cli_command_t * cmd)
{
  ip6_ioam_analyser_main_t *am = &ioam_analyser_main;
  ioam_analyser_data_t *record = NULL;
  u8 i;
  u8 *s = 0;

  vec_reset_length(s);
  s = format (0, "iOAM Analyse Information: \n");
  vec_foreach_index(i, am->data)
  {
    record = am->data + i;
    if (record->is_free)
      continue;

    s = format (s, "Flow Number: %u\n", i);
    s = print_analyse_flow(s, record);
    s = format(s, "\n");
  }
  vlib_cli_output (vm, "%v", s);

  vec_free(s);

  return 0;
}

VLIB_CLI_COMMAND (ip6_show_ioam_ipfix_cmd, static) =
{
  .path = "show ioam analyse ",
  .short_help = "show ioam analyser information",
  .function = show_ioam_analyse_cmd_fn,
};

static clib_error_t *
ioam_analyse_init (vlib_main_t *vm)
{
  clib_error_t *error;
  ip6_ioam_analyser_main_t *am = &ioam_analyser_main;
  u16 i;

  error = ioam_flow_report_init(vm);
  if (error)
    return error;

  error = ip6_ioam_analyse_init(vm);
  if (error)
    return error;

  vec_validate_aligned(am->data,
                       200,
                       CLIB_CACHE_LINE_BYTES);

  vec_foreach_index(i, am->data)
  {
    ioam_analyse_init_data(am->data + i);
  }

  return 0;
}

VLIB_INIT_FUNCTION(ioam_analyse_init);
