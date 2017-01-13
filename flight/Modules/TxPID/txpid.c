/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup TxPIDModule TxPID Module
 * @{
 *
 * @file       txpid.c
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2011.
 * @brief      Optional module to tune PID settings using R/C transmitter.
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

/**
 * Output object: StabilizationSettings
 *
 * This module will periodically update values of stabilization PID settings
 * depending on configured input control channels. New values of stabilization
 * settings are not saved to flash, but updated in RAM. It is expected that the
 * module will be enabled only for tuning. When desired values are found, they
 * can be read via GCS and saved permanently. Then this module should be
 * disabled again.
 *
 * UAVObjects are automatically generated by the UAVObjectGenerator from
 * the object definition XML file.
 */

#include "openpilot.h"
#include <eventdispatcher.h>

#include "txpidsettings.h"
#include "accessorydesired.h"
#include "manualcontrolcommand.h"
#include "stabilizationsettings.h"
#include "vbarsettings.h"
#ifndef SMALLF1
#include "vtolpathfollowersettings.h"
#endif

#include "flightstatus.h"
#include "modulesettings.h"

//
// Configuration
//
#define SAMPLE_PERIOD_MS		200
#define TELEMETRY_UPDATE_PERIOD_MS	0	// 0 = update on change (default)

// Sanity checks
#if (TXPIDSETTINGS_PIDS_NUMELEM != TXPIDSETTINGS_INPUTS_NUMELEM) || \
    (TXPIDSETTINGS_PIDS_NUMELEM != TXPIDSETTINGS_MINPID_NUMELEM) || \
    (TXPIDSETTINGS_PIDS_NUMELEM != TXPIDSETTINGS_MAXPID_NUMELEM)
#error Invalid TxPID UAVObject definition (inconsistent number of field elements)
#endif

// Private types
struct txpid_struct {
	TxPIDSettingsData inst;
	StabilizationSettingsData stab;
	VbarSettingsData vbar;
	AccessoryDesiredData accessory;
#ifndef SMALLF1
	VtolPathFollowerSettingsData vtolPathFollowerSettingsData;
#endif
#if (TELEMETRY_UPDATE_PERIOD_MS != 0)
	UAVObjMetadata metadata;
#endif
};

// Private variables
static struct txpid_struct *txpid_data;

// Private functions
static void updatePIDs(UAVObjEvent* ev, void *ctx, void *obj, int len);
static bool update(float *var, float val);
static float scale(float val, float inMin, float inMax, float outMin, float outMax);

/**
 * Initialise the module, called on startup
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t TxPIDInitialize(void)
{
	bool module_enabled = false;

#ifdef MODULE_TxPID_BUILTIN
	module_enabled = true;
#else
	uint8_t module_state[MODULESETTINGS_ADMINSTATE_NUMELEM];
	ModuleSettingsAdminStateGet(module_state);
	if (module_state[MODULESETTINGS_ADMINSTATE_TXPID] == MODULESETTINGS_ADMINSTATE_ENABLED) {
		module_enabled = true;
	}
#endif

#ifndef SMALLF1
	if (TxPIDSettingsInitialize() == -1) {
		module_enabled = false;
		return -1;
	}
#endif

	if (module_enabled) {
		txpid_data = PIOS_malloc(sizeof(*txpid_data));

		if (txpid_data != NULL) {
			memset(txpid_data, 0x00, sizeof(*txpid_data));

			if (TxPIDSettingsInitialize() == -1 || AccessoryDesiredInitialize() == -1) {
				module_enabled = false;
				return -1;
			}

			return 0;
		}
	}

	return -1;
}

/**
 * Module start routine automatically called after initialization routine
 * @return 0 when was successful
 */
static int32_t TxPIDStart(void)
{
	if (txpid_data == NULL) {
		return -1;
	}

	UAVObjEvent ev = {
			.obj = AccessoryDesiredHandle(),
			.instId = 0,
			.event = 0,
		};
		EventPeriodicCallbackCreate(&ev, updatePIDs, SAMPLE_PERIOD_MS);

#if (TELEMETRY_UPDATE_PERIOD_MS != 0)
	// Change StabilizationSettings update rate from OnChange to periodic
	// to prevent telemetry link flooding with frequent updates in case of
	// control channel jitter.
	// Warning: saving to flash with this code active will change the
	// StabilizationSettings update rate permanently. Use Metadata via
	// browser to reset to defaults (telemetryAcked=true, OnChange).
	if (StabilizationSettingsInitialize() == -1){
		module_enabled = false;
		return -1;
	}
	StabilizationSettingsGetMetadata(&txpid_data->metadata);
	txpid_data->metadata.telemetryAcked = 0;
	txpid_data->metadata.telemetryUpdateMode = UPDATEMODE_PERIODIC;
	txpid_data->metadata.telemetryUpdatePeriod = TELEMETRY_UPDATE_PERIOD_MS;
	StabilizationSettingsSetMetadata(&txpid_data->metadata);
#endif

	return 0;
}

MODULE_INITCALL(TxPIDInitialize, TxPIDStart);

/**
 * Update PIDs callback function
 */
static void updatePIDs(UAVObjEvent* ev, void *ctx, void *obj, int len)
{
	(void) ev; (void) ctx; (void) obj; (void) len;

	if (ev->obj != AccessoryDesiredHandle())
		return;

	TxPIDSettingsGet(&txpid_data->inst);

	if (txpid_data->inst.UpdateMode == TXPIDSETTINGS_UPDATEMODE_NEVER)
		return;

	uint8_t armed;
	FlightStatusArmedGet(&armed);
	if ((txpid_data->inst.UpdateMode == TXPIDSETTINGS_UPDATEMODE_WHENARMED) &&
			(armed == FLIGHTSTATUS_ARMED_DISARMED))
		return;

	StabilizationSettingsGet(&txpid_data->stab);
	VbarSettingsGet(&txpid_data->vbar);

#ifndef SMALLF1
	// Check to make sure the settings UAVObject has been instantiated
	if (VtolPathFollowerSettingsHandle()) {
		VtolPathFollowerSettingsGet(&txpid_data->vtolPathFollowerSettingsData);
	}

	bool vtolPathFollowerSettingsNeedsUpdate = false;
#endif

	bool stabilizationSettingsNeedsUpdate = false;
	bool vbarSettingsNeedsUpdate = false;

	// Loop through every enabled instance
	for (uint8_t i = 0; i < TXPIDSETTINGS_PIDS_NUMELEM; i++) {
		if (txpid_data->inst.PIDs[i] != TXPIDSETTINGS_PIDS_DISABLED) {

			float value;
			if (txpid_data->inst.Inputs[i] == TXPIDSETTINGS_INPUTS_THROTTLE) {
				ManualControlCommandThrottleGet(&value);
				value = scale(value,
						txpid_data->inst.ThrottleRange[TXPIDSETTINGS_THROTTLERANGE_MIN],
						txpid_data->inst.ThrottleRange[TXPIDSETTINGS_THROTTLERANGE_MAX],
						txpid_data->inst.MinPID[i], txpid_data->inst.MaxPID[i]);
			} else if (AccessoryDesiredInstGet(txpid_data->inst.Inputs[i] - TXPIDSETTINGS_INPUTS_ACCESSORY0, &txpid_data->accessory) == 0) {
				value = scale(txpid_data->accessory.AccessoryVal, -1.0, 1.0, txpid_data->inst.MinPID[i], txpid_data->inst.MaxPID[i]);
			} else {
				continue;
			}

			switch (txpid_data->inst.PIDs[i]) {
			case TXPIDSETTINGS_PIDS_ROLLRATEKP:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLRATEKI:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLRATEKD:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KD], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLRATEILIMIT:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_ILIMIT], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLATTITUDEKP:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollPI[STABILIZATIONSETTINGS_ROLLPI_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLATTITUDEKI:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollPI[STABILIZATIONSETTINGS_ROLLPI_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLATTITUDEILIMIT:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollPI[STABILIZATIONSETTINGS_ROLLPI_ILIMIT], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHRATEKP:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHRATEKI:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHRATEKD:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KD], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHRATEILIMIT:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_ILIMIT], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHATTITUDEKP:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchPI[STABILIZATIONSETTINGS_PITCHPI_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHATTITUDEKI:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchPI[STABILIZATIONSETTINGS_PITCHPI_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHATTITUDEILIMIT:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchPI[STABILIZATIONSETTINGS_PITCHPI_ILIMIT], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHRATEKP:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KP], value);
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHRATEKI:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KI], value);
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHRATEKD:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_KD], value);
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_KD], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHRATEILIMIT:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollRatePID[STABILIZATIONSETTINGS_ROLLRATEPID_ILIMIT], value);
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchRatePID[STABILIZATIONSETTINGS_PITCHRATEPID_ILIMIT], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHATTITUDEKP:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollPI[STABILIZATIONSETTINGS_ROLLPI_KP], value);
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchPI[STABILIZATIONSETTINGS_PITCHPI_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHATTITUDEKI:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollPI[STABILIZATIONSETTINGS_ROLLPI_KI], value);
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchPI[STABILIZATIONSETTINGS_PITCHPI_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHATTITUDEILIMIT:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.RollPI[STABILIZATIONSETTINGS_ROLLPI_ILIMIT], value);
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.PitchPI[STABILIZATIONSETTINGS_PITCHPI_ILIMIT], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWRATEKP:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWRATEKI:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWRATEKD:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_KD], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWRATEILIMIT:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.YawRatePID[STABILIZATIONSETTINGS_YAWRATEPID_ILIMIT], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWATTITUDEKP:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.YawPI[STABILIZATIONSETTINGS_YAWPI_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWATTITUDEKI:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.YawPI[STABILIZATIONSETTINGS_YAWPI_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWATTITUDEILIMIT:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.YawPI[STABILIZATIONSETTINGS_YAWPI_ILIMIT], value);
				break;
			case TXPIDSETTINGS_PIDS_GYROCUTOFF:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.GyroCutoff, value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLVBARSENSITIVITY:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarSensitivity[VBARSETTINGS_VBARSENSITIVITY_ROLL], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHVBARSENSITIVITY:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarSensitivity[VBARSETTINGS_VBARSENSITIVITY_PITCH], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHVBARSENSITIVITY:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarSensitivity[VBARSETTINGS_VBARSENSITIVITY_ROLL], value);
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarSensitivity[VBARSETTINGS_VBARSENSITIVITY_PITCH], value);
				break;
			
			case TXPIDSETTINGS_PIDS_YAWVBARSENSITIVITY:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarSensitivity[VBARSETTINGS_VBARSENSITIVITY_YAW], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLVBARKP:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarRollPID[VBARSETTINGS_VBARROLLPID_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLVBARKI:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarRollPID[VBARSETTINGS_VBARROLLPID_KI], value);
				break;	
			case TXPIDSETTINGS_PIDS_ROLLVBARKD:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarRollPID[VBARSETTINGS_VBARROLLPID_KD], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHVBARKP:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarPitchPID[VBARSETTINGS_VBARPITCHPID_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHVBARKI:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarPitchPID[VBARSETTINGS_VBARPITCHPID_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_PITCHVBARKD:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarPitchPID[VBARSETTINGS_VBARPITCHPID_KD], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHVBARKP:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarRollPID[VBARSETTINGS_VBARROLLPID_KP], value);
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarPitchPID[VBARSETTINGS_VBARPITCHPID_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHVBARKI:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarRollPID[VBARSETTINGS_VBARROLLPID_KI], value);
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarPitchPID[VBARSETTINGS_VBARPITCHPID_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_ROLLPITCHVBARKD:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarRollPID[VBARSETTINGS_VBARROLLPID_KD], value);
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarPitchPID[VBARSETTINGS_VBARPITCHPID_KD], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWVBARKP:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarYawPID[VBARSETTINGS_VBARYAWPID_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWVBARKI:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarYawPID[VBARSETTINGS_VBARYAWPID_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_YAWVBARKD:
				vbarSettingsNeedsUpdate |= update(&txpid_data->vbar.VbarYawPID[VBARSETTINGS_VBARYAWPID_KD], value);
				break;
			case TXPIDSETTINGS_PIDS_CAMERATILT:
				stabilizationSettingsNeedsUpdate |= update(&txpid_data->stab.CameraTilt, value);
				break;
#ifndef SMALLF1
			case TXPIDSETTINGS_PIDS_HORIZONTALPOSKP:
				vtolPathFollowerSettingsNeedsUpdate |= update(&txpid_data->vtolPathFollowerSettingsData.HorizontalPosPI[VTOLPATHFOLLOWERSETTINGS_HORIZONTALPOSPI_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_HORIZONTALPOSKI:
				vtolPathFollowerSettingsNeedsUpdate |= update(&txpid_data->vtolPathFollowerSettingsData.HorizontalPosPI[VTOLPATHFOLLOWERSETTINGS_HORIZONTALPOSPI_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_HORIZONTALPOSILIMIT:
				vtolPathFollowerSettingsNeedsUpdate |= update(&txpid_data->vtolPathFollowerSettingsData.HorizontalPosPI[VTOLPATHFOLLOWERSETTINGS_HORIZONTALPOSPI_ILIMIT], value);
				break;
			case TXPIDSETTINGS_PIDS_HORIZONTALVELKP:
				vtolPathFollowerSettingsNeedsUpdate |= update(&txpid_data->vtolPathFollowerSettingsData.HorizontalVelPID[VTOLPATHFOLLOWERSETTINGS_HORIZONTALVELPID_KP], value);
				break;
			case TXPIDSETTINGS_PIDS_HORIZONTALVELKI:
				vtolPathFollowerSettingsNeedsUpdate |= update(&txpid_data->vtolPathFollowerSettingsData.HorizontalVelPID[VTOLPATHFOLLOWERSETTINGS_HORIZONTALVELPID_KI], value);
				break;
			case TXPIDSETTINGS_PIDS_HORIZONTALVELKD:
				vtolPathFollowerSettingsNeedsUpdate |= update(&txpid_data->vtolPathFollowerSettingsData.HorizontalVelPID[VTOLPATHFOLLOWERSETTINGS_HORIZONTALVELPID_KD], value);
				break;
#endif /* !SMALLF1 */
			default:
				// Previously this would assert.  But now the
				// object may be missing and it's not worth a
				// crash.
				break;
			}
		}
	}

	// Update UAVOs, if necessary
	if (stabilizationSettingsNeedsUpdate) {
		StabilizationSettingsSet(&txpid_data->stab);
	}

	if (vbarSettingsNeedsUpdate) {
		VbarSettingsSet(&txpid_data->vbar);
	}

#ifndef SMALLF1
	if (vtolPathFollowerSettingsNeedsUpdate) {
		// Check to make sure the settings UAVObject has been instantiated
		if (VtolPathFollowerSettingsHandle()) {
			VtolPathFollowerSettingsSet(&txpid_data->vtolPathFollowerSettingsData);
		}
	}
#endif
}

/**
 * Scales input val from [inMin..inMax] range to [outMin..outMax].
 * If val is out of input range (inMin <= inMax), it will be bound.
 * (outMin > outMax) is ok, in that case output will be decreasing.
 *
 * \returns scaled value
 */
static float scale(float val, float inMin, float inMax, float outMin, float outMax)
{
	// bound input value
	if (val > inMax) val = inMax;
	if (val < inMin) val = inMin;

	// normalize input value to [0..1]
	if (inMax <= inMin)
		val = 0.0;
	else
		val = (val - inMin) / (inMax - inMin);

	// update output bounds
	if (outMin > outMax) {
		float t = outMin;
		outMin = outMax;
		outMax = t;
		val = 1.0f - val;
	}

	return (outMax - outMin) * val + outMin;
}

/**
 * Updates var using val if needed.
 * \returns 1 if updated, 0 otherwise
 */
static bool update(float *var, float val)
{
	if (*var != val) {
		*var = val;
		return true;
	}
	return false;
}

/**
 * @}
 * @}
 */
