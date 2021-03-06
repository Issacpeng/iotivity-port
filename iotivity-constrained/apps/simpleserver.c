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

static bool state = false;
int power;
oc_string_t name;

static int
app_init(void)
{
  int ret = oc_init_platform("Intel", NULL, NULL);
  ret |=
    oc_add_device("/oic/d", "oic.d.light", "Lamp", "1.0", "1.0", NULL, NULL);
  oc_new_string(&name, "John's Light", 12);
  return ret;
}

static void
get_light(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)user_data;
  ++power;

  PRINT("GET_light:\n");
  oc_rep_start_root_object();
  switch (interface) {
  case OC_IF_BASELINE:
    oc_process_baseline_interface(request->resource);
  case OC_IF_RW:
    oc_rep_set_boolean(root, state, state);
    oc_rep_set_int(root, power, power);
    oc_rep_set_text_string(root, name, oc_string(name));
    break;
  default:
    break;
  }
  oc_rep_end_root_object();
  oc_send_response(request, OC_STATUS_OK);
}

static void
post_light(oc_request_t *request, oc_interface_mask_t interface, void *user_data)
{
  (void)interface;
  (void)user_data;
  PRINT("POST_light:\n");
  oc_rep_t *rep = request->request_payload;
  while (rep != NULL) {
    PRINT("key: %s ", oc_string(rep->name));
    switch (rep->type) {
    case BOOL:
      state = rep->value.boolean;
      PRINT("value: %d\n", state);
      break;
    case INT:
      power = rep->value.integer;
      PRINT("value: %d\n", power);
      break;
    case STRING:
      oc_free_string(&name);
      oc_new_string(&name, oc_string(rep->value.string),
                    oc_string_len(rep->value.string));
      break;
    default:
      oc_send_response(request, OC_STATUS_BAD_REQUEST);
      return;
      break;
    }
    rep = rep->next;
  }
  oc_send_response(request, OC_STATUS_CHANGED);
}

static void
put_light(oc_request_t *request, oc_interface_mask_t interface,
           void *user_data)
{
  (void)interface;
  (void)user_data;
  post_light(request, interface, user_data);
}

static void
register_resources(void)
{
  oc_resource_t *res = oc_new_resource("/a/light", 2, 0);
  oc_resource_bind_resource_type(res, "core.light");
  oc_resource_bind_resource_type(res, "core.brightlight");
  oc_resource_bind_resource_interface(res, OC_IF_RW);
  oc_resource_set_default_interface(res, OC_IF_RW);
  oc_resource_set_discoverable(res, true);
  oc_resource_set_periodic_observable(res, 1);
  oc_resource_set_request_handler(res, OC_GET, get_light, NULL);
  oc_resource_set_request_handler(res, OC_PUT, put_light, NULL);
  oc_resource_set_request_handler(res, OC_POST, post_light, NULL);
  oc_add_resource(res);
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
                                       .register_resources =
                                         register_resources };

  nano_sem_init(&block);
  PRINT("oc_main: simple server111\n"); 
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

#endif  /* Zephyr */

#include "port/oc_clock.h"
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include "oc_buffer.h"
#include "port/oc_connectivity.h"

#include "los_mux.h"
#include "los_api_mutex.h"
#include "los_event.h"

/*事件控制结构体*/
EVENT_CB_S  handle_events;

/*事件等待类型*/
#define event_wait 0x00000001

//struct timespec
//{
//  time_t tv_sec; /*second*/
//  long tv_nsec;  /*nanosecond*/
//};

static UINT32 mutexs = 0x00000001;
//pthread_cond_t cv;
//struct timespec tss;

int quits = 0;

static void
signal_event_loop(void)
{
  LOS_MuxPend(mutexs, LOS_WAIT_FOREVER);
	
	UINT32 uwRet;
	uwRet = LOS_EventWrite(&handle_events, event_wait);
	if(uwRet != LOS_OK)
	{
			PRINT("event write failed .\n");
	}
  LOS_MuxPost(mutexs);
}

void
handle_signal_client(int signal)
{
  (void)signal;
  signal_event_loop();
  quits = 1;
}

int
serverstart(void)
{
  return 0;
}

int
simpleserverstart(void)
{
  int init;
	
	//初始化event
	UINT32 uwRet;
	uwRet = LOS_EventInit(&handle_events);
	if(uwRet != LOS_OK)
	{
			PRINT("init event failed .\n");
			return LOS_NOK;
	}
/*
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = handle_signal_client;
  sigaction(SIGINT, &sa, NULL);
*/

  static const oc_handler_t handlers = {.init = app_init,
                                       .signal_event_loop = signal_event_loop,
                                       .register_resources =
                                         register_resources };

  oc_clock_time_t next_event;

#ifdef OC_SECURITY
  oc_storage_config("./creds");
#endif /* OC_SECURITY */

  init = oc_main_init(&handlers);
  PRINT("oc_main: liteos server\n");  
  if (init < 0)
    return init;

  while (quits != 1) {
    next_event = oc_main_poll();
    LOS_MuxPend(mutexs, LOS_WAIT_FOREVER);
    if (next_event == 0) {
      //pthread_cond_wait(&cv, &mutex);
			LOS_EventRead(&handle_events, event_wait, LOS_WAITMODE_AND, LOS_WAIT_FOREVER);
			
    } else {
			next_event -= oc_clock_time();
			LOS_EventRead(&handle_events, event_wait, LOS_WAITMODE_AND, next_event);
      //tss.tv_sec = (next_event / OC_CLOCK_SECOND);
      //tss.tv_nsec = (next_event % OC_CLOCK_SECOND) * 1.e09 / OC_CLOCK_SECOND;
      //pthread_cond_timedwait(&cv, &mutex, tss);
    }
    LOS_MuxPost(mutexs);
  }

  oc_main_shutdown();
  return 0;
}
