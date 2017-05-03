/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "oc_api.h"

static int
app_init(void)
{
  int ret = oc_init_platform("Apple", NULL, NULL);
  ret |= oc_add_device("/oic/d", "oic.d.phone", "Kishen's IPhone", "1.0", "1.0",
                       NULL, NULL);
  return ret;
}

#define MAX_URI_LENGTH (30)
static char a_light[MAX_URI_LENGTH];
static oc_server_handle_t light_server;

static bool state;
static int power;
static oc_string_t name;

static oc_event_callback_retval_t
stop_observe(void *data)
{
  (void)data;
  PRINT("Stopping OBSERVE\n");
  oc_stop_observe(a_light, &light_server);
  return DONE;
}

static void
observe_light(oc_client_response_t *data)
{
  PRINT("OBSERVE_light:\n");
  oc_rep_t *rep = data->payload;
  while (rep != NULL) {
    PRINT("key %s, value ", oc_string(rep->name));
    switch (rep->type) {
    case BOOL:
      PRINT("%d\n", rep->value.boolean);
      state = rep->value.boolean;
      break;
    case INT:
      PRINT("%d\n", rep->value.integer);
      power = rep->value.integer;
      break;
    case STRING:
      PRINT("%s\n", oc_string(rep->value.string));
      if (oc_string_len(name))
        oc_free_string(&name);
      oc_new_string(&name, oc_string(rep->value.string),
                    oc_string_len(rep->value.string));
      break;
    default:
      break;
    }
    rep = rep->next;
  }
}

static void
post2_light(oc_client_response_t *data)
{
  PRINT("POST2_light:\n");
  if (data->code == OC_STATUS_CHANGED)
    PRINT("POST response: CHANGED\n");
  else if (data->code == OC_STATUS_CREATED)
    PRINT("POST response: CREATED\n");
  else
    PRINT("POST response code %d\n", data->code);

  oc_do_observe(a_light, &light_server, NULL, &observe_light, LOW_QOS, NULL);
  oc_set_delayed_callback(NULL, &stop_observe, 30);
  PRINT("Sent OBSERVE request\n");
}

static void
post_light(oc_client_response_t *data)
{
  PRINT("POST_light:\n");
  if (data->code == OC_STATUS_CHANGED)
    PRINT("POST response: CHANGED\n");
  else if (data->code == OC_STATUS_CREATED)
    PRINT("POST response: CREATED\n");
  else
    PRINT("POST response code %d\n", data->code);

  if (oc_init_post(a_light, &light_server, NULL, &post2_light, LOW_QOS, NULL)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, true);
    oc_rep_set_int(root, power, 55);
    oc_rep_end_root_object();
    if (oc_do_post())
      PRINT("Sent POST request\n");
    else
      PRINT("Could not send POST request\n");
  } else
    PRINT("Could not init POST request\n");
}

static void
put_light(oc_client_response_t *data)
{
  PRINT("PUT_light:\n");

  if (data->code == OC_STATUS_CHANGED)
    PRINT("PUT response: CHANGED\n");
  else
    PRINT("PUT response code %d\n", data->code);

  if (oc_init_post(a_light, &light_server, NULL, &post_light, LOW_QOS, NULL)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, false);
    oc_rep_set_int(root, power, 105);
    oc_rep_end_root_object();
    if (oc_do_post())
      PRINT("Sent POST request\n");
    else
      PRINT("Could not send POST request\n");
  } else
    PRINT("Could not init POST request\n");
}

static void
get_light(oc_client_response_t *data)
{
  PRINT("GET_light:\n");
  oc_rep_t *rep = data->payload;
  while (rep != NULL) {
    PRINT("key %s, value ", oc_string(rep->name));
    switch (rep->type) {
    case BOOL:
      PRINT("%d\n", rep->value.boolean);
      state = rep->value.boolean;
      break;
    case INT:
      PRINT("%d\n", rep->value.integer);
      power = rep->value.integer;
      break;
    case STRING:
      PRINT("%s\n", oc_string(rep->value.string));
      if (oc_string_len(name))
        oc_free_string(&name);
      oc_new_string(&name, oc_string(rep->value.string),
                    oc_string_len(rep->value.string));
      break;
    default:
      break;
    }
    rep = rep->next;
  }

  if (oc_init_put(a_light, &light_server, NULL, &put_light, LOW_QOS, NULL)) {
    oc_rep_start_root_object();
    oc_rep_set_boolean(root, state, true);
    oc_rep_set_int(root, power, 15);
    oc_rep_end_root_object();

    if (oc_do_put())
      PRINT("Sent PUT request\n");
    else
      PRINT("Could not send PUT request\n");
  } else
    PRINT("Could not init PUT request\n");
}

static oc_discovery_flags_t
discovery(const char *di, const char *uri, oc_string_array_t types,
          oc_interface_mask_t interfaces, oc_server_handle_t *server,
          void *user_data)
{
  (void)di;
  (void)user_data;
  (void)interfaces;
  int i;
  int uri_len = strlen(uri);
  uri_len = (uri_len >= MAX_URI_LENGTH) ? MAX_URI_LENGTH - 1 : uri_len;

  for (i = 0; i < (int)oc_string_array_get_allocated_size(types); i++) {
    char *t = oc_string_array_get_item(types, i);
    if (strlen(t) == 10 && strncmp(t, "core.light", 10) == 0) {
      memcpy(&light_server, server, sizeof(oc_server_handle_t));

      strncpy(a_light, uri, uri_len);
      a_light[uri_len] = '\0';

      oc_do_get(a_light, &light_server, NULL, &get_light, LOW_QOS, NULL);
      PRINT("Sent GET request\n");

      return OC_STOP_DISCOVERY;
    }
  }
  return OC_CONTINUE_DISCOVERY;
}

static void
issue_requests(void)
{
  oc_do_ip_discovery("core.light", &discovery, NULL);
}

#if defined(CONFIG_MICROKERNEL) || defined(CONFIG_NANOKERNEL) /* Zephyr */

#include <sections.h>
#include <string.h>
#include <zephyr.h>

static struct nano_sem block;

static void
signal_event_loop(void)
{
  nano_sem_give(&block);
}

void
main(void)
{
  static const oc_handler_t handler = {.init = app_init,
                                       .signal_event_loop = signal_event_loop,
                                       .requests_entry = issue_requests };

  nano_sem_init(&block);
  PRINT("oc_main: simple client\n"); 
  if (oc_main_init(&handler) < 0)
    return;

  oc_clock_time_t next_event;

  while (true) {
    next_event = oc_main_poll();
    if (next_event == 0)
      next_event = TICKS_UNLIMITED;
    else
      next_event -= oc_clock_time();
    nano_task_sem_take(&block, next_event);
  }

  oc_main_shutdown();
}

#endif /* Zephyr */

#include "port/oc_clock.h"
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include "los_mux.h"
#include "los_api_mutex.h"
#include "los_event.h"

EVENT_CB_S  handle_event;
#define event_wait 0x00000001
struct timespec
{
  time_t tv_sec; /*second*/
  long tv_nsec;  /*nanosecond*/
};

static UINT32 mutex;
//pthread_cond_t cv;
struct timespec ts;

int quit = 0;

static void
signal_event_loop(void)
{
  LOS_MuxPend(mutex, LOS_WAIT_FOREVER);
//  pthread_cond_signal(&cv);
	
	UINT32 uwRet;
	uwRet = LOS_EventWrite(&handle_event, event_wait);
	if(uwRet != LOS_OK)
	{
			PRINT("event write failed .\n");
	}
  LOS_MuxPost(mutex);
}

void
handle_signal(int signal)
{
  (void)signal;
  signal_event_loop();
  quit = 1;
}

int
simpleclientstart(void)
{

  int init;
	UINT32 uwRet;
	uwRet = LOS_EventInit(&handle_event);
	if(uwRet != LOS_OK)
	{
			PRINT("init event failed .\n");
			return LOS_NOK;
	}
/*
	struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_signal;
  sigaction(SIGINT, &sa, NULL);
*/
  static const oc_handler_t handler = {.init = app_init,
                                       .signal_event_loop = signal_event_loop,
                                       .requests_entry = issue_requests };

  oc_clock_time_t next_event;

#ifdef OC_SECURITY
  oc_storage_config("./creds");
#endif /* OC_SECURITY */
  PRINT("oc_main: simple client111\n"); 
  init = oc_main_init(&handler);
  if (init < 0)
    return init;

  while (quit != 1) {
    next_event = oc_main_poll();
    LOS_MuxPend(mutex, LOS_WAIT_FOREVER);
    if (next_event == 0) {
      //pthread_cond_wait(&cv, &mutex);
			LOS_EventRead(&handle_event, event_wait, LOS_WAITMODE_AND, LOS_WAIT_FOREVER);
			
    } else {
			next_event -= oc_clock_time();
			LOS_EventRead(&handle_event, event_wait, LOS_WAITMODE_AND, next_event);
//			LOS_EventRead(&handle_event, event_wait, LOS_WAITMODE_AND, 10000);
      //tss.tv_sec = (next_event / OC_CLOCK_SECOND);
      //tss.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
      //pthread_cond_timedwait(&cv, &mutex, tss);
    }
    LOS_MuxPost(mutex);
  }

  oc_main_shutdown();
  return 0;
}
