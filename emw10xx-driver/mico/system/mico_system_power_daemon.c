/* MiCO Team
 * Copyright (c) 2017 MXCHIP Information Tech. Co.,Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mico.h"
#include "system_internal.h"

static bool                     needs_update          = false;
static mico_timed_event_t _timed_sys_power_state_change;

extern system_context_t* sys_context;

extern void sendNotifySYSWillPowerOff(void);

void PlatformEasyLinkButtonClickedCallback(void)
{
  system_log_trace();
  
  require_quiet( sys_context, exit );
  
#ifdef EasyLink_Needs_Reboot
  if(sys_context->flashContentInRam.micoSystemConfig.easyLinkByPass != EASYLINK_BYPASS_NO){
      sys_context->flashContentInRam.micoSystemConfig.easyLinkByPass = EASYLINK_BYPASS_NO;
    needs_update = true;
  }
  
  /* Enter easylink mode temporary in configed mode */
  if(sys_context->flashContentInRam.micoSystemConfig.configured == allConfigured){
      sys_context->flashContentInRam.micoSystemConfig.configured = wLanUnConfigured;
    needs_update = true;
  }

#if (MICO_WLAN_CONFIG_MODE_TRIGGER) && (MICO_WLAN_CONFIG_MODE_TRIGGER == CONFIG_MODE_TRIGGER_EASYLINK_BTN )
  /* Enter easylink mode temporary in un-configed mode by EasyLink button*/
  if(sys_context->flashContentInRam.micoSystemConfig.configured == unConfigured){
      sys_context->flashContentInRam.micoSystemConfig.configured = unConfigured2;
      needs_update = true;
  }
#endif

  mico_system_power_perform( &sys_context->flashContentInRam, eState_Software_Reset );
#else
  mico_system_wlan_start_autoconf( );
#endif

exit: 
  return;
}

void PlatformEasyLinkButtonLongPressedCallback(void)
{
  system_log_trace();
  mico_Context_t* context = NULL;
  
  context = mico_system_context_get( );
  require( context, exit );

  mico_system_context_restore( context );
  
  mico_system_power_perform( context, eState_Software_Reset );

exit:
  return;
}

USED void PlatformStandbyButtonClickedCallback(void)
{
  system_log_trace();
  mico_Context_t* context = NULL;
  
  context = mico_system_context_get( );
  require( context, exit );
  
  mico_system_power_perform( context, eState_Standby );

exit: 
  return;
}

static OSStatus _sys_power_state_change_handler(void *arg)
{  
    
  switch( sys_context->micoStatus.current_sys_state )
  {
    case eState_Normal:
      break;
    case eState_Software_Reset:
      mico_system_reboot( );
      break;
    case eState_Wlan_Powerdown:
      micoWlanPowerOff( );
      break;
    case eState_Standby:
      micoWlanPowerOff( );
      MicoSystemStandBy( MICO_WAIT_FOREVER );
      break;
    default:
      break;
  }
  return kNoErr;
}

static OSStatus _sys_will_power_off_handler(void *arg)
{
  OSStatus err = kNoErr;
  
  require_action( sys_context, exit, err = kNotPreparedErr );

  if(needs_update == true)
  {
    mico_system_context_update( &sys_context->flashContentInRam );
    needs_update = false;
  }

  mico_rtos_register_timed_event( &_timed_sys_power_state_change, MICO_NETWORKING_WORKER_THREAD, _sys_power_state_change_handler, 500, 0 );
  sendNotifySYSWillPowerOff();

exit:
  return err;
}


OSStatus mico_system_power_perform( mico_Context_t* const in_context, mico_system_state_t new_state )
{
  OSStatus err = kNoErr;

  require_action( sys_context, exit, err = kNotPreparedErr );

  sys_context->micoStatus.current_sys_state = new_state;

  /* Send an envent to do some operation before power off, and wait some time to perform power change */
  mico_rtos_send_asynchronous_event( MICO_NETWORKING_WORKER_THREAD, _sys_will_power_off_handler, NULL );

exit:
  return err; 
}







