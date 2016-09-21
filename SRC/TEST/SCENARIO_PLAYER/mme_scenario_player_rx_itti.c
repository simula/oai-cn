/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include "bstrlib.h"

#include "xml_load.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "mme_scenario_player.h"
#include "3gpp_23.003.h"
#include "3gpp_24.008.h"
#include "3gpp_33.401.h"
#include "3gpp_24.007.h"
#include "3gpp_36.401.h"
#include "3gpp_36.331.h"
#include "3gpp_24.301.h"
#include "security_types.h"
#include "securityDef.h"
#include "common_types.h"
#include "mme_api.h"
#include "emm_data.h"
#include "emm_msg.h"
#include "esm_msg.h"
#include "intertask_interface.h"
#include "timer.h"
#include "dynamic_memory_check.h"
#include "common_defs.h"
#include "xml_msg_tags.h"
#include "xml_msg_load_itti.h"
#include "itti_free_defined_msg.h"
#include "itti_comp.h"

extern scenario_player_t g_msp_scenarios;

// TODO code bellow can be generated by a macro

//------------------------------------------------------------------------------
void mme_scenario_player_handle_nas_downlink_data_req (instance_t instance, const itti_nas_dl_data_req_t * const nas_dl_data_req)
{
  // get message specified in scenario
  scenario_t *scenario = g_msp_scenarios.current_scenario;
  if (!scenario) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No scenario started, NAS_DOWNLINK_DATA_REQ discarded\n");
    return;
  }

  pthread_mutex_lock(&scenario->lock);
  scenario_player_item_t *item = scenario->last_played_item;
  if (item) item = item->next_item;
  else {
    pthread_mutex_unlock(&scenario->lock);
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, NAS_DOWNLINK_DATA_REQ discarded\n");
    return;
  }

  if ((SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) && !(item->u.msg.is_tx) && !(item->u.msg.is_processed)) {
    if (NAS_DOWNLINK_DATA_REQ == ITTI_MSG_ID (item->u.msg.itti_msg)) {
      // OK messages seems to match
      OAILOG_TRACE(LOG_MME_SCENARIO_PLAYER, "Found matching NAS_DOWNLINK_DATA_REQ message UID %d\n", item->uid);

      msp_get_elapsed_time_since_scenario_start(scenario, &item->u.msg.time_stamp);

      // check if variables to load
      struct load_list_item_s *list_var_item = item->u.msg.vars_to_load;
      while (list_var_item) {
        scenario_player_var_t * var_item = &list_var_item->item->u.var;
        list_var_item->value_getter_func(nas_dl_data_req, &var_item->value.value_u64);
        var_item->value_changed = true;
        OAILOG_TRACE(LOG_MME_SCENARIO_PLAYER, "Loaded var %s=0x%" PRIx64 " from NAS_DOWNLINK_DATA_REQ message\n", bdata(var_item->name), var_item->value.value_u64);

        // notify listeners of this var
        msp_var_notify_listeners(list_var_item->item);

        list_var_item = list_var_item->next;
      }

      if ((item->u.msg.xml_dump2struct_needed) || (item->u.msg.vars_to_load)) {
        // reload scenario rx message
        if (msp_reload_message(scenario, item)) {
          scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED);
          OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error in reloading NAS_DOWNLINK_DATA_REQ message, scenario failed\n");
        }
      }

      // compare messages
      if (!itti_msg_comp_nas_downlink_data_req(&item->u.msg.itti_msg->ittiMsg.nas_dl_data_req, nas_dl_data_req)) {
        OAILOG_DEBUG(LOG_MME_SCENARIO_PLAYER, "Pending RX NAS_DOWNLINK_DATA_REQ message successfully received\n");
        if (SCENARIO_STATUS_PAUSED == scenario->status) {
          scenario_set_status(scenario, SCENARIO_STATUS_PLAYING);
        }
        scenario->last_played_item = item;
      } else {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED);
        OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Pending RX NAS_DOWNLINK_DATA_REQ message in scenario differ from received NAS_DOWNLINK_DATA_REQ, scenario failed\n");
      }
      item->u.msg.is_processed = true;
    } else {
      scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED);
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Pending RX message in scenario is not NAS_DOWNLINK_DATA_REQ, scenario failed\n");
    }
  } else {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, NAS_DOWNLINK_DATA_REQ discarded\n");
  }
  pthread_mutex_unlock(&scenario->lock);
}
//------------------------------------------------------------------------------
void mme_scenario_player_handle_mme_app_connection_establishment_cnf (instance_t instance, const itti_mme_app_connection_establishment_cnf_t * const mme_app_connection_establishment_cnf)
{
  // get message specified in scenario
  scenario_t *scenario = g_msp_scenarios.current_scenario;
  if (!scenario) {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No scenario started, MME_APP_CONNECTION_ESTABLISHMENT_CNF discarded\n");
    return;
  }

  pthread_mutex_lock(&scenario->lock);
  scenario_player_item_t *item = scenario->last_played_item;
  if (item) item = item->next_item;
  else {
    pthread_mutex_unlock(&scenario->lock);
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, MME_APP_CONNECTION_ESTABLISHMENT_CNF discarded\n");
    return;
  }

  if ((SCENARIO_PLAYER_ITEM_ITTI_MSG == item->item_type) && !(item->u.msg.is_tx) && !(item->u.msg.is_processed)) {
    if (MME_APP_CONNECTION_ESTABLISHMENT_CNF == ITTI_MSG_ID (item->u.msg.itti_msg)) {
      // OK messages seems to match
      msp_get_elapsed_time_since_scenario_start(scenario, &item->u.msg.time_stamp);

      // check if variables to load
      struct load_list_item_s *list_var_item = item->u.msg.vars_to_load;
      bool reload_scenario_message = (NULL != item->u.msg.vars_to_load);
      while (list_var_item) {
        scenario_player_var_t * var_item = &list_var_item->item->u.var;
        list_var_item->value_getter_func(mme_app_connection_establishment_cnf, &var_item->value.value_u64);
        var_item->value_changed = true;
        OAILOG_TRACE(LOG_MME_SCENARIO_PLAYER, "Loaded var %s=0x%" PRIx64 " from MME_APP_CONNECTION_ESTABLISHMENT_CNF message\n", bdata(var_item->name), var_item->value.value_u64);

        // notify listeners of this var
        msp_var_notify_listeners(list_var_item->item);

        list_var_item = list_var_item->next;
      }

      if (reload_scenario_message) {
        // reload scenario rx message
        if (msp_reload_message(scenario, item)) {
          scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED);
          OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Error in reloading MME_APP_CONNECTION_ESTABLISHMENT_CNF message, scenario failed\n");
        }
      }

      // compare messages
      if (!itti_msg_comp_mme_app_connection_establishment_cnf(&item->u.msg.itti_msg->ittiMsg.mme_app_connection_establishment_cnf, mme_app_connection_establishment_cnf)) {
        OAILOG_DEBUG(LOG_MME_SCENARIO_PLAYER, "Pending RX MME_APP_CONNECTION_ESTABLISHMENT_CNF message successfully received\n");
        if (SCENARIO_STATUS_PAUSED == scenario->status) {
          scenario_set_status(scenario, SCENARIO_STATUS_PLAYING);
        }
        scenario->last_played_item = item;
      } else {
        scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED);
        OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Pending RX MME_APP_CONNECTION_ESTABLISHMENT_CNF message in scenario differ from received MME_APP_CONNECTION_ESTABLISHMENT_CNF, scenario failed\n");
      }
      item->u.msg.is_processed = true;
    } else {
      scenario_set_status(scenario, SCENARIO_STATUS_PLAY_FAILED);
      OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "Pending RX message in scenario is not MME_APP_CONNECTION_ESTABLISHMENT_CNF, scenario failed\n");
    }
  } else {
    OAILOG_ERROR(LOG_MME_SCENARIO_PLAYER, "No Pending RX message in scenario, MME_APP_CONNECTION_ESTABLISHMENT_CNF discarded\n");
  }
  pthread_mutex_unlock(&scenario->lock);

}
