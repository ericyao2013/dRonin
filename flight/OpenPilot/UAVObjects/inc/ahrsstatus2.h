/**
 ******************************************************************************
 * @addtogroup UAVObjects OpenPilot UAVObjects
 * @{ 
 * @addtogroup AhrsStatus2 AhrsStatus2 
 * @brief Status for the @ref AHRSCommsModule, including communication errors
 *
 * Autogenerated files and functions for AhrsStatus2 Object
 
 * @{ 
 *
 * @file       ahrsstatus2.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Implementation of the AhrsStatus2 object. This file has been 
 *             automatically generated by the UAVObjectGenerator.
 * 
 * @note       Object definition file: ahrsstatus2.xml. 
 *             This is an automatically generated file.
 *             DO NOT modify manually.
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
 */

#ifndef AHRSSTATUS2_H
#define AHRSSTATUS2_H

// Object constants
#define AHRSSTATUS2_OBJID 805003662U
#define AHRSSTATUS2_NAME "AhrsStatus2"
#define AHRSSTATUS2_METANAME "AhrsStatus2Meta"
#define AHRSSTATUS2_ISSINGLEINST 1
#define AHRSSTATUS2_ISSETTINGS 0
#define AHRSSTATUS2_NUMBYTES sizeof(AhrsStatus2Data)

// Object access macros
/**
 * @function AhrsStatus2Get(dataOut)
 * @brief Populate a AhrsStatus2Data object
 * @param[out] dataOut 
 */
#define AhrsStatus2Get(dataOut) UAVObjGetData(AhrsStatus2Handle(), dataOut)
#define AhrsStatus2Set(dataIn) UAVObjSetData(AhrsStatus2Handle(), dataIn)
#define AhrsStatus2InstGet(instId, dataOut) UAVObjGetInstanceData(AhrsStatus2Handle(), instId, dataOut)
#define AhrsStatus2InstSet(instId, dataIn) UAVObjSetInstanceData(AhrsStatus2Handle(), instId, dataIn)
#define AhrsStatus2ConnectQueue(queue) UAVObjConnectQueue(AhrsStatus2Handle(), queue, EV_MASK_ALL_UPDATES)
#define AhrsStatus2ConnectCallback(cb) UAVObjConnectCallback(AhrsStatus2Handle(), cb, EV_MASK_ALL_UPDATES)
#define AhrsStatus2CreateInstance() UAVObjCreateInstance(AhrsStatus2Handle())
#define AhrsStatus2RequestUpdate() UAVObjRequestUpdate(AhrsStatus2Handle())
#define AhrsStatus2RequestInstUpdate(instId) UAVObjRequestInstanceUpdate(AhrsStatus2Handle(), instId)
#define AhrsStatus2Updated() UAVObjUpdated(AhrsStatus2Handle())
#define AhrsStatus2InstUpdated(instId) UAVObjUpdated(AhrsStatus2Handle(), instId)
#define AhrsStatus2GetMetadata(dataOut) UAVObjGetMetadata(AhrsStatus2Handle(), dataOut)
#define AhrsStatus2SetMetadata(dataIn) UAVObjSetMetadata(AhrsStatus2Handle(), dataIn)
#define AhrsStatus2ReadOnly(dataIn) UAVObjReadOnly(AhrsStatus2Handle())

// Object data
typedef struct {
    uint8_t SerialNumber[8];
    uint8_t CPULoad;
    uint8_t IdleTimePerCyle;
    uint8_t RunningTimePerCyle;
    uint8_t DroppedUpdates;
    uint8_t AlgorithmSet;
    uint8_t CalibrationSet;
    uint8_t HomeSet;
    uint8_t LinkRunning;
    uint8_t AhrsKickstarts;
    uint8_t AhrsCrcErrors;
    uint8_t AhrsRetries;
    uint8_t AhrsInvalidPackets;
    uint8_t OpCrcErrors;
    uint8_t OpRetries;
    uint8_t OpInvalidPackets;

} __attribute__((packed)) AhrsStatus2Data;

// Field information
// Field SerialNumber information
/* Number of elements for field SerialNumber */
#define AHRSSTATUS2_SERIALNUMBER_NUMELEM 8
// Field CPULoad information
// Field IdleTimePerCyle information
// Field RunningTimePerCyle information
// Field DroppedUpdates information
// Field AlgorithmSet information
/* Enumeration options for field AlgorithmSet */
typedef enum { AHRSSTATUS2_ALGORITHMSET_FALSE=0, AHRSSTATUS2_ALGORITHMSET_TRUE=1 } AhrsStatus2AlgorithmSetOptions;
// Field CalibrationSet information
/* Enumeration options for field CalibrationSet */
typedef enum { AHRSSTATUS2_CALIBRATIONSET_FALSE=0, AHRSSTATUS2_CALIBRATIONSET_TRUE=1 } AhrsStatus2CalibrationSetOptions;
// Field HomeSet information
/* Enumeration options for field HomeSet */
typedef enum { AHRSSTATUS2_HOMESET_FALSE=0, AHRSSTATUS2_HOMESET_TRUE=1 } AhrsStatus2HomeSetOptions;
// Field LinkRunning information
/* Enumeration options for field LinkRunning */
typedef enum { AHRSSTATUS2_LINKRUNNING_FALSE=0, AHRSSTATUS2_LINKRUNNING_TRUE=1 } AhrsStatus2LinkRunningOptions;
// Field AhrsKickstarts information
// Field AhrsCrcErrors information
// Field AhrsRetries information
// Field AhrsInvalidPackets information
// Field OpCrcErrors information
// Field OpRetries information
// Field OpInvalidPackets information


// Generic interface functions
int32_t AhrsStatus2Initialize();
UAVObjHandle AhrsStatus2Handle();

#endif // AHRSSTATUS2_H

/**
 * @}
 * @}
 */
